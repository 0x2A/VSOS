#include "PCIDevice.h"
#include <Assert.h>
#include "PCIBus.h"
#include <map>



PCIDevice::PCIDevice(PCIBus* pciBus, PCIDeviceDescriptor descr)
: m_PCIBus(pciBus), m_Descriptor(descr)
{
	Name = "Unknown";
	Description = "Unknown PCI Device";
	
	char buffer[1024];
	sprintf(this->Path, "\\_SB\\PCI(%d)\\DEV(%d)\\FUNC(%d)", m_Descriptor.bus, m_Descriptor.device, m_Descriptor.function);
	this->Type = DeviceType::Unknown;
}

void PCIDevice::Initialize(void* context)
{
	DisplayDetails();
}

const void* PCIDevice::GetResource(uint32_t type) const
{
	return nullptr;
}

void PCIDevice::DisplayDetails() const
{
	Printf("PCI Device: %s\r\n", m_Descriptor.GetType());
	Printf("   Bus=%d, Device=0x%4x, Func=0x%x\r\n", m_Descriptor.bus,m_Descriptor.device,m_Descriptor.function);
	Printf("   Ven=0x%x, DevID=0x%x\r\n", m_Descriptor.vendor_id, m_Descriptor.device_id);
	Printf("   Class=0x%x, SubClass=0x%x, IF=0x%x\r\n", m_Descriptor.class_id, m_Descriptor.vendor_id, m_Descriptor.interface_id);
	Printf("   Rev=0x%x, INT=0x%x\r\n", m_Descriptor.revision, m_Descriptor.interruptLine);
}

uint32_t PCIDevice::readBus(uint32_t registeroffset)
{
	return m_PCIBus->read(m_Descriptor.bus, m_Descriptor.device, m_Descriptor.function, registeroffset);
}

void PCIDevice::writeBus(uint32_t registeroffset, uint32_t value)
{
	m_PCIBus->write(m_Descriptor.bus, m_Descriptor.device, m_Descriptor.function, registeroffset, value);
}

const BaseAddressRegister& PCIDevice::GetBAR(uint8_t id)
{
	Assert(id < 6);
	return m_Descriptor.bars[id];
}

void PCIDevice::SetMemEnabled(bool enabled)
{
	/* Mem space enable, IO space enable, bus mastering. */
	const uint16_t flags = 0x0007;
	if(enabled)
		m_Descriptor.command |= flags;
	else
		m_Descriptor.command &= ~flags;

	m_PCIBus->write(m_Descriptor.bus, m_Descriptor.device, m_Descriptor.function, PCI_DESCR_OFFSET_COMMAND, m_Descriptor.command);
}
