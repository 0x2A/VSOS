#include "LocalAPIC.h"
#include <Assert.h>
#include <os.internal.h>
#include "kernel/Kernel.h"
#include "kernel/hal/HAL.h"

// ------------------------------------------------------------------------------------------------
// Local APIC Registers
#define LAPIC_ID                        0x0020  // Local APIC ID
#define LAPIC_VER                       0x0030  // Local APIC Version
#define LAPIC_TPR                       0x0080  // Task Priority
#define LAPIC_APR                       0x0090  // Arbitration Priority
#define LAPIC_PPR                       0x00a0  // Processor Priority
#define LAPIC_EOI                       0x00b0  // EOI
#define LAPIC_RRD                       0x00c0  // Remote Read
#define LAPIC_LDR                       0x00d0  // Logical Destination
#define LAPIC_DFR                       0x00e0  // Destination Format
#define LAPIC_SVR                       0x00f0  // Spurious Interrupt Vector
#define LAPIC_ISR                       0x0100  // In-Service (8 registers)
#define LAPIC_TMR                       0x0180  // Trigger Mode (8 registers)
#define LAPIC_IRR                       0x0200  // Interrupt Request (8 registers)
#define LAPIC_ESR                       0x0280  // Error Status
#define LAPIC_ICRLO                     0x0300  // Interrupt Command
#define LAPIC_ICRHI                     0x0310  // Interrupt Command [63:32]

#define LAPIC_TIMER                     0x0320  // LVT Timer
#define LAPIC_THERMAL                   0x0330  // LVT Thermal Sensor
#define LAPIC_PERF                      0x0340  // LVT Performance Counter
#define LAPIC_LINT0                     0x0350  // LVT LINT0
#define LAPIC_LINT1                     0x0360  // LVT LINT1
#define LAPIC_ERROR                     0x0370  // LVT Error

#define LAPIC_TICR                      0x0380  // Initial Count (for Timer)
#define LAPIC_TCCR                      0x0390  // Current Count (for Timer)
#define LAPIC_TDCR                      0x03e0  // Divide Configuration (for Timer)

// ------------------------------------------------------------------------------------------------
// Interrupt Command Register

// Delivery Mode
#define ICR_FIXED                       0x00000000
#define ICR_LOWEST                      0x00000100
#define ICR_SMI                         0x00000200
#define ICR_NMI                         0x00000400
#define ICR_INIT                        0x00000500
#define ICR_STARTUP                     0x00000600

// Destination Mode
#define ICR_PHYSICAL                    0x00000000
#define ICR_LOGICAL                     0x00000800

// Delivery Status
#define ICR_IDLE                        0x00000000
#define ICR_SEND_PENDING                0x00001000

// Level
#define ICR_DEASSERT                    0x00000000
#define ICR_ASSERT                      0x00004000

// Trigger Mode
#define ICR_EDGE                        0x00000000
#define ICR_LEVEL                       0x00008000

// Destination Shorthand
#define ICR_NO_SHORTHAND                0x00000000
#define ICR_SELF                        0x00040000
#define ICR_ALL_INCLUDING_SELF          0x00080000
#define ICR_ALL_EXCLUDING_SELF          0x000c0000

// Destination Field
#define ICR_DESTINATION_SHIFT           24

#define APIC_DISABLE            (1 << 16)
#define LAPIC_TIMER_UNMASK          0xFFFEFFFF
#define APIC_NMI	 (4<<8)
#define LAPIC_TIMER_PERIODIC        (1 << 17)


#define PIT_OUTPUT_CHANNEL2	0x61
#define PIT_CHANNEL2_DATA	0x42
#define PIT_COMMAND	0x43



LocalAPIC::LocalAPIC(HAL* hal)
: m_HAL(hal), x2Apic(false), Device(), m_Addr(0)
{
	Name = "LAPIC";
	Description = "LocalAPIC";
	Path = "ACPI/LAPIC";
	Type = DeviceType::System;
}


void LocalAPIC::Initialize(void* context)
{
	Assert(m_HAL);
	Assert(context);

	x2Apic = false;//x64::SupportsX2APIC();
	
	
	m_Addr = (uint64_t)context;
	m_PhysicalAddr = m_Addr;

	if (x2Apic)
	{
		// Enable x2APIC
		uint64_t msr_info = __readmsr(0x1B);
		Printf(__FUNCTION__": 0x%16x\r\n", msr_info);
		msr_info |= (1 << 10);
		__writemsr(0x1B, __readmsr(0x1B) | 0xc00);
		Printf(__FUNCTION__": x2APIC Enabled\r\n");
		return;
	}
	else
	{
		m_Addr = (uint64_t)kernel.VirtualMapRT(0x0, { m_Addr });
	}
		
	
	// Get the vector table
	uint32_t spurious_vector = read(LAPIC_SVR);
	//Printf("APIC Spurious Vector: 0x%x\n", spurious_vector & 0xFF);

	// Enable the APIC; set spurious interrupt vector.
	write(LAPIC_SVR, (1 << 8) | 0x100);
	//Printf("LAPIC Enabled\n");

	// Read the APIC version
	uint32_t version = read(LAPIC_VER);
	//Printf("LAPIC Version: 0x%x\n", version & 0xFF);
	Printf("Local APIC Addr: 0x%16x, ver: 0x%x\r\n", m_Addr, version);

	//write(LAPIC_DFR, 0xffffffff); //flat mode
	//write(LAPIC_LDR, 0x01000000); // All cpus use logical id 1

	// The timer repeatedly counts down at bus frequency
	// from lapic[TICR] and then issues an interrupt.  
	// If we cared more about precise timekeeping,
	// TICR would be calibrated using an external time source.
	/* Set the divisor to 16 */
	write(LAPIC_TDCR, 0b11);


	CalibrateTimer();
	/* Set the inital count to the calibration */
	write(LAPIC_TICR, apicCalibVal);


	/* Set the timer interrupt vector and put the timer into periodic mode */
	write(LAPIC_TIMER, (uint8_t)X64_INTERRUPT_VECTOR::Timer0 | LAPIC_TIMER_PERIODIC);

	// Leave LINT0 of the BSP enabled so that it can get
	// interrupts from the 8259A chip.
	//
	// According to Intel MP Specification, the BIOS should initialize
	// BSP's local APIC in Virtual Wire Mode, in which 8259A's
	// INTR is virtually connected to BSP's LINTIN0. In this mode,
	// we do not need to program the IOAPIC.
	if (id() != BOOT_CPU)
	{
		write(LAPIC_LINT0, 0x00010000);
	}

	// Disable NMI (LINT1) on all CPUs
	write(LAPIC_LINT1, APIC_DISABLE);

	// Disable performance counter overflow interrupts
	// on machines that provide that interrupt entry.
	if (((version >> 16) & 0xFF) >= 4)
		write(LAPIC_PERF, APIC_DISABLE);

	// Map error interrupt to IRQ_ERROR.
	write(LAPIC_ERROR, (uint8_t)X64_INTERRUPT_VECTOR::IRQ_ERROR);

	// Clear error status register (requires back-to-back writes).
	write(LAPIC_ESR, 0);
	write(LAPIC_ESR, 0);

	// Ack any outstanding interrupts.
	write(LAPIC_EOI, 0);

	// Send an Init Level De-Assert to synchronize arbitration ID's.
	//write(LAPIC_ICRHI, 0);
	//write(LAPIC_ICRLO, ICR_ALL_INCLUDING_SELF | ICR_INIT | ICR_LEVEL);
	//while (read(LAPIC_ICRLO) & ICR_SEND_PENDING)
//		;

	// Enable interrupts on the APIC (but not on the processor).
	write(LAPIC_TPR, 0);

	Printf("LAPIC Initialized\n");
}



const void* LocalAPIC::GetResource(uint32_t type) const
{
	return nullptr;
}


void LocalAPIC::SignalEOI()
{
	eoi_required = false;
	write(LAPIC_EOI, 0);
}

int LocalAPIC::EOIPending()
{
	if(eoi_required)
		return last_interrupt;
	else
		return 0;
}

void LocalAPIC::ipi(int vector)
{
	write(LAPIC_ICRLO, ICR_ALL_EXCLUDING_SELF | ICR_FIXED | vector);
	while (read(LAPIC_ICRLO) & ICR_SEND_PENDING)
		;
}

void LocalAPIC::SetProcessorAPIC(uint8_t processorID, uint8_t apicID)
{
	m_HAL->RegisterCPU(processorID);
}

uint32_t LocalAPIC::id()
{
	uint32_t id = read(LAPIC_ID);

	return x2Apic ? id : (id >> 24);

	
}

void LocalAPIC::DisplayDetails() const
{
	Printf("LocalAPIC: Addr: 0x%016x\r\n", m_Addr);
}


void LocalAPIC::NotifyEOIRequired(int vector)
{
	eoi_required = true;
	last_interrupt = vector;
}

void LocalAPIC::CalibrateTimer()
{
	write(LAPIC_TIMER, APIC_DISABLE);

	write(LAPIC_LINT0, APIC_DISABLE);
	write(LAPIC_LINT1, APIC_DISABLE);

	uint8_t inp = m_HAL->ReadPort(PIT_OUTPUT_CHANNEL2, 8);
	inp &= 0xFD;
	inp |= 1;
	m_HAL->WritePort(PIT_OUTPUT_CHANNEL2, inp, 8);
	m_HAL->WritePort(PIT_COMMAND, 0b10110010, 8); //Channel 2, Access Mode: lobyte/hibyte, Mode 1, 16-bit binary

	// 1193180 / 100 Hz = 11931 = 2e9bh
	m_HAL->WritePort(PIT_CHANNEL2_DATA, 0x9B, 8);	//LSB
	m_HAL->ReadPort(0x60, 8); //short delay
	m_HAL->WritePort(PIT_CHANNEL2_DATA, 0x2E, 8);	//MSB

	//reset PIT one-shot counter (start counting)
	inp = m_HAL->ReadPort(PIT_OUTPUT_CHANNEL2, 8);
	inp &= 0xFE;
	m_HAL->WritePort(PIT_OUTPUT_CHANNEL2, inp, 8); //gate low
	inp |= 1;
	m_HAL->WritePort(PIT_OUTPUT_CHANNEL2, inp, 8);

	/* Set the intial count to max */
	write(LAPIC_TICR, 0xffffffff);

	//now wait until PIT counter reaches zero
	uint8_t cntr;
	do
	{
		cntr = m_HAL->ReadPort(PIT_OUTPUT_CHANNEL2, 8);
	} while ((cntr & 0x20) != 0);

	/* Mask the timer (prevents interrupts) */
	write(LAPIC_TIMER, APIC_DISABLE);

	apicCalibVal = 0xffffffff - (read(LAPIC_TCCR));

#if _DEBUG	//when debugging hardcode freq to 66 mhz since thats almost the qemu freq
	Freq = 66000000;
#else
	Freq = apicCalibVal;
	Freq *= 16; //we used divide value different than 1, so now we have to multiply the result by 16
	Freq *= 100;	//moreover, PIT did not wait a whole sec, only a fraction, so multiply by that too	
#endif
	apicCalibVal = Freq / APIC_TICKS_PER_SEC;
	
	Printf("APIC Freq: %d\r\n", Freq);
}

uint32_t LocalAPIC::read(uint32_t reg)
{
	if(x2Apic)
		return __readmsr((reg >> 4) + 0x800);
	else
		return *(volatile uint32_t*)(m_Addr + reg);
}

void LocalAPIC::write(uint32_t reg, uint32_t data)
{
	if (x2Apic)
	{
		__writemsr((reg >> 4) + 0x800, data);
	}
	else
		*(volatile uint32_t*)(m_Addr + reg) = data;
}
;