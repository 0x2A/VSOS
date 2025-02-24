#pragma once

#include "x64.h"
#include "kernel/hal/HAL.h"

#define DefineInterruptHandler(x) void InterruptHandler(x) ## ()

//X64 Architecture structs
#pragma pack(push, 1)
struct X64_INTERRUPT_FRAME : public INTERRUPT_FRAME
{
	//Pushed by PUSH_INTERRUPT_FRAME
	uint64_t RAX;
	uint64_t RCX;
	uint64_t RDX;
	uint64_t RBX;
	uint64_t RSI;
	uint64_t RDI;
	uint64_t R8;
	uint64_t R9;
	uint64_t R10;
	uint64_t R11;
	uint64_t R12;
	uint64_t R13;
	uint64_t R14;
	uint64_t R15;

	uint64_t FS;
	uint64_t GS;

	uint64_t RBP; //Position between automatically pushed context and additional context

	//Intel SDM Vol3A Figure 6-4
	//Pushed conditionally by CPU, ensured to exist by x64_INTERRUPT_HANDLER, 0 by default
	uint64_t ErrorCode;

	//Pushed automatically
	uint64_t RIP;
	uint64_t CS;
	uint64_t RFlags;
	uint64_t RSP;
	uint64_t SS;
};
#pragma pack(pop)

enum class X64_INTERRUPT_VECTOR : uint8_t
{
	//External Interrupts
	DivideError = 0x0,
	DebugException = 0x1,
	NMIInterrupt = 0x2,
	Breakpoint = 0x3,
	Overflow = 0x4,
	BoundRangeExceeded = 0x5,
	InvalidOpcode = 0x6,
	DeviceNotAvailable = 0x7,
	DoubleFault = 0x8,
	CoprocessorSegmentOverrun = 0x9,
	InvalidTSS = 0xA,
	SegmentNotPresent = 0xB,
	StackSegmentFault = 0xC,
	GeneralProtectionFault = 0xD,
	PageFault = 0xE,
	Reserved = 0xF,
	FPUMathFault = 0x10,
	AlignmentCheck = 0x11,
	MachineCheck = 0x12,
	SIMDException = 0x13,
	VirtualizationException = 0x14,
	ControlProtectionFault = 0x15,
	//0x16 - 0x1B reserved
	HypervisorInjectionException = 0x1C,

	//IRQs
	IRQ_BASE = 0x20,
	PITChannel0 = 0x20,
	IRQ_ERROR = 0x33,

	Timer0 = 0x80,
	COM2 = 0x83,
	COM1 = 0x84,
	HypervisorVmBus = 0x90,
};


extern "C"
{
	DefineInterruptHandler(0);
	DefineInterruptHandler(1);
	DefineInterruptHandler(2);
	DefineInterruptHandler(3);
	DefineInterruptHandler(4);
	DefineInterruptHandler(5);
	DefineInterruptHandler(6);
	DefineInterruptHandler(7);
	DefineInterruptHandler(8);
	DefineInterruptHandler(9);
	DefineInterruptHandler(10);
	DefineInterruptHandler(11);
	DefineInterruptHandler(12);
	DefineInterruptHandler(13);
	DefineInterruptHandler(14);
	DefineInterruptHandler(15);
	DefineInterruptHandler(16);
	DefineInterruptHandler(17);
	DefineInterruptHandler(18);
	DefineInterruptHandler(19);
	DefineInterruptHandler(20);
	DefineInterruptHandler(21);
	DefineInterruptHandler(22);
	DefineInterruptHandler(23);
	DefineInterruptHandler(24);
	DefineInterruptHandler(25);
	DefineInterruptHandler(26);
	DefineInterruptHandler(27);
	DefineInterruptHandler(28);
	DefineInterruptHandler(29);
	DefineInterruptHandler(30);
	DefineInterruptHandler(31);
	DefineInterruptHandler(32);
	DefineInterruptHandler(33);
	DefineInterruptHandler(34);
	DefineInterruptHandler(35);
	DefineInterruptHandler(36);
	DefineInterruptHandler(37);
	DefineInterruptHandler(38);
	DefineInterruptHandler(39);
	DefineInterruptHandler(40);
	DefineInterruptHandler(41);
	DefineInterruptHandler(42);
	DefineInterruptHandler(43);
	DefineInterruptHandler(44);
	DefineInterruptHandler(45);
	DefineInterruptHandler(46);
	DefineInterruptHandler(47);
	DefineInterruptHandler(48);
	DefineInterruptHandler(49);
	DefineInterruptHandler(50);
	DefineInterruptHandler(51);
	DefineInterruptHandler(52);
	DefineInterruptHandler(53);
	DefineInterruptHandler(54);
	DefineInterruptHandler(55);
	DefineInterruptHandler(56);
	DefineInterruptHandler(57);
	DefineInterruptHandler(58);
	DefineInterruptHandler(59);
	DefineInterruptHandler(60);
	DefineInterruptHandler(61);
	DefineInterruptHandler(62);
	DefineInterruptHandler(63);
	DefineInterruptHandler(64);
	DefineInterruptHandler(65);
	DefineInterruptHandler(66);
	DefineInterruptHandler(67);
	DefineInterruptHandler(68);
	DefineInterruptHandler(69);
	DefineInterruptHandler(70);
	DefineInterruptHandler(71);
	DefineInterruptHandler(72);
	DefineInterruptHandler(73);
	DefineInterruptHandler(74);
	DefineInterruptHandler(75);
	DefineInterruptHandler(76);
	DefineInterruptHandler(77);
	DefineInterruptHandler(78);
	DefineInterruptHandler(79);
	DefineInterruptHandler(80);
	DefineInterruptHandler(81);
	DefineInterruptHandler(82);
	DefineInterruptHandler(83);
	DefineInterruptHandler(84);
	DefineInterruptHandler(85);
	DefineInterruptHandler(86);
	DefineInterruptHandler(87);
	DefineInterruptHandler(88);
	DefineInterruptHandler(89);
	DefineInterruptHandler(90);
	DefineInterruptHandler(91);
	DefineInterruptHandler(92);
	DefineInterruptHandler(93);
	DefineInterruptHandler(94);
	DefineInterruptHandler(95);
	DefineInterruptHandler(96);
	DefineInterruptHandler(97);
	DefineInterruptHandler(98);
	DefineInterruptHandler(99);
	DefineInterruptHandler(100);
	DefineInterruptHandler(101);
	DefineInterruptHandler(102);
	DefineInterruptHandler(103);
	DefineInterruptHandler(104);
	DefineInterruptHandler(105);
	DefineInterruptHandler(106);
	DefineInterruptHandler(107);
	DefineInterruptHandler(108);
	DefineInterruptHandler(109);
	DefineInterruptHandler(110);
	DefineInterruptHandler(111);
	DefineInterruptHandler(112);
	DefineInterruptHandler(113);
	DefineInterruptHandler(114);
	DefineInterruptHandler(115);
	DefineInterruptHandler(116);
	DefineInterruptHandler(117);
	DefineInterruptHandler(118);
	DefineInterruptHandler(119);
	DefineInterruptHandler(120);
	DefineInterruptHandler(121);
	DefineInterruptHandler(122);
	DefineInterruptHandler(123);
	DefineInterruptHandler(124);
	DefineInterruptHandler(125);
	DefineInterruptHandler(126);
	DefineInterruptHandler(127);
	DefineInterruptHandler(128);
	DefineInterruptHandler(129);
	DefineInterruptHandler(130);
	DefineInterruptHandler(131);
	DefineInterruptHandler(132);
	DefineInterruptHandler(133);
	DefineInterruptHandler(134);
	DefineInterruptHandler(135);
	DefineInterruptHandler(136);
	DefineInterruptHandler(137);
	DefineInterruptHandler(138);
	DefineInterruptHandler(139);
	DefineInterruptHandler(140);
	DefineInterruptHandler(141);
	DefineInterruptHandler(142);
	DefineInterruptHandler(143);
	DefineInterruptHandler(144);
	DefineInterruptHandler(145);
	DefineInterruptHandler(146);
	DefineInterruptHandler(147);
	DefineInterruptHandler(148);
	DefineInterruptHandler(149);
	DefineInterruptHandler(150);
	DefineInterruptHandler(151);
	DefineInterruptHandler(152);
	DefineInterruptHandler(153);
	DefineInterruptHandler(154);
	DefineInterruptHandler(155);
	DefineInterruptHandler(156);
	DefineInterruptHandler(157);
	DefineInterruptHandler(158);
	DefineInterruptHandler(159);
	DefineInterruptHandler(160);
	DefineInterruptHandler(161);
	DefineInterruptHandler(162);
	DefineInterruptHandler(163);
	DefineInterruptHandler(164);
	DefineInterruptHandler(165);
	DefineInterruptHandler(166);
	DefineInterruptHandler(167);
	DefineInterruptHandler(168);
	DefineInterruptHandler(169);
	DefineInterruptHandler(170);
	DefineInterruptHandler(171);
	DefineInterruptHandler(172);
	DefineInterruptHandler(173);
	DefineInterruptHandler(174);
	DefineInterruptHandler(175);
	DefineInterruptHandler(176);
	DefineInterruptHandler(177);
	DefineInterruptHandler(178);
	DefineInterruptHandler(179);
	DefineInterruptHandler(180);
	DefineInterruptHandler(181);
	DefineInterruptHandler(182);
	DefineInterruptHandler(183);
	DefineInterruptHandler(184);
	DefineInterruptHandler(185);
	DefineInterruptHandler(186);
	DefineInterruptHandler(187);
	DefineInterruptHandler(188);
	DefineInterruptHandler(189);
	DefineInterruptHandler(190);
	DefineInterruptHandler(191);
	DefineInterruptHandler(192);
	DefineInterruptHandler(193);
	DefineInterruptHandler(194);
	DefineInterruptHandler(195);
	DefineInterruptHandler(196);
	DefineInterruptHandler(197);
	DefineInterruptHandler(198);
	DefineInterruptHandler(199);
	DefineInterruptHandler(200);
	DefineInterruptHandler(201);
	DefineInterruptHandler(202);
	DefineInterruptHandler(203);
	DefineInterruptHandler(204);
	DefineInterruptHandler(205);
	DefineInterruptHandler(206);
	DefineInterruptHandler(207);
	DefineInterruptHandler(208);
	DefineInterruptHandler(209);
	DefineInterruptHandler(210);
	DefineInterruptHandler(211);
	DefineInterruptHandler(212);
	DefineInterruptHandler(213);
	DefineInterruptHandler(214);
	DefineInterruptHandler(215);
	DefineInterruptHandler(216);
	DefineInterruptHandler(217);
	DefineInterruptHandler(218);
	DefineInterruptHandler(219);
	DefineInterruptHandler(220);
	DefineInterruptHandler(221);
	DefineInterruptHandler(222);
	DefineInterruptHandler(223);
	DefineInterruptHandler(224);
	DefineInterruptHandler(225);
	DefineInterruptHandler(226);
	DefineInterruptHandler(227);
	DefineInterruptHandler(228);
	DefineInterruptHandler(229);
	DefineInterruptHandler(230);
	DefineInterruptHandler(231);
	DefineInterruptHandler(232);
	DefineInterruptHandler(233);
	DefineInterruptHandler(234);
	DefineInterruptHandler(235);
	DefineInterruptHandler(236);
	DefineInterruptHandler(237);
	DefineInterruptHandler(238);
	DefineInterruptHandler(239);
	DefineInterruptHandler(240);
	DefineInterruptHandler(241);
	DefineInterruptHandler(242);
	DefineInterruptHandler(243);
	DefineInterruptHandler(244);
	DefineInterruptHandler(245);
	DefineInterruptHandler(246);
	DefineInterruptHandler(247);
	DefineInterruptHandler(248);
	DefineInterruptHandler(249);
	DefineInterruptHandler(250);
	DefineInterruptHandler(251);
	DefineInterruptHandler(252);
	DefineInterruptHandler(253);
	DefineInterruptHandler(254);
	DefineInterruptHandler(255);
}