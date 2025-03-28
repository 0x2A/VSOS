#include "DiskManager.h"
#include "kernel/Kernel.h"
#include <kernel\vfs\FAT.h>

DiskManager::DiskManager()
{
	m_Disks = new std::list<Disk *>();
}

void DiskManager::AddDisk(Disk* d)
{
	m_Disks->push_back(d);

	DetectAndLoadFilesystem(d);

}

void DiskManager::RemoveDisk(Disk* d)
{
	m_Disks->remove(d);

	//TODO: unmount filesystem
}

char DiskManager::ReadSector(uint16_t diskIndex, uint64_t sector, uint8_t* buf)
{
	if (diskIndex < m_Disks->size())
	{
		return (m_Disks->front() + diskIndex)->ReadSector(sector, buf);
	}
	return 0;
}

char DiskManager::WriteSector(uint16_t diskIndex, uint64_t sector, uint8_t* buf)
{
	if (diskIndex < m_Disks->size())
	{
		return (m_Disks->front() + diskIndex)->WriteSector(sector, buf);
	}
	return 0;
}

void DiskManager::AssignVFS(PartitionTableEntry partition, Disk* disk)
{
	if (partition.partition_id == 0xCD)
	{
		Printf(" [ISO9660]\r\n");
		//TODO
	}
	else if (partition.partition_id == 0x0B || partition.partition_id == 0x0C || partition.partition_id == 0x01 || partition.partition_id == 0x04 || partition.partition_id == 0x06)
	{
		Printf(" [FAT(12/16/32)]\r\n");
		FAT* fatFS = new FAT(disk, partition.start_lba, partition.length);
		if(fatFS->Initialize())
			kernel.VFS()->Mount(fatFS); // Mount the filesystem
		else
			delete fatFS;
	}
}

void DiskManager::DetectAndLoadFilesystem(Disk* disk)
{
	char* diskIdentifier = disk->identifier;
	Printf("Detecting partitions on disk %s\r\n", diskIdentifier);

	uint8_t* Readbuf = new uint8_t[2048];
	memset(Readbuf, 0, 2048);

	char ret = disk->ReadSector(0, Readbuf);
	if (ret == 0)
	{
		MasterBootRecord* mbr = (MasterBootRecord*)Readbuf;
		Printf("Sig: 0x%04x\r\n", mbr->signature);
		if (mbr->magicnumber != 0xAA55)
		{
			Printf("WARNING: MBR magic number is not 0xAA55, instead 0x%04x\r\n", mbr->magicnumber);
			delete[] Readbuf;
			return;
		}

		if (disk->GetType() == DiskType::Floppy) // Floppy don't contain partitions
		{
			//TODO
		}
		else
		{
			//Loop trough partitions
			for (int p = 0; p < 4; p++)
			{
				if (mbr->primaryPartitions[p].partition_id == 0x00)
					continue;

				PartitionInfo info;
				info.attributes = mbr->primaryPartitions[p].bootable;
				info.startLBA = mbr->primaryPartitions[p].start_lba;
				info.endLBA = info.startLBA + mbr->primaryPartitions[p].length;
				info.GUID = p;
				info.PartitionType = mbr->primaryPartitions[p].partition_id;
				disk->AddPartitionInfo(info);

				Printf("- Disk %s Part=%d Boot=%d ID=%x Sectors=%d\r\n", diskIdentifier, p, mbr->primaryPartitions[p].bootable == 0x80, mbr->primaryPartitions[p].partition_id, mbr->primaryPartitions[p].length);
				AssignVFS(mbr->primaryPartitions[p], disk);
			}
		}
	}

}
