#include "Kernel.h"
#include <Assert.h>

#define PACKED
#include <OS.arch.h>
#include <mem\pagetables.h>
#include "hal\x64\interrupt.h"
#include "hal\x64\x64.h"
#include "panic.h"
#include "pdb/Pdb.h"
//#include "devices/SoftwareDevice.h"
#include "drivers/io/RamDriveDriver.h"
//#include "devices\hv\HyperV_def.h"
#include "proc\Loader.h"
#include "types\PortableExecutable.h"
#include <Path.h>



Kernel::Kernel(const LoaderParams& params, BootHeap& bootHeap) :
	//Boot params
	m_params(params),
	m_runtime(*params.Runtime),
	m_bootHeap(bootHeap),

	m_display((void*)KernelGraphicsDevice, params.Display.VerticalResolution, params.Display.HorizontalResolution),
	m_loadingScreen(m_display),
	//Page tables
	m_pool((void*)KernelPageTablesPool, params.PageTablesPoolAddress, params.PageTablesPoolPageCount),
	m_memoryMap(params.MemoryMap.Table, params.MemoryMap.Size, params.MemoryMap.DescriptorSize),
	m_physicalMemory((void*)KernelPageFrameDBStart, params.PageFrameCount),
	m_heap(m_physicalMemory, (void*)KernelHeapStart, (void*)KernelHeapEnd),
	m_virtualMemory(m_physicalMemory),
	
	//Copy before PT switch
	m_configTables(params.ConfigTables, params.ConfigTablesCount),
	
	m_librarySpace(KernelLibraryStart, KernelLibraryEnd, true),
	m_pdbSpace(KernelPdbStart, KernelPdbEnd, true),
	m_stackSpace(KernelStackStart, KernelStackEnd, true),
	m_runtimeSpace(KernelRuntimeStart, KernelRuntimeEnd, true),
	m_windowsSpace(KernelWindowsStart, KernelWindowsEnd, true),
	m_HAL(&m_configTables)
{

}


void Kernel::Initialize()
{

	//Initialize Display
	m_loadingScreen.Initialize();
	m_printer = &m_loadingScreen;

	m_HAL.initialize();

	//Page tables
	m_pool.Initialize();
	PageTables::Pool = &m_pool;

	//Memory and Heap
	//m_memoryMap.Display();
	m_physicalMemory.Initialize(m_memoryMap);

	//Copy from UEFI to kernel boot heap
	m_memoryMap.Reallocate();
	m_configTables.Reallocate();

	//Build new page table with just Kernel space
	PageTables pageTables;
	pageTables.CreateNew();
	pageTables.MapPages(KernelBaseAddress, m_params.KernelAddress, SizeToPages(m_params.KernelImageSize), true);
	pageTables.MapPages(KernelPageTablesPool, m_params.PageTablesPoolAddress, m_params.PageTablesPoolPageCount, true);
	pageTables.MapPages(KernelGraphicsDevice, m_params.Display.FrameBufferBase, SizeToPages(m_params.Display.FrameBufferSize), true);
	pageTables.MapPages(KernelPageFrameDBStart, m_params.PageFrameAddr, SizeToPages(m_physicalMemory.GetSize()), true);
	pageTables.MapPages(KernelKernelPdb, m_params.PdbAddress, SizeToPages(m_params.PdbSize), true);
	m_memoryMap.MapRuntime(pageTables);
	m_HAL.SetupPaging(pageTables.GetRoot());

	Printf("Page table created\r\n");

	//Initialize heap now that paging works
	m_heap.Initialize();

	Printf("VSOS.Kernel  - Base:0x%16x Size: 0x%x\n", m_params.KernelAddress, m_params.KernelImageSize);
	Printf("  PhysicalAddressSize: 0x%16x\n", m_memoryMap.GetPhysicalAddressSize());

	//Test UEFI runtime access --> BUG: Doesn't work on qemu
	//EFI_TIME time;
	//Printf("Runtime 0x%16x\r\n", (uint64_t)m_params.Runtime->GetTime);
	//m_runtime.GetTime(&time, nullptr);
	//Printf("  Date: %02d-%02d-%02d %02d:%02d:%02d\n", time.Month, time.Day, time.Year, time.Hour, time.Minute, time.Second);
	
	//Initialize address spaces
	m_librarySpace.Initialize();
	m_pdbSpace.Initialize();
	m_stackSpace.Initialize();
	m_runtimeSpace.Initialize();
	m_windowsSpace.Initialize();


	m_HAL.InitDevices();

	Printf("Current CPU id: %d, total: %d CPU(s)\r\n", m_HAL.CurrentCPU(), m_HAL.CPUCount());
	//m_HAL.SendShutdown();


	Printf("Running idle...\r\n");
	while(true)
		__halt();
}


void Kernel::Printf(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	this->Printf(format, args);
	va_end(args);
}

void Kernel::Printf(const char* format, va_list args)
{
	//if ((m_debugger != nullptr) && m_debugger->IsBrokenIn())
		//m_debugger->KdpDprintf(format, args);
	m_printer->Printf(format, args);
}



void Kernel::Bugcheck(const char* file, const char* line, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	Bugcheck(file, line, format, args);
	va_end(args);
}

void Kernel::Bugcheck(const char* file, const char* line, const char* format, va_list args)
{
	static bool inBugcheck = false;
	//this->KePauseSystem();

	if (inBugcheck)
	{
		//Bugcheck during bugcheck, print what's available and bail
		this->Printf("\n%s\n%s\n", file, line);
		this->Printf(format, args);
		this->Printf("\n");

		while (true)
			m_HAL.Halt();
	}
	inBugcheck = true;

	this->Printf("Kernel Bugcheck\n");
	this->Printf("\n%s\n%s\n", file, line);

	this->Printf(format, args);
	this->Printf("\n");

	/*
	if (m_debugger.Enabled())
	{
		__debugbreak();
		return;
	}
	*/
	X64_CONTEXT context = {};
	m_HAL.SaveContext(&context);
	//this->ShowStack(&context);

	//if (m_scheduler.Enabled)
	{
		//KThread& thread = m_scheduler.GetCurrentThread();
		//thread.Display();
	}

	//Pause
	while (true)
		m_HAL.Halt();
}

void Kernel::Panic(const char* message)
{
	m_display.FillScreen(gfx::Colors::DarkRed);
	m_display.DrawPrintf({ 10,10 }, gfx::Colors::Red, " === KERNEL PANIC === \r\n\r\n");
	m_display.WriteFrame({ m_display.GetWidth() - 200, 50, 150, 150 }, gfx_panic);
	m_display.DrawText({ 10,30 }, message, gfx::Colors::Red);

	X64_CONTEXT context = {};
	m_HAL.SaveContext(&context);

	m_display.DrawText({ 10,65 }, "CPU Context:", gfx::Colors::Red);

	m_display.DrawPrintf({ 10,65 + 16 }, gfx::Colors::Red, "  RBX: 0x%016x, RBP: 0x%016x\r\n", context.Rbx, context.Rbp);
	m_display.DrawPrintf({ 10,65 + 32 }, gfx::Colors::Red, "  RDI: 0x%016x, RFLAGS: 0x%016x\r\n", context.Rdi, context.Rflags);
	m_display.DrawPrintf({ 10,65 + 48 }, gfx::Colors::Red, "  RIP: 0x%016x, RSI: 0x%016x, RSP: 0x%016x\r\n", context.Rip, context.Rsi, context.Rsp);
	m_display.DrawPrintf({ 10,65 + 64 }, gfx::Colors::Red, "  R12: 0x%016x, R13: 0x%016x\r\n", context.R12, context.R13);
	m_display.DrawPrintf({ 10,65 + 80 }, gfx::Colors::Red, "  R14: 0x%016x, R15: 0x%016x\r\n", context.R14, context.R15);

	__halt();
	Assert(false);
}

void Kernel::OnAPICTimerEvent()
{
	
}

void* Kernel::Allocate(const size_t size)
{
	if (m_heap.IsInitialized())
		return m_heap.Allocate(size);
	else
		return m_bootHeap.Allocate(size);
}

void Kernel::Deallocate(void* const address)
{
	if (m_heap.IsInitialized())
		m_heap.Deallocate(address);
	else
		return m_bootHeap.Deallocate(address);
}

uint32_t Kernel::PrepareShutdown()
{
	//Nothing to do yet
	return 0;
}

void* Kernel::VirtualMapRT(const void* address, const std::vector<paddr_t>& addresses)
{
	return m_virtualMemory.VirtualMap(address, addresses, m_runtimeSpace);
}
