#include "AcpiProcessor.h"
#include <Assert.h>
#include <vector>
#include <kernel\devices\ACPIDevice.h>
#include "kernel\Kernel.h"

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

AcpiProcessor::AcpiProcessor(Device& device)
: Driver(device)
{

}

const std::string AcpiProcessor::GetCompatHID()
{
	return "ACPI0007";
}

Driver* AcpiProcessor::CreateInstance(Device* device)
{
	AcpiProcessor* driver = new AcpiProcessor(*device);
	//driver->m_device = device;
	return driver;
}

Result AcpiProcessor::Initialize()
{

	std::vector<paddr_t> addresses;

	AcpiDevice* device = (AcpiDevice*)&m_device;
	auto deviceInfo = device->GetDeviceInfo();

	m_device.Name = "CPU";

	cpuID = atoi(deviceInfo->UniqueId.String);

	Printf("AcpiProcessor::Initialized. ID:%d\r\n", cpuID);

	
	return Result::Success;
	//Printf("Try mapping virtual address 0x%016x\r\n", resource->Data.FixedMemory32.Address);
	//m_apic = (IoApic*)kernel.VirtualMap(nullptr, addresses);
}

Result AcpiProcessor::Read(char* buffer, size_t length, size_t* bytesRead /*= nullptr*/)
{
	return Result::Success;
}

Result AcpiProcessor::Write(const char* buffer, size_t length)
{
	return Result::Success;
}

Result AcpiProcessor::EnumerateChildren()
{
	return Result::Success;
}

void AcpiProcessor::SetLocalAPIC(ACPI_MADT_LOCAL_APIC* lapictable)
{
	m_LocalAPIC = lapictable;
}
