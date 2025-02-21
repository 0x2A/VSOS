#pragma once
#include "os.System.h"

#define PLATFORM_X64  defined(__x86_64__) || defined(_M_X64)

class HAL
{
public:
	void initialize();

	void SetupPaging(paddr_t root);
	void Wait();
	void SaveContext(void* context);
};