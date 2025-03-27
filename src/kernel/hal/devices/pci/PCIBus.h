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


typedef struct 
{
	uint8_t classID;
	uint8_t subclassID;
	const char* Description;

} ClassCode;
const ClassCode ClassCodes[] = {
{ 0x00, 0x00, "Unclassified" },
{ 0x01, 0x00, "Mass Storage Controller - SCSI Bus Controller"},
{ 0x01, 0x01, "Mass Storage Controller - IDE Controller"},
{ 0x01, 0x02, "Mass Storage Controller - Floppy Disk Controller"},
{ 0x01, 0x03, "Mass Storage Controller - IPI Bus Controller"},
{ 0x01, 0x04, "Mass Storage Controller - RAID Controller"},
{ 0x01, 0x05, "Mass Storage Controller - ATA Controller"},
{ 0x01, 0x06, "Mass Storage Controller - Serial ATA"},
{ 0x01, 0x07, "Mass Storage Controller - Serial Attached SCSI"},
{ 0x01, 0x08, "Mass Storage Controller - Non - Volatile Memory Controller"},
{ 0x01, 0x80, "Mass Storage Controller - Other"},
{ 0x02, 0x00, "Network Controller - Ethernet Controller"},
{ 0x02, 0x01, "Network Controller - Token Ring Controller"},
{ 0x02, 0x02, "Network Controller - FDDI Controller"},
{ 0x02, 0x03, "Network Controller - ATM Controller"},
{ 0x02, 0x04, "Network Controller - ISDN Controller"},
{ 0x02, 0x05, "Network Controller - WorldFip Controller"},
{ 0x02, 0x06, "Network Controller - PICMG 2.14 Multi Computing"},
{ 0x02, 0x07, "Network Controller - Infiniband Controller"},
{ 0x02, 0x08, "Network Controller - Fabric Controller"},
{ 0x02, 0x80, "Network Controller - Other"},
{ 0x03, 0x00, "Display Controller - VGA Compatible Controller"},
{ 0x03, 0x01, "Display Controller - XGA Controller"},
{ 0x03, 0x02, "Display Controller - 3D Controller(Not VGA - Compatible)"},
{ 0x03, 0x80, "Display Controller - Other"},
{ 0x04, 0x00, "Multimedia Controller - Multimedia Video Controller"},
{ 0x04, 0x01, "Multimedia Controller - Multimedia Audio Controller"},
{ 0x04, 0x02, "Multimedia Controller - Computer Telephony Device"},
{ 0x04, 0x03, "Multimedia Controller - Audio Device"},
{ 0x04, 0x80, "Multimedia Controller - Other"},
{ 0x05, 0x00, "Memory Controller - RAM Controller"},
{ 0x05, 0x01, "Memory Controller - Flash Controller"},
{ 0x05, 0x80, "Memory Controller - Other"},
{ 0x06, 0x00, "Bridge Device - Host Bridge"},
{ 0x06, 0x01, "Bridge Device - ISA Bridge"},
{ 0x06, 0x02, "Bridge Device - EISA Bridge"},
{ 0x06, 0x03, "Bridge Device - MCA Bridge"},
{ 0x06, 0x04, "Bridge Device - PCI - to - PCI Bridge"},
{ 0x06, 0x05, "Bridge Device - PCMCIA Bridge"},
{ 0x06, 0x06, "Bridge Device - NuBus Bridge"},
{ 0x06, 0x07, "Bridge Device - CardBus Bridge"},
{ 0x06, 0x08, "Bridge Device - RACEway Bridge"},
{ 0x06, 0x09, "Bridge Device - PCI - to - PCI Bridge"},
{ 0x06, 0x0A, "Bridge Device - InfiniBand - to - PCI Host Bridge"},
{ 0x06, 0x80, "Bridge Device - Other"},
{ 0x07, 0x00, "Simple Communication Controller - Serial Controller"},
{ 0x07, 0x01, "Simple Communication Controller - Parallel Controller"},
{ 0x07, 0x02, "Simple Communication Controller - Multiport Serial Controller"},
{ 0x07, 0x03, "Simple Communication Controller - Modem"},
{ 0x07, 0x04, "Simple Communication Controller - IEEE 488.1 / 2 (GPIB)Controller"},
{ 0x07, 0x05, "Simple Communication Controller - Smart Card"},
{ 0x07, 0x80, "Simple Communication Controller - Other"},
{ 0x08, 0x00, "Base System Peripheral - PIC"},
{ 0x08, 0x01, "Base System Peripheral - DMA Controller"},
{ 0x08, 0x02, "Base System Peripheral - Timer"},
{ 0x08, 0x03, "Base System Peripheral - RTC Controller"},
{ 0x08, 0x04, "Base System Peripheral - PCI Hot - Plug Controller"},
{ 0x08, 0x05, "Base System Peripheral - SD Host controller"},
{ 0x08, 0x06, "Base System Peripheral - IOMMU"},
{ 0x08, 0x80, "Base System Peripheral - Other"},
{ 0x09, 0x00, "Input Device Controller - Keyboard Controller"},
{ 0x09, 0x01, "Input Device Controller - Digitizer Pen"},
{ 0x09, 0x02, "Input Device Controller - Mouse Controller"},
{ 0x09, 0x03, "Input Device Controller - Scanner Controller"},
{ 0x09, 0x04, "Input Device Controller - Gameport Controller"},
{ 0x09, 0x80, "Input Device Controller - Other"},
{ 0x0A, 0x00, "Docking Station - Generic"},
{ 0x0A, 0x80, "Docking Station - Other"},
{ 0x0B, 0x00, "Processor - 386"},
{ 0x0B, 0x01, "Processor - 486"},
{ 0x0B, 0x02, "Processor - Pentium"},
{ 0x0B, 0x10, "Processor - Alpha"},
{ 0x0B, 0x20, "Processor - PowerPC"},
{ 0x0B, 0x30, "Processor - MIPS"},
{ 0x0B, 0x40, "Processor - Co - Processor"},
{ 0x0C, 0x00, "Serial Bus Controller - FireWire(IEEE 1394) Controller"},
{ 0x0C, 0x01, "Serial Bus Controller - ACCESS Bus"},
{ 0x0C, 0x02, "Serial Bus Controller - SSA"},
{ 0x0C, 0x03, "Serial Bus Controller - USB Controller"},
{ 0x0C, 0x04, "Serial Bus Controller - Fibre Channel"},
{ 0x0C, 0x05, "Serial Bus Controller - SMBus"},
{ 0x0C, 0x06, "Serial Bus Controller - InfiniBand"},
{ 0x0C, 0x07, "Serial Bus Controller - IPMI Interface"},
{ 0x0C, 0x08, "Serial Bus Controller - SERCOS Interface(IEC 61491)"},
{ 0x0C, 0x09, "Serial Bus Controller - CANbus"},
{ 0x0D, 0x00, "Wireless Controller - iRDA Compatible Controller"},
{ 0x0D, 0x01, "Wireless Controller - Consumer IR Controller"},
{ 0x0D, 0x10, "Wireless Controller - RF Controller"},
{ 0x0D, 0x11, "Wireless Controller - Bluetooth Controller"},
{ 0x0D, 0x12, "Wireless Controller - Broadband Controller"},
{ 0x0D, 0x20, "Wireless Controller - Ethernet Controller(802.1a)"},
{ 0x0D, 0x21, "Wireless Controller - Ethernet Controller(802.1b)"},
{ 0x0D, 0x80, "Wireless Controller - Other"},
{ 0x0E, 0x00, "Intelligent Controller - I20"},
{ 0x0F, 0x01, "Satellite Communication Controller - Satellite TV Controller"},
{ 0x0F, 0x02, "Satellite Communication Controller - Satellite Audio Controller"},
{ 0x0F, 0x03, "Satellite Communication Controller - Satellite Voice Controller"},
{ 0x0F, 0x04, "Satellite Communication Controller - Satellite Data Controller"},
{ 0x10, 0x00, "Encryption Controller - Network and Computing Encrpytion / Decryption"},
{ 0x10, 0x10, "Encryption Controller - Entertainment Encryption / Decryption"},
{ 0x10, 0x80, "Encryption Controller - Other Encryption / Decryption"},
{ 0x11, 0x00, "Signal Processing Controller - DPIO Modules"},
{ 0x11, 0x01, "Signal Processing Controller - Performance Counters"},
{ 0x11, 0x10, "Signal Processing Controller - Communication Synchronizer"},
{ 0x11, 0x20, "Signal Processing Controller - Signal Processing Management"},
{ 0x11, 0x80, "Signal Processing Controller - Other"},
{ 0x12, 0x00, "Processing Accelerator"},
{ 0x13, 0x00, "Non - Essential Instrumentation"},
{ 0x40, 0x00, "Co - Processor"},
{ 0xFF, 0x00, "Unassigned Class "},
{ 0x00, 0x00, NULL } };


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
		const ClassCode* Info;


		for (Info = ClassCodes; Info->Description; Info++)
		{
			if (Info->classID == class_id && Info->subclassID == subclass_id)
			{
				return (Info->Description);
			}
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