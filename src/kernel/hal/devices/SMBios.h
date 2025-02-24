#pragma once
#include <cstdint>
#include <vector>
#include <string>

#pragma pack(push, 1)

struct EPS_64
{
	char Anchor[5];
	uint8_t Checksum;
	uint8_t EntryLength;
	uint8_t MajorVersion;
	uint8_t MinorVersion;
	uint8_t Docrev;
	uint8_t entryRevision;
	uint8_t reserved;
	uint32_t MaxSize;
	uint64_t TableAddress;
};

struct SMBIOSHeader {
	uint8_t type;
	uint8_t length;
	uint16_t handle;
};

enum FirmwareFlags
{
	FI_Reserved = 1,
	FI_Reserved2 = 1<<1,
	unknown =  1<<2,
	NotSupported = 1<<3,
	ISASupport = 1<<4,
	MCASupport = 1<<5,
	EISASupport = 1<<6,
	PCISupport = 1<<7,
	PCMCIASupport = 1<<8,
	PlugAndPlay = 1<<9,
	APM = 1<<10,
	Upgradable = 1<<11,
	Shadowing = 1<<12,
	VLVESA = 1<<13,
	ESCD = 1<<14,
	BootFromCD = 1<<15,
	SelectableBoot = 1<<16,
	ROMSocketed = 1<<17,
	BootFromPCMCIA = 1<<18,
	EDD = 1<<19,
	Int13h_NEC9800 = 1<<20,
	Int13h_Toshiba = 1<<21,
	//...
};

struct smbios2_bios_information {
	uint8_t type;
	uint8_t length;
	uint16_t handle;

	uint8_t vendor;
	uint8_t version;
	uint16_t start_address_segment;
	uint8_t release_date;
	uint8_t rom_size;
	uint64_t characteristics;
} ;

struct smbios24_bios_information {
	uint8_t type;
	uint8_t length;
	uint16_t handle; //4 bytes

	uint8_t vendor;
	uint8_t version;
	uint16_t start_address_segment; //8 bytes
	uint8_t release_date;
	uint8_t rom_size; //10bytes
	uint64_t characteristics; //14bytes

	uint8_t characteristics_extension_bytes[2]; //16bytes
	uint8_t system_bios_major_release;
	uint8_t system_bios_minor_release;
	uint8_t embedded_controller_major_release;
	uint8_t embedded_controller_minor_release; //20bytes

} ;

struct smbios31_bios_information {
	uint8_t type;
	uint8_t length;
	uint16_t handle;

	uint8_t vendor;
	uint8_t version;
	uint16_t start_address_segment;
	uint8_t release_date;
	uint8_t rom_size;
	uint64_t characteristics;

	uint8_t characteristics_extension_bytes[2];
	uint8_t system_bios_major_release;
	uint8_t system_bios_minor_release;
	uint8_t embedded_controller_major_release;
	uint8_t embedded_controller_minor_release;

	uint16_t extended_bios_rom_size;
} ;

struct smbios_cpu_info
{
	SMBIOSHeader header;

	uint8_t socket_designation;        //2.0+   18 bytes
	uint8_t processor_type;
	uint8_t processor_family;
	uint8_t processor_manufacturer;
	uint64_t processor_id;
	uint8_t processor_version;
	uint8_t voltage;
	uint16_t external_clock;
	uint16_t max_speed;
	uint16_t current_speed;
	uint8_t status;
	uint8_t processor_upgrade;
};

struct smbios_cpu_info_21 : smbios_cpu_info 
{
	uint16_t l1_cache_handle;           //2.1+   6 bytes
	uint16_t l2_cache_handle;
	uint16_t l3_cache_handle;
} ;

struct smbios_cpu_info_23 : smbios_cpu_info_21 {
	
	uint8_t serial_number;              //2.3+   3 bytes
	uint8_t asset_tag;
	uint8_t part_number;
} ;

struct smbios_cpu_info_25 : smbios_cpu_info_23 {
	
	uint8_t core_count;                 //2.5+   5 bytes
	uint8_t core_enabled;
	uint8_t thread_count;
	uint16_t characteristics;
} ;

struct smbios_cpu_info_26 : smbios_cpu_info_25 {
	
	uint16_t processor_family2;          //2.6+    2 byte
} ;

struct smbios_cpu_info_30 : smbios_cpu_info_26 {
	
	uint16_t core_count2;                //3.0+    6 bytes
	uint16_t core_enabled2;
	uint16_t thread_count2;

} ;

struct smbios_cpu_info_36 : smbios_cpu_info_30 {

	uint16_t thread_enabled;             //3.6+    2 bytes
} ;


struct smbios_oem_info
{
	SMBIOSHeader header;
	uint8_t NumStrings;
};
#pragma pack(pop)

enum SMBiosHeaderType
{
	BiosInfo = 0,
	SystemInfo = 1,
	MainboardInfo = 2,
	ChasisInfo = 3,
	ProcessorInfo = 4,
	CacheInfo = 7,
	PortConnectorInformation = 8,
	SystemSlotInfo = 9,
	OnboardDeviceInfo = 10,
	OEMStrings = 11,
	PhysicalMemArray = 16,
	MemoryDeviceInfo = 17,
	MemoryArrayMappedAddr = 19,
	MemoryDeviceMappedAddr = 20,
	SystemBootInfo = 32

};


struct CPUInfo
{
	smbios_cpu_info* smbiosInfo;
	std::vector<std::string> strings;
};

class SMBios
{
public:
	SMBios();
	bool Init(uint64_t ESP);
	uint64_t smbios_struct_end(SMBIOSHeader *hd);
		
	uint16_t GetExtBusSpeed();
private:

	EPS_64* header;
	SMBIOSHeader* tables;

	uint64_t smBiosVersion;

	CPUInfo cpuInfo;
};