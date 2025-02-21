#pragma once

#include <efi.h>
#include "OS.list.h"

//PixelsPerScanLine could be larger than HorizonalResolution if theres padding
struct EFI_GRAPHICS_DEVICE
{
	UINTN FrameBufferBase;
	UINTN FrameBufferSize;
	UINT32 HorizontalResolution;
	UINT32 VerticalResolution;
	UINT32 PixelsPerScanLine;
};

struct EFI_MEMORY_MAP
{
	//MemoryMap - allocated on its own page
	EFI_MEMORY_DESCRIPTOR* Table;
	UINTN Size;
	UINTN DescriptorSize;
};

enum PageState
{
	Free,
	Active,
	Reserved //Used for acpi/runtime/reserved pages
};

//Simple struct to represent a physical page. Its state and list pointers
struct PageFrame
{
	//Pointers when placing frames on the lists
	struct ListEntry Link;
	enum PageState State;
};

//No point in supporting multiple monitors since this is built for hyper-v
struct LoaderParams
{
	//Kernel
	EFI_PHYSICAL_ADDRESS KernelAddress;
	UINT64 KernelImageSize;

	//UEFI Runtime
	EFI_RUNTIME_SERVICES* Runtime;

	//Memory map
	struct EFI_MEMORY_MAP MemoryMap;

	//Page Table Pool
	EFI_PHYSICAL_ADDRESS PageTablesPoolAddress;
	UINT32 PageTablesPoolPageCount;

	//Page Frame DB
	EFI_PHYSICAL_ADDRESS PageFrameAddr;
	size_t PageFrameCount;

	struct EFI_GRAPHICS_DEVICE Display;

	//Config tables
	EFI_CONFIGURATION_TABLE* ConfigTables;
	size_t ConfigTablesCount;

	//Ramdrive
	EFI_PHYSICAL_ADDRESS RamDriveAddress;

	//PDB
	EFI_PHYSICAL_ADDRESS PdbAddress;
	size_t PdbSize;
};

//KernelMain
typedef void (*KernelMain)();