#include "PCIDevice.h"
#include <Assert.h>
#include "PCIBus.h"



PCIDevice::PCIDevice(PCIBus* pciBus, PCIDeviceDescriptor descr)
: m_PCIBus(pciBus), m_Descriptor(descr)
{
	Name = "Unknown";
	Description = "Unknown PCI Device";
	this->Path = "\\_SB\\PCI";
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
	Printf("   Rev=0x%x, INT=0x%x\r\n", m_Descriptor.revision, m_Descriptor.interrupt);
}
