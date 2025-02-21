#pragma once

#include <efi.h>
#include "LoaderParams.h"
#include <cstdint>

#include <string>
#include "kernel/Kernel.h"
#include <windows\types.h>
#include <windows\winnt.h>

//https://deplinenoise.wordpress.com/2013/06/14/getting-your-pdb-name-from-a-running-executable-windows/
//https://gist.github.com/gimelfarb/8642282
class PortableExecutable
{
public:
	static DWORD GetEntryPoint(void* const imageBase);
	static DWORD GetSizeOfImage(void* const imageBase);
	static PIMAGE_SECTION_HEADER GetPESection(void* const imageBase, const std::string& name);
	static PIMAGE_SECTION_HEADER GetPESection(void* const imageBase, const uint32_t index);
	static void* GetProcAddress(void* const imageBase, const std::string& procName);
	static const char* GetPdbName(void* const imageBase);
	static bool Contains(const void* const imageBase, const uintptr_t ip);

private:

	//ReactOS::sdk\include\reactos\wine\mscvpdb.h
	typedef struct OMFSignatureRSDS
	{
		char        Signature[4];
		guid_t        guid;
		DWORD       age;
		CHAR        name[1];
	} OMFSignatureRSDS;
};