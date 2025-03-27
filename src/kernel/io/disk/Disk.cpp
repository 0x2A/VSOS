#include "Disk.h"

Disk::Disk(DiskDriver* driver, uint32_t driverIndex, DiskType type, uint64_t size, uint32_t blocks, uint32_t blockSize)
: m_Driver(driver), m_DriverIndex(driverIndex), m_Type(type), m_Size(size), m_BlockSize(blockSize), m_NumBlocks(blocks)
{

}

char Disk::ReadSector(uint64_t lba, uint8_t* buf)
{
	if(m_Driver)
		return m_Driver->ReadSector(m_DriverIndex, lba, buf);
	return 1;
}

char Disk::WriteSector(uint64_t lba, uint8_t* buf)
{
	if(m_Driver)
		return m_Driver->WriteSector(m_DriverIndex, lba, buf);
	return 1;
}

void Disk::AddPartitionInfo(PartitionInfo info)
{
	m_Partitions[partitionCount] = info;
	partitionCount++;
}
