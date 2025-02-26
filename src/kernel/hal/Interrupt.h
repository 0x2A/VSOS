#pragma once

#include <stdint.h>

typedef uint32_t(*InterruptHandler)(void* arg);
struct InterruptContext
{
	InterruptHandler Handler;
	void* Context;
};


typedef struct InterruptRedirect {
	uint8_t type;
	uint8_t index;
	uint8_t interrupt;
	uint8_t destination;
	uint32_t flags;
	bool mask;

} interrupt_redirect_t;