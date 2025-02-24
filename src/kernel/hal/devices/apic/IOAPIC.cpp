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
	Path = "ACPI/IOAPIC";
	Type = DeviceType::System;
}

void IOAPIC::Initialize(void* context)
{
	Assert(context);
	Assert(m_HAL);

	ACPI_MADT_IO_APIC* ioApic = (ACPI_MADT_IO_APIC*)context;

	m_GlobalIrqBase = ioApic->GlobalIrqBase;
	m_Addr = (uint64_t)kernel.VirtualMapRT(0x0, {ioApic->Address});


	// Get the IO APIC version and max redirection entry
	m_version = ReadReg(Version);
	m_max_redirect_entry = (uint8_t)(m_version >> 16);

	// Log the IO APIC information
	Printf("IO APIC Version: 0x%x\n", m_version);
	Printf("IO APIC Max Redirection Entry: 0x%x\n", m_max_redirect_entry);
}

const void* IOAPIC::GetResource(uint32_t type) const
{
	return nullptr;
}

void IOAPIC::DisplayDetails() const
{
	
}

void IOAPIC::RegisterOverrideEntry(uint8_t indx, Override ovr)
{
	Assert(indx < 0x10);
	m_override_array[indx] = ovr;
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
	WriteReg(static_cast<IoApicReg>(IoApicReg::FirstEntry + 2 * pin), entry.Low);
	WriteReg(static_cast<IoApicReg>(IoApicReg::FirstEntry + 1 + 2 * pin), entry.High);
}

uint32_t IOAPIC::ReadReg(IoApicReg reg)
{
	
	// Write the register
	*(volatile uint32_t*)(m_Addr + 0x00) = reg;

	// Return the value
	return *(volatile uint32_t*)(m_Addr + 0x10);
}

void IOAPIC::WriteReg(IoApicReg reg, const uint32_t value)
{
	// Write the register
	*(volatile uint32_t*)(m_Addr + 0x00) = reg;

	// Write the value
	*(volatile uint32_t*)(m_Addr + 0x10) = value;
}

