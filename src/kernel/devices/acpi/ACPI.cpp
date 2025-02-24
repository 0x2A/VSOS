#include "ACPI.h"
#include "kernel/kernel.h"
#include <Assert.h>
#include <OS.arch.h>
#include <kernel\hal\x64\ctrlregs.h>

extern "C"
{
#include <acpi.h>
}

//Acpi is single threaded, just stub these out
typedef int semaphore_t;
typedef int spinlock_t;

static ACPI* acpi;

ACPI::ACPI(HAL* hal, ConfigTables* configTables)
: m_HAL(hal)
{
	this->m_ConfigTables = configTables;
	//m_RuntimeServices = runtimeServices;
	acpiRoot = 0;
	acpi = this;
}

//This file is for forwarding global ACPI entries Kernel.Acpi needs to the kernel.
ACPI_STATUS AcpiOsInitialize()
{
	return ACPI_STATUS();
}

ACPI_STATUS AcpiOsTerminate()
{
	return ACPI_STATUS();
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer()
{
	return acpi->AcpiOsGetRootPointer();
}

ACPI_PHYSICAL_ADDRESS ACPI::AcpiOsGetRootPointer()
{
	if (acpiRoot == 0)
		acpiRoot = (ACPI_PHYSICAL_ADDRESS)m_ConfigTables->GetAcpiTable();
	return acpiRoot;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES* PredefinedObject, ACPI_STRING* NewValue)
{
	//Currently not used
	*NewValue = 0;
	return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER* ExistingTable, ACPI_TABLE_HEADER** NewTable)
{
	*NewTable = nullptr;
	return AE_OK;
}

void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length)
{
	return acpi->AcpiOsMapMemory(PhysicalAddress, Length);
}

void* ACPI::AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length)
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

void AcpiOsUnmapMemory(void* where, ACPI_SIZE length)
{
	UNUSED(where);
	UNUSED(length);
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void* LogicalAddress, ACPI_PHYSICAL_ADDRESS* PhysicalAddress)
{
	return acpi->AcpiOsGetPhysicalAddress(LogicalAddress, PhysicalAddress);
}

ACPI_STATUS ACPI::AcpiOsGetPhysicalAddress(void* LogicalAddress, ACPI_PHYSICAL_ADDRESS* PhysicalAddress)
{
	PageTables tables;
	tables.OpenCurrent();
	*PhysicalAddress = tables.ResolveAddress((uintptr_t)LogicalAddress);
	
	return AE_OK;

}

void* AcpiOsAllocate(ACPI_SIZE Size)
{
	return acpi->AcpiOsAllocate(Size);
}

void* ACPI::AcpiOsAllocate(ACPI_SIZE Size)
{
	return operator new((size_t)Size);
}

void AcpiOsFree(void* Memory)
{
	return acpi->AcpiOsFree(Memory);
}

void ACPI::AcpiOsFree(void* Memory)
{
	operator delete(Memory);
}
BOOLEAN AcpiOsReadable(void* Memory, ACPI_SIZE Length)
{
	// This is never used (at least i did never see it used).
	return TRUE;
}


BOOLEAN AcpiOsWritable(void* Memory, ACPI_SIZE Length)
{
	//This is never used (at least i did never see it used)
	return TRUE;
}

ACPI_THREAD_ID AcpiOsGetThreadId()
{
	//ACPI is single-threaded, just return 1
	//TODO: support multithreading
	return 1;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void* Context)
{
	//Create a new thread (or process) with entry point at Function using parameter Context. Type is not really useful. 
	//When the scheduler chooses this thread it has to pass in Context to the first argument (RDI for x86-64, stack for x86-32 (using System V ABI)
	NotImplemented();
	return AE_ERROR;
}

void AcpiOsSleep(UINT64 Milliseconds)
{
	NotImplemented();
}

void AcpiOsStall(UINT32 Microseconds)
{
	NotImplemented();
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE* OutHandle)
{
	*OutHandle = new semaphore_t();
	return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
	if (!Handle)
		return AE_BAD_PARAMETER;

	delete Handle;
	return AE_OK;
}


void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char* Format, ...)
{
	va_list args;
	va_start(args, Format);
	AcpiOsVprintf(Format, args);
	va_end(args);
}


void AcpiOsVprintf(const char* Format, va_list Args)
{
	//Reduce ACPI talk for now (soon, pump to uart?)
	//Printf(Format, Args);
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{
	if (!Handle)
		return AE_BAD_PARAMETER;

	Assert(Units == 1);

	return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
	if (!Handle)
		return AE_BAD_PARAMETER;

	return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK* OutHandle)
{
	*OutHandle = new spinlock_t();
	return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
	delete Handle;
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
	if (!Handle)
		return AE_BAD_PARAMETER;

	return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
	//NOTE(tsharpe): Nothing to do, spinlocks aren't implemented
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void* Info)
{
	NotImplemented();
	return AE_OK;
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64* Value, UINT32 Width)
{
	//NotImplemented();
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

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
	//NotImplemented();
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


ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32* Value, UINT32 Width)
{
	return acpi->AcpiOsReadPort(Address, Value, Width);
}


ACPI_STATUS ACPI::AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32* Value, UINT32 Width)
{
	switch (Width)
	{
	case 8:
	case 16:
	case 32:
		*Value = m_HAL->ReadPort((uint32_t)Address, Width);
		break;
	default:
		return AE_BAD_VALUE;
	}

	return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
	return acpi->AcpiOsWritePort(Address, Value, Width);
}

ACPI_STATUS ACPI::AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
	switch (Width)
	{
	case 8:
	case 16:
	case 32:
		m_HAL->WritePort((uint32_t)Address, Value, Width);
		break;
	default:
		return AE_BAD_VALUE;
	}
	return AE_OK;
}

UINT64 AcpiOsGetTimer()
{
	return acpi->AcpiOsGetTimer();
}
UINT64 ACPI::AcpiOsGetTimer()
{
	return 0;
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

void AcpiOsWaitEventsComplete()
{
	NotImplemented();
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID* PciId, UINT32 Reg, UINT64* Value, UINT32 Width)
{
	//NotImplemented();
	return acpi->AcpiOsReadPciConfiguration(PciId, Reg, Value, Width);
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID* PciId, UINT32 Reg, UINT64 Value, UINT32 Width)
{
	//NotImplemented();
	return acpi->AcpiOsWritePciConfiguration(PciId, Reg, Value, Width);
}

ACPI_STATUS ACPI::AcpiOsReadPciConfiguration(ACPI_PCI_ID* PciId, UINT32 Reg, UINT64* Value, UINT32 Width)
{
	return AE_SUPPORT;
}

ACPI_STATUS ACPI::AcpiOsWritePciConfiguration(ACPI_PCI_ID* PciId, UINT32 Reg, UINT64 Value, UINT32 Width)
{
	return AE_SUPPORT;
}



ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void* Context)
{
	return acpi->AcpiOsInstallInterruptHandler(InterruptLevel, Handler, Context);
}
ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler)
{
	return acpi->AcpiOsRemoveInterruptHandler(InterruptNumber, Handler);
}

ACPI_STATUS ACPI::AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void* Context)
{
	m_HAL->RegisterInterrupt((uint8_t)InterruptLevel, { Handler, Context });
	return AE_OK;
}

ACPI_STATUS ACPI::AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler)
{
	m_HAL->UnRegisterInterrupt((uint8_t)InterruptNumber);
	return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER* ExistingTable, ACPI_PHYSICAL_ADDRESS* NewAddress, UINT32* NewTableLength)
{
	*NewAddress = 0;
	*NewTableLength = 0;
	return AE_OK;
}


ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue)
{
	return acpi->AcpiOsEnterSleep(SleepState, RegaValue, RegbValue);
}

ACPI_STATUS ACPI::AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue)
{
	uint32_t status = kernel.PrepareShutdown();
	if(status != 0)
		return AE_CTRL_TERMINATE;
	return AE_OK;
}

ACPI_TABLE_DESC* ACPI::GetAcpiTableBySignature(char sig[4])
{
	for (int i = 0; i < MAX_ACPI_TABLES; i++)
	{
		if(strcmp(acpi_tables[i].Signature.Ascii, sig) == 0)
			return &acpi_tables[i];
	}
	return nullptr;
}

void ACPI::Init()
{

	Printf("Loading ACPI Devices...\r\n");

	ACPI_STATUS Status;
	Status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiInitializeSubsystem: %d\n", Status);
		m_HAL->Halt();
	}

	Status = AcpiInitializeTables(acpi_tables, 16, FALSE);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiInitializeTables: %d\n", Status);
		m_HAL->Halt();
	}

	/* Install the default address space handlers. */
	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT, ACPI_ADR_SPACE_SYSTEM_MEMORY, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not initialise SystemMemory handler, %s!", AcpiFormatException(Status));
		m_HAL->Halt();
	}

	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT, ACPI_ADR_SPACE_SYSTEM_IO, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not initialise SystemIO handler, %s!", AcpiFormatException(Status));
		m_HAL->Halt();
	}

	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT, ACPI_ADR_SPACE_PCI_CONFIG, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not initialise PciConfig handler, %s!", AcpiFormatException(Status));
		m_HAL->Halt();
	}

	Status = AcpiLoadTables();
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiLoadTables: %d\n", Status);
		m_HAL->Halt();
	}

	//Local handlers should be installed here

	Status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiEnableSubsystem: %d\n", Status);
		m_HAL->Halt();
	}

	Status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiInitializeObjects: %d\n", Status);
		m_HAL->Halt();
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

void ACPI::PowerOffSystem()
{
	AcpiEnterSleepStatePrep(5);
	_cli(); //disable interrupts
	AcpiEnterSleepState(5);
	kernel.Panic("power off"); // in case it didn't work!
}


ACPI_TABLE_MADT* ACPI::GetMADTTable()
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


bool ACPI::parseMADT()
{
#if 0
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
	m_HAL->AddDevice(localApicDevice);
#endif
	return true;
}

uint32_t ACPI::RemapIRQ(uint32_t irq)
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
