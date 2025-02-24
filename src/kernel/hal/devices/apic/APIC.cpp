#include "apic.h"
#include <Assert.h>
#include "kernel/hal/HAL.h"
#include <os.internal.h>

#define PIC_MASTER_COMMAND_PORT 0x20
#define PIC_MASTER_DATA_PORT	0x21
#define PIC_SLAVE_COMMAND_PORT	0xA0
#define PIC_SLAVE_DATA_PORT		0xA1

APIC::APIC(HAL* hal)
: m_HAL(hal), m_io_apic(hal), m_local_apic(hal)
{
	
}

APIC::~APIC()
{

}

void APIC::Init()
{
	ACPI_TABLE_DESC* descr = m_HAL->GetACPI()->GetAcpiTableBySignature(ACPI_SIG_MADT);
	Assert(descr);

	ACPI_TABLE_MADT* madt = nullptr;
	if (descr->Flags & ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL)
	{
		madt = (ACPI_TABLE_MADT*)(KernelAcpiStart + descr->Address);
	}
	else
		madt = (ACPI_TABLE_MADT*)(descr->Address);

	Assert(madt);

	uint64_t madt_end, entry;
	entry = (uint64_t)madt;
	uint64_t localApicAddr = madt->Address;

	madt_end = entry + madt->Header.Length;
	entry += sizeof(ACPI_TABLE_MADT);

	Printf("Local APIC Address: 0x%0x16x\r\n", localApicAddr);
	
	m_local_apic.Initialize((void*)localApicAddr);

	uint8_t overrideIndx = 0;
	while (entry + sizeof(ACPI_SUBTABLE_HEADER) < madt_end) {
		ACPI_SUBTABLE_HEADER* header =
			(ACPI_SUBTABLE_HEADER*)entry;

		if (header->Type == ACPI_MADT_TYPE_LOCAL_APIC)
		{
			ACPI_MADT_LOCAL_APIC* localApic = (ACPI_MADT_LOCAL_APIC*)entry;
			if (localApic->LapicFlags & ACPI_MADT_ENABLED)
			{
				m_local_apic.SetProcessorAPIC(localApic->ProcessorId, localApic->Id);
				//Printf("Found local APIC (%d) for CPU (%d)\r\n", localApic->Id, localApic->ProcessorId);
			}
		}
		else if (header->Type == ACPI_MADT_TYPE_IO_APIC)
		{
			ACPI_MADT_IO_APIC* ioApic = (ACPI_MADT_IO_APIC*)entry;

			Printf(__FUNCTION__": Initialising LOCAL APIC\r\n");
			m_io_apic.Initialize(ioApic);
		}
		else if (header->Type == ACPI_MADT_TYPE_INTERRUPT_OVERRIDE)
		{
			ACPI_MADT_INTERRUPT_OVERRIDE* intOverride = (ACPI_MADT_INTERRUPT_OVERRIDE*)entry;
			
			IOAPIC::Override ovr;			
			ovr.bus = intOverride->Bus;
			ovr.source = intOverride->SourceIrq;
			ovr.global_system_interrupt = intOverride->GlobalIrq;
			ovr.flags = intOverride->IntiFlags;
			m_io_apic.RegisterOverrideEntry(overrideIndx, ovr);
			overrideIndx++;

			//Printf("Interrupt Override: bus: 0x%x, source: 0x%x, global: 0x%x, Flags: 0x%x\r\n", ovr.bus, ovr.source, ovr.global_system_interrupt, ovr.flags);
		}
			

		entry += header->Length;
	}

	Printf(__FUNCTION__": Disable PIC\r\n");
	disable_pic();
}

void APIC::disable_pic()
{
	// Initialise the PIC
	m_HAL->WritePort(PIC_MASTER_COMMAND_PORT, 0x11, 8);
	m_HAL->WritePort(PIC_SLAVE_COMMAND_PORT, 0x11, 8);

	// Set the offsets
	m_HAL->WritePort(PIC_MASTER_DATA_PORT, 0x20, 8);
	m_HAL->WritePort(PIC_SLAVE_DATA_PORT, 0x28, 8);

	// Set the slave/master relationships
	m_HAL->WritePort(PIC_MASTER_DATA_PORT, 0x04, 8);
	m_HAL->WritePort(PIC_SLAVE_DATA_PORT, 0x02, 8);

	// Set the modes (8086/8086)
	m_HAL->WritePort(PIC_MASTER_DATA_PORT, 0x01, 8);
	m_HAL->WritePort(PIC_SLAVE_DATA_PORT, 0x01, 8);

	// Mask the interrupts
	m_HAL->WritePort(PIC_MASTER_DATA_PORT, 0xFF, 8);
	m_HAL->WritePort(PIC_SLAVE_DATA_PORT, 0xFF, 8);
}

void APIC::EOI()
{
	m_local_apic.SignalEOI();
}

