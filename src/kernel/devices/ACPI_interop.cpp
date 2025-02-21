#include "ACPI_interop.h"
#include "kernel/kernel.h"
#include <Assert.h>
#include <OS.arch.h>
#include <kernel\arch\x64\ctrlregs.h>
#include "LocalAPIC.h"


//Acpi is single threaded, just stub these out
typedef int semaphore_t;
typedef int spinlock_t;

ACPI_Interop::ACPI_Interop(ConfigTables* configTables, EFI_RUNTIME_SERVICES* runtimeServices)
{
	this->m_ConfigTables = configTables;
	m_RuntimeServices = runtimeServices;
	acpiRoot = 0;
}

ACPI_STATUS ACPI_Interop::AcpiOsInitialize()
{
	return ACPI_STATUS();	
}

ACPI_STATUS ACPI_Interop::AcpiOsTerminate()
{
	NotImplemented();
	return ACPI_STATUS();
}

ACPI_PHYSICAL_ADDRESS ACPI_Interop::AcpiOsGetRootPointer()
{
	if (acpiRoot == 0)
		acpiRoot = (ACPI_PHYSICAL_ADDRESS)m_ConfigTables->GetAcpiTable();
	return acpiRoot;
}

ACPI_STATUS ACPI_Interop::AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES* PredefinedObject, ACPI_STRING* NewValue)
{
	*NewValue = 0;
	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsTableOverride(ACPI_TABLE_HEADER* ExistingTable, ACPI_TABLE_HEADER** NewTable)
{
	*NewTable = nullptr;
	return AE_OK;
}

void* ACPI_Interop::AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length)
{
	//Handle unaligned addresses
	const size_t pageOffset = PhysicalAddress & PageMask;
	const size_t pageCount = DivRoundUp(pageOffset + Length, PageSize);

	const uintptr_t physicalBase = PhysicalAddress & ~PageMask;

	PageTables tables;
	tables.OpenCurrent();
	Assert(tables.MapPages(KernelAcpiStart + physicalBase, physicalBase, pageCount, true));
	return (void*)(KernelAcpiStart + physicalBase + pageOffset);
}

void ACPI_Interop::AcpiOsUnmapMemory(void* where, ACPI_SIZE length)
{
	UNUSED(where);
	UNUSED(length);
}

ACPI_STATUS ACPI_Interop::AcpiOsGetPhysicalAddress(void* LogicalAddress, ACPI_PHYSICAL_ADDRESS* PhysicalAddress)
{
	PageTables tables;
	tables.OpenCurrent();
	*PhysicalAddress = tables.ResolveAddress((uintptr_t)LogicalAddress);
	
	return AE_OK;

}

void* ACPI_Interop::AcpiOsAllocate(ACPI_SIZE Size)
{
	return operator new((size_t)Size);
}

void ACPI_Interop::AcpiOsFree(void* Memory)
{
	operator delete(Memory);
}

BOOLEAN ACPI_Interop::AcpiOsReadable(void* Memory, ACPI_SIZE Length)
{
	//This is never used (at least i did never see it used)
	return TRUE;
}

BOOLEAN ACPI_Interop::AcpiOsWritable(void* Memory, ACPI_SIZE Length)
{
	//This is never used (at least i did never see it used)
	return TRUE;
}

ACPI_THREAD_ID ACPI_Interop::AcpiOsGetThreadId()
{
	//ACPI is single-threaded, just return 1
	return 1;
}

ACPI_STATUS ACPI_Interop::AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void* Context)
{
	NotImplemented();
	return AE_ERROR;
}

void ACPI_Interop::AcpiOsSleep(UINT64 Milliseconds)
{
	NotImplemented();
}

void ACPI_Interop::AcpiOsStall(UINT32 Microseconds)
{
	NotImplemented();
}

ACPI_STATUS ACPI_Interop::AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE* OutHandle)
{
	*OutHandle = new semaphore_t();
	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
	if (!Handle)
		return AE_BAD_PARAMETER;

	delete Handle;
	return AE_OK;
}

void ACPI_Interop::AcpiOsVprintf(const char* Format, va_list Args)
{
	//Reduce ACPI talk for now (soon, pump to uart?)
	//Printf(Format, Args);
}

ACPI_STATUS ACPI_Interop::AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{
	if (!Handle)
		return AE_BAD_PARAMETER;

	Assert(Units == 1);

	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
	if (!Handle)
		return AE_BAD_PARAMETER;

	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsCreateLock(ACPI_SPINLOCK* OutHandle)
{
	*OutHandle = new spinlock_t();
	return AE_OK;
}

void ACPI_Interop::AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
	delete Handle;
}

ACPI_CPU_FLAGS ACPI_Interop::AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
	if (!Handle)
		return AE_BAD_PARAMETER;

	return 0;
}

void ACPI_Interop::AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
	//NOTE(tsharpe): Nothing to do, spinlocks aren't implemented
}

ACPI_STATUS ACPI_Interop::AcpiOsSignal(UINT32 Function, void* Info)
{
	NotImplemented();
	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64* Value, UINT32 Width)
{
	NotImplemented();
	switch (Width)
	{
	case 8:
		*Value = *(UINT8*)Address;
		break;
	case 16:
		*Value = *(UINT16*)Address;
		break;
	case 32:
		*Value = *(UINT32*)Address;
		break;
	case 64:
		*Value = *(UINT64*)Address;
		break;
	default:
		return AE_BAD_VALUE;
	}
	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
	NotImplemented();
	switch (Width)
	{
	case 8:
		*(UINT8*)Address = (UINT8)Value;
		break;
	case 16:
		*(UINT16*)Address = (UINT16)Value;
		break;
	case 32:
		*(UINT32*)Address = (UINT32)Value;
		break;
	case 64:
		*(UINT64*)Address = (UINT64)Value;
		break;
	default:
		return AE_BAD_VALUE;
	}
	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32* Value, UINT32 Width)
{
	switch (Width)
	{
	case 8:
	case 16:
	case 32:
		*Value = ArchReadPort((uint32_t)Address, Width);
		break;
	default:
		return AE_BAD_VALUE;
	}

	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
	switch (Width)
	{
	case 8:
	case 16:
	case 32:
		ArchWritePort((uint32_t)Address, Value, Width);
		break;
	default:
		return AE_BAD_VALUE;
	}
	return AE_OK;
}

UINT64 ACPI_Interop::AcpiOsGetTimer()
{
	EFI_TIME time;
	m_RuntimeServices->GetTime(&time, nullptr);
	uint64_t timer;
	uint16_t year = time.Year;
	uint8_t month = time.Month;
	uint8_t day = time.Day;
	timer = ((uint64_t)(year / 4 - year / 100 + year / 400 + 367 * month / 12 + day) +
		year * 365 - 719499);
	timer *= 24;
	timer += time.Hour;
	timer *= 60;
	timer += time.Minute;
	timer *= 60;
	timer += time.Second;
	timer *= 10000000;	//100 n intervals
	timer += time.Nanosecond / 100;
	return timer;
}

ACPI_STATUS ACPI_Interop::AcpiOsReadPciConfiguration(ACPI_PCI_ID* PciId, UINT32 Reg, UINT64* Value, UINT32 Width)
{
	NotImplemented();
	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsWritePciConfiguration(ACPI_PCI_ID* PciId, UINT32 Reg, UINT64 Value, UINT32 Width)
{
	NotImplemented();
	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void* Context)
{
	kernel.KeRegisterInterrupt((X64_INTERRUPT_VECTOR)InterruptLevel, { Handler, Context });
	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler)
{
	kernel.KeUnregisterInterrupt((X64_INTERRUPT_VECTOR)InterruptNumber);
	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER* ExistingTable, ACPI_PHYSICAL_ADDRESS* NewAddress, UINT32* NewTableLength)
{
	*NewAddress = 0;
	*NewTableLength = 0;
	return AE_OK;
}

ACPI_STATUS ACPI_Interop::AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue)
{
	uint32_t status = kernel.PrepareShutdown();
	if(status != 0)
		return AE_CTRL_TERMINATE;
	return AE_OK;
}

ACPI_TABLE_DESC* ACPI_Interop::GetAcpiTableBySignature(char sig[4])
{
	for (int i = 0; i < MAX_ACPI_TABLES; i++)
	{
		if(strcmp(acpi_tables[i].Signature.Ascii, sig) == 0)
			return &acpi_tables[i];
	}
	return nullptr;
}

void ACPI_Interop::Init()
{

	Printf("Loading ACPI Devices...\r\n");

	ACPI_STATUS Status;
	Status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiInitializeSubsystem: %d\n", Status);
		ArchWait();
	}

	Status = AcpiInitializeTables(acpi_tables, 16, FALSE);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiInitializeTables: %d\n", Status);
		ArchWait();
	}

	/* Install the default address space handlers. */
	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT, ACPI_ADR_SPACE_SYSTEM_MEMORY, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not initialise SystemMemory handler, %s!", AcpiFormatException(Status));
		ArchWait();
	}

	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT, ACPI_ADR_SPACE_SYSTEM_IO, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not initialise SystemIO handler, %s!", AcpiFormatException(Status));
		ArchWait();
	}

	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT, ACPI_ADR_SPACE_PCI_CONFIG, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not initialise PciConfig handler, %s!", AcpiFormatException(Status));
		ArchWait();
	}

	Status = AcpiLoadTables();
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiLoadTables: %d\n", Status);
		ArchWait();
	}

	//Local handlers should be installed here

	Status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiEnableSubsystem: %d\n", Status);
		ArchWait();
	}

	Status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiInitializeObjects: %d\n", Status);
		ArchWait();
	}

	ACPI_TABLE_FADT* fadt = (ACPI_TABLE_FADT*)GetAcpiTableBySignature((char*)ACPI_SIG_FADT);
	if (fadt)
	{
		if (fadt->PmTimerLength != 4)
		{
			Printf("No PMTIMER!\r\n");
			HasPMTimer = false;
		}
		else
			HasPMTimer = true;
	}



	//ACPI_TABLE_FADT* fadt_table = (ACPI_TABLE_FADT*)GetAcpiTableBySignature((char*)ACPI_SIG_FADT);
	//hasLegacyDevices =  (fadt_table->BootFlags & ACPI_FADT_LEGACY_DEVICES);
	//hasPS2 = (fadt_table->BootFlags & ACPI_FADT_8042);

	Printf("ACPI Finished\n");

}

void ACPI_Interop::PowerOffSystem()
{
	AcpiEnterSleepStatePrep(5);
	_cli(); //disable interrupts
	AcpiEnterSleepState(5);
	kernel.Panic("power off"); // in case it didn't work!
}


ACPI_TABLE_MADT* ACPI_Interop::GetMADTTable()
{
	ACPI_TABLE_DESC* descr = GetAcpiTableBySignature((char*)ACPI_SIG_MADT);
	if (!descr)
	{
		Printf("Unable to retrieve APIC table from ACPI\r\n");
		return nullptr;
	}
	ACPI_TABLE_MADT* madt = nullptr;
	if (descr->Flags & ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL)
	{
		madt = (ACPI_TABLE_MADT*)(KernelAcpiStart + descr->Address);
	}
	else
		madt = (ACPI_TABLE_MADT*)(descr->Address);


	if (!madt)
	{
		Printf("Invalid MADT Table\r\n");
		return nullptr;
	}
	return madt;
}


bool ACPI_Interop::parseMADT()
{
	ACPI_TABLE_MADT* madt = GetMADTTable();
	if (!madt)
	{
		return false;
	}

	uint64_t madt_end, entry;
	entry = (uint64_t)madt;
	uint64_t localApicAddr = madt->Address;

	madt_end = entry + madt->Header.Length;
	entry += sizeof(ACPI_TABLE_MADT);

	Printf("Local APIC Address: 0x%0x16x\r\n", localApicAddr);
	LocalAPIC* localApicDevice = new LocalAPIC(localApicAddr);

	while (entry + sizeof(ACPI_SUBTABLE_HEADER) < madt_end) {
		ACPI_SUBTABLE_HEADER* header =
			(ACPI_SUBTABLE_HEADER*)entry;

		if (header->Type == ACPI_MADT_TYPE_LOCAL_APIC)
		{
			ACPI_MADT_LOCAL_APIC* localApic = (ACPI_MADT_LOCAL_APIC*)entry;
			if(localApic->LapicFlags & ACPI_MADT_ENABLED)
			{
				localApicDevice->SetProcessorAPIC(localApic->ProcessorId, localApic->Id);
				Printf("Found local APIC (%d) for CPU (%d)\r\n", localApic->Id, localApic->ProcessorId);
			}
		}

		entry += header->Length;
	}
	localApicDevice->Initialize();
	kernel.GetDeviceTree()->AddRootDevice(*localApicDevice);
	return true;
}

uint32_t ACPI_Interop::RemapIRQ(uint32_t irq)
{
	ACPI_TABLE_MADT* madt = GetMADTTable();
	if (!madt)
	{
		return 0;
	}

	uint64_t madt_end, entry;
	entry = (uint64_t)madt;
	uint64_t localApicAddr = madt->Address;

	madt_end = entry + madt->Header.Length;
	entry += sizeof(ACPI_TABLE_MADT);
	while (entry + sizeof(ACPI_SUBTABLE_HEADER) < madt_end) {
		ACPI_SUBTABLE_HEADER* header =
			(ACPI_SUBTABLE_HEADER*)entry;

		if (header->Type == ACPI_MADT_TYPE_INTERRUPT_OVERRIDE)
		{
			ACPI_MADT_INTERRUPT_OVERRIDE* localApic = (ACPI_MADT_INTERRUPT_OVERRIDE*)entry;
			if (localApic->SourceIrq == irq)
			{
				return localApic->GlobalIrq;
			}
		}

		entry += header->Length;
	}
	return irq;
}
