#pragma once


#define KERNEL_VERSION	"0.1"
#define KERNEL_DESCR	"dev"


#include <efi.h>
#include "gfx/LinearFrameBuffer.h"
#include "kernel/mem/MemoryMap.h"
#include "hal/devices/ConfigTables.h"
extern "C"
{
#include <acpi.h>
}
#include <vector>
#include <list>
#include <map>
#include "mem/BootHeap.h"
//#include "Debugger.h"
//#include "devices/DeviceTree.h"
//#include "kernel/devices/hv/HyperVTimer.h"
//#include "HyperV.h"
#include "kernel/mem/PMM.h"
#include "kernel/mem/VAS.h"
#include "kernel/mem/VMM.h"
#include "kernel/sched/Scheduler.h"
#include "kernel/mem/KHeap.h"
#include <queue>
#include "Pdb/Pdb.h"
#include "kernel/sched/KThread.h"
#include "kernel/proc/UProc.h"
//#include "WindowingSystem.h"
//#include "Kernel/Obj/KEvent.h"
#include "os.Arch.h"
#include "kernel/os/Types.h"
//#include "user/MetalOS.h"
#include "kernel/io/LoadingScreen.h"
#include "LoaderParams.h"
//#include "Kernel/EarlyUart.h"
#include "mem/PageTablesPool.h"

#include <memory>
//#include "drivers\DriverManager.h"
//#include "devices\SMBios.h"
//#include "obj\KFile.h"
//#include "devices\LocalAPIC.h"
#include "kernel/drivers/HyperV/HyperVPlatform.h"
#include "hal\x64\interrupt.h"
#include "hal\HAL.h"
#include "io\disk\DiskManager.h"
#include "vfs\VFSManager.h"

class Kernel
{
	//friend class Debugger;

public:
	Kernel(const LoaderParams& params, BootHeap& heap);

	void Initialize();


	void Printf(const char* format, ...);
	void Printf(const char* format, va_list args);
	void PrintBytes(const char* buffer, const size_t length)
	{
		this->m_printer->PrintBytes(buffer, length);
	}


	__declspec(noreturn) void Bugcheck(const char* file, const char* line, const char* format, ...);
	__declspec(noreturn) void Bugcheck(const char* file, const char* line, const char* format, va_list args);

	__declspec(noreturn) void Panic(const char* message);

	HAL* GetHAL() { return &m_HAL; }

	uint32_t PrepareShutdown();

#pragma region Heap Interface
	void* Allocate(const size_t size);
	void Deallocate(void* const address);
#pragma endregion

#pragma region Virtual Memory Interface

	paddr_t AllocatePhysical(const size_t count);
	void* AllocateStack(const size_t count);

	void* MapPhysicalMemory(uint64_t PhysicalAddress, uint64_t Length, KernelAddress mapStartAddr = KernelSharedPageStart);
	//maps phyiscal to virtual address in runtime space
	void* VirtualMapRT(const void* address, const std::vector<paddr_t>& addresses);

	//User process address space
	void* VirtualAlloc(UserProcess& process, const void* address, const size_t size);
	void* VirtualMap(UserProcess& process, const void* address, const std::vector<paddr_t>& addresses);

	//This method only works because the loader ensures we are physically contiguous
	/*paddr_t VirtualToPhysical(uintptr_t virtualAddress)
	{
		//TODO: assert
		uint64_t rva = virtualAddress - KernelBaseAddress;
		return m_params.KernelAddress + rva;
	}

	paddr_t PhysicalToVirtual(paddr_t physicalAddr)
	{
		uint64_t p = physicalAddr - m_params.KernelAddress;
		return p + KernelBaseAddress;
	}*/
#pragma endregion
	
#pragma region Driver Interface
	void* DriverMapPages(paddr_t address, size_t count);
#pragma endregion

#pragma region ThreadStarts
	__declspec(noreturn) static void KernelThreadInitThunk();
	__declspec(noreturn) static size_t UserThreadInitThunk(void* unused);
#pragma endregion

#pragma region Internal Interface

	//Threads
	std::shared_ptr<KThread> KeCreateThread(const ThreadStart start, void* const arg, const std::string& name = "");
	void KeSleepThread(const nano_t value);
	void KeExitThread();
	std::shared_ptr<KThread> CreateThread(UserProcess& process, size_t stackSize, ThreadStart startAddress, void* arg, void* entry);

	//Libraries
	//KeModule& KeLoadLibrary(const std::string& path);

	KThread& KeGetCurrentThread();
#pragma endregion

#pragma region System Calls
	void Sleep(const uint32_t milliseconds);
#pragma endregion

	DiskManager* GetDiskManager() { return m_DiskManager; }
	VFSManager* VFS(){ return m_VFSManager; }

	LoadingScreen* GetLoadingScreen() { return &m_loadingScreen; }

	void HexDump(uint8_t* buffer, size_t size, size_t lineLength = 16);

	static size_t IdleThread(void* unused);
private:

	

	HAL m_HAL;

	//Boot params
	const LoaderParams& m_params;
	EFI_RUNTIME_SERVICES m_runtime;
	BootHeap& m_bootHeap;

	//Basic output drivers

	gfx::LinearFrameBuffer m_display;
	LoadingScreen m_loadingScreen;

	//EarlyUart m_uart;
	StringPrinter* m_printer;
	//SMBios m_SMBios;

	//Page tables
	PageTablesPool m_pool;

	//Memory and Heap
	MemoryMap m_memoryMap;
	PMM m_physicalMemory;
	KHeap m_heap;

	//Copy to kernel heap
	ConfigTables m_configTables;

	DiskManager* m_DiskManager;
	VFSManager* m_VFSManager;

	VMM m_virtualMemory;


	//Create virtual address spaces
	//TODO(tsharpe): Condense into one?
	VirtualAddressSpace m_librarySpace;
	VirtualAddressSpace m_pdbSpace;
	VirtualAddressSpace m_stackSpace;
	VirtualAddressSpace m_runtimeSpace;
	VirtualAddressSpace m_windowsSpace;

	//Process and Thread management
	Scheduler m_scheduler;
	std::map<std::string, UserRingBuffer*>* m_objectsRingBuffers;
	std::list<std::shared_ptr<UserProcess>>* m_processes;

	//Debugger m_debugger;
#if 0

	

	//UI
	WindowingSystem m_windows;

#endif
	::NO_COPY_OR_ASSIGN(Kernel);
};

//TODO: find a better way to access kernel object from any class!
extern Kernel kernel;