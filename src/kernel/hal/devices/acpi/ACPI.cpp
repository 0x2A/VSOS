#include "ACPI.h"
#include "kernel/kernel.h"
#include <Assert.h>
#include <OS.arch.h>
#include <kernel\hal\x64\ctrlregs.h>

#include "kernel/hal/devices/pci/PCIBus.h"

extern "C"
{
#include <acpi.h>
#include <aclocal.h>
	const AH_DEVICE_ID* AcpiAhMatchHardwareId(char* HardwareId);
}



/*
   The figure below shows an example ACPI namespace.

   +------+
   | \    |                     Root
   +------+
	 |
	 | +------+
	 +-| _PR  |                 Scope(_PR): the processor namespace
	 | +------+
	 |   |
	 |   | +------+
	 |   +-| CPU0 |             Processor(CPU0): the first processor
	 |     +------+
	 |
	 | +------+
	 +-| _SB  |                 Scope(_SB): the system bus namespace
	 | +------+
	 |   |
	 |   | +------+
	 |   +-| LID0 |             Device(LID0); the lid device
	 |   | +------+
	 |   |   |
	 |   |   | +------+
	 |   |   +-| _HID |         Name(_HID, "PNP0C0D"): the hardware ID
	 |   |   | +------+
	 |   |   |
	 |   |   | +------+
	 |   |   +-| _STA |         Method(_STA): the status control method
	 |   |     +------+
	 |   |
	 |   | +------+
	 |   +-| PCI0 |             Device(PCI0); the PCI root bridge
	 |     +------+
	 |       |
	 |       | +------+
	 |       +-| _HID |         Name(_HID, "PNP0A08"): the hardware ID
	 |       | +------+
	 |       |
	 |       | +------+
	 |       +-| _CID |         Name(_CID, "PNP0A03"): the compatible ID
	 |       | +------+
	 |       |
	 |       | +------+
	 |       +-| RP03 |         Scope(RP03): the PCI0 power scope
	 |       | +------+
	 |       |   |
	 |       |   | +------+
	 |       |   +-| PXP3 |     PowerResource(PXP3): the PCI0 power resource
	 |       |     +------+
	 |       |
	 |       | +------+
	 |       +-| GFX0 |         Device(GFX0): the graphics adapter
	 |         +------+
	 |           |
	 |           | +------+
	 |           +-| _ADR |     Name(_ADR, 0x00020000): the PCI bus address
	 |           | +------+
	 |           |
	 |           | +------+
	 |           +-| DD01 |     Device(DD01): the LCD output device
	 |             +------+
	 |               |
	 |               | +------+
	 |               +-| _BCL | Method(_BCL): the backlight control method
	 |                 +------+
	 |
	 | +------+
	 +-| _TZ  |                 Scope(_TZ): the thermal zone namespace
	 | +------+
	 |   |
	 |   | +------+
	 |   +-| FN00 |             PowerResource(FN00): the FAN0 power resource
	 |   | +------+
	 |   |
	 |   | +------+
	 |   +-| FAN0 |             Device(FAN0): the FAN0 cooling device
	 |   | +------+
	 |   |   |
	 |   |   | +------+
	 |   |   +-| _HID |         Name(_HID, "PNP0A0B"): the hardware ID
	 |   |     +------+
	 |   |
	 |   | +------+
	 |   +-| TZ00 |             ThermalZone(TZ00); the FAN thermal zone
	 |     +------+
	 |
	 | +------+
	 +-| _GPE |                 Scope(_GPE): the GPE namespace ( GPE = General Purpose Event )
	   +------+


*/



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

ACPI_TABLE_DESC* ACPI::GetAcpiTableBySignature(const char sig[4])
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
		m_HAL->Wait();
	}

	Status = AcpiInitializeTables(acpi_tables, 16, FALSE);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiInitializeTables: %d\n", Status);
		m_HAL->Wait();
	}

	/* Install the default address space handlers. */
	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT, ACPI_ADR_SPACE_SYSTEM_MEMORY, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not initialise SystemMemory handler, %s!", AcpiFormatException(Status));
		m_HAL->Wait();
	}

	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT, ACPI_ADR_SPACE_SYSTEM_IO, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not initialise SystemIO handler, %s!", AcpiFormatException(Status));
		m_HAL->Wait();
	}

	Status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT, ACPI_ADR_SPACE_PCI_CONFIG, ACPI_DEFAULT_HANDLER, NULL, NULL);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not initialise PciConfig handler, %s!", AcpiFormatException(Status));
		m_HAL->Wait();
	}

	Status = AcpiLoadTables();
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiLoadTables: %d\n", Status);
		m_HAL->Wait();
	}

	//Local handlers should be installed here

	Status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiEnableSubsystem: %d\n", Status);
		m_HAL->Wait();
	}

	Status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(Status))
	{
		Printf("Could not AcpiInitializeObjects: %d\n", Status);
		m_HAL->Wait();
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

	//setIrqApicMode
	ACPI_OBJECT_LIST param;
	ACPI_OBJECT arg;
	arg.Type = ACPI_TYPE_INTEGER;
	arg.Integer.Value = 1;  // 1 = IOAPIC, 0 = PIC, 2 = IOSAPIC
	param.Count = 1;
	param.Pointer = &arg;
	AcpiEvaluateObject(nullptr, ACPI_STRING("\\_PIC"), &param, nullptr);


	parseMcfg();

	//ACPI_TABLE_FADT* fadt_table = (ACPI_TABLE_FADT*)GetAcpiTableBySignature((char*)ACPI_SIG_FADT);
	//hasLegacyDevices =  (fadt_table->BootFlags & ACPI_FADT_LEGACY_DEVICES);
	//hasPS2 = (fadt_table->BootFlags & ACPI_FADT_8042);

	Printf("ACPI Finished\n");

}

void ACPI::PowerOffSystem()
{
	AcpiEnterSleepStatePrep(ACPI_STATE_S5);
	_cli(); //disable interrupts
	AcpiEnterSleepState(ACPI_STATE_S5);
	
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



void ACPI::parseMcfg()
{
	ACPI_TABLE_DESC* descr = GetAcpiTableBySignature((char*)ACPI_SIG_MCFG);
	if (!descr)
	{
		Printf("Unable to retrieve MCFG table from ACPI\r\n");
		return;
	}
	ACPI_TABLE_MCFG* mcfg;
	if (descr->Flags & ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL)
	{
		mcfg = (ACPI_TABLE_MCFG*)(KernelAcpiStart +  descr->Address);
	}
	else
		mcfg = (ACPI_TABLE_MCFG*)(descr->Address);

	if (!mcfg || mcfg->Header.Signature != ACPI_SIG_MCFG)
	{
	
		Printf("Invalid MCFG table from ACPI\r\n");
		return;
	}

	uint64_t mcfg_end, entry;
	entry = (uint64_t)mcfg;

	mcfg_end = entry + mcfg->Header.Length;
	entry += sizeof(ACPI_TABLE_MCFG);
	while (entry + sizeof(ACPI_MCFG_ALLOCATION) < mcfg_end)
	{
		ACPI_MCFG_ALLOCATION* item = (ACPI_MCFG_ALLOCATION*)entry;
		Printf("PCI enhanced segment, START_BUS = %d, END_BUS = %d, MMIO = 0x%x \r\n", static_cast<int>(item->StartBusNumber), static_cast<int>(item->EndBusNumber), item->Address);

		m_PciEnhancedSegments.push_back(item);

		entry += sizeof(ACPI_MCFG_ALLOCATION);
	}
}

bool getPciDeviceAddr(ACPI_HANDLE device, PciAddress& pciAddr, int parentPciBus)
{
	ACPI_BUFFER addrBuf;
	ACPI_OBJECT obj;
	addrBuf.Length = sizeof(obj);
	addrBuf.Pointer = &obj;

	if (AcpiEvaluateObject(device, ACPI_STRING(METHOD_NAME__ADR), nullptr, &addrBuf) == AE_OK)
	{
		const uint32_t pciDevAndFunc = obj.Integer.Value;
		if (obj.Type == ACPI_TYPE_INTEGER)
		{
			pciAddr.m_bus = parentPciBus;
			pciAddr.m_device = (pciDevAndFunc >> 16) & 0xFFFF;
			pciAddr.m_function = pciDevAndFunc & 0xFFFF;
			return true;
		}
	}

	return false;
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

struct CrsContext
{
	ACPI_PCI_ROUTING_TABLE* m_routingTable;
	PCIBus* pciController;
	unsigned int m_bus;
};

ACPI_STATUS processCrs(ACPI_RESOURCE* resource, void* context)
{
	CrsContext* crsContext = (CrsContext*)context;
	ACPI_PCI_ROUTING_TABLE* routingTable = crsContext->m_routingTable;
	if (resource->Type == ACPI_RESOURCE_TYPE_IRQ)
	{
		const ACPI_RESOURCE_IRQ* irq = &resource->Data.Irq;
		crsContext->pciController->AddDeviceRouting(crsContext->m_bus, (routingTable->Address >> 16), routingTable->Pin + 1, irq->Interrupts[routingTable->SourceIndex]);
	}
	else if (resource->Type == ACPI_RESOURCE_TYPE_EXTENDED_IRQ)
	{
		const ACPI_RESOURCE_EXTENDED_IRQ* irq = &resource->Data.ExtendedIrq;
		crsContext->pciController->AddDeviceRouting(crsContext->m_bus, (routingTable->Address >> 16), routingTable->Pin + 1, irq->Interrupts[routingTable->SourceIndex]);
	}
	return AE_OK;
}

ACPI_STATUS acpiProcessSystemBridge(ACPI_HANDLE bridge, UINT32 level, void* context, void**)
{
	if (level >= 10)
	{
		Printf("ACPI: too long PCI bus depth\r\n");
		return AE_ERROR;
	}

	PCIBus* controller = (PCIBus*)context;

	ACPI_BUFFER addrBuf;
	ACPI_OBJECT obj;
	addrBuf.Length = sizeof(obj);
	addrBuf.Pointer = &obj;

	PciAddress pciAddr;
	/*if (!getPciDeviceAddr(bridge, pciAddr, 0))
	{

		ACPI_DEVICE_INFO* info;
		if (AcpiGetObjectInfo(bridge, &info) != AE_OK)
		{
			Printf("Unable to get PCI System bridge addr\r\n");
			return AE_ERROR;
		}
	}*/

	ACPI_BUFFER rtblBuf;
	rtblBuf.Length = ACPI_ALLOCATE_BUFFER;
	rtblBuf.Pointer = nullptr;
	if (AcpiGetIrqRoutingTable(bridge, &rtblBuf) == AE_OK)
	{
		uint8_t* tableBytes = static_cast<uint8_t*>(rtblBuf.Pointer);
		ACPI_PCI_ROUTING_TABLE* routingTable = static_cast<ACPI_PCI_ROUTING_TABLE*>(rtblBuf.Pointer);
		if(!routingTable) return AE_ERROR;
		while (routingTable->Length != 0)
		{
			if (routingTable->Source[0] == '\0')
			{
				controller->AddDeviceRouting(0, (routingTable->Address >> 16), routingTable->Pin + 1, routingTable->SourceIndex);
			}
			else
			{
				ACPI_HANDLE srcHadle;
				if (AcpiGetHandle(bridge, routingTable->Source, &srcHadle) == AE_OK)
				{
					CrsContext ctx{ routingTable, controller, static_cast<unsigned int>(0) };
					static char crsObject[] = "_CRS";
					AcpiWalkResources(srcHadle, ACPI_STRING(METHOD_NAME__CRS), &processCrs, &ctx);
				}
			}
			tableBytes += routingTable->Length;
			routingTable = reinterpret_cast<ACPI_PCI_ROUTING_TABLE*>(tableBytes);
		}
	}


	return AE_OK;
}

ACPI_STATUS ACPI::EvaluateOneDevice(ACPI_HANDLE ObjHandle, UINT32 NestingLevel, void* Context, void** ReturnValue)
{
	ACPI_DEVICE_INFO* info;
	if (AcpiGetObjectInfo(ObjHandle, &info) != AE_OK)
	{
		Printf("Unable to get PCI System bridge addr\r\n");
		return AE_ERROR;
	}

	if (info->HardwareId.Length > 0)
	{
		const AH_DEVICE_ID* id = ::AcpiAhMatchHardwareId(info->HardwareId.String);
		if(id)
			Printf("---- HID: %s, %s\r\n", id->Name, id->Description);
		else
			Printf("---- HID: %s\r\n", info->HardwareId.String);

	}
	
	
	return AE_OK;
}



void ACPI::EnumerateDevices()
{
	AcpiGetHandle(NULL, ACPI_STRING("\\_SB"), &SysBusHandle);

	Printf("- Listing ACPI Devices: -\r\n");
	//Construct ACPI root device
	ACPI_HANDLE root;
	ACPI_STATUS status = AcpiGetHandle(NULL, ACPI_STRING(ACPI_NS_ROOT_PATH), &root);
	Assert(ACPI_SUCCESS(status));
	
	
	AcpiWalkNamespace(ACPI_TYPE_DEVICE, SysBusHandle, INT_MAX, ACPI::EvaluateOneDevice, NULL, this, nullptr);

	Printf("---------------------\r\n");
}