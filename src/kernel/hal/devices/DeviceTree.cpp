#include "DeviceTree.h"
#include "kernel/drivers/Driver.h"
#include <Assert.h>
#include <map>
#include <stack>
#include <queue>
#include "kernel/drivers/DriverManager.h"
#include "kernel/Kernel.h"

DeviceTree::DeviceTree() 
{

}


void DeviceTree::AddPCIDevice(PCIDevice* device)
{
	m_PCIDevices.push_back(device);
}


Device* DeviceTree::GetDeviceByHid(const std::string& hid)
{
	for (auto pciDevice : m_PCIDevices)
	{
		if(pciDevice->GetHid() == hid)
			return pciDevice;
	}
	for (auto acpiDevice : m_ACPIDevices)
	{
		if(acpiDevice->GetHid() == hid)
			return acpiDevice;
	}
	return nullptr;
}

Device* DeviceTree::GetDeviceByName(const std::string& name)
{
	for (auto pciDevice : m_PCIDevices)
	{
		if (pciDevice->Name == name)
			return pciDevice;
	}
	for (auto acpiDevice : m_ACPIDevices)
	{
		if (acpiDevice->Name == name)
			return acpiDevice;
	}
	return nullptr;
}

Device* DeviceTree::GetDeviceByType(const DeviceType type)
{
	for (auto pciDevice : m_PCIDevices)
	{
		if (pciDevice->Type == type)
			return pciDevice;
	}
	for (auto acpiDevice : m_ACPIDevices)
	{
		if (acpiDevice->Type == type)
			return acpiDevice;
	}
	return nullptr;
}

//This could be smarter and split the path to search. For now just DFS
Device* DeviceTree::GetDeviceByPath(const std::string& path)
{
	for (auto pciDevice : m_PCIDevices)
	{
		if (pciDevice->Path == path)
			return pciDevice;
	}
	for (auto acpiDevice : m_ACPIDevices)
	{
		if (acpiDevice->Path == path)
			return acpiDevice;
	}
	return nullptr;
	return nullptr;
}

std::vector<AcpiDevice*> DeviceTree::m_ACPIDevices;

std::vector<PCIDevice*> DeviceTree::m_PCIDevices;

