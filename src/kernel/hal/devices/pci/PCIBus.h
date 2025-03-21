#pragma once

#include "kernel/hal/devices/Device.h"
#include <string>
#include <vector>

class HAL;

namespace PciLimits
{
	enum
	{
		Functions = 8,
		Devices = 32,
		Buses = 256,
		Pins = 4
	};
}


struct PciAddress
{
	unsigned int m_bus;
	unsigned int m_device;
	unsigned int m_function;
};

struct PciBusRounting
{
	PciBusRounting(unsigned int parent, unsigned int device)
		: m_parent(parent)
		, m_device(device)
	{
	}
	unsigned int m_parent;
	unsigned int m_device;
	unsigned int m_irq[PciLimits::Devices][PciLimits::Pins] = {};
};

enum BaseAddressRegisterType
{        //Used for the last bit of the address register
	MemoryMapping = 0,
	InputOutput = 1
};

/**
 * @class BaseAddressRegister
 * @brief Used to store the base address register
 */
struct BaseAddressRegister {
	bool pre_fetchable;
	uint8_t* address;
	uint32_t size;
	BaseAddressRegisterType type;

};


/**
* @class PCIDeviceDescriptor
* @brief Stores information about a PCI device
*/

#define PCI_DESCR_OFFSET_VEN_ID		0x00
#define PCI_DESCR_OFFSET_DEV_ID		0x02
#define PCI_DESCR_OFFSET_COMMAND	0x04
#define PCI_DESCR_OFFSET_STATUS		0x06
#define PCI_DESCR_OFFSET_REV		0x08
#define PCI_DESCR_OFFSET_INTF_ID	0x09
#define PCI_DESCR_OFFSET_HEAD_TYPE	0x0E
#define PCI_DESCR_OFFSET_BAR0		0x10

class PCIDeviceDescriptor
{
public:
    bool has_port_base;
    uint32_t port_base;  //Port used for communication

    bool has_memory_base;
    uint32_t memory_base;  //Mem address used for communication

    uint8_t interruptLine; //The interrupt
	uint8_t interruptPin;

    uint16_t bus;
    uint16_t device;
    uint16_t function;
	
	uint16_t command;
	uint16_t status;

    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t class_id;
    uint8_t subclass_id;
    uint8_t interface_id;

    uint8_t revision;

	uint8_t cacheLineSize;
	uint8_t  latTimer;
	uint8_t  BIST;

	BaseAddressRegister bars[6];

	const char* GetType() const
	{
		switch (class_id)
		{
		case 0x00: return (subclass_id == 0x01) ? "VGA" : "Legacy";
		case 0x01:
			switch (subclass_id)
			{
			case 0x01:  return "IDE interface";
			case 0x06:  return "SATA controller";
			default:    return "Storage";
			}
		case 0x02: return "Network";
		case 0x03: return "Display";
		case 0x04:
			switch (subclass_id)
			{
			case 0x00:  return "Video";
			case 0x01:
			case 0x03:  return "Audio";
			default:    return "Multimedia";
			}
		case 0x06:
			switch (subclass_id)
			{
			case 0x00:  return "Host bridge";
			case 0x01:  return "ISA bridge";
			case 0x04:  return "PCI bridge";
			default:    return "Bridge";
			}
		case 0x07:
			switch (subclass_id)
			{
			case 0x00:  return "Serial controller";
			case 0x80:  return "Communication controller";
			}
			break;
		case 0x0C:
			switch (subclass_id)
			{
			case 0x03:  return "USB";
			case 0x05:  return "System Management Bus";
			}
			break;
		}
		return "Unknown";
	}
} ;


class PCIBus
{
friend class PCIDevice;

public:
	PCIBus(HAL* hal);

	void Initialize(void* context);
	
	bool RegisterRoutingBus(uint32_t bus, uint32_t parentBus, uint32_t device);
	bool AddDeviceRouting(uint32_t bus, uint32_t device, uint32_t pin, uint32_t irq);
	uint32_t getPciDeviceIrq(uint32_t bus, uint32_t device, uint32_t pin);

	void FindDeviceDrivers();
protected:



	// I/O
	uint32_t read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset);
	void write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value);

	bool device_has_functions(uint16_t bus, uint16_t device);
	PCIDeviceDescriptor get_device_descriptor(uint16_t bus, uint16_t device, uint16_t function);
	BaseAddressRegister get_base_address_register(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar);

	HAL* m_HAL;

	PciBusRounting* g_routingTable[PciLimits::Buses];
	std::vector<PCIDeviceDescriptor> m_DeviceDescriptors;
};