#pragma once

#include "kernel/drivers/io/DiskDriver.h"

enum DiskType
{
	HardDisk,
	MassStorage,
	Floppy,
	CDROM
};

struct PartitionInfo
{
	uint16_t PartitionType;
	uint16_t GUID;
	uint8_t startLBA;
	uint8_t endLBA;
	uint8_t attributes;
	char PartitionName[72];
};

class Disk
{
public:
	Disk(DiskDriver* driver, uint32_t driverIndex, DiskType type, uint64_t size, uint32_t blocks, uint32_t blockSize);


	virtual char ReadSector(uint64_t lba, uint8_t* buf);
	virtual char WriteSector(uint64_t lba, uint8_t* buf);

	void AddPartitionInfo(PartitionInfo info);

	DiskType GetType() { return m_Type; }

	char* identifier = 0;
private:
	DiskDriver* m_Driver;	// disk driver
	uint32_t m_DriverIndex; // Index of device inside driver
	
	
	DiskType m_Type;

	uint64_t m_Size;
	uint32_t m_NumBlocks;
	uint32_t m_BlockSize;

	uint8_t partitionCount = 0;
	PartitionInfo m_Partitions[128]; //set to 128 to be GPT compatible
};