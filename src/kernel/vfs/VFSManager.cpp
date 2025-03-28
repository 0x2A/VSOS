#include "VFSManager.h"
#include "kernel/Kernel.h"
#include <list>

VFSManager::VFSManager()
{
	Filesystems = new std::vector<VirtualFileSystem*>();
}

void VFSManager::Mount(VirtualFileSystem* vfs)
{
	Filesystems->push_back(vfs); //Just add it to the list of known filesystems, easy.
}

void VFSManager::Unmount(VirtualFileSystem* vfs)
{
	Filesystems->erase(std::remove(Filesystems->begin(), Filesystems->end(), vfs), Filesystems->end());
}

void VFSManager::UnmountByDisk(Disk* disk)
{
	for (VirtualFileSystem* vfs : *Filesystems)
		if (vfs->disk == disk)
			Unmount(vfs);
}

int VFSManager::ExtractDiskNumber(const char* path, uint8_t* idSizeReturn)
{
	const char* idPos = strchr(path, ':');
	if (idPos != NULL && strchr(path, PATH_SEPERATOR_C) != NULL)
	{
		int idLength = idPos - path;
		char* idStr = new char[idLength];
		memcpy(idStr, path, idLength);

		int idValue = 0;

		if (isalpha(idStr[0])) //are we using a character instead of a integer?
		{
			switch (idStr[0])
			{
				case 'b':
				case 'B':
					idValue = this->bootPartitionID;
					break;
				default:
					delete[] idStr;
					return -1;
					break;
			}
		}
		else
		{
			idValue = strtol(idStr, NULL, 10);
		}

		delete[] idStr;

		if(idSizeReturn != 0)
			*idSizeReturn = idLength;

		return idValue;
	}
	return -1;
}

bool VFSManager::SearchBootPartition()
{
	//std::list<Disk*> posibleDisks;
	const char* pathString = "####:\\efi\\boot\\vsoskrnl.exe";

	// Include all disks in the search
	/*for (Disk* d : *kernel.GetDiskManager()->GetDisks())
	{
		if (d->GetType() != Floppy) // we cant be booted from floppy
		{
			posibleDisks.push_back(d);
		}
	}*/

	char* idStr = new char[16];
	// Now loop though all the filesystems mounted and check if the disk can be the booted one
	// If that is the case check for the kernel file
	// At this point we can assume this is the boot disk
	for (int i = 0; i < Filesystems->size(); i++)
	{
		//std::find(posibleDisks.begin(), posibleDisks.end(), (*Filesystems)[i]->disk)
//			continue; // This partition is not present on the posible disks we booted from

		itoa(i, idStr, 10);
		int idStrLen = strlen(idStr);

		memcpy((void*)(pathString + (4 - idStrLen)),idStr, idStrLen);

		if (Filesystems->at(i)->FileExists(pathString + (4 - idStrLen) + 3))
		{
			this->bootPartitionID = i;
			delete[] idStr;
			return true;
		}
	}
	delete[] idStr;
	return false;
}

int VFSManager::ReadFile(const char* path, uint8_t* buffer, uint32_t offset /*= 0*/, uint32_t len /*= -1*/)
{
	uint8_t idSize = 0;
	int disk = ExtractDiskNumber(path, &idSize);

	if (disk != -1 && Filesystems->size() > disk)
		return Filesystems->at(disk)->ReadFile(path + idSize + 2, buffer, offset, len);
	else
		return -1;
}

int VFSManager::WriteFile(const char* path, uint8_t* buffer, uint32_t len, bool create /*= true*/)
{
	uint8_t idSize = 0;
	int disk = ExtractDiskNumber(path, &idSize);

	if (disk != -1 && Filesystems->size() > disk)
		return Filesystems->at(disk)->WriteFile(path + idSize + 2, buffer, len, create);
	else
		return -1;
}

bool VFSManager::FileExists(const char* path)
{
	uint8_t idSize = 0;
	int disk = ExtractDiskNumber(path, &idSize);

	if (disk != -1 && Filesystems->size() > disk)
		return Filesystems->at(disk)->FileExists(path + idSize + 2);
	else
		return false;
}

bool VFSManager::DirectoryExists(const char* path)
{
	uint8_t idSize = 0;
	int disk = ExtractDiskNumber(path, &idSize);

	if (disk != -1 && Filesystems->size() > disk) {
		if (strlen(path) == idSize + 2) //Only disk part, like 0:\ ofcourse this is a directory as well
			return true;
		else
			return Filesystems->at(disk)->DirectoryExists(path + idSize + 2);
	}
	else
		return false;
}

int VFSManager::CreateFile(const char* path)
{
	uint8_t idSize = 0;
	int disk = ExtractDiskNumber(path, &idSize);

	if (disk != -1 && Filesystems->size() > disk) {
		if (strlen(path) == idSize + 2) //Only disk part, like 0:\ ofcourse this is a directory as well
			return true;
		else
			return Filesystems->at(disk)->CreateFile(path + idSize + 2);
	}
	else
		return -1;
}

int VFSManager::CreateDirectory(const char* path)
{
	uint8_t idSize = 0;
	int disk = ExtractDiskNumber(path, &idSize);

	if (disk != -1 && Filesystems->size() > disk) {
		if (strlen(path) == idSize + 2) //Only disk part, like 0:\ ofcourse this is a directory as well
			return true;
		else
			return Filesystems->at(disk)->CreateDirectory(path + idSize + 2);
	}
	else
		return -1;
}

uint32_t VFSManager::GetFileSize(const char* filename)
{
	uint8_t idSize = 0;
	int disk = ExtractDiskNumber(filename, &idSize);
	if (disk != -1 && Filesystems->size() > disk)
		return Filesystems->at(disk)->GetFileSize(filename + idSize + 2); // skips the 0:\ part
	else
		return -1;
}

std::list<VFSEntry>* VFSManager::DirectoryList(const char* path)
{
	uint8_t idSize = 0;
	int disk = ExtractDiskNumber(path, &idSize);
	if (disk != -1 && Filesystems->size() > disk)
		return Filesystems->at(disk)->DirectoryList(path + idSize + 2); // skips the 0:\ part
	else
		return 0;
}

bool VFSManager::EjectDrive(const char* path)
{
	uint8_t idSize = 0;
	int disk = ExtractDiskNumber(path, &idSize);
	Printf("Disk Number: %d\r\n", disk);

	if (disk != -1 && Filesystems->size() > disk) {
		VirtualFileSystem* fs = Filesystems->at(disk);
		//TODO
		//return fs->disk->GetController()->EjectDrive(fs->disk->);
		return true;
	}
	else
		return false;
}
