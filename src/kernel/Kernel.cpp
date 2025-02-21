#include "Kernel.h"
#include <Assert.h>

#define PACKED
#include <OS.arch.h>
#include <mem\pagetables.h>
#include "hal\x64\interrupt.h"
#include "hal\x64\x64.h"
#include "panic.h"
#include "pdb/Pdb.h"
#include "devices/SoftwareDevice.h"
#include "drivers/io/RamDriveDriver.h"
#include "devices\hv\HyperV_def.h"
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
	m_windowsSpace(KernelWindowsStart, KernelWindowsEnd, true)
{

}


void Kernel::Initialize()
{

	//Initialize Display
	m_loadingScreen.Initialize();
	m_printer = &m_loadingScreen;

	Printf("Step1\r\n");

	hal.initialize();


	//Page tables
	m_pool.Initialize();
	PageTables::Pool = &m_pool;

	Printf("Step2\r\n");


	//Memory and Heap
	//m_memoryMap.Display(); //These strings go on boot heap
	m_physicalMemory.Initialize(m_memoryMap);

	//Copy from UEFI to kernel boot heap
	m_memoryMap.Reallocate();
	m_configTables.Reallocate();

	Printf("Step3\r\n");

	//Build new page table with just Kernel space
	PageTables pageTables;
	pageTables.CreateNew();
	pageTables.MapPages(KernelBaseAddress, m_params.KernelAddress, SizeToPages(m_params.KernelImageSize), true);
	pageTables.MapPages(KernelPageTablesPool, m_params.PageTablesPoolAddress, m_params.PageTablesPoolPageCount, true);
	pageTables.MapPages(KernelGraphicsDevice, m_params.Display.FrameBufferBase, SizeToPages(m_params.Display.FrameBufferSize), true);
	pageTables.MapPages(KernelPageFrameDBStart, m_params.PageFrameAddr, SizeToPages(m_physicalMemory.GetSize()), true);
	pageTables.MapPages(KernelKernelPdb, m_params.PdbAddress, SizeToPages(m_params.PdbSize), true);
	m_memoryMap.MapRuntime(pageTables);
	hal.SetupPaging(pageTables.GetRoot());

	Printf("Step4\r\n");

	//Initialize heap now that paging works
	m_heap.Initialize();

	Printf("BareMetalOS.Kernel - Base:0x%16x Size: 0x%x\n", m_params.KernelAddress, m_params.KernelImageSize);
	Printf("  PhysicalAddressSize: 0x%16x\n", m_memoryMap.GetPhysicalAddressSize());


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



void Kernel::HandleInterrupt(X64_INTERRUPT_VECTOR vector, X64_INTERRUPT_FRAME* frame)
{
	if (vector == X64_INTERRUPT_VECTOR::DoubleFault)
	{
		//KePauseSystem();
		Panic("Double Fault!\r\n");
		__halt();
	}

	//Break into kernel debugger if not user code
	//if (m_debugger.Enabled() && vector == X64_INTERRUPT_VECTOR::Breakpoint)
	{
		//m_debugger.DebuggerEvent(vector, frame);
		//return;
	}

	const auto& it = m_interruptHandlers->find(vector);
	if (it != m_interruptHandlers->end())
	{
		InterruptContext ctx = it->second;
		ctx.Handler(ctx.Context);
		HyperV::EOI();
		return;
	}

	//Show interrupt context
	this->Printf("ISR: 0x%x, Code: %x\n", vector, frame->ErrorCode);
	this->Printf("    RIP: 0x%016x\n", frame->RIP);
	this->Printf("    RBP: 0x%016x\n", frame->RBP);
	this->Printf("    RSP: 0x%016x\n", frame->RSP);
	this->Printf("    RAX: 0x%016x\n", frame->RAX);
	this->Printf("    RBX: 0x%016x\n", frame->RBX);
	this->Printf("    RCX: 0x%016x\n", frame->RCX);
	this->Printf("    RDX: 0x%016x\n", frame->RDX);
	this->Printf("    CS: 0x%x, SS: 0x%x\n", frame->CS, frame->SS);

	switch (vector)
	{
	case X64_INTERRUPT_VECTOR::PageFault:
		this->Printf("    CR2: 0x%16x\n", __readcr2());
		if (__readcr2() == 0)
			this->Printf("        Null pointer\n");
	}

	//Build context
	X64_CONTEXT context = {};
	context.Rip = frame->RIP;
	context.Rsp = frame->RSP;
	context.Rbp = frame->RBP;

	#if 0
	if (IsValidUserPointer((void*)frame->RIP))
	{
		//Interrupt is in userspace. Write Stack to stdout, write message, kill process.
		UserProcess& proc = m_scheduler.GetCurrentProcess();
		std::shared_ptr<UObject> uObject = proc.GetObject((Handle)StandardHandle::Output);
		if (uObject)
		{
			//Write stack
			AssertEqual(uObject->Type, UObjectType::Pipe);
			const UPipe* uPipe = (UPipe*)uObject.get();
			UserPipe& pipe = *uPipe->Pipe.get();

			//Write exception
			if (vector == X64_INTERRUPT_VECTOR::DivideError)
				pipe.Printf("Exception: Divide by zero\n");
			else if (vector == X64_INTERRUPT_VECTOR::Breakpoint)
				pipe.Printf("Exception: Breakpoint\n");
			else if (vector == X64_INTERRUPT_VECTOR::PageFault && __readcr2() == 0)
				pipe.Printf("Exception: Null pointer dereference\n");

			//Convert to unwind context
			CONTEXT ctx = { 0 };
			ctx.Rip = context.Rip;
			ctx.Rsp = context.Rsp;
			ctx.Rbp = context.Rbp;

			//Unwind stack, writing to process stdout
			StackWalk sw(&ctx);
			while (sw.HasNext())
			{
				PdbFunctionLookup lookup = {};
				Assert(IsValidUserPointer((void*)ctx.Rip));
				ResolveUserIP(ctx.Rip, lookup);

				Module* module = proc.GetModule(ctx.Rip);

				pipe.Printf("    %s::%s (%d)\n", module->Name, lookup.Name.c_str(), lookup.LineNumber);
				pipe.Printf("        IP: 0x%016x Base: 0x%016x, RVA: 0x%08x\n", ctx.Rip, lookup.Base, lookup.RVA);

				if (lookup.Base == nullptr)
					break;

				sw.Next((uintptr_t)lookup.Base);
			}
		}

		m_scheduler.KillCurrentProcess();
		return;
	}
	else
	{
	#endif
		//this->ShowStack(&context);

		//Bugcheck
		Fatal("Unhandled exception");
	//}
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
			hal.Wait();
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
	hal.SaveContext(&context);
	//this->ShowStack(&context);

	//if (m_scheduler.Enabled)
	{
		//KThread& thread = m_scheduler.GetCurrentThread();
		//thread.Display();
	}

	//Pause
	while (true)
		hal.Wait();
}

void Kernel::Panic(const char* message)
{
	m_display.FillScreen(gfx::Colors::DarkRed);
	m_display.DrawPrintf({ 10,10 }, gfx::Colors::Red, " === KERNEL PANIC === \r\n\r\n");
	m_display.WriteFrame({ m_display.GetWidth() - 200, 50, 150, 150 }, gfx_panic);
	m_display.DrawText({ 10,30 }, message, gfx::Colors::Red);

	X64_CONTEXT context = {};
	hal.SaveContext(&context);

	m_display.DrawText({ 10,65 }, "CPU Context:", gfx::Colors::Red);

	m_display.DrawPrintf({ 10,65 + 16 }, gfx::Colors::Red, "  RBX: 0x%016x, RBP: 0x%016x\r\n", context.Rbx, context.Rbp);
	m_display.DrawPrintf({ 10,65 + 32 }, gfx::Colors::Red, "  RDI: 0x%016x, RFLAGS: 0x%016x\r\n", context.Rdi, context.Rflags);
	m_display.DrawPrintf({ 10,65 + 48 }, gfx::Colors::Red, "  RIP: 0x%016x, RSI: 0x%016x, RSP: 0x%016x\r\n", context.Rip, context.Rsi, context.Rsp);
	m_display.DrawPrintf({ 10,65 + 64 }, gfx::Colors::Red, "  R12: 0x%016x, R13: 0x%016x\r\n", context.R12, context.R13);
	m_display.DrawPrintf({ 10,65 + 80 }, gfx::Colors::Red, "  R14: 0x%016x, R15: 0x%016x\r\n", context.R14, context.R15);

	__halt();
	Assert(false);
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
