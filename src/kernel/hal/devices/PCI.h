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

/**
         * @class PCIDeviceDescriptor
         * @brief Stores information about a PCI device
         */
struct PCIDeviceDescriptor
{
    bool has_port_base;
    uint32_t port_base;  //Port used for communication

    bool has_memory_base;
    uint32_t memory_base;  //Mem address used for communication

    uint32_t interrupt; //The interrupt

    uint16_t bus;
    uint16_t device;
    uint16_t function;

    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t class_id;
    uint8_t subclass_id;
    uint8_t interface_id;

    uint8_t revision;

	std::string get_type() 
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

} ;


class PCIController
{

public:
	PCIController(HAL* hal);

	void Initialize(void* context);
	
	bool RegisterRoutingBus(uint32_t bus, uint32_t parentBus, uint32_t device);
	bool AddDeviceRouting(uint32_t bus, uint32_t device, uint32_t pin, uint32_t irq);
	uint32_t getPciDeviceIrq(uint32_t bus, uint32_t device, uint32_t pin);
private:
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