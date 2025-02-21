#pragma once

// x64/amd64 specific declarations and functions

#include "os.System.h"
#include "os.internal.h"
#include <cstdint>


#define InterruptHandler(x) x64_interrupt_handler_ ## x
#define IST_DOUBLEFAULT_IDX 1
#define IST_NMI_IDX 2
#define IST_DEBUG_IDX 3
#define IST_MCE_IDX 4

#define UserDPL 3
#define KernelDPL 0

#define MSR_PLATFORM_INFO_MAX_NON_TURBO_LIM_RATIO	0x00000000ff00

enum CPU_FEATURE 
{
	ECX_SSE3 = 1 << 0,
	ECX_PCLMUL = 1 << 1,
	ECX_DTES64 = 1 << 2,
	ECX_MONITOR = 1 << 3,
	ECX_DS_CPL = 1 << 4,
	ECX_VMX = 1 << 5,
	ECX_SMX = 1 << 6,
	ECX_EST = 1 << 7,
	ECX_TM2 = 1 << 8,
	ECX_SSSE3 = 1 << 9,
	ECX_CID = 1 << 10,
	ECX_SDBG = 1 << 11,
	ECX_FMA = 1 << 12,
	ECX_CX16 = 1 << 13,
	ECX_XTPR = 1 << 14,
	ECX_PDCM = 1 << 15,
	ECX_PCID = 1 << 17,
	ECX_DCA = 1 << 18,
	ECX_SSE4_1 = 1 << 19,
	ECX_SSE4_2 = 1 << 20,
	ECX_X2APIC = 1 << 21,
	ECX_MOVBE = 1 << 22,
	ECX_POPCNT = 1 << 23,
	ECX_TSC = 1 << 24,
	ECX_AES = 1 << 25,
	ECX_XSAVE = 1 << 26,
	ECX_OSXSAVE = 1 << 27,
	ECX_AVX = 1 << 28,
	ECX_F16C = 1 << 29,
	ECX_RDRAND = 1 << 30,
	ECX_HYPERVISOR = 1 << 31,

	EDX_FPU = 1 << 0,
	EDX_VME = 1 << 1,
	EDX_DE = 1 << 2,
	EDX_PSE = 1 << 3,
	EDX_TSC = 1 << 4,
	EDX_MSR = 1 << 5,
	EDX_PAE = 1 << 6,
	EDX_MCE = 1 << 7,
	EDX_CX8 = 1 << 8,
	EDX_APIC = 1 << 9,
	EDX_SEP = 1 << 11,
	EDX_MTRR = 1 << 12,
	EDX_PGE = 1 << 13,
	EDX_MCA = 1 << 14,
	EDX_CMOV = 1 << 15,
	EDX_PAT = 1 << 16,
	EDX_PSE36 = 1 << 17,
	EDX_PSN = 1 << 18,
	EDX_CLFLUSH = 1 << 19,
	EDX_DS = 1 << 21,
	EDX_ACPI = 1 << 22,
	EDX_MMX = 1 << 23,
	EDX_FXSR = 1 << 24,
	EDX_SSE = 1 << 25,
	EDX_SSE2 = 1 << 26,
	EDX_SS = 1 << 27,
	EDX_HTT = 1 << 28,
	EDX_TM = 1 << 29,
	EDX_IA64 = 1 << 30,
	EDX_PBE = 1 << 31
};

class x64
{
public:
	x64() = delete;

	static void SetupDescriptorTables();
	static void SetUserCpuContext(void* teb);
	static void SetKernelInterruptStack(void* stack);

	static void PrintGDT();

	static uint32_t OnPITTimer0(void* arg);

	static bool HasMSR();
	static bool HasEDXFeature(CPU_FEATURE f);
	static bool HasECXFeature(CPU_FEATURE f);
	
	static void InitPIT();
	static uint64_t ReadTSC();

	static void EnableFSGSBASE();

	//TSC Frequency in mhz
	static uint32_t TSCFreq;
private:
	static void InitPIC();

	static volatile uint32_t g_pitTicks;

	static constexpr size_t IdtCount = 256;
	static constexpr size_t IstStackSize = (1 << 12);//4k Stack
	

	/* CPU model specific register (MSR) numbers */

	/* x86-64 specific MSRs */
	enum class MSR : uint32_t
	{
		IA32_MSR_TSC = 0x10,
		IA32_FS_BASE = 0xC0000100,
		IA32_GS_BASE = 0xC0000101,
		IA32_KERNELGS_BASE = 0xC0000102,
		IA32_MSR_PLATFORM_INFO = 0xce,

		//INTEL SDM Vol 3A 5-22. 5.8.8
		IA32_STAR = 0xC0000081, //IA32_STAR_REG
		IA32_LSTAR = 0xC0000082, //Target RIP
		IA32_FMASK = 0xC0000084, //IA32_FMASK_REG
		IA32_EFER = 0xC0000080,
	};

	enum class GDT : uint16_t
	{
		Empty,
		KernelCode,
		KernelData,
		User32Code,
		UserData,
		UserCode,
		TssEntry,
	};

	//Syscall/Sysret requires specific orderings in GDT
	static_assert(
		(uint32_t)GDT::KernelData == (uint32_t)GDT::KernelCode + 1 &&
		(uint32_t)GDT::UserData == (uint32_t)GDT::User32Code + 1 &&
		(uint32_t)GDT::UserCode == (uint32_t)GDT::User32Code + 2,
		"Invalid kernel GDTs");

	enum IDT_GATE_TYPE
	{
		TaskGate32 = 0x5,
		InterruptGate16 = 0x6,
		TrapGate16 = 0x7,
		InterruptGate32 = 0xE,
		TrapGate32 = 0xF
	};

#pragma pack(push, 1)
	// Intel SDM Vol 3A 3.4.2 Figure 3-6
	struct SEGMENT_SELECTOR
	{
		union
		{
			struct
			{
				uint16_t PrivilegeLevel : 2;
				uint16_t TableIndicator : 1; // 0 is GDT, 1 is LDT
				uint16_t Index : 13;
			};
			uint16_t Value;
		};

		SEGMENT_SELECTOR() { };

		SEGMENT_SELECTOR(uint16_t index, uint16_t level = KernelDPL)
		{
			PrivilegeLevel = level;
			TableIndicator = 0;
			Index = index;
		}
	};
	static_assert(sizeof(SEGMENT_SELECTOR) == sizeof(uint16_t), "Size mismatch, only 64-bit supported.");

	// Intel SDM Vol 3A Figure 3-8
	struct SEGMENT_DESCRIPTOR
	{
		union
		{
			struct
			{
				uint64_t SegmentLimit1 : 16;
				uint64_t BaseAddress1 : 16;
				uint64_t BaseAddress2 : 8;
				uint64_t Type : 4;
				uint64_t S : 1; // 1 if code/data, 0 if system segment
				uint64_t DPL : 2; //Descriptor Privilege Level
				uint64_t Present : 1;
				uint64_t SegmentLimit2 : 4;
				uint64_t Available : 1; // For use by OS
				uint64_t L : 1; //Should always be 0 for data
				uint64_t DB : 1;
				uint64_t Granulatiry : 1; // 0=1b-1mb, 1=4kb-4gb
				uint64_t BaseAddress3 : 8;
			};
			uint64_t Value;
		};
	};
	static_assert(sizeof(SEGMENT_DESCRIPTOR) == sizeof(uintptr_t), "Size mismatch, only 64-bit supported.");

	// Intel SDM Vol 3A Figure 7-4
	struct TSS_LDT_ENTRY
	{
		uint16_t SegmentLimit1;
		uint16_t BaseAddress1;

		uint16_t BaseAddress2 : 8;
		uint16_t Type : 4;
		uint16_t Zero1 : 1;
		uint16_t PrivilegeLevel : 2; // DPL
		uint16_t Present : 1;

		uint16_t Limit : 4;
		uint16_t Available : 1;
		uint16_t Zero2 : 1;
		uint16_t Zero3 : 1;
		uint16_t Granularity : 1;
		uint16_t BaseAddress3 : 8;

		uint32_t BaseAddress4;

		uint32_t Reserved1 : 8;
		uint32_t Zeros : 4;
		uint32_t Reserved2 : 20;
	};
	static_assert(sizeof(TSS_LDT_ENTRY) == 16, "Size mismatch, only 64-bit supported.");

	// Intel SDM Vol 3A Figure 7-11
	struct TASK_STATE_SEGMENT_64
	{
		uint32_t Reserved_0;
		//RSP for privilege levels 0-2
		uint32_t RSP_0_low;
		uint32_t RSP_0_high;
		uint32_t RSP_1_low;
		uint32_t RSP_1_high;
		uint32_t RSP_2_low;
		uint32_t RSP_2_high;
		uint32_t Reserved_1;
		uint32_t Reserved_2;
		//ISTs
		uint32_t IST_1_low;
		uint32_t IST_1_high;
		uint32_t IST_2_low;
		uint32_t IST_2_high;
		uint32_t IST_3_low;
		uint32_t IST_3_high;
		uint32_t IST_4_low;
		uint32_t IST_4_high;
		uint32_t IST_5_low;
		uint32_t IST_5_high;
		uint32_t IST_6_low;
		uint32_t IST_6_high;
		uint32_t IST_7_low;
		uint32_t IST_7_high;
		uint32_t Reserved_3;
		uint32_t Reserved_4;
		uint16_t Reserved_5;
		uint16_t IO_Map_Base;
	};
	static_assert(sizeof(TASK_STATE_SEGMENT_64) == 104, "Size mismatch, only 64-bit supported.");

	// Intel SDM Vol 3A Figure 6-7
	struct IDT_GATE
	{
		uint16_t Offset1;
		SEGMENT_SELECTOR SegmentSelector;
		uint16_t InterruptStackTable : 3;
		uint16_t Zeros : 5;
		uint16_t Type : 4;
		uint16_t Zero : 1;
		uint16_t PrivilegeLevel : 2; // DPL
		uint16_t Present : 1;
		uint16_t Offset2;
		uint32_t Offset3;
		uint32_t Reserved;

		IDT_GATE() {}

		IDT_GATE(uint64_t isrAddress)
		{
			Offset1 = (uint16_t)isrAddress;
			SegmentSelector.Value = 0;
			SegmentSelector.PrivilegeLevel = KernelDPL;
			SegmentSelector.Index = static_cast<uint16_t>(GDT::KernelCode);
			InterruptStackTable = 0;
			Zeros = 0;
			Type = IDT_GATE_TYPE::InterruptGate32;
			Zero = 0;
			PrivilegeLevel = KernelDPL;
			Present = true;
			Offset2 = (uint16_t)(isrAddress >> 16);
			Offset3 = (uint32_t)(isrAddress >> 32);
			Reserved = 0;
		}

		IDT_GATE(uint64_t isrAddress, uint16_t stack, IDT_GATE_TYPE type)
		{
			Offset1 = (uint16_t)isrAddress;
			SegmentSelector.Value = 0;
			SegmentSelector.PrivilegeLevel = KernelDPL;
			SegmentSelector.Index = static_cast<uint16_t>(GDT::KernelCode);
			InterruptStackTable = stack;
			Zeros = 0;
			Type = type;
			Zero = 0;
			PrivilegeLevel = KernelDPL;
			Present = true;
			Offset2 = (uint16_t)(isrAddress >> 16);
			Offset3 = (uint32_t)(isrAddress >> 32);
			Reserved = 0;
		}

		IDT_GATE(uint64_t isrAddress, uint16_t stack, IDT_GATE_TYPE type, uint16_t priv)
		{
			Offset1 = (uint16_t)isrAddress;
			SegmentSelector.Value = 0;
			SegmentSelector.PrivilegeLevel = KernelDPL;
			SegmentSelector.Index = static_cast<uint16_t>(GDT::KernelCode);
			InterruptStackTable = stack;
			Zeros = 0;
			Type = type;
			Zero = 0;
			PrivilegeLevel = priv;
			Present = true;
			Offset2 = (uint16_t)(isrAddress >> 16);
			Offset3 = (uint32_t)(isrAddress >> 32);
			Reserved = 0;
		}
	};
	static_assert(sizeof(IDT_GATE) == 16, "Size mismatch, only 64-bit supported.");

	// Intel SDM Vol 3A Figure 3-11
	struct DESCRIPTOR_TABLE
	{
		uint16_t Limit;
		uint64_t BaseAddress;
	};
	static_assert(sizeof(DESCRIPTOR_TABLE) == 10, "Size mismatch, only 64-bit supported.");

	//Modern kernel has 5 GDTs (first has to be empty, plus 2x user and 2x kernel), plus the last entry is actually a TSS entry, mandatory.
	struct KERNEL_GDTS
	{
		SEGMENT_DESCRIPTOR Empty;
		SEGMENT_DESCRIPTOR KernelCode;
		SEGMENT_DESCRIPTOR KernelData;
		SEGMENT_DESCRIPTOR UserCode32;
		SEGMENT_DESCRIPTOR UserData;
		SEGMENT_DESCRIPTOR UserCode;
		TSS_LDT_ENTRY TssEntry;
	};

	struct IA32_FMASK_REG
	{
		union
		{
			struct
			{
				uint32_t EFlagsMask;
				uint32_t Reserved;
			};
			uint64_t AsUint64;
		};
	};

	struct IA32_STAR_REG
	{
		union
		{
			struct
			{
				uint64_t Reserved : 32;
				uint64_t SyscallCS : 16; //Adds 8 to this value to get SyscallSS
				uint64_t SysretCS : 16; //Adds 8 to this value to get SysretSS;
			};
			uint64_t AsUint64;
		};
	};
#pragma pack(pop)

	KERNEL_PAGE_ALIGN static volatile uint8_t DOUBLEFAULT_STACK[];
	KERNEL_PAGE_ALIGN static volatile uint8_t NMI_Stack[];
	KERNEL_PAGE_ALIGN static volatile uint8_t DEBUG_STACK[];
	KERNEL_PAGE_ALIGN static volatile uint8_t MCE_STACK[];

	KERNEL_GLOBAL_ALIGN static volatile TASK_STATE_SEGMENT_64 TSS64;
	KERNEL_GLOBAL_ALIGN static volatile KERNEL_GDTS KernelGDT;
	KERNEL_GLOBAL_ALIGN static volatile IDT_GATE IDT[IdtCount];

	//Aligned on word boundary so address load is on correct boundary
	__declspec(align(2)) static DESCRIPTOR_TABLE GDTR;
	__declspec(align(2)) static DESCRIPTOR_TABLE IDTR;
};


//X64 Architecture structs
#pragma pack(push, 1)
struct X64_CONTEXT
{
	uint64_t R12;
	uint64_t R13;
	uint64_t R14;
	uint64_t R15;
	uint64_t Rdi;
	uint64_t Rsi;
	uint64_t Rbx;
	uint64_t Rbp;
	uint64_t Rsp;
	uint64_t Rip;
	uint64_t Rflags;
};

struct X64_SYSCALL_FRAME
{
	SystemCall SystemCall;
	uint64_t UserIP;
	uint64_t RFlags;

	//Args - TODO: find a way for the compiler to do this for us
	uint64_t Arg0;
	uint64_t Arg1;
	uint64_t Arg2;
	uint64_t Arg3;
};
#pragma pack(pop)