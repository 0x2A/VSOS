
#include <efi.h>
#include <efilib.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>

static constexpr size_t MaxBuffer = 256;

void Printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[MaxBuffer / 2] = { 0 };
	int retval = vsprintf(buffer, format, args);
	buffer[retval] = '\0';
	va_end(args);

	CHAR16 wide[MaxBuffer] = { 0 };
	size_t num;
	mbstowcs(wide, buffer, MaxBuffer);

	//Write to UEFI
	ST->ConOut->OutputString(ST->ConOut, wide);
	VPrint(wide, args);
}


void Bugcheck(const char* file, const char* line, const char* format, ...)
{
	Printf("Init Bugcheck\r\n");
	Printf("%s\r\n%s\r\n", file, line);

	va_list args;
	va_start(args, format);
	Printf(format, args);
	Printf("\r\n");
	va_end(args);

	while (true);
}

void CPrintf(const bool enable, const char* format, ...)
{
	if (!enable)
		return;

	va_list args;
	va_start(args, format);
	char buffer[MaxBuffer / 2] = { 0 };
	int retval = vsprintf(buffer, format, args);
	buffer[retval] = '\0';
	va_end(args);

	CHAR16 wide[MaxBuffer] = { 0 };
	mbstowcs(wide, buffer, MaxBuffer);

	//Write to UEFI
	ST->ConOut->OutputString(ST->ConOut, wide);

}
