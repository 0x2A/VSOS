#include <efi.h>
#include <efilib.h>
#include <libsmbios.h>

#include "Error.h"
#include <OS.System.h>
#include "LoaderParams.h"
#include <Ramdrive.h>
#include <os.internal.h>
#include <stdlib.h>
#include <wchar.h>
#include <Path.h>
#include "EfiLoader.h"
#include <mem\PageTablesPool.h>

#ifdef __cplusplus 
extern "C" {
#endif 
#include <intrin.h>
	int _fltused = 0;
#ifdef __cplusplus 
}
#endif
#include <Assert.h>
#include <mem\pagetables.h>


#if defined(_M_X64) || defined(__x86_64__)
static const CHAR16* ArchName = L"x86 64-bit";
#elif defined(_M_IX86) || defined(__i386__)
static const CHAR16* ArchName = L"x86 32-bit";
#elif defined (_M_ARM64) || defined(__aarch64__)
static const CHAR16* ArchName = L"ARM 64-bit";
#elif defined (_M_ARM) || defined(__arm__)
static const CHAR16* ArchName = L"ARM 32-bit";
#elif defined (_M_RISCV64) || (defined(__riscv) && (__riscv_xlen == 64))
static const CHAR16* ArchName = L"RISC-V 64-bit";
#else
#  error Unsupported architecture
#endif

static constexpr wchar_t Kernel[] = L"vsoskrnl.exe";
static constexpr wchar_t KernelPDB[] = L"vsoskrnl.pdb";
static constexpr size_t MaxKernelPath = 64;
static constexpr size_t BootloaderPagePoolCount = 256; //1Mb
static constexpr size_t ReservedPageTablePages = 512; //2Mb


EFI_MEMORY_TYPE AllocationType;

// Tri-state status for Secure Boot: -1 = Setup, 0 = Disabled, 1 = Enabled
INTN SecureBootStatus = 0;
/*
 * Query SMBIOS to display some info about the system hardware and UEFI firmware.
 * Also display the current Secure Boot status.
 */
static EFI_STATUS PrintSystemInfo(VOID)
{
	EFI_STATUS Status;
	SMBIOS_STRUCTURE_POINTER Smbios;
	SMBIOS_STRUCTURE_TABLE* SmbiosTable;
	SMBIOS3_STRUCTURE_TABLE* Smbios3Table;
	UINT8 Found = 0, * Raw, * SecureBoot, * SetupMode;
	UINTN MaximumSize, ProcessedSize = 0;

	Print(L"UEFI v%d.%d (%s, 0x%08X)\n", gST->Hdr.Revision >> 16, gST->Hdr.Revision & 0xFFFF,
		gST->FirmwareVendor, gST->FirmwareRevision);

	Status = LibGetSystemConfigurationTable(&SMBIOS3TableGuid, (VOID**)&Smbios3Table);
	if (Status == EFI_SUCCESS) {
		Smbios.Hdr = (SMBIOS_HEADER*)Smbios3Table->TableAddress;
		MaximumSize = (UINTN)Smbios3Table->TableMaximumSize;
	}
	else {
		Status = LibGetSystemConfigurationTable(&SMBIOSTableGuid, (VOID**)&SmbiosTable);
		if (EFI_ERROR(Status))
			return EFI_NOT_FOUND;
		Smbios.Hdr = (SMBIOS_HEADER*)(UINTN)SmbiosTable->TableAddress;
		MaximumSize = (UINTN)SmbiosTable->TableLength;
	}

	while ((Smbios.Hdr->Type != 0x7F) && (Found < 2)) {
		Raw = Smbios.Raw;
		if (Smbios.Hdr->Type == 0) {
			Print(L"%a %a\n", LibGetSmbiosString(&Smbios, Smbios.Type0->Vendor),
				LibGetSmbiosString(&Smbios, Smbios.Type0->BiosVersion));
			Found++;
		}
		if (Smbios.Hdr->Type == 1) {
			Print(L"%a %a\n", LibGetSmbiosString(&Smbios, Smbios.Type1->Manufacturer),
				LibGetSmbiosString(&Smbios, Smbios.Type1->ProductName));
			Found++;
		}
		if (Smbios.Hdr->Type == 4)
		{
			Print(L"CPU: %a %a @ %d \n", LibGetSmbiosString(&Smbios, Smbios.Type4->ProcessorManufacture),
				LibGetSmbiosString(&Smbios, Smbios.Type4->ProcessorFamily), (Smbios.Type4->CurrentSpeed[0] << 8) & Smbios.Type4->CurrentSpeed[1]);
			
			Found++;
		}
		LibGetSmbiosString(&Smbios, -1);
		ProcessedSize += (UINTN)Smbios.Raw - (UINTN)Raw;
		if (ProcessedSize > MaximumSize) {
			Print(L"%EAborting system report due to noncompliant SMBIOS%N\n");
			return EFI_ABORTED;
		}
	}

	SecureBoot = (uint8_t*)LibGetVariable((CHAR16*)L"SecureBoot", &EfiGlobalVariable);
	SetupMode = (uint8_t*)LibGetVariable((CHAR16*)L"SetupMode", &EfiGlobalVariable);
	SecureBootStatus = ((SecureBoot != NULL) && (*SecureBoot != 0)) ? 1 : 0;
	// You'd expect UEFI platforms to properly clear SetupMode after they
	// installed all the certs... but most of them don't. Hence Secure Boot
	// disabled having precedence over SetupMode. Looking at you OVMF!
	if ((SetupMode != NULL) && (*SetupMode != 0))
		SecureBootStatus *= -1;
	// Wasteful, but we can't highlight "Enabled"/"Setup" from a %s argument...
	if (SecureBootStatus > 0)
		Print(L"Secure Boot status: %HEnabled%N\n");
	else if (SecureBootStatus < 0)
		Print(L"Secure Boot status: %ESetup%N\n");
	else
		Print(L"Secure Boot status: Disabled\n");

	return EFI_SUCCESS;
}

EFI_STATUS DumpMemoryMap(const EFI_MEMORY_MAP& map)
{
	const CHAR16 mem_types[16][27] = {
	  L"EfiReservedMemoryType     ",
	  L"EfiLoaderCode             ",
	  L"EfiLoaderData             ",
	  L"EfiBootServicesCode       ",
	  L"EfiBootServicesData       ",
	  L"EfiRuntimeServicesCode    ",
	  L"EfiRuntimeServicesData    ",
	  L"EfiConventionalMemory     ",
	  L"EfiUnusableMemory         ",
	  L"EfiACPIReclaimMemory      ",
	  L"EfiACPIMemoryNVS          ",
	  L"EfiMemoryMappedIO         ",
	  L"EfiMemoryMappedIOPortSpace",
	  L"EfiPalCode                ",
	  L"EfiPersistentMemory       ",
	  L"EfiMaxMemoryType          "
	};

	Print(L"MapSize: 0x%016X, Size: 0x%x, DescSize: 0x%x\n", map.Table, map.Size, map.DescriptorSize);

	for (EFI_MEMORY_DESCRIPTOR* current = map.Table; current < NextMemoryDescriptor(map.Table, map.Size); current = NextMemoryDescriptor(current, map.DescriptorSize))
	{
		const bool runtime = (current->Attribute & EFI_MEMORY_RUNTIME) != 0;
		Print(L"P: %016x V: %016x T:%s #: 0x%x A:0x%016x %c\n", current->PhysicalStart, current->VirtualStart, mem_types[current->Type], current->NumberOfPages, current->Attribute, runtime ? 'R' : ' ');
	}

	return EFI_SUCCESS;
}

UINTN GetPhysicalAddressSize(const EFI_MEMORY_MAP& map)
{
	UINTN highest = 0;

	for (const EFI_MEMORY_DESCRIPTOR* current = map.Table;
		current < NextMemoryDescriptor(map.Table, map.Size);
		current = NextMemoryDescriptor(current, map.DescriptorSize))
	{
		const uintptr_t address = current->PhysicalStart + (current->NumberOfPages << PageShift);
		if (address > highest)
			highest = address;
	}

	return highest;
}


EFI_STATUS PopulateDrive(RamDrive& drive, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, EFI_FILE* dir)
{
	EFI_STATUS status;

	const size_t MAX_FILE_INFO_SIZE = 1024;
	uint8_t buffer[MAX_FILE_INFO_SIZE] = { 0 };
	EFI_FILE_INFO* fileInfo = (EFI_FILE_INFO*)(buffer);
	
	dir->SetPosition(dir, 0);

	while (true)
	{
		UINTN size = MAX_FILE_INFO_SIZE;
		ReturnIfNotSuccess(status = dir->Read(dir, &size, buffer));
		if (size == 0)
			break; //No more directories

		if (wcscmp(fileInfo->FileName, L".") == 0 || wcscmp(fileInfo->FileName, L"..") == 0)
			continue;

		if (fileInfo->Attribute & EFI_FILE_DIRECTORY)
		{
			//Recurse
			EFI_FILE_HANDLE subDir;
			ReturnIfNotSuccess(dir->Open(dir, &subDir, fileInfo->FileName, EFI_FILE_MODE_READ, 0));
			subDir->SetPosition(subDir, 0);

			PopulateDrive(drive, fs, subDir);

			dir->Close(subDir);
		}
		else
		{
			//Convert to asci
			char buffer[32] = { 0 };
			wcstombs(buffer, fileInfo->FileName, sizeof(buffer));

			//Allocate in ram
			void* address = drive.Allocate(buffer, fileInfo->FileSize);

			//Open
			EFI_FILE_HANDLE file;
			ReturnIfNotSuccess(dir->Open(dir, &file, fileInfo->FileName, EFI_FILE_MODE_READ, 0));

			//Copy
			ReturnIfNotSuccess(file->Read(file, &fileInfo->FileSize, (void*)address));

			/* close the file */
			uefi_call_wrapper(file->Close, 1, file);
		}
	}

	return status;
}

// On Hyper-V, console/locate directly returns 1 GOP, whereas locating by handle returns 2. They all look the same
// Each GOP has one mode. So for now, take the 1 mode discovered via console
EFI_STATUS InitializeGraphics(EFI_GRAPHICS_DEVICE& device)
{
	EFI_STATUS status;

	//Get current mode
	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = nullptr;
	status = BS->HandleProtocol(ST->ConsoleOutHandle, &GraphicsOutputProtocol, (void**)&gop);
	if (EFI_ERROR(status))
	{
		ReturnIfNotSuccess(BS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (void **)&gop));
	}

	//Allocate space for full graphics info
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = nullptr;
	UINTN size = 0;
	ReturnIfNotSuccess(gop->QueryMode(gop, gop->Mode->Mode, &size, &info));

	//Kernel will only support one pixel format
	if (info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor)
		return EFI_UNSUPPORTED;

	//Populate kernel structure
	device.FrameBufferBase = gop->Mode->FrameBufferBase;
	device.FrameBufferSize = gop->Mode->FrameBufferSize;
	device.HorizontalResolution = info->HorizontalResolution;
	device.VerticalResolution = info->VerticalResolution;
	device.PixelsPerScanLine = info->PixelsPerScanLine;

	return status;
}

EFI_STATUS DisplayLoaderParams(const LoaderParams& params)
{
	EFI_STATUS status;

	ReturnIfNotSuccess(Print(L"LoaderParams: 0x%016x, Pages: 0x%x\r\n", &params, SizeToPages(sizeof(LoaderParams))));
	ReturnIfNotSuccess(Print(L"  Kernel-Address: 0x%016x, Pages: 0x%x\r\n", params.KernelAddress, SizeToPages(params.KernelImageSize)));
	ReturnIfNotSuccess(Print(L"  MemoryMap-Address: 0x%016x, Pages: 0x%x\r\n", params.MemoryMap.Table, SizeToPages(params.MemoryMap.Size)));
	ReturnIfNotSuccess(Print(L"  PageTablesPool-Address: 0x%016x, Pages: 0x%x\r\n", params.PageTablesPoolAddress, params.PageTablesPoolPageCount));
	ReturnIfNotSuccess(Print(L"  PFN Database-Address: 0x%016x, Count: 0x%x\r\n", params.PageFrameAddr, params.PageFrameCount));
	ReturnIfNotSuccess(Print(L"  ConfigTables-Address: 0x%016x, Count: 0x%x\r\n", params.ConfigTables, params.ConfigTablesCount));
	ReturnIfNotSuccess(Print(L"  RamDrive-Address: 0x%016x, Size: 0x%x\r\n", params.RamDriveAddress, RamDriveSize));
	ReturnIfNotSuccess(Print(L"  PDB-Address: 0x%016x, Size: 0x%x\r\n", params.PdbAddress, params.PdbSize));

	ReturnIfNotSuccess(Print(L"Graphics:\r\n"));
	ReturnIfNotSuccess(Print(L"  FrameBuffer-Base 0x%016x, Size: 0x%08x\r\n", params.Display.FrameBufferBase, params.Display.FrameBufferSize));
	ReturnIfNotSuccess(Print(L"  Resulution %d (%d) x %d\r\n", params.Display.HorizontalResolution, params.Display.PixelsPerScanLine, params.Display.VerticalResolution));
	return status;
}

EFI_STATUS Keywait(const CHAR16* String)
{
	EFI_STATUS status = EFI_SUCCESS;
	EFI_INPUT_KEY Key;

	if (String != nullptr)
		ReturnIfNotSuccess(Print(String));
	ReturnIfNotSuccess(Print(L"Press any key to continue..."));
	ReturnIfNotSuccess(ST->ConIn->Reset(ST->ConIn, FALSE));

	// Poll for key
	while ((status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);

	// Clear keystroke buffer (this is just a pause)
	ReturnIfNotSuccess(ST->ConIn->Reset(ST->ConIn, FALSE));

	Print(L"\r\n");
	return status;
}

// Application entrypoint (must be set to 'efi_main' for gnu-efi crt0 compatibility)
EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
	static_assert(PageShift == EFI_PAGE_SHIFT, "PageShift mismatch");
	static_assert(PageSize == EFI_PAGE_SIZE, "PageSize mismatch");

#if defined(_GNU_EFI)
	InitializeLib(ImageHandle, SystemTable);
#endif

	//Boot Params for kernel
	LoaderParams params;
	params.ConfigTables = ST->ConfigurationTable;
	params.ConfigTablesCount = ST->NumberOfTableEntries;
	params.Runtime = RT;



	EFI_STATUS status;
	// The platform logo may still be displayed -> remove it
	ReturnIfNotSuccess(SystemTable->ConOut->ClearScreen(SystemTable->ConOut));

	/*
	 * In addition to the standard %-based flags, Print() supports the following:
	 *   %N       Set output attribute to normal
	 *   %H       Set output attribute to highlight
	 *   %E       Set output attribute to error
	 *   %r       Human readable version of a status code
	 */
	Print(L"\n%H*** VSOS Bootloader (%s) ***%N\n\n", ArchName);

	PrintSystemInfo();


	//Get handle to bootloader.
	EFI_LOADED_IMAGE* LoadedImage = nullptr;
	ReturnIfNotSuccess(BS->OpenProtocol(ImageHandle, &LoadedImageProtocol, (void**)&LoadedImage, NULL, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL));
	AllocationType = LoadedImage->ImageDataType;
	const CHAR16* BootFilePath = ((FILEPATH_DEVICE_PATH*)LoadedImage->FilePath)->PathName;

	//Display time
	EFI_TIME time;
	RT->GetTime(&time, nullptr);
	Print(L"Date: %02d-%02d-%02d %02d:%02d:%02d\r\n", time.Month, time.Day, time.Year, time.Hour, time.Minute, time.Second);
	
	//Determine size of memory map, allocate it in Boot Services Data so Kernel cleans it up
	UINTN memoryMapKey = 0;
	UINT32 descriptorVersion = 0;
	params.MemoryMap.Table = NULL;
	params.MemoryMap.Size = 0;
	status = BS->GetMemoryMap(&params.MemoryMap.Size, params.MemoryMap.Table, &memoryMapKey, &params.MemoryMap.DescriptorSize, &descriptorVersion);
	if (status != EFI_BUFFER_TOO_SMALL)
		ReturnIfNotSuccess(status);

	
	//We need an initial memory map to know physical address space for PFN DB
	params.MemoryMap.Size *= 2;//*params.MemoryMap.DescriptorSize;//Increase allocation size to leave room for additional allocations
	ReturnIfNotSuccess(BS->AllocatePool(EfiBootServicesData, params.MemoryMap.Size, (void**)&params.MemoryMap.Table));
	ReturnIfNotSuccess(BS->GetMemoryMap(&params.MemoryMap.Size, params.MemoryMap.Table, &memoryMapKey, &params.MemoryMap.DescriptorSize, &descriptorVersion));
	DumpMemoryMap(params.MemoryMap);

	//Allocate pages for Bootloader PageTablesPool in BootServicesData
	//Pages from this pool will be used to bootstrap the kernel
	//Boot PT is allocated in boot services data so kernel knows it can be cleared
	EFI_PHYSICAL_ADDRESS bootloaderPagePoolAddress;
	ReturnIfNotSuccess(BS->AllocatePages(AllocateAnyPages, EfiBootServicesData, BootloaderPagePoolCount, &bootloaderPagePoolAddress));

	//Allocate pages for our Kernel's page pools
	//This pool will be kept for the kernel to use during runtime	
	ReturnIfNotSuccess(uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, AllocationType, ReservedPageTablePages, &params.PageTablesPoolAddress));
	params.PageTablesPoolPageCount = ReservedPageTablePages;

	//Allocate space for Page Frame DB
	const UINTN address = GetPhysicalAddressSize(params.MemoryMap);
	Print(L"Physical Address Max: 0x%016x\r\n", address);

	const size_t pageCount = address >> PageShift;
	params.PageFrameCount = pageCount;
	ReturnIfNotSuccess(BS->AllocatePages(AllocateAnyPages, AllocationType, SizeToPages(pageCount * sizeof(PageFrame)), &params.PageFrameAddr));


	//Allocate space for RamDrive
	ReturnIfNotSuccess(BS->AllocatePages(AllocateAnyPages, AllocationType, SizeToPages(RamDriveSize), &params.RamDriveAddress));
	RamDrive drive((void*)params.RamDriveAddress, RamDriveSize);

	//Load kernel path
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystem = nullptr;
	ReturnIfNotSuccess(ST->BootServices->OpenProtocol(LoadedImage->DeviceHandle, &FileSystemProtocol, (void**)&fileSystem, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL));	
	EFI_FILE* CurrentDriveRoot = nullptr;
	ReturnIfNotSuccess(fileSystem->OpenVolume(fileSystem, &CurrentDriveRoot));

	//Populate
	PopulateDrive(drive, fileSystem, CurrentDriveRoot);
	
	for (const RamDrive::Entry& entry : drive)
	{
		if (*entry.Name == '\0')
			break;

		CHAR16 buffer[RamDrive::EntrySize] = {};
		mbstowcs(buffer, entry.Name, RamDrive::EntrySize);

		Print(L"  File: %s PageNumber: 0x%x Length: 0x%x\n", buffer, entry.PageNumber, entry.Length);
	}

	

	//Build path to kernel
	CHAR16* KernelPath;
	KernelPath = (CHAR16*)AllocatePool(MaxKernelPath * sizeof(CHAR16));
	memset((void*)KernelPath, 0, MaxKernelPath * sizeof(CHAR16));
	GetDirectoryName(BootFilePath, KernelPath);
	wcscpy(KernelPath + wcslen(KernelPath), Kernel);

	EFI_FILE* KernelFile = nullptr;
	Print(L"Loading: %s\r\n", KernelPath);
	ReturnIfNotSuccess(CurrentDriveRoot->Open(CurrentDriveRoot, &KernelFile, KernelPath, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY));
	FreePool(KernelPath);

	//Map kernel into memory. It will be relocated at KernelBaseAddress
	UINT64 entryPoint;
	ReturnIfNotSuccess(EfiLoader::MapKernel(KernelFile, params.KernelImageSize, entryPoint, params.KernelAddress));

	LoaderParams* kParams = (LoaderParams*)EfiLoader::GetProcAddress((void*)params.KernelAddress, "BootParams");
	//Assert(kParams);
	Print(L"  Params: 0x%016x\r\n", kParams);

	if (kParams == nullptr)
	{
		return Error::DisplayError(L"BootParams export not defined in kernel\n\n", WFILE, LLINE, EFI_UNSUPPORTED);
	}

	//Build path to kernelpdb
	CHAR16* KernelPdbPath = (CHAR16*)AllocatePool(MaxKernelPath * sizeof(CHAR16));
	memset((void*)KernelPdbPath, 0, MaxKernelPath * sizeof(CHAR16));
	GetDirectoryName(BootFilePath, KernelPdbPath);
	wcscpy(KernelPdbPath + wcslen(KernelPdbPath), KernelPDB);

	//Load pdb file
	EFI_FILE* KernelPdbFile = nullptr;
	Print(L"Loading: %s\r\n", KernelPdbPath);
	ReturnIfNotSuccess(CurrentDriveRoot->Open(CurrentDriveRoot, &KernelPdbFile, KernelPdbPath, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY));
	FreePool(KernelPdbPath);

	//Map PDB into memory
	ReturnIfNotSuccess(EfiLoader::MapFile(KernelPdbFile, params.PdbAddress, params.PdbSize));
	Print(L"  Address: 0x%016x Size:0x%x\r\n", params.PdbAddress, params.PdbSize);


	//Initialize graphics
	ReturnIfNotSuccess(InitializeGraphics(params.Display));

	//Disable write protection, allowing current page tables to be modified
	__writecr0(__readcr0() & ~(1 << 16));

	//Map in Kernel Space
	PageTablesPool pageTablesPool((void*)bootloaderPagePoolAddress, bootloaderPagePoolAddress, BootloaderPagePoolCount);
	pageTablesPool.Initialize();
	PageTables::Pool = &pageTablesPool;

	PageTables currentPT;
	currentPT.OpenCurrent();

	//Some platforms have Kernel addresses already mapped (B550i/5950x). Clear them.
	//The physical page tables will be reclaimed by Kernel, so just remove pointers.
	currentPT.ClearKernelEntries();
	Assert(currentPT.MapPages(KernelBaseAddress, params.KernelAddress, SizeToPages(params.KernelImageSize), true));
	Assert(currentPT.MapPages(KernelPageTablesPool, params.PageTablesPoolAddress, params.PageTablesPoolPageCount, true));
	Assert(currentPT.MapPages(KernelGraphicsDevice, params.Display.FrameBufferBase, SizeToPages(params.Display.FrameBufferSize), true));
	Assert(currentPT.MapPages(KernelPageFrameDBStart, params.PageFrameAddr, SizeToPages(params.PageFrameCount * sizeof(PageFrame)), true));

	//Re-enable write protection
	__writecr0(__readcr0() | (1 << 16));

	//Display graphics
	ReturnIfNotSuccess(DisplayLoaderParams(params));

	//Retrieve map from UEFI
	//This could fail on EFI_BUFFER_TOO_SMALL
	params.MemoryMap.Size *= 2;//*params.MemoryMap.DescriptorSize;
	uefi_call_wrapper(BS->GetMemoryMap, 5, &params.MemoryMap.Size, params.MemoryMap.Table, &memoryMapKey, &params.MemoryMap.DescriptorSize, &descriptorVersion);

	//Output final map to uart
	//DumpMemoryMap(params.MemoryMap);

	//Pause here before booting kernel to inspect bootloader outputs
	//Keywait(NULL);

	//Printf("Mapkey: %d\r\n", memoryMapKey);
	//After ExitBootServices we can no longer use the BS handle (no print, memory, etc)
	status = BS->ExitBootServices(ImageHandle, memoryMapKey);
	if (EFI_ERROR(status)) 
	{
		//Some UEFIs need the call twise, so if this fails we call getmemoryMap again and try exiting bootservices again
		params.MemoryMap.Size *= 2;// * params.MemoryMap.DescriptorSize;
		uefi_call_wrapper(BS->GetMemoryMap, 5, &params.MemoryMap.Size, params.MemoryMap.Table, &memoryMapKey, &params.MemoryMap.DescriptorSize, &descriptorVersion);
		ReturnIfNotSuccess(BS->ExitBootServices(ImageHandle, memoryMapKey));
	}

	//Assign virtual mappings for runtime sections
	for (EFI_MEMORY_DESCRIPTOR* current = params.MemoryMap.Table;
		current < NextMemoryDescriptor(params.MemoryMap.Table, params.MemoryMap.Size);
		current = NextMemoryDescriptor(current, params.MemoryMap.DescriptorSize))
	{
		if ((current->Attribute & EFI_MEMORY_RUNTIME) == 0)
			continue;

		current->VirtualStart = current->PhysicalStart + KernelUefiStart;
	}

	//Update UEFI virtual address map
	ReturnIfNotSuccessNoDisplay(RT->SetVirtualAddressMap(params.MemoryMap.Size, params.MemoryMap.DescriptorSize, descriptorVersion, params.MemoryMap.Table));
	
	
	//Call into kernel
	const KernelMain kernelMain = (KernelMain)(entryPoint);
	*kParams = params;

	//Finish kernel image initialization now that address space is constructed
	ReturnIfNotSuccess(EfiLoader::CrtInitialization(KernelBaseAddress));
	kernelMain();

	return EFI_ABORTED;
}