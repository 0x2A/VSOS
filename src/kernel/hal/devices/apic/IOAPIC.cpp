#include "IOAPIC.h"
#include <Assert.h>
#include <os.internal.h>
#include "kernel/hal/HAL.h"
#include "kernel/Kernel.h"

IOAPIC::IOAPIC(HAL* hal)
: m_HAL(hal), Device()
{
	Name = "IOAPIC";
	Description = "I/O APIC";
	sprintf(Path, "ACPI/IOAPIC");
	Type = DeviceType::System;
	m_override_array_size = 0;
}

void IOAPIC::Initialize(void* context)
{
	Assert(context);
	Assert(m_HAL);

	ACPI_MADT_IO_APIC* ioApic = (ACPI_MADT_IO_APIC*)context;

	m_GlobalIrqBase = ioApic->GlobalIrqBase;
	m_Addr = (uint64_t)kernel.VirtualMapRT(0x0, {ioApic->Address});
	m_PhysicalAddr = ioApic->Address;

	// Get the IO APIC version and max redirection entry
	m_version = ReadReg(Version);
	m_max_redirect_entry = ((uint8_t)(m_version >> 16) & 0XFF) + 1;

	// Log the IO APIC information
	Printf("IO APIC Addr: 0x%16x, Version: 0x%x\n", m_Addr, m_version);
	//Printf("IO APIC Max Redirection Entry: 0x%x\n", m_max_redirect_entry);
	//Printf("IO APIC Global IRQ Base: 0x%x\r\n", m_GlobalIrqBase);



	// Mark all interrupts edge-triggered, active high, disabled,
	// and not routed to any CPUs.
	RedirectionEntry entry = {0};
	entry.InterruptMasked = true;
	for (int i = 0; i < m_max_redirect_entry; i++)
	{
		entry.InterruptVector = 32 + i;
		WriteEntry(i, entry);
	}
	
}

const void* IOAPIC::GetResource(uint32_t type) const
{
	return nullptr;
}

void IOAPIC::DisplayDetails() const
{
	
}

void IOAPIC::SetRedirection(const interrupt_redirect_t* redirect)
{
	RedirectionEntry entry;
	entry.AsUint64 = redirect->flags | (redirect->interrupt & 0xFF);
	entry.Destination = redirect->destination;
	entry.InterruptMasked = redirect->mask;
	entry.InterruptVector = redirect->interrupt;
	entry.DeliveryMode = DeliveryMode::Fixed;
	entry.DestinationMode = 0;
	
	for (uint8_t i = 0; i < m_override_array_size; i++) {
		if (m_override_array[i].source != redirect->type)
			continue;
			
			
		// Set the lower 4 bits of the pin
		entry.InterruptPolarity = (m_override_array[i].flags & 0b11 == 2) ? 0b1 : 0b0;
		entry.InterruptPolarity = ((m_override_array[i].flags >> 2) & 0b11 == 2) ? 0b1 : 0b0;

		// Set the trigger mode
		entry.TriggerMode = (((m_override_array[i].flags >> 2) & 0b11) == 2);

		uint32_t reg = (m_override_array[i].global_system_interrupt - m_GlobalIrqBase);
		Printf("Global System Interrupt: 0x%x\r\n", reg);

		break;
	}

	Printf(__FUNCTION__": Writing redirect: vec: 0x%x, pin: %d\r\n", entry.InterruptVector, redirect->index);

	WriteEntry(redirect->index, entry);
}

void IOAPIC::RegisterOverrideEntry(uint8_t indx, Override ovr)
{
	Assert(indx < 0x10);
	m_override_array[indx] = ovr;
	m_override_array_size++;
}

IOAPIC::RedirectionEntry IOAPIC::ReadEntry(int pin)
{
	RedirectionEntry entry;
	entry.Low = ReadReg(static_cast<IoApicReg>(IoApicReg::FirstEntry + 2 * pin));
	entry.High = ReadReg(static_cast<IoApicReg>(IoApicReg::FirstEntry + 1 + 2 * pin));
	return entry;
}

void IOAPIC::WriteEntry(int pin, RedirectionEntry entry)
{
	// If the index is out of bounds, return
	//if (pin < 0x10 || pin > 0x3F)
		//return;

	// Low and high registers
	//uint32_t low = (uint32_t)entry.AsUint64;
	//uint32_t high = (uint32_t)(entry.AsUint64 >> 32);

	//Printf("Low: 0x%x, High: 0x%x\r\n", low, high);

	WriteReg(static_cast<IoApicReg>(IoApicReg::FirstEntry + 2 * pin), entry.Low);
	WriteReg(static_cast<IoApicReg>(IoApicReg::FirstEntry + 1 + 2 * pin), entry.High);
}

uint32_t IOAPIC::ReadReg(IoApicReg reg)
{
	// Write the register
	*(volatile uint32_t*)(m_Addr ) = static_cast<uint32_t>(reg);

	// Return the value
	return *(volatile uint32_t*)(m_Addr + 0x10);
}

void IOAPIC::WriteReg(IoApicReg reg, const uint32_t value)
{

	// Write the register
	/* tell IOREGSEL where we want to write to */
	*(volatile uint32_t*)(m_Addr) = static_cast<uint8_t>(reg);

	// Write the value
	*(volatile uint32_t*)(m_Addr + 0x10) = value;
}

