#include <cstddef>
#include <efi.h>
#include <efilib.h>

void* operator new(size_t n)
{
	void* addr = nullptr;
	BS->AllocatePool(EFI_MEMORY_TYPE::EfiLoaderData, n, &addr);
	return addr;
}

void operator delete(void* p, size_t count)
{
	BS->FreePool(p);
}