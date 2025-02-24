#include "LocalAPIC.h"
#include <Assert.h>

#include "kernel/Kernel.h"
#include <kernel\hal\x64\ctrlregs.h>

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


LocalAPIC::LocalAPIC(uint64_t addr) : m_Addr(addr)
{
	Name = "LOCALAPIC";
}

void LocalAPIC::SetProcessorAPIC(uint8_t processorID, uint8_t apicID)
{
	m_ProcessorAPICs.insert({processorID, apicID});
}

void LocalAPIC::Initialize()
{
	
	m_Addr = (uint64_t)kernel.VirtualMap(0x0,{m_Addr});
	//Printf("Mapped to virtual address: 0x%016x\r\n",m_Addr);
	
	// initialize LAPIC to a well known state

	// Clear task priority to enable all interrupts
	write(LAPIC_TPR, 0);
	
	write(LAPIC_DFR, 0xffffffff);   // Flat mode, as we are 64-bit
	write(LAPIC_LDR, 0x01000000);   // All cpus use logical id 1

	/* Mask the timer (prevents interrupts) */
	write(LAPIC_TIMER, APIC_DISABLE);

	//write(LAPIC_PERF, APIC_NMI);

	write(LAPIC_LINT0, APIC_DISABLE);
	write(LAPIC_LINT1, APIC_DISABLE);

	// Configure Spurious Interrupt Vector Register
	write(LAPIC_SVR, 0x100 | 0x27);

	//map APIC timer to an interrupt, and by that enable it in one - shot mode
	//write(LAPIC_TIMER, 32);

	/* Set the divisor to 16 */
	
	write(LAPIC_TDCR, 0b11);
	
	write(LAPIC_TICR, 0);


#if 0	//We dont calibrate using PIT, since it might not exist anymore, HPET also might not exist. this is ridiculous!
	uint8_t inp = ArchReadPort(PIT_OUTPUT_CHANNEL2, 8);
	inp &= 0xFD;
	inp |= 1;
	ArchWritePort(PIT_OUTPUT_CHANNEL2, inp, 8);
	ArchWritePort(PIT_COMMAND, 0b10110010, 8); //Channel 2, Access Mode: lobyte/hibyte, Mode 1, 16-bit binary

	// 1193180 / 100 Hz = 11931 = 2e9bh
	ArchWritePort(PIT_CHANNEL2_DATA,0x9B, 8);	//LSB
	ArchReadPort(0x60, 8); //short delay
	ArchWritePort(PIT_CHANNEL2_DATA, 0x2E, 8);	//MSB

	//reset PIT one-shot counter (start counting)
	inp = ArchReadPort(PIT_OUTPUT_CHANNEL2, 8);
	inp &= 0xFE;
	ArchWritePort(PIT_OUTPUT_CHANNEL2, inp, 8); //gate low
	inp |= 1;
	ArchWritePort(PIT_OUTPUT_CHANNEL2, inp, 8);

	/* Set the intial count to max */
	write(LAPIC_TICR, 0xffffffff);

	//now wait until PIT counter reaches zero
	uint8_t cntr;
	do 
	{
		cntr = ArchReadPort(PIT_OUTPUT_CHANNEL2, 8);
		Printf("cntr: 0x%x\r\n", cntr);
	} while ((cntr & 0x20) != 0);
#endif
	

	/* Mask the timer (prevents interrupts) */
	write(LAPIC_TIMER, APIC_DISABLE);
	/* Set the timer speed in microseconds */
	//timer_speed_us = 1000;

	/* Call a delay function based on the available timer */
	//kernel.ACPIInterop()->pmt_delay(timer_speed_us);

	/* Mask the timer (prevents interrupts) */
	//write(LAPIC_TIMER, LAPIC_TIMER_MASK);


	uint32_t calibration = 0;
	
	if (kernel.ACPIInterop()->HasPMTimer)
	{
		//TODO: Use PMTimer for calibration
		
		/* Determine the inital count to be used for a delay */
		//moreover, PIT did not wait a whole sec, only a fraction, so multiply by that too
		calibration = 0xffffffff - (read(LAPIC_TCCR));

	}
	else
	{
		//use smbios information, this might be wrong...better use TSC..
		uint16_t ctr = kernel.GetSMBios()->GetExtBusSpeed();
		//ctr is in MHZ, convert to hz

		//ctr now holds number of ticks in 1 sec
		calibration = ctr * 1000 * 1000 * 2;
		
		//we want 1/512 seconds
		calibration /= 512;

		//APIC divisor is 16, so divide by 16
		calibration /= 16;
	}
		
	
	/* Set the timer interrupt vector and put the timer into periodic mode */
	write(LAPIC_TIMER, (uint8_t)X64_INTERRUPT_VECTOR::Timer0 | LAPIC_TIMER_PERIODIC);

	/* Set the inital count to the calibration */
	write(LAPIC_TICR, calibration);

	// setting divide value register again not needed by the manuals
	// although I have found buggy hardware that required it
	//write(LAPIC_TDCR, 0b11);

	_sti();
	//Printf("APIC VER: 0x%08x\r\n", id);

	//DisplayDetails();
}

const void* LocalAPIC::GetResource(uint32_t type) const
{
	return nullptr;
}

void LocalAPIC::DisplayDetails() const
{
	Printf("LocalAPIC: Addr: 0x%016x\r\n", m_Addr);
	for (auto t : m_ProcessorAPICs)
	{
		Printf("   CPU %d -> APIC ID %d\r\n", t.first, t.second);
	}

	
}

void LocalAPIC::SignalEOI()
{
	write(LAPIC_EOI, 0);
}

uint32_t LocalAPIC::read(uint32_t reg)
{
	return *(volatile uint32_t*)(m_Addr + reg);
}

void LocalAPIC::write(uint32_t reg, uint32_t data)
{
	*(volatile uint32_t*)(m_Addr + reg) = data;
}
