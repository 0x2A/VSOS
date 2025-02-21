#pragma once


#include "PMM.h"
#include <intrin.h>
#include <cstdint>
#include "VAS.h"

//Ask address space for block
//Request physical pages
//Map

class VMM
{
public:
	VMM(PMM& physicalMemory);

	void* Allocate(const void* address, const size_t count, VirtualAddressSpace& addressSpace);
	void* VirtualMap(const void* address, const std::vector<paddr_t>& addresses, VirtualAddressSpace& addressSpace);

private:
	PMM& m_physicalMemory;
};