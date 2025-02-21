#pragma once

#include <stdint.h>

typedef uint32_t(*InterruptHandler)(void* arg);
struct InterruptContext
{
	InterruptHandler Handler;
	void* Context;
};

