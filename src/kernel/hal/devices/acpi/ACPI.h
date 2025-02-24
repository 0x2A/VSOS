#pragma once

#include "kernel/hal/devices/ConfigTables.h"
#include <vector>

extern "C"
{
#include <acpi.h>
}

#define MAX_ACPI_TABLES 16

class HAL;
class ACPI
{

public:
	ACPI(HAL* hal, ConfigTables* configTables);


	void Init();
	bool parseMADT();

	uint32_t RemapIRQ(uint32_t irq);

	void PowerOffSystem();

	ACPI_TABLE_DESC* GetAcpiTableBySignature(const char sig[4]);

	ACPI_TABLE_DESC acpi_tables[MAX_ACPI_TABLES];
	bool HasPMTimer;

public:

	//ACPICA interop
	ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer();
	void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length);

	ACPI_STATUS AcpiOsGetPhysicalAddress(void* LogicalAddress, ACPI_PHYSICAL_ADDRESS* PhysicalAddress);
	void* AcpiOsAllocate(ACPI_SIZE Size);
	void AcpiOsFree(void* Memory);
	ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32* Value, UINT32 Width);
	ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width);
	UINT64 AcpiOsGetTimer();

	ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID* PciId, UINT32 Reg, UINT64* Value, UINT32 Width);
	ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID* PciId, UINT32 Reg, UINT64 Value, UINT32 Width);
	ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void* Context);
	ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler);

	ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue);

private:

	ACPI_TABLE_MADT* GetMADTTable();
	
	//bool hasLegacyDevices; // System has LPC or ISA bus devices
	//bool hasPS2; //System has an 8042 controller on port 60/64 */

	ConfigTables* m_ConfigTables;
	EFI_RUNTIME_SERVICES* m_RuntimeServices;

	ACPI_PHYSICAL_ADDRESS acpiRoot;
	HAL* m_HAL;
};