#pragma once
#include "Disk.h"
#include <list>

#pragma pack(push, 1)
struct PartitionTableEntry
{
	uint8_t bootable : 8;

	uint8_t start_head : 8;
	uint16_t start_sector : 6;
	uint16_t start_cylinder : 10;
		

	uint8_t partition_id : 8;
	uint8_t end_head : 8;
	uint16_t end_sector : 6;
	uint16_t end_cylinder : 10;

	uint32_t start_lba;
	uint32_t length;
} ;
#pragma pack(pop)

#pragma pack(push, 1)
struct MasterBootRecord
{
	uint8_t bootloader[440];
	uint32_t signature;
	uint16_t unused;

	PartitionTableEntry primaryPartitions[4];

	uint16_t magicnumber;
} ;

#pragma pack(pop)

class DiskManager
{
public:
	DiskManager();

	//Add disk to system and check for filesystems
	void AddDisk(Disk* d);
	//Remove disk from system and unmount filesystems
	void RemoveDisk(Disk* d);

	char ReadSector(uint16_t diskIndex, uint64_t sector, uint8_t* buf);
	char WriteSector(uint16_t diskIndex, uint64_t sector, uint8_t* buf);

	const std::list<Disk*>* GetDisks(){ return m_Disks; }

private:

	void AssignVFS(PartitionTableEntry partition, Disk* disk);
	void DetectAndLoadFilesystem(Disk* disk);

	std::list<Disk*>* m_Disks;
};