#include "DriverManager.h"
#include <Assert.h>
#include "io\IOApicDriver.h"
#include "kernel\devices\Device.h"
#include "platform\AcpiProcessor.h"
#include "kernel\drivers\HyperV\VmBusDriver.h"
#include "kernel\drivers\HyperV\HyperVMouseDriver.h"

DriverManager::DriverManager()
	: m_Drivers()
{

}

void DriverManager::Initialize()
{
	//TODO: Create system to load/register drivers dynamically based on config file?
	//Register all drivers
	//m_Drivers.push_back(new IOApicDriver());
	//m_Drivers.push_back(new AcpiProcessor());
	//m_Drivers.push_back(new VmBusDriver());
	//m_Drivers.push_back(new HyperVMouseDriver());
}

Driver* DriverManager::CreateDriverForDevice(Device* device)
{
	for (auto& d : m_Drivers)
	{
		if(d->GetCompatHID() != device->GetHid())
			continue;

		return d->CreateInstance(device);
	}


	//if(device->GetHid().length() > 0)
		//Printf("Warn: No compatible driver found for device HID %s (%s)\r\n", device->GetHid().c_str(), device->Description.c_str());
	return nullptr;
}
