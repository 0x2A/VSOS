#pragma once

#include "kernel/devices/ConfigTables.h"
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

	ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer();
	void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length);
	
	ACPI_STATUS AcpiOsGetPhysicalAddress(void* LogicalAddress, ACPI_PHYSICAL_ADDRESS* PhysicalAddress);
	void* AcpiOsAllocate(ACPI_SIZE Size);
	void AcpiOsFree(void* Memory);
	BOOLEAN AcpiOsReadable(void* Memory, ACPI_SIZE Length);
	BOOLEAN AcpiOsWritable(void* Memory, ACPI_SIZE Length);
	ACPI_THREAD_ID AcpiOsGetThreadId();
	ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void* Context);
	void AcpiOsSleep(UINT64 Milliseconds);
	void AcpiOsStall(UINT32 Microseconds);
	ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE* OutHandle);
	ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle);
	void AcpiOsVprintf(const char* Format, va_list Args);
	ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout);
	ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units);
	ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK* OutHandle);
	void AcpiOsDeleteLock(ACPI_SPINLOCK Handle);
	ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle);
	void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags);
	ACPI_STATUS AcpiOsSignal(UINT32 Function, void* Info);
	ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64* Value, UINT32 Width);
	ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width);
	ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32* Value, UINT32 Width);
	ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width);
	UINT64 AcpiOsGetTimer();
	void AcpiOsWaitEventsComplete();
	ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID* PciId, UINT32 Reg, UINT64* Value, UINT32 Width);
	ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID* PciId, UINT32 Reg, UINT64 Value, UINT32 Width);
	ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void* Context);
	ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler);
	ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER* ExistingTable, ACPI_PHYSICAL_ADDRESS* NewAddress, UINT32* NewTableLength);
	ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue);
	
	ACPI_TABLE_DESC* GetAcpiTableBySignature(char sig[4]);

	void Init();
	bool parseMADT();

	uint32_t RemapIRQ(uint32_t irq);

	void PowerOffSystem();


	ACPI_TABLE_DESC acpi_tables[MAX_ACPI_TABLES];

	bool HasPMTimer;

private:

	ACPI_TABLE_MADT* GetMADTTable();


	
	//bool hasLegacyDevices; // System has LPC or ISA bus devices
	//bool hasPS2; //System has an 8042 controller on port 60/64 */

	ConfigTables* m_ConfigTables;
	EFI_RUNTIME_SERVICES* m_RuntimeServices;

	ACPI_PHYSICAL_ADDRESS acpiRoot;
	HAL* m_HAL;
};