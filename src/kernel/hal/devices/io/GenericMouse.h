#pragma once


#include "kernel/hal/devices/Device.h"
#include <Assert.h>

class GenericMouse : public Device
{

public:
	GenericMouse()
	{
		Name = "Mouse";
		sprintf(Path,"HID\\GEN_MOUSE");
		Description = "Generic Mouse";
		Type = DeviceType::Mouse;
	}
	
	void Initialize(void* context) override
	{
		//Nothing to do
	}




	const void* GetResource(uint32_t type) const override
	{
		return nullptr;
	}


	void DisplayDetails() const override
	{
		Printf("Generic Mouse Device\r\n");
	}

};