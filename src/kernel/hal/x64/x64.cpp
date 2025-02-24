#include "x64.h"
#include <intrin.h>
#include "ctrlregs.h"
#include "interrupt.h"
#include <kernel\Kernel.h>

//Asm functions
extern "C" void x64_SYSTEMCALL();



// "Known good stacks" Intel SDM Vol 3A 6.14.5
volatile uint8_t x64::DOUBLEFAULT_STACK[IstStackSize] = { 0 };
volatile uint8_t x64::NMI_Stack[IstStackSize] = { 0 };
volatile uint8_t x64::DEBUG_STACK[IstStackSize] = { 0 };
volatile uint8_t x64::MCE_STACK[IstStackSize] = { 0 };


//Kernel Structures
volatile x64::TASK_STATE_SEGMENT_64 x64::TSS64 =
{
	0, //Reserved
	0, 0, //RSP 0 low/high
	0, 0, //RSP 1 low/high
	0, 0, //RSP 2 low/high
	0, 0, //Reserved
	QWordLow(DOUBLEFAULT_STACK + IstStackSize), QWordHigh(DOUBLEFAULT_STACK + IstStackSize), //IST1
	QWordLow(NMI_Stack + IstStackSize), QWordHigh(NMI_Stack + IstStackSize), //IST2
	QWordLow(DEBUG_STACK + IstStackSize), QWordHigh(DEBUG_STACK + IstStackSize), //IST3
	QWordLow(MCE_STACK + IstStackSize), QWordHigh(MCE_STACK + IstStackSize), //IST4
	0, 0, //IST5
	0, 0, //IST6
	0, 0, //IST7
	0, 0, 0, //Reserved
	0 //IO Map Base
};

#define GDT_TYPE_RW 0x2
#define GDT_TYPE_EX 0x8

//1 empty, 4 GDTs, and 1 TSS
volatile x64::KERNEL_GDTS x64::KernelGDT =
{
	{ 0 }, //First entry has to be empty
	// Seg1   Base  type						  S    DPL		   P    Seg2   OS      L     DB   4GB   Base
	{ 0xFFFF, 0, 0, GDT_TYPE_RW | GDT_TYPE_EX,    true, KernelDPL, true, 0xF, false, true, false, true, 0x00 }, //64-bit code Kernel
	{ 0xFFFF, 0, 0, GDT_TYPE_RW,                  true, KernelDPL, true, 0xF, false, false, true, true, 0x00 }, //64-bit data Kernel
	{ 0xFFFF, 0, 0, GDT_TYPE_RW | GDT_TYPE_EX,    true, UserDPL,   true, 0xF, false, false, true, true, 0x00 }, //32-bit code User
	{ 0xFFFF, 0, 0, GDT_TYPE_RW,                  true, UserDPL,   true, 0xF, false, false, true, true, 0x00 }, //64-bit data User
	{ 0xFFFF, 0, 0, GDT_TYPE_RW | GDT_TYPE_EX,    true, UserDPL,   true, 0xF, false, true, false, true, 0x00 }, //64-bit code User
	{
		// Seg1				Base1			Base2							type  S    DPL  P   Seg2	OS      L   DB     4GB   Base3
		sizeof(x64::TSS64) - 1, (uint16_t)&x64::TSS64, (uint8_t)((uint64_t)&x64::TSS64 >> 16), 0x9, false, 0, true, 0, false, false, false, true, (uint8_t)((uint64_t)&x64::TSS64 >> 24),
		// Base4						Zeroes
		QWordHigh(&x64::TSS64), 0, 0, 0
	}
};

//cs = starhigh + 2
//ss = starhigh + 1
//

volatile x64::IDT_GATE x64::IDT[IdtCount] =
{
	IDT_GATE((uint64_t)&InterruptHandler(0)),
	IDT_GATE((uint64_t)&InterruptHandler(1)),
	IDT_GATE((uint64_t)&InterruptHandler(2), IST_NMI_IDX, IDT_GATE_TYPE::InterruptGate32),
	IDT_GATE((uint64_t)&InterruptHandler(3), IST_DEBUG_IDX, IDT_GATE_TYPE::InterruptGate32, UserDPL),
	IDT_GATE((uint64_t)&InterruptHandler(4)),
	IDT_GATE((uint64_t)&InterruptHandler(5)),
	IDT_GATE((uint64_t)&InterruptHandler(6)),
	IDT_GATE((uint64_t)&InterruptHandler(7)),
	IDT_GATE((uint64_t)&InterruptHandler(8), IST_DOUBLEFAULT_IDX, IDT_GATE_TYPE::InterruptGate32),
	IDT_GATE((uint64_t)&InterruptHandler(9)),
	IDT_GATE((uint64_t)&InterruptHandler(10)),
	IDT_GATE((uint64_t)&InterruptHandler(11)),
	IDT_GATE((uint64_t)&InterruptHandler(12)),
	IDT_GATE((uint64_t)&InterruptHandler(13)),
	IDT_GATE((uint64_t)&InterruptHandler(14)),
	IDT_GATE((uint64_t)&InterruptHandler(15)),
	IDT_GATE((uint64_t)&InterruptHandler(16)),
	IDT_GATE((uint64_t)&InterruptHandler(17)),
	IDT_GATE((uint64_t)&InterruptHandler(18), IST_MCE_IDX, IDT_GATE_TYPE::InterruptGate32),
	IDT_GATE((uint64_t)&InterruptHandler(19)),
	IDT_GATE((uint64_t)&InterruptHandler(20)),
	IDT_GATE((uint64_t)&InterruptHandler(21)),
	IDT_GATE((uint64_t)&InterruptHandler(22)),
	IDT_GATE((uint64_t)&InterruptHandler(23)),
	IDT_GATE((uint64_t)&InterruptHandler(24)),
	IDT_GATE((uint64_t)&InterruptHandler(25)),
	IDT_GATE((uint64_t)&InterruptHandler(26)),
	IDT_GATE((uint64_t)&InterruptHandler(27)),
	IDT_GATE((uint64_t)&InterruptHandler(28)),
	IDT_GATE((uint64_t)&InterruptHandler(29)),
	IDT_GATE((uint64_t)&InterruptHandler(30)),
	IDT_GATE((uint64_t)&InterruptHandler(31)),
	IDT_GATE((uint64_t)&InterruptHandler(32)),
	IDT_GATE((uint64_t)&InterruptHandler(33)),
	IDT_GATE((uint64_t)&InterruptHandler(34)),
	IDT_GATE((uint64_t)&InterruptHandler(35)),
	IDT_GATE((uint64_t)&InterruptHandler(36)),
	IDT_GATE((uint64_t)&InterruptHandler(37)),
	IDT_GATE((uint64_t)&InterruptHandler(38)),
	IDT_GATE((uint64_t)&InterruptHandler(39)),
	IDT_GATE((uint64_t)&InterruptHandler(40)),
	IDT_GATE((uint64_t)&InterruptHandler(41)),
	IDT_GATE((uint64_t)&InterruptHandler(42)),
	IDT_GATE((uint64_t)&InterruptHandler(43)),
	IDT_GATE((uint64_t)&InterruptHandler(44)),
	IDT_GATE((uint64_t)&InterruptHandler(45)),
	IDT_GATE((uint64_t)&InterruptHandler(46)),
	IDT_GATE((uint64_t)&InterruptHandler(47)),
	IDT_GATE((uint64_t)&InterruptHandler(48)),
	IDT_GATE((uint64_t)&InterruptHandler(49)),
	IDT_GATE((uint64_t)&InterruptHandler(50)),
	IDT_GATE((uint64_t)&InterruptHandler(51)),
	IDT_GATE((uint64_t)&InterruptHandler(52)),
	IDT_GATE((uint64_t)&InterruptHandler(53)),
	IDT_GATE((uint64_t)&InterruptHandler(54)),
	IDT_GATE((uint64_t)&InterruptHandler(55)),
	IDT_GATE((uint64_t)&InterruptHandler(56)),
	IDT_GATE((uint64_t)&InterruptHandler(57)),
	IDT_GATE((uint64_t)&InterruptHandler(58)),
	IDT_GATE((uint64_t)&InterruptHandler(59)),
	IDT_GATE((uint64_t)&InterruptHandler(60)),
	IDT_GATE((uint64_t)&InterruptHandler(61)),
	IDT_GATE((uint64_t)&InterruptHandler(62)),
	IDT_GATE((uint64_t)&InterruptHandler(63)),
	IDT_GATE((uint64_t)&InterruptHandler(64)),
	IDT_GATE((uint64_t)&InterruptHandler(65)),
	IDT_GATE((uint64_t)&InterruptHandler(66)),
	IDT_GATE((uint64_t)&InterruptHandler(67)),
	IDT_GATE((uint64_t)&InterruptHandler(68)),
	IDT_GATE((uint64_t)&InterruptHandler(69)),
	IDT_GATE((uint64_t)&InterruptHandler(70)),
	IDT_GATE((uint64_t)&InterruptHandler(71)),
	IDT_GATE((uint64_t)&InterruptHandler(72)),
	IDT_GATE((uint64_t)&InterruptHandler(73)),
	IDT_GATE((uint64_t)&InterruptHandler(74)),
	IDT_GATE((uint64_t)&InterruptHandler(75)),
	IDT_GATE((uint64_t)&InterruptHandler(76)),
	IDT_GATE((uint64_t)&InterruptHandler(77)),
	IDT_GATE((uint64_t)&InterruptHandler(78)),
	IDT_GATE((uint64_t)&InterruptHandler(79)),
	IDT_GATE((uint64_t)&InterruptHandler(80)),
	IDT_GATE((uint64_t)&InterruptHandler(81)),
	IDT_GATE((uint64_t)&InterruptHandler(82)),
	IDT_GATE((uint64_t)&InterruptHandler(83)),
	IDT_GATE((uint64_t)&InterruptHandler(84)),
	IDT_GATE((uint64_t)&InterruptHandler(85)),
	IDT_GATE((uint64_t)&InterruptHandler(86)),
	IDT_GATE((uint64_t)&InterruptHandler(87)),
	IDT_GATE((uint64_t)&InterruptHandler(88)),
	IDT_GATE((uint64_t)&InterruptHandler(89)),
	IDT_GATE((uint64_t)&InterruptHandler(90)),
	IDT_GATE((uint64_t)&InterruptHandler(91)),
	IDT_GATE((uint64_t)&InterruptHandler(92)),
	IDT_GATE((uint64_t)&InterruptHandler(93)),
	IDT_GATE((uint64_t)&InterruptHandler(94)),
	IDT_GATE((uint64_t)&InterruptHandler(95)),
	IDT_GATE((uint64_t)&InterruptHandler(96)),
	IDT_GATE((uint64_t)&InterruptHandler(97)),
	IDT_GATE((uint64_t)&InterruptHandler(98)),
	IDT_GATE((uint64_t)&InterruptHandler(99)),
	IDT_GATE((uint64_t)&InterruptHandler(100)),
	IDT_GATE((uint64_t)&InterruptHandler(101)),
	IDT_GATE((uint64_t)&InterruptHandler(102)),
	IDT_GATE((uint64_t)&InterruptHandler(103)),
	IDT_GATE((uint64_t)&InterruptHandler(104)),
	IDT_GATE((uint64_t)&InterruptHandler(105)),
	IDT_GATE((uint64_t)&InterruptHandler(106)),
	IDT_GATE((uint64_t)&InterruptHandler(107)),
	IDT_GATE((uint64_t)&InterruptHandler(108)),
	IDT_GATE((uint64_t)&InterruptHandler(109)),
	IDT_GATE((uint64_t)&InterruptHandler(110)),
	IDT_GATE((uint64_t)&InterruptHandler(111)),
	IDT_GATE((uint64_t)&InterruptHandler(112)),
	IDT_GATE((uint64_t)&InterruptHandler(113)),
	IDT_GATE((uint64_t)&InterruptHandler(114)),
	IDT_GATE((uint64_t)&InterruptHandler(115)),
	IDT_GATE((uint64_t)&InterruptHandler(116)),
	IDT_GATE((uint64_t)&InterruptHandler(117)),
	IDT_GATE((uint64_t)&InterruptHandler(118)),
	IDT_GATE((uint64_t)&InterruptHandler(119)),
	IDT_GATE((uint64_t)&InterruptHandler(120)),
	IDT_GATE((uint64_t)&InterruptHandler(121)),
	IDT_GATE((uint64_t)&InterruptHandler(122)),
	IDT_GATE((uint64_t)&InterruptHandler(123)),
	IDT_GATE((uint64_t)&InterruptHandler(124)),
	IDT_GATE((uint64_t)&InterruptHandler(125)),
	IDT_GATE((uint64_t)&InterruptHandler(126)),
	IDT_GATE((uint64_t)&InterruptHandler(127)),
	IDT_GATE((uint64_t)&InterruptHandler(128)),
	IDT_GATE((uint64_t)&InterruptHandler(129)),
	IDT_GATE((uint64_t)&InterruptHandler(130)),
	IDT_GATE((uint64_t)&InterruptHandler(131)),
	IDT_GATE((uint64_t)&InterruptHandler(132)),
	IDT_GATE((uint64_t)&InterruptHandler(133)),
	IDT_GATE((uint64_t)&InterruptHandler(134)),
	IDT_GATE((uint64_t)&InterruptHandler(135)),
	IDT_GATE((uint64_t)&InterruptHandler(136)),
	IDT_GATE((uint64_t)&InterruptHandler(137)),
	IDT_GATE((uint64_t)&InterruptHandler(138)),
	IDT_GATE((uint64_t)&InterruptHandler(139)),
	IDT_GATE((uint64_t)&InterruptHandler(140)),
	IDT_GATE((uint64_t)&InterruptHandler(141)),
	IDT_GATE((uint64_t)&InterruptHandler(142)),
	IDT_GATE((uint64_t)&InterruptHandler(143)),
	IDT_GATE((uint64_t)&InterruptHandler(144)),
	IDT_GATE((uint64_t)&InterruptHandler(145)),
	IDT_GATE((uint64_t)&InterruptHandler(146)),
	IDT_GATE((uint64_t)&InterruptHandler(147)),
	IDT_GATE((uint64_t)&InterruptHandler(148)),
	IDT_GATE((uint64_t)&InterruptHandler(149)),
	IDT_GATE((uint64_t)&InterruptHandler(150)),
	IDT_GATE((uint64_t)&InterruptHandler(151)),
	IDT_GATE((uint64_t)&InterruptHandler(152)),
	IDT_GATE((uint64_t)&InterruptHandler(153)),
	IDT_GATE((uint64_t)&InterruptHandler(154)),
	IDT_GATE((uint64_t)&InterruptHandler(155)),
	IDT_GATE((uint64_t)&InterruptHandler(156)),
	IDT_GATE((uint64_t)&InterruptHandler(157)),
	IDT_GATE((uint64_t)&InterruptHandler(158)),
	IDT_GATE((uint64_t)&InterruptHandler(159)),
	IDT_GATE((uint64_t)&InterruptHandler(160)),
	IDT_GATE((uint64_t)&InterruptHandler(161)),
	IDT_GATE((uint64_t)&InterruptHandler(162)),
	IDT_GATE((uint64_t)&InterruptHandler(163)),
	IDT_GATE((uint64_t)&InterruptHandler(164)),
	IDT_GATE((uint64_t)&InterruptHandler(165)),
	IDT_GATE((uint64_t)&InterruptHandler(166)),
	IDT_GATE((uint64_t)&InterruptHandler(167)),
	IDT_GATE((uint64_t)&InterruptHandler(168)),
	IDT_GATE((uint64_t)&InterruptHandler(169)),
	IDT_GATE((uint64_t)&InterruptHandler(170)),
	IDT_GATE((uint64_t)&InterruptHandler(171)),
	IDT_GATE((uint64_t)&InterruptHandler(172)),
	IDT_GATE((uint64_t)&InterruptHandler(173)),
	IDT_GATE((uint64_t)&InterruptHandler(174)),
	IDT_GATE((uint64_t)&InterruptHandler(175)),
	IDT_GATE((uint64_t)&InterruptHandler(176)),
	IDT_GATE((uint64_t)&InterruptHandler(177)),
	IDT_GATE((uint64_t)&InterruptHandler(178)),
	IDT_GATE((uint64_t)&InterruptHandler(179)),
	IDT_GATE((uint64_t)&InterruptHandler(180)),
	IDT_GATE((uint64_t)&InterruptHandler(181)),
	IDT_GATE((uint64_t)&InterruptHandler(182)),
	IDT_GATE((uint64_t)&InterruptHandler(183)),
	IDT_GATE((uint64_t)&InterruptHandler(184)),
	IDT_GATE((uint64_t)&InterruptHandler(185)),
	IDT_GATE((uint64_t)&InterruptHandler(186)),
	IDT_GATE((uint64_t)&InterruptHandler(187)),
	IDT_GATE((uint64_t)&InterruptHandler(188)),
	IDT_GATE((uint64_t)&InterruptHandler(189)),
	IDT_GATE((uint64_t)&InterruptHandler(190)),
	IDT_GATE((uint64_t)&InterruptHandler(191)),
	IDT_GATE((uint64_t)&InterruptHandler(192)),
	IDT_GATE((uint64_t)&InterruptHandler(193)),
	IDT_GATE((uint64_t)&InterruptHandler(194)),
	IDT_GATE((uint64_t)&InterruptHandler(195)),
	IDT_GATE((uint64_t)&InterruptHandler(196)),
	IDT_GATE((uint64_t)&InterruptHandler(197)),
	IDT_GATE((uint64_t)&InterruptHandler(198)),
	IDT_GATE((uint64_t)&InterruptHandler(199)),
	IDT_GATE((uint64_t)&InterruptHandler(200)),
	IDT_GATE((uint64_t)&InterruptHandler(201)),
	IDT_GATE((uint64_t)&InterruptHandler(202)),
	IDT_GATE((uint64_t)&InterruptHandler(203)),
	IDT_GATE((uint64_t)&InterruptHandler(204)),
	IDT_GATE((uint64_t)&InterruptHandler(205)),
	IDT_GATE((uint64_t)&InterruptHandler(206)),
	IDT_GATE((uint64_t)&InterruptHandler(207)),
	IDT_GATE((uint64_t)&InterruptHandler(208)),
	IDT_GATE((uint64_t)&InterruptHandler(209)),
	IDT_GATE((uint64_t)&InterruptHandler(210)),
	IDT_GATE((uint64_t)&InterruptHandler(211)),
	IDT_GATE((uint64_t)&InterruptHandler(212)),
	IDT_GATE((uint64_t)&InterruptHandler(213)),
	IDT_GATE((uint64_t)&InterruptHandler(214)),
	IDT_GATE((uint64_t)&InterruptHandler(215)),
	IDT_GATE((uint64_t)&InterruptHandler(216)),
	IDT_GATE((uint64_t)&InterruptHandler(217)),
	IDT_GATE((uint64_t)&InterruptHandler(218)),
	IDT_GATE((uint64_t)&InterruptHandler(219)),
	IDT_GATE((uint64_t)&InterruptHandler(220)),
	IDT_GATE((uint64_t)&InterruptHandler(221)),
	IDT_GATE((uint64_t)&InterruptHandler(222)),
	IDT_GATE((uint64_t)&InterruptHandler(223)),
	IDT_GATE((uint64_t)&InterruptHandler(224)),
	IDT_GATE((uint64_t)&InterruptHandler(225)),
	IDT_GATE((uint64_t)&InterruptHandler(226)),
	IDT_GATE((uint64_t)&InterruptHandler(227)),
	IDT_GATE((uint64_t)&InterruptHandler(228)),
	IDT_GATE((uint64_t)&InterruptHandler(229)),
	IDT_GATE((uint64_t)&InterruptHandler(230)),
	IDT_GATE((uint64_t)&InterruptHandler(231)),
	IDT_GATE((uint64_t)&InterruptHandler(232)),
	IDT_GATE((uint64_t)&InterruptHandler(233)),
	IDT_GATE((uint64_t)&InterruptHandler(234)),
	IDT_GATE((uint64_t)&InterruptHandler(235)),
	IDT_GATE((uint64_t)&InterruptHandler(236)),
	IDT_GATE((uint64_t)&InterruptHandler(237)),
	IDT_GATE((uint64_t)&InterruptHandler(238)),
	IDT_GATE((uint64_t)&InterruptHandler(239)),
	IDT_GATE((uint64_t)&InterruptHandler(240)),
	IDT_GATE((uint64_t)&InterruptHandler(241)),
	IDT_GATE((uint64_t)&InterruptHandler(242)),
	IDT_GATE((uint64_t)&InterruptHandler(243)),
	IDT_GATE((uint64_t)&InterruptHandler(244)),
	IDT_GATE((uint64_t)&InterruptHandler(245)),
	IDT_GATE((uint64_t)&InterruptHandler(246)),
	IDT_GATE((uint64_t)&InterruptHandler(247)),
	IDT_GATE((uint64_t)&InterruptHandler(248)),
	IDT_GATE((uint64_t)&InterruptHandler(249)),
	IDT_GATE((uint64_t)&InterruptHandler(250)),
	IDT_GATE((uint64_t)&InterruptHandler(251)),
	IDT_GATE((uint64_t)&InterruptHandler(252)),
	IDT_GATE((uint64_t)&InterruptHandler(253)),
	IDT_GATE((uint64_t)&InterruptHandler(254)),
	IDT_GATE((uint64_t)&InterruptHandler(255))
};

x64::DESCRIPTOR_TABLE x64::GDTR =
{
	sizeof(KernelGDT) - 1,
	(uint64_t)&KernelGDT
};

x64::DESCRIPTOR_TABLE x64::IDTR =
{
	sizeof(IDT) - 1,
	(uint64_t)IDT
};


void x64::SetupDescriptorTables()
{
#if _VERBOSE_
	Printf(__FUNCTION__ "\r\n");
#endif
	//Load new segments
	_lgdt(&GDTR);

#if _VERBOSE_
	Printf(__FUNCTION__ ": GDT loaded\r\n");
#endif
	//Update segment registers
	const SEGMENT_SELECTOR dataSelector(static_cast<uint16_t>(GDT::KernelData));
	const SEGMENT_SELECTOR codeSelector(static_cast<uint16_t>(GDT::KernelCode));
	UpdateSegments(dataSelector.Value, codeSelector.Value);	

	const SEGMENT_SELECTOR tssSelector(static_cast<uint16_t>(GDT::TssEntry));
	_ltr(tssSelector.Value);

#if _VERBOSE_
	Printf(__FUNCTION__ ": TSS selector loaded\r\n");
#endif
	//Load interrupt handlers
	__lidt(&IDTR);

#if _VERBOSE_
	Printf(__FUNCTION__ ": IDT loaded\r\n");
#endif
	//init PIT
	//InitPIC();

	//Enable interrupts
	_sti();	

#if _VERBOSE_
	Printf(__FUNCTION__ ": Interrupts enabled\r\n");
#endif
	//Enable syscalls
	const SEGMENT_SELECTOR userCodeSelector(static_cast<uint16_t>(GDT::User32Code), UserDPL);
	const IA32_STAR_REG starReg = { {0, codeSelector.Value, userCodeSelector.Value } };
	__writemsr(static_cast<uint32_t>(MSR::IA32_STAR), starReg.AsUint64);
	__writemsr(static_cast<uint32_t>(MSR::IA32_LSTAR), (uintptr_t)&x64_SYSTEMCALL);
	__writemsr(static_cast<uint32_t>(MSR::IA32_FMASK), 0x200 | 0x100); //Disable interrupts and traps

#if _VERBOSE_
	Printf(__FUNCTION__ ": syscalls prepared\r\n");
#endif

	//Enable syscall
	const size_t value = __readmsr(static_cast<uint32_t>(MSR::IA32_EFER));
	__writemsr(static_cast<uint32_t>(MSR::IA32_EFER), value | 1);

#if _VERBOSE_
	Printf(__FUNCTION__ ": syscalls enabled\r\n");
#endif

	//Enable WRGSBASE instruction
	//__writecr4(__readcr4() | (1 << 16));
	
	
	int regs[4];
	__cpuid(regs, 0x15); //get TSC frequency
	if (regs[2] == 0)  //On some processors (e.g. Intel Skylake), CPUID_15h_ECX is zero but CPUID_16h_EAX is present
	{
		int eax = regs[0];
		int ebx = regs[1];
		__cpuid(regs, 0x16);

		if(regs[0] != 0 && regs[1] != 0 && eax != 0 && ebx != 0)
			TSCFreq = (regs[0] * 10000000) * (eax/ebx);
	}
	else if (regs[1] != 0 && regs[2] != 0)
	{
		TSCFreq = regs[2]* (regs[1]/regs[0]); // ECX * (EBX/EAX)
	}

	//fallback on Intel
	if(TSCFreq == 0)
	{
		uint64_t platform_info = __readmsr((uint32_t)MSR::IA32_MSR_PLATFORM_INFO);
		TSCFreq = ((platform_info >> 8) && 0xFF) * 100;
	}

#if _VERBOSE_
	Printf(__FUNCTION__ ": TSCFreq read: %d\r\n", TSCFreq);
#endif
}

void x64::SetUserCpuContext(void* teb)
{
	_writegsbase_u64((uintptr_t)teb);
}

void x64::SetKernelInterruptStack(void* stack)
{
	TSS64.RSP_0_low = QWordLow(stack);
	TSS64.RSP_0_high = QWordHigh(stack);
}


// ------------------------------------------------------------------------------------------------
#define PIC1_CMD                        0x0020
#define PIC1_DATA                       0x0021
#define PIC2_CMD                        0x00a0
#define PIC2_DATA                       0x00a1

#define ICW1_ICW4                       0x01        // ICW4 command word: 0 = not needed, 1 = needed
#define ICW1_SINGLE                     0x02        // Single mode: 0 = cascade, 1 = single
#define ICW1_ADI                        0x04        // Call address interval: 0 = interval of 8, 1 = interval of 4
#define ICW1_LTIM                       0x08        // Interrupt trigger mode: 0 = edge, 1 = level
#define ICW1_INIT                       0x10        // Initialization

#define ICW4_8086                       0x01        // Microprocessor mode: 0=MCS-80/85, 1=8086/8088
#define ICW4_AUTO                       0x02        // Auto EOI: 0 = disabled, 1 = enabled
#define ICW4_BUF_SLAVE                  0x04        // Buffered mode/slave
#define ICW4_BUF_MASTER                 0x0C        // Buffered mode/master
#define ICW4_SFNM                       0x10        // Special fully nested is programmed

// ------------------------------------------------------------------------------------------------
// I/O Ports

#define PIT_COUNTER0                    0x40
#define PIT_CMD                         0x43

// ------------------------------------------------------------------------------------------------
// Command Register

// BCD
#define CMD_BINARY                      0x00    // Use Binary counter values
#define CMD_BCD                         0x01    // Use Binary Coded Decimal counter values

// Mode
#define CMD_MODE0                       0x00    // Interrupt on Terminal Count
#define CMD_MODE1                       0x02    // Hardware Retriggerable One-Shot
#define CMD_MODE2                       0x04    // Rate Generator
#define CMD_MODE3                       0x06    // Square Wave
#define CMD_MODE4                       0x08    // Software Trigerred Strobe
#define CMD_MODE5                       0x0a    // Hardware Trigerred Strobe

// Read/Write
#define CMD_LATCH                       0x00
#define CMD_RW_LOW                      0x10    // Least Significant Byte
#define CMD_RW_HI                       0x20    // Most Significant Byte
#define CMD_RW_BOTH                     0x30    // Least followed by Most Significant Byte

// Counter Select
#define CMD_COUNTER0                    0x00
#define CMD_COUNTER1                    0x40
#define CMD_COUNTER2                    0x80
#define CMD_READBACK                    0xc0

// ------------------------------------------------------------------------------------------------

#define PIT_FREQUENCY                   1193182

void x64::InitPIT()
{

	uint32_t hz = 1000;
	uint32_t divisor = PIT_FREQUENCY / hz;

	__outbyte(PIT_CMD, CMD_BINARY |CMD_MODE3 | CMD_RW_BOTH | CMD_COUNTER0);
	__outbyte(PIT_COUNTER0, divisor);
	__outbyte(PIT_COUNTER0, divisor >> 8);
}


uint64_t x64::ReadTSC()
{
	//Assert(HasEDXFeature(EDX_TSC));
	

	return __rdtsc();
}

bool x64::SupportsX2APIC()
{
	int regs[4];
	__cpuid(regs, 0x01);

	if(regs[2] & (1 < 21))
		return true;
	return false;
}

void x64::EnableFSGSBASE()
{
	//Enable WRGSBASE instruction
	__writecr4(__readcr4() | (1 << 16));
}

void x64::InitPIC()
{
	// ICW1: start initialization, ICW4 needed
	__outbyte(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
	__outbyte(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

	// ICW2: interrupt vector address
	__outbyte(PIC1_DATA, (uint8_t)X64_INTERRUPT_VECTOR::IRQ_BASE);
	__outbyte(PIC2_DATA, (uint8_t)X64_INTERRUPT_VECTOR::IRQ_BASE + 8);

	// ICW3: master/slave wiring
	__outbyte(PIC1_DATA, 4);
	__outbyte(PIC2_DATA, 2);

	// ICW4: 8086 mode, not special fully nested, not buffered, normal EOI
	__outbyte(PIC1_DATA, ICW4_8086);
	__outbyte(PIC2_DATA, ICW4_8086);

	// OCW1: Disable all IRQs
	__outbyte(PIC1_DATA, 0xff);
	__outbyte(PIC2_DATA, 0xff);
}

volatile uint32_t x64::g_pitTicks = 0;

uint32_t x64::TSCFreq = 0;

uint32_t x64::OnPITTimer0(void* arg)
{
	g_pitTicks++;
	Printf("Timer tick\r\n");

	return 0;
}

bool x64::HasMSR()
{	
	return HasEDXFeature(EDX_MSR);
}

bool x64::HasEDXFeature(CPU_FEATURE f)
{
	int regs[4];
	__cpuid(regs, 1);
	return regs[3] & f;
}

bool x64::HasECXFeature(CPU_FEATURE f)
{
	int regs[4];
	__cpuid(regs, 1);
	return regs[2] & f;
}

