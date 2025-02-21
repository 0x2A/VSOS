#include "IOApicDriver.h"
#include <Assert.h>
#include <vector>
#include <kernel\devices\ACPIDevice.h>
#include "kernel/Kernel.h"




const std::string IOApicDriver::GetCompatHID()
{
	return "PNP0003";
}

Driver* IOApicDriver::CreateInstance(Device* device)
{
	IOApicDriver* driver = new IOApicDriver(*device);
	//driver->m_device = device;
	return driver;
}

#define APIC_APICID 0x20
#define APIC_APICVER 0x30
#define APIC_TASKPRIOR 0x80
#define APIC_EOI 0x0B0
#define APIC_LDR 0x0D0
#define APIC_DFR 0x0E0
#define APIC_SPURIOUS 0x0F0
#define APIC_ESR 0x280
#define APIC_ICRL 0x300
#define APIC_ICRH 0x310
#define APIC_LVT_TMR 0x320
#define APIC_LVT_PERF 0x340
#define APIC_LVT_LINT0 0x350
#define APIC_LVT_LINT1 0x360
#define APIC_LVT_ERR 0x370
#define APIC_TMRINITCNT 0x380
#define APIC_TMRCURRCNT 0x390
#define APIC_TMRDIV 0x3E0
#define APIC_LAST 0x38F
#define APIC_DISABLE 0x10000
#define APIC_SW_ENABLE 0x100
#define APIC_CPUFOCUS 0x200
#define APIC_NMI (4 << 8)
#define TMR_PERIODIC 0x20000
#define TMR_BASEDIV (1 << 20)

Result IOApicDriver::Initialize()
{
	Printf("IoApicDriver::Initialize...\r\n");

	std::vector<paddr_t> addresses;
	
	AcpiDevice* device = (AcpiDevice*)&m_device;	
	const ACPI_RESOURCE* resource = (const ACPI_RESOURCE*)device->GetResource(ACPI_RESOURCE_TYPE_FIXED_MEMORY32);
	if (resource == nullptr)
	{
		Printf("ApicDriver: Failed to get ressource ACPI_RESOURCE_TYPE_FIXED_MEMORY32!\r\n");
		return Result::Failed;
	}
	addresses.push_back(resource->Data.FixedMemory32.Address);
	

	//Printf("Try mapping virtual address 0x%016x\r\n", resource->Data.FixedMemory32.Address);
	m_apic = (IoApic*)kernel.VirtualMap(nullptr, addresses);

	IdReg idReg = { 0 };
	idReg.AsUint32 = ReadReg(IoApicReg::ID);

	VersionReg versionReg = { 0 };
	versionReg.AsUint32 = ReadReg(IoApicReg::Version);

	m_device.Name = "IOAPIC";

	RedirectionEntry entry = {0};
	entry.InterruptMasked = 1;
	for (uint32_t i = 0; i < versionReg.MaxEntry; i++)
	{
		WriteEntry(i, entry);
	}

	

	Printf("Success\r\n");
	return Result::Success;
}

Result IOApicDriver::Read(char* buffer, size_t length, size_t* bytesRead /*= nullptr*/)
{
	return Result::NotImplemented;
}

Result IOApicDriver::Write(const char* buffer, size_t length)
{
	return Result::NotImplemented;
}

Result IOApicDriver::EnumerateChildren()
{
	return Result::NotImplemented;
}

Result IOApicDriver::MapInterrupt(X64_INTERRUPT_VECTOR vector, uint16_t Irq)
{
	RedirectionEntry entry = ReadEntry(Irq);
	entry.InterruptVector = static_cast<uint8_t>(vector);
	WriteEntry(Irq, entry);
	return Result::Success;
}

Result IOApicDriver::UnmaskInterrupt(uint16_t Irq)
{
	RedirectionEntry entry = ReadEntry(Irq);
	entry.InterruptMasked = 0;
	WriteEntry(Irq, entry);
	return Result::Success;
}

IOApicDriver::IOApicDriver(Device& device)
 : Driver(device), m_apic(), m_maxEntry(0)
{
}

IOApicDriver::RedirectionEntry IOApicDriver::ReadEntry(int pin)
{
	RedirectionEntry entry;
	entry.Low = ReadReg(static_cast<IoApicReg>(IoApicReg::FirstEntry + 2 * pin));
	entry.High = ReadReg(static_cast<IoApicReg>(IoApicReg::FirstEntry + 1 + 2 * pin));
	return entry;
}

void IOApicDriver::WriteEntry(int pin, RedirectionEntry entry)
{
	WriteReg(static_cast<IoApicReg>(IoApicReg::FirstEntry + 2 * pin), entry.Low);
	WriteReg(static_cast<IoApicReg>(IoApicReg::FirstEntry + 1 + 2 * pin), entry.High);
}

uint32_t IOApicDriver::ReadReg(IoApicReg reg)
{
	m_apic->Index = static_cast<uint32_t>(reg);
	return m_apic->Data;
}

void IOApicDriver::WriteReg(IoApicReg reg, const uint32_t value)
{
	m_apic->Index = static_cast<uint32_t>(reg);
	m_apic->Data = value;
}

