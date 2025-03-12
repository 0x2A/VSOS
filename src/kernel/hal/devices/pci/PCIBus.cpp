#include "PCIBus.h"
#include "kernel/hal/HAL.h"
#include <Assert.h>
#include "PCIDevice.h"
#include <kernel\hal\devices\DeviceTree.h>
#include <kernel\drivers\io\AHCI.h>
#include <kernel\drivers\video\vmware_svga2.h>

#define PCI_PORT_COMMAND	0xCF8
#define PCI_PORT_DATA		0xCFC

PCIBus::PCIBus(HAL* hal)
	: m_HAL(hal)
{

}

void PCIBus::Initialize(void* context)
{	
	Printf("- Listing all PCI Devices: -\r\n");
	//Brute force all busses
	for (int bus = 0; bus < 8; ++bus) {
		for (int device = 0; device < 32; ++device) {
			
			int numFunctions = (device_has_functions(bus, device)) ? 8 : 1;
			for (int function = 0; function < numFunctions; ++function) 
			{
				// Get the device descriptor, if the vendor id is 0x0000 or 0xFFFF, the device is not present/ready
				PCIDeviceDescriptor deviceDescriptor = get_device_descriptor(bus, device, function);
				if (deviceDescriptor.vendor_id == 0x0000 || deviceDescriptor.vendor_id == 0x0001 || deviceDescriptor.vendor_id == 0xFFFF)
					continue;

				// Get port number
				for (int barNum = 5; barNum >= 0; barNum--) {
					BaseAddressRegister bar = get_base_address_register(bus, device, function, barNum);
					
					if (bar.address && (bar.type == InputOutput))
					{
						deviceDescriptor.port_base = (uint32_t)bar.address;
						deviceDescriptor.has_port_base = true;
					}

					deviceDescriptor.bars[barNum] = bar;
				}

				m_DeviceDescriptors.push_back(deviceDescriptor);
				//Printf("----- PCI: %s, VID: 0x%x, DEV: 0x%x\r\n", deviceDescriptor.GetType(), deviceDescriptor.vendor_id, deviceDescriptor.device_id);
				PCIDevice* device = new PCIDevice(this, deviceDescriptor);
				device->Initialize(this);

				DeviceTree::AddPCIDevice(device);
			}
		}
	}
	FindDeviceDrivers();

	Printf("---------------------\r\n");
}

bool PCIBus::RegisterRoutingBus(uint32_t bus, uint32_t parentBus, uint32_t device)
{
	if ((bus >= PciLimits::Buses) || (parentBus >= PciLimits::Buses) || (device >= PciLimits::Devices))
		return false;

	g_routingTable[bus] = new PciBusRounting(parentBus, device);
	return true;
}

bool PCIBus::AddDeviceRouting(uint32_t bus, uint32_t device, uint32_t pin, uint32_t irq)
{
	
	--pin;
	if ((pin >= PciLimits::Pins) || (bus >= PciLimits::Buses) || (device >= PciLimits::Devices))
		return false;

	PciBusRounting* routing = g_routingTable[bus];
	if (!routing)
		return false;

	routing->m_irq[device][pin] = irq;

	Printf("PCI: Added device routing on bus %d, dev= %d, pin=%d, irq=%d\r\n", bus, device, pin, irq);
	return true;
}

uint32_t PCIBus::getPciDeviceIrq(uint32_t bus, uint32_t device, uint32_t pin)
{
	--pin;
	if ((pin >= PciLimits::Pins) || (bus >= PciLimits::Buses) || (device >= PciLimits::Devices))
		return 0;

	const PciBusRounting* routing = g_routingTable[bus];
	if (!routing)
		return 0;

	unsigned int irq = routing->m_irq[device][pin];
	unsigned int offset = device + pin;
	while (irq == 0)
	{
		const unsigned int paretBus = g_routingTable[bus]->m_parent;
		if (bus == paretBus)
			break;

		offset += g_routingTable[bus]->m_device;
		bus = paretBus;
		irq = g_routingTable[bus]->m_irq[0][offset % PciLimits::Pins];
	}
	return irq;
}

void PCIBus::FindDeviceDrivers()
{
	for (auto dev : DeviceTree::m_PCIDevices)
	{
		auto descr = dev->GetDeviceDescriptor();

		switch (descr.vendor_id)
		{
			case PCI_VEN_ID_INTEL:
			switch(descr.device_id)
			{
				case PCI_DEVICE_ID_INTEL_82801IR:
					AHCIDriver* ahciDriver = new AHCIDriver(dev);
					m_HAL->driverManager->OnDriverSelected(ahciDriver);
					Printf("PCIBUS: Driver registered for VID=%x DEV=%0x BUS=%x DEV=%x\r\n", descr.vendor_id, descr.device_id, descr.bus, descr.device);
				break;
				
			}
			break;
			case PCI_VENDOR_ID_VMWARE:
			{
				switch (descr.device_id)
				{
					case PCI_DEVICE_ID_VMWARE_SVGA2:
						VMware_SVGA2* svga = new VMware_SVGA2(dev, m_HAL);
						m_HAL->driverManager->OnDriverSelected(svga);
						Printf("PCIBUS: Driver registered for VID=%x DEV=%0x BUS=%x DEV=%x\r\n", descr.vendor_id, descr.device_id, descr.bus, descr.device);
					break;
				}
			}
			break;
		}
	}
}

uint32_t PCIBus::read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset)
{
	// Calculate the id
	uint32_t id = 0x1 << 31
		| ((bus & 0xFF) << 16)
		| ((device & 0x1F) << 11)
		| ((function & 0x07) << 8)
		| (registeroffset & 0xFC);
	m_HAL->WritePort(PCI_PORT_COMMAND, id, 32);

	// read the data from the port
	uint32_t result = m_HAL->ReadPort(PCI_PORT_DATA, 32);
	return result >> (8 * (registeroffset % 4));
}

void PCIBus::write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value)
{
	// Calculate the id
	uint32_t id = 0x1 << 31
		| ((bus & 0xFF) << 16)
		| ((device & 0x1F) << 11)
		| ((function & 0x07) << 8)
		| (registeroffset & 0xFC);
	m_HAL->WritePort(PCI_PORT_COMMAND, id, 32);

	// write the data to the port
	m_HAL->WritePort(PCI_PORT_DATA, value, 32);
}


bool PCIBus::device_has_functions(uint16_t bus, uint16_t device)
{
	return read(bus, device, 0, 0x0E) & (1 << 7);
}

PCIDeviceDescriptor PCIBus::get_device_descriptor(uint16_t bus, uint16_t device, uint16_t function)
{
	PCIDeviceDescriptor result;
	result.bus = bus;
	result.device = device;
	result.function = function;

	result.vendor_id = read(bus, device, function, PCI_DESCR_OFFSET_VEN_ID);
	result.device_id = read(bus, device, function, PCI_DESCR_OFFSET_DEV_ID);
	result.command = read(bus, device, function, PCI_DESCR_OFFSET_COMMAND);
	result.status = read(bus, device, function, 0x06);
	result.revision = read(bus, device, function, 0x8);
	result.interface_id = read(bus, device, function, 0x09);
	result.subclass_id = read(bus, device, function, 0x0A);
	result.class_id = read(bus, device, function, 0x0B);
	result.cacheLineSize = read(bus, device, function, 0x0C);
	result.latTimer = read(bus, device, function, 0x0D);
	result.BIST = read(bus, device, function, 0x0F);
	
	result.interruptLine = read(bus, device, function, 0x3C);
	result.interruptPin = read(bus, device, function, 0x3D);

	return result;
}

BaseAddressRegister PCIBus::get_base_address_register(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar)
{
	BaseAddressRegister result;

	// only types 0x00 (normal devices) and 0x01 (PCI-to-PCI bridges) are supported:
	uint32_t headerType = read(bus, device, function, PCI_DESCR_OFFSET_HEAD_TYPE);
	if (headerType & 0x3F)
		return result;

	// read the base address register
	uint32_t bar_value = read(bus, device, function, PCI_DESCR_OFFSET_BAR0 + 4 * bar);
	result.type = (bar_value & 0x1) ? InputOutput : MemoryMapping;
	result.address = (uint8_t*)(bar_value & ~0xF);

	// read the size of the base address register
	write(bus, device, function, PCI_DESCR_OFFSET_BAR0 + 4 * bar, 0xFFFFFFF0 | result.type);
	result.size = read(bus, device, function, PCI_DESCR_OFFSET_BAR0 + 4 * bar);
	result.size = (~result.size | 0xF) + 1;

	// Restore the original value of the base address register
	write(bus, device, function, PCI_DESCR_OFFSET_BAR0 + 4 * bar, bar_value);

	return result;
}
