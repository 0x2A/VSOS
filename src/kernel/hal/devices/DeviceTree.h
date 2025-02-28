#pragma once

#include "Device.h"
#include "acpi/ACPIDevice.h"
#include "pci/PCIDevice.h"
#include "kernel/drivers/DriverManager.h"
#include <vector>

class DeviceTree
{
public:
	DeviceTree();


	static void AddPCIDevice(PCIDevice* device);

	static void Display();

	static Device* GetDeviceByHid(const std::string& hid);
	static Device* GetDeviceByName(const std::string& name);
	static Device* GetDeviceByType(const DeviceType type);
	static Device* GetDeviceByPath(const std::string& path);

	
	static std::vector<AcpiDevice*> m_ACPIDevices;
	static std::vector<PCIDevice*> m_PCIDevices;
};