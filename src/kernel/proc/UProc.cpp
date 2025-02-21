#include "UProc.h"

uint32_t UserProcess::LastId = 0;

UserProcess::UserProcess(const std::string& name, const bool isConsole) :
	KSignalObject(),
	InitProcess(),
	InitThread(),
	Name(name),
	Id(++LastId),
	IsConsole(isConsole),

	m_imageBase(),
	m_createTime(),
	m_exitTime(),
	m_pageTables(),
	m_addressSpace(),
	m_heap(),
	m_peb(),
	m_threads(),
	m_ringBuffers(),
	m_lastHandle(StartingHandle),
	m_objects(),
	m_state(ProcessState::Running)
{
	//Create new page tables, using current top level kernel mappings
	m_pageTables.CreateNew();
	m_pageTables.LoadKernelMappings();
	m_addressSpace.Initialize();
}

uintptr_t UserProcess::GetCR3() const
{
	return m_pageTables.GetRoot();
}

VirtualAddressSpace& UserProcess::GetAddressSpace()
{
	return m_addressSpace;
}

void UserProcess::AddModule(const char* name, void* address)
{
	//Has to be called within context of process for now
	Assert(m_pageTables.IsActive());

	//TODO: check name length

	m_peb->LoadedModules[m_peb->ModuleIndex].ImageBase = address;
	strcpy(m_peb->LoadedModules[m_peb->ModuleIndex].Name, name);
	m_peb->ModuleIndex++;
}

void UserProcess::Display() const
{
	Printf("UserProcess::Display\n");
	Printf("     ID: %d\n", Id);
	Printf("   Name: %s\n", Name.c_str());
	Printf("   Base: 0x%016x\n", m_imageBase);
	Printf("Threads: %d\n", m_threads.size());
	Printf("    PEB: 0x%016x\n", m_peb);
}

bool UserProcess::IsSignalled() const
{
	return m_state == ProcessState::Terminated;
}
