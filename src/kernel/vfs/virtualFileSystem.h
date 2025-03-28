#pragma once
#include <kernel\io\disk\Disk.h>

#define VFS_NAME_LENGTH 255

// Info about a item represent on the disk
struct VFSEntry
{
	uint32_t size;  // Size of the entry in bytes
	bool isDir;     // Is this item a directory or not?
	struct
	{
		uint8_t sec;
		uint8_t min;
		uint8_t hour;
	} creationTime; // Time file was created
	struct
	{
		uint8_t day;
		uint8_t month;
		uint16_t year;
	} creationDate; // Date time was created
	char name[VFS_NAME_LENGTH]; // Name of the file 
	// TODO: Is there a better way for buffering this?
};

#define PATH_SEPERATOR_C '\\' //Path Seperator as char
#define PATH_SEPERATOR_S "\\" //Path Seperator as string

class VirtualFileSystem
{
public:
	VirtualFileSystem(Disk* _disk, uint64_t start, uint64_t size, const char* name = 0) : disk(_disk), StartLBA(start), SizeInSectors(size), Name(name)
	{}
	virtual ~VirtualFileSystem() {};

	virtual bool Initialize() = 0;

	/////////////
	// VFS Functions (Read, Write, etc.)
	///////////// 

	// Read file contents into buffer
	virtual int ReadFile(const char* filename, uint8_t* buffer, uint32_t offset = 0, uint32_t len = -1) = 0;
	// Write buffer to file, file will be created when create equals true
	virtual int WriteFile(const char* filename, uint8_t* buffer, uint32_t len, bool create = true) = 0;

	// Check if file exist
	virtual bool FileExists(const char* filename) = 0;
	// Check if directory exist
	virtual bool DirectoryExists(const char* filename) = 0;

	// Create a file at the filepath
	virtual int CreateFile(const char* path) = 0;
	// Create a new directory
	virtual int CreateDirectory(const char* path) = 0;

	// Get size of specified file in bytes
	virtual uint32_t GetFileSize(const char* filename) = 0;

	// Returns list of context inside a directory
	virtual std::list<VFSEntry>* DirectoryList(const char* path) = 0;

public:
	Disk* disk;

protected:
	uint64_t StartLBA;
	uint64_t SizeInSectors;

	const char* Name = "Unkown";
};