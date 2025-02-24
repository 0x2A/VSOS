#pragma once
#include <cstdint>


#define MAX_CPUS	256
#define BOOT_CPU	0

class LocalAPIC;
class CPU
{
public:
	CPU(LocalAPIC* apic);

	uint8_t cpunum();

private:
	LocalAPIC* m_APIC;
};