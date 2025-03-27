#pragma once

#include <stdint.h>

#include "kernel/drivers/Driver.h"
#include <kernel\hal\devices\pci\PCIDevice.h>
#include "DiskDriver.h"

#define SATA_SIG_ATAPI 0xEB140101
#define SATA_SIG_ATA   0x00000101
#define SATA_SIG_SEMB  0xC33C0101
#define SATA_SIG_PM    0x96690101

#define HBA_PxCMD_CR		0x8000		// Command List Running (DMA active)
#define HBA_PxCMD_FRE		0x0010		// FIS Receive Enable
#define HBA_PxCMD_FR		0x4000		// FIS Receive Running
#define HBA_PxCMD_ST		0x0001		// Start DMA
#define HBA_PxCMD_ATAPI		(1 << 24)	// Device is ATAPI
#define HBA_PxCMD_POD		0x0004		// Power on Device
#define HBA_PxCMD_SUD		0x0002		// Spin-up Device

#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX 0x35
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_PACKET 0xA0

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
#define HBA_PxIS_TFES (1 << 30)

#define ATAPI_READ_CMD 0xA8

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

enum {
	CAP_S64A = (1U << 31),	// Supports 64-bit Addressing
	CAP_SNCQ = (1 << 30),	// Supports Native Command Queuing
	CAP_SSNTF = (1 << 29),	// Supports SNotification Register
	CAP_SMPS = (1 << 28),	// Supports Mechanical Presence Switch
	CAP_SSS = (1 << 27),	// Supports Staggered Spin-up
	CAP_SALP = (1 << 26),	// Supports Aggressive Link Power Management
	CAP_SAL = (1 << 25),	// Supports Activity LED
	CAP_SCLO = (1 << 24),	// Supports Command List Override
	CAP_ISS_MASK = 0xf,			// Interface Speed Support
	CAP_ISS_SHIFT = 20,
	CAP_SNZO = (1 << 19),	// Supports Non-Zero DMA Offsets
	CAP_SAM = (1 << 18),	// Supports AHCI mode only
	CAP_SPM = (1 << 17),	// Supports Port Multiplier
	CAP_FBSS = (1 << 16),	// FIS-based Switching Supported
	CAP_PMD = (1 << 15),	// PIO Multiple DRQ Block
	CAP_SSC = (1 << 14),	// Slumber State Capable
	CAP_PSC = (1 << 13),	// Partial State Capable
	CAP_NCS_MASK = 0x1f,			// Number of Command Slots
	// (zero-based number)
	CAP_NCS_SHIFT = 8,
	CAP_CCCS = (1 << 7),		// Command Completion Coalescing Supported
	CAP_EMS = (1 << 6),		// Enclosure Management Supported
	CAP_SXS = (1 << 5),		// Supports External SATA
	CAP_NP_MASK = 0x1f,			// Number of Ports (zero-based number)
	CAP_NP_SHIFT = 0,
};

enum {
	CAP2_DESO = (1 << 5),		// DevSleep Entrance from Slumber Only
	CAP2_SADM = (1 << 4),		// Supports Aggressive Device Sleep
	// Management
	CAP2_SDS = (1 << 3),		// Supports Device Sleep
	CAP2_APST = (1 << 2),		// Automatic Partial to Slumber Transitions
	CAP2_NVMP = (1 << 1),		// NVMHCI Present
	CAP2_BOH = (1 << 0),		// BIOS/OS Handoff
};

enum port_type {
	PORT_TYPE_NONE = 0,
	PORT_TYPE_SATA = 1,
	PORT_TYPE_SEMB = 2,
	PORT_TYPE_PM = 3,
	PORT_TYPE_SATAPI = 4
};

enum fis_type : uint8_t {
	FIS_TYPE_REG_H2D = 0x27,
	FIS_TYPE_REG_D2H = 0x34,
	FIS_TYPE_DMA_ACT = 0x39,
	FIS_TYPE_DMA_SETUP = 0x41,
	FIS_TYPE_DATA = 0x46,
	FIS_TYPE_BIST = 0x58,
	FIS_TYPE_PIO_SETUP = 0x5F,
	FIS_TYPE_DEV_BITS = 0xA1
};


#pragma pack(push, 1)

enum {
	PORT_INT_CPD = (1 << 31),	// Cold Presence Detect Status/Enable
	PORT_INT_TFE = (1 << 30),	// Task File Error Status/Enable
	PORT_INT_HBF = (1 << 29),	// Host Bus Fatal Error Status/Enable
	PORT_INT_HBD = (1 << 28),	// Host Bus Data Error Status/Enable
	PORT_INT_IF = (1 << 27),	// Interface Fatal Error Status/Enable
	PORT_INT_INF = (1 << 26),	// Interface Non-fatal Error Status/Enable
	PORT_INT_OF = (1 << 24),	// Overflow Status/Enable
	PORT_INT_IPM = (1 << 23),	// Incorrect Port Multiplier Status/Enable
	PORT_INT_PRC = (1 << 22),	// PhyRdy Change Status/Enable
	PORT_INT_DI = (1 << 7),		// Device Interlock Status/Enable
	PORT_INT_PC = (1 << 6),		// Port Change Status/Enable
	PORT_INT_DP = (1 << 5),		// Descriptor Processed Interrupt
	PORT_INT_UF = (1 << 4),		// Unknown FIS Interrupt
	PORT_INT_SDB = (1 << 3),		// Set Device Bits FIS Interrupt
	PORT_INT_DS = (1 << 2),		// DMA Setup FIS Interrupt
	PORT_INT_PS = (1 << 1),		// PIO Setup FIS Interrupt
	PORT_INT_DHR = (1 << 0),		// Device to Host Register FIS Interrupt
};

#define PORT_INT_ERROR	(PORT_INT_TFE | PORT_INT_HBF | PORT_INT_HBD \
                                        | PORT_INT_IF | PORT_INT_INF | PORT_INT_OF \
                                        | PORT_INT_IPM | PORT_INT_PRC | PORT_INT_PC \
                                        | PORT_INT_UF)

#define PORT_INT_MASK	(PORT_INT_ERROR | PORT_INT_DP | PORT_INT_SDB \
                                        | PORT_INT_DS | PORT_INT_PS | PORT_INT_DHR)

struct hba_command_fis {
	uint8_t fis_type;

	uint8_t pm_port : 4;
	uint8_t reserved0 : 3;
	uint8_t command_control : 1;

	uint8_t command;
	uint8_t feature_low;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device_register;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t feature_high;

	uint8_t count_low;
	uint8_t count_high;
	uint8_t icc;
	uint8_t control;

	uint8_t reserved1[4];
};

struct hba_port {
	uint32_t command_list_base;
	uint32_t command_list_base_upper;
	uint32_t fis_base_address;
	uint32_t fis_base_address_upper;
	uint32_t interrupt_status;
	uint32_t interrupt_enable;
	uint32_t command_status;
	uint32_t reserved;
	uint32_t task_file_data;
	uint32_t signature;
	uint32_t sata_status;
	uint32_t sata_control;
	uint32_t sata_error;
	uint32_t sata_active;
	uint32_t command_issue;
	uint32_t sata_notification;
	uint32_t fis_based_switching_control;
	uint32_t device_sleep;
	uint32_t reserved2[10];
	uint32_t vendor[4];
} ;

struct hba_memory {
	uint32_t host_capabilities;
	uint32_t global_host_control;
	uint32_t interrupt_status;
	uint32_t ports_implemented;
	uint32_t version;
	uint32_t ccc_control;
	uint32_t ccc_ports;
	uint32_t enclosure_management_location;
	uint32_t enclosure_management_control;
	uint32_t host_capabilities_extended;
	uint32_t bios_handoff_control_status;
	uint8_t reserved[116];
	uint8_t vendor_specific[96];
	struct hba_port ports[1];
} ;


struct hba_command_header {
	uint8_t command_fis_length : 5;
	uint8_t atapi : 1;
	uint8_t write : 1;
	uint8_t prefetchable : 1;

	uint8_t reset : 1;
	uint8_t bist : 1;
	uint8_t clear_busy_on_ok : 1;
	uint8_t reserved0 : 1;
	uint8_t port_multiplier : 4;

	uint16_t prdt_length;
	uint32_t prdb_count;
	uint32_t command_table_base_address;
	uint32_t command_table_base_address_upper;
	uint32_t reserved1[4];
} ;

struct hba_prdt_entry {
	uint32_t data_base_address;
	uint32_t data_base_address_upper;
	uint32_t reserved0;

	uint32_t byte_count : 22;
	uint32_t reserved1 : 9;
	uint32_t interrupt_on_completion : 1;
} ;

struct hba_command_table {
	uint8_t command_fis[64];
	uint8_t atapi_command[16];
	uint8_t reserved[48];
	struct hba_prdt_entry prdt_entry[32];
} ;

/*struct ahci_port {
	volatile struct hba_port* hba_port;
	enum port_type port_type;
	uint8_t* buffer;
	uint8_t port_number;
	uint64_t bufferPhysicalAddr;
	hba_command_header* command_header; //used to save virtual address of command headers
	hba_command_table* command_table[32];
};*/

#pragma pack(pop)


class AHCIPort;
class AHCIDriver : public Driver, public DiskDriver
{
friend class AHCIPort;
public:
	AHCIDriver(PCIDevice* device);

	DriverResult Activate() override;
	DriverResult Deactivate() override;
	DriverResult Initialize() override;

	DriverResult Reset() override;

	std::string get_vendor_name() override;
	std::string get_device_name() override;

	AHCIPort* GetPort(uint8_t port_no);
	uint8_t get_port_count();


	char ReadSector(uint16_t drive, uint64_t sector, uint8_t* buffer) const override;
	char WriteSector(uint16_t drive, uint64_t sector, uint8_t* buffer) const override;
	bool EjectDrive(uint8_t drive) override;

private:

	uint32_t OnInterrupt();
	static uint32_t HandleInterrupt(void* context);

	void probe_ports(hba_memory* abar);
	//void configure_port(ahci_port* port);
	//void port_start_command(ahci_port* port);
	//void port_stop_command(ahci_port* port);
	//int8_t find_cmd_slot(AHCIPort* port);

	hba_memory* abar = 0;
	AHCIPort* ahci_ports[32];
	uint8_t port_count = 0;
};