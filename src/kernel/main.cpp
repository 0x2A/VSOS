// #include <LoaderParams.h>
// #include "mem/pagetables.h"
// #include "os.internal.h"
// #include <intrin.h>
// #include <Assert.h>
// #include <cstdarg>
// #include "BootHeap.h"
// #include "Kernel.h"
// #include "arch\x64\interrupt.h"
// 
// //Boot Heap
// //Only needed for bitvector in physical memory manager
// static constexpr size_t BootHeapSize = PageSize << 12; //4MB Boot Heap
// KERNEL_PAGE_ALIGN static volatile UINT8 BOOT_HEAP[BootHeapSize] = { 0 };
// KERNEL_GLOBAL_ALIGN BootHeap bootHeap((void*)BOOT_HEAP, BootHeapSize);
// 
// //The one and only, statically allocated
// LoaderParams BootParams;
// Kernel kernel(BootParams, bootHeap);
// 
// //Init Stack - set by x64_main
// //TODO: reclaim
// static constexpr size_t InitStackSize = PageSize << 8; //16kb Init Stack
// KERNEL_PAGE_ALIGN volatile UINT8 KERNEL_STACK[InitStackSize] = { 0 };
// extern "C" volatile UINT64 KERNEL_STACK_STOP = (UINT64)&KERNEL_STACK[InitStackSize];
// 
// extern "C" void INTERRUPT_HANDLER(X64_INTERRUPT_VECTOR vector, X64_INTERRUPT_FRAME* frame)
// {
// 	kernel.HandleInterrupt(vector, frame);
// }
// 
// //function declarations for implementations also used in bootloader
// void Printf(const char* format, ...)
// {
// 	va_list args;
// 
// 	va_start(args, format);
// 	kernel.Printf(format, args);
// 	va_end(args);
// }
// 
// void CPrintf(const bool enable, const char* format, ...)
// {
// 	if (!enable)
// 		return;
// 
// 	va_list args;
// 	va_start(args, format);
// 	kernel.Printf(format, args);
// 	va_end(args);
// }
// 
// void Printf(const char* format, va_list args)
// {
// 	kernel.Printf(format, args);
// }
// 
// 
// void Bugcheck(const char* file, const char* line, const char* format, ...)
// {
// 	va_list args;
// 	va_start(args, format);
// 	kernel.Bugcheck(file, line, format, args);
// 	va_end(args);
// }
// 
// extern "C" uint64_t SYSTEMCALL_HANDLER(X64_SYSCALL_FRAME* frame)
// {
// 	return 0;//kernel.Syscall(frame);
// }
// 


#include <intrin.h>
#include <cstdint>
#include <bootboot/bootboot.h>
#include <LoaderParams.h>
#include <Assert.h>
#include <mem\pagetables.h>
#include "kernel\Kernel.h"


//Boot Heap
//Only needed for bitvector in physical memory manager
static constexpr size_t BootHeapSize = PageSize << 12; //4MB Boot Heap
KERNEL_PAGE_ALIGN static volatile UINT8 BOOT_HEAP[BootHeapSize] = { 0 };
KERNEL_GLOBAL_ALIGN BootHeap bootHeap((void*)BOOT_HEAP, BootHeapSize);

//Init Stack - set by x64_main
//TODO: reclaim
static constexpr size_t InitStackSize = PageSize << 8; //16kb Init Stack
KERNEL_PAGE_ALIGN volatile UINT8 KERNEL_STACK[InitStackSize] = { 0 };
extern "C" UINT64 KERNEL_STACK_STOP = (UINT64)&KERNEL_STACK[InitStackSize];


LoaderParams BootParams;
Kernel kernel(BootParams, bootHeap);


extern "C" void INTERRUPT_HANDLER(X64_INTERRUPT_VECTOR vector, X64_INTERRUPT_FRAME* frame)
{
	kernel.HandleInterrupt(vector, frame);
}

extern "C" int __cdecl atexit(void(__cdecl*)(void))
{
	return 0;
}

void Printf(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	kernel.Printf(format, args);
	va_end(args);
}

void CPrintf(const bool enable, const char* format, ...)
{
	if (!enable)
		return;

	va_list args;
	va_start(args, format);
	kernel.Printf(format, args);
	va_end(args);
}

void Printf(const char* format, va_list args)
{
	kernel.Printf(format, args);
}

void Bugcheck(const char* file, const char* line, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	kernel.Bugcheck(file, line, format, args);
	va_end(args);
}

extern "C" uint64_t SYSTEMCALL_HANDLER(X64_SYSCALL_FRAME* frame)
{
	return 0;//kernel.Syscall(frame);
}

void main()
{
	
	//Initialize kernel
 	kernel.Initialize();
// 
// 	//Load init process
// 	kernel.KeCreateProcess(std::string("init.exe"));
// 
// 	//Exit init thread
// 	kernel.KeExitThread();

	//Assert(false);
	Assert(false);
}
