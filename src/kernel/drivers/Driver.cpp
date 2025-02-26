#include "Driver.h"



Driver::Driver(Device* device)
: m_device(device)
{
	
}

DeviceType Driver::get_device_type()
{
	if(m_device)
		return m_device->Type;
	return DeviceType::Unknown;
}

