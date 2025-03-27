#include "ACPIDevice.h"

#include <windows/types.h>
#include <cstdint>

extern "C"
{
#include <aclocal.h>
	const AH_DEVICE_ID* AcpiAhMatchHardwareId(char* HardwareId);
}
#include "Assert.h"
#include <algorithm>

AcpiDevice::AcpiDevice(const ACPI_HANDLE object) :
	Device(),
	m_acpiObject(object),
	m_resources(),
	m_objectInfo()
{

}

void AcpiDevice::Initialize(void* context)
{
	ACPI_BUFFER result = { ACPI_ALLOCATE_BUFFER, nullptr };
	ACPI_STATUS status = AcpiGetName(m_acpiObject, ACPI_FULL_PATHNAME, &result);
	Assert(ACPI_SUCCESS(status));

	Assert(result.Pointer);
	memcpy(Path, (char*)result.Pointer, result.Length);
	//std::replace(Path.begin(), Path.end(), '.', '\\');
	delete result.Pointer;

	//ACPI_DEVICE_INFO* info;
	status = AcpiGetObjectInfo(m_acpiObject, &m_objectInfo);
	Assert(ACPI_SUCCESS(status));

	if (m_objectInfo->Valid & ACPI_VALID_HID)
	{
		if(m_objectInfo->HardwareId.Length > 0)
		{
			m_hid = m_objectInfo->HardwareId.String;
			Printf("HID: %s\r\n", m_hid.c_str());
		}

		//Get description if available
		const AH_DEVICE_ID* id = ::AcpiAhMatchHardwareId(m_objectInfo->HardwareId.String);
		if (id != NULL)
		{
			char buff[1024];
			strcpy(buff, id->Description);
			buff[strlen(id->Description)] = '\0';
			Description = buff;
			//Printf("Descr: %s\r\n", Description.c_str());
			
		}
	}

	//These comments were helpful during bringup, probably dont need them anymore
	/*
	Printf("F: 0x%02x V: 0x%04x T: 0x%08x\n", m_objectInfo->Flags, m_objectInfo->Valid, m_objectInfo->Type);

	if (m_objectInfo->Valid & ACPI_VALID_ADR)
		Printf("    ADR: 0x%x\n", m_objectInfo->Address);

	if (m_objectInfo->Valid & ACPI_VALID_HID)
		Printf("    HID: %s\n", m_objectInfo->HardwareId.String);

	if (m_objectInfo->Valid & ACPI_VALID_UID)
		Printf("    UID: %s\n", m_objectInfo->UniqueId.String);

	if (m_objectInfo->Valid & ACPI_VALID_CID)
		for (size_t i = 0; i < m_objectInfo->CompatibleIdList.Count; i++)
		{
			Printf("    CID: %s\n", m_objectInfo->CompatibleIdList.Ids[i].String);
		}
	
	*/
	//DDN: Object that associates a logical software name (for example, COM1) with a device.
	result = { ACPI_ALLOCATE_BUFFER, nullptr };
	status = AcpiEvaluateObjectTyped(m_acpiObject, ACPI_STRING(METHOD_NAME__DDN), nullptr, &result, ACPI_TYPE_STRING);
	if (ACPI_SUCCESS(status))
	{
		ACPI_OBJECT* object = (ACPI_OBJECT*)result.Pointer;
		Assert(object);
		Name = object->String.Pointer;
		delete result.Pointer;
	}



	//Wire up resources
	AcpiWalkResources(m_acpiObject, ACPI_STRING(METHOD_NAME__CRS), AcpiDevice::AttachResource, this);

	AcpiWalkResources(m_acpiObject, ACPI_STRING(METHOD_NAME__DMA), AcpiDevice::AttachDMA, this);

	result = { ACPI_ALLOCATE_BUFFER, nullptr };
	status = AcpiGetIrqRoutingTable(m_acpiObject, &result);
	if (ACPI_SUCCESS(status))
	{
		ACPI_PCI_ROUTING_TABLE* routingTable = ACPI_CAST_PTR(ACPI_PCI_ROUTING_TABLE, result.Pointer);
		Printf("Routing Table: %x, %x, %x, %x, %x\r\n", routingTable->Address, routingTable->Length, routingTable->Pin, routingTable->Source, routingTable->SourceIndex);
	}

}

const void* AcpiDevice::GetResource(uint32_t type) const
{
	for (const auto& resource : this->m_resources)
	{
		if (resource.Type == type)
			return &resource;
	}
	return nullptr;
}

const void* AcpiDevice::GetDeviceObject(char* PathName, ACPI_OBJECT_LIST* ExternalParams)
{
	ACPI_BUFFER buffer = { ACPI_ALLOCATE_BUFFER, nullptr };

	ACPI_STATUS status = AcpiEvaluateObject(m_acpiObject, PathName, ExternalParams,&buffer );
	if(ACPI_FAILURE(status))
		return nullptr;

	return buffer.Pointer;
}

void AcpiDevice::DisplayDetails() const
{
	for (const auto& item : m_resources)
		DisplayResource(item);
}

//TODO: this should convert ACPI descriptors to abstract ones, but for now just bin them
ACPI_STATUS AcpiDevice::AttachResource(ACPI_RESOURCE* Resource, void* Context)
{
	AcpiDevice* pDevice = (AcpiDevice*)Context;
	switch (Resource->Type)
	{
	case ACPI_RESOURCE_TYPE_ADDRESS32:
	case ACPI_RESOURCE_TYPE_ADDRESS64:
	case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
	case ACPI_RESOURCE_TYPE_IRQ:
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
	case ACPI_RESOURCE_TYPE_IO:
		//Remove if resource already exists
		pDevice->m_resources.remove_if([&](const ACPI_RESOURCE& _Other) { return _Other.Type == Resource->Type; });
		pDevice->m_resources.push_back(*Resource);
		break;
	}

	return AE_OK;
}

ACPI_STATUS AcpiDevice::DisplayResource(const ACPI_RESOURCE& Resource)
{
	int i;
	switch (Resource.Type) {
	case ACPI_RESOURCE_TYPE_IRQ:
		Printf("    IRQ: dlen=%d trig=%d pol=%d shar=%d intc=%d cnt=%d int=",
			Resource.Data.Irq.DescriptorLength,
			Resource.Data.Irq.Triggering,
			Resource.Data.Irq.Polarity,
			Resource.Data.Irq.Shareable,
			Resource.Data.Irq.WakeCapable,
			Resource.Data.Irq.InterruptCount);
		for (i = 0; i < Resource.Data.Irq.InterruptCount; i++)
			Printf("%.02X ", Resource.Data.Irq.Interrupts[i]);
		Printf("\n");
		break;
	case ACPI_RESOURCE_TYPE_IO:
	{
		Printf("     IO: Decode=0x%x Align=0x%x AddrLen=%d Min=0x%.04X Max=0x%.04X\n",
			Resource.Data.Io.IoDecode,
			Resource.Data.Io.Alignment,
			Resource.Data.Io.AddressLength,
			Resource.Data.Io.Minimum,
			Resource.Data.Io.Maximum);
	}
	break;
	case ACPI_RESOURCE_TYPE_END_TAG:
		Printf("    END:\n");
		break;
	case ACPI_RESOURCE_TYPE_ADDRESS16:
		Printf("    A16: 0x%04X-0x%04X, Gran=0x%04X, Offset=0x%04X\n",
			Resource.Data.Address16.Address.Minimum,
			Resource.Data.Address16.Address.Maximum,
			Resource.Data.Address16.Address.Granularity,
			Resource.Data.Address16.Address.TranslationOffset);
		break;
	case ACPI_RESOURCE_TYPE_ADDRESS32:
		Printf("    A32: 0x%08X-0x%08X, Gran=0x%08X, Offset=0x%08X\n",
			Resource.Data.Address32.Address.Minimum,
			Resource.Data.Address32.Address.Maximum,
			Resource.Data.Address32.Address.Granularity,
			Resource.Data.Address32.Address.TranslationOffset);
		Printf("         T: %d, PC: %d, Decode=0x%x, Min=0x%02X, Max=0x%02X\n",
			Resource.Data.Address32.ResourceType,
			Resource.Data.Address32.ProducerConsumer,
			Resource.Data.Address32.Decode,
			Resource.Data.Address32.MinAddressFixed,
			Resource.Data.Address32.MaxAddressFixed);
		break;
	case ACPI_RESOURCE_TYPE_ADDRESS64:
		Printf("    A64: 0x%016X-0x%016X, Gran=0x%016X, Offset=0x%016X\n",
			Resource.Data.Address64.Address.Minimum,
			Resource.Data.Address64.Address.Maximum,
			Resource.Data.Address64.Address.Granularity,
			Resource.Data.Address64.Address.TranslationOffset);
		Printf("       : T: %d, PC: %d, Decode=0x%x, Min=0x%02X, Max=0x%02X\n",
			Resource.Data.Address64.ResourceType,
			Resource.Data.Address64.ProducerConsumer,
			Resource.Data.Address64.Decode,
			Resource.Data.Address64.MinAddressFixed,
			Resource.Data.Address64.MaxAddressFixed);
		break;
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
		Printf("   EIRQ: PC=%d Trig=%d Pol=%d Share=%d Wake=%d Cnt=%d Int=",
			Resource.Data.ExtendedIrq.ProducerConsumer,
			Resource.Data.ExtendedIrq.Triggering,
			Resource.Data.ExtendedIrq.Polarity,
			Resource.Data.ExtendedIrq.Shareable,
			Resource.Data.ExtendedIrq.WakeCapable,
			Resource.Data.ExtendedIrq.InterruptCount);
		for (i = 0; i < Resource.Data.ExtendedIrq.InterruptCount; i++)
			Printf("%.02X ", Resource.Data.ExtendedIrq.Interrupts[i]);
		Printf("\n");
		break;
	case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
		Printf("    M32: Addr=0x%016x, Len=0x%x, WP=%d\n",
			Resource.Data.FixedMemory32.Address,
			Resource.Data.FixedMemory32.AddressLength,
			Resource.Data.FixedMemory32.WriteProtect);
		break;
	case ACPI_RESOURCE_TYPE_DMA:
		Printf("   DMA: BM=%d CC=%d Tr=%d Type=%d Int=",
			Resource.Data.Dma.BusMaster,
			Resource.Data.Dma.ChannelCount,
			Resource.Data.Dma.Transfer,
			Resource.Data.Dma.Type);
		for(i = 0; i < Resource.Data.Dma.ChannelCount; i++)
			Printf("%.02X ", Resource.Data.Dma.Channels[i]);
		Printf("\n");
	default:
		Printf("    Unknown: Type=%d\n", Resource.Type);
		break;

	}
	return AE_OK;
}

ACPI_STATUS AcpiDevice::AttachDMA(ACPI_RESOURCE* Resource, void* Context)
{
	if (Resource->Type == ACPI_RESOURCE_TYPE_DMA)
	{
		DisplayResource(*Resource);
	}
	return AE_OK;
}
