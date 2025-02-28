#pragma once

#include "kernel/hal/devices/Device.h"
#include <Assert.h>

class GenericKeyboard : public Device
{

public:
	GenericKeyboard()
	{
		Name = "Keyboard";
		Path = "HID\\GEN_KEY";
		Description = "Generic Keyboard";
		Type = DeviceType::Keyboard;
	}

	void Initialize(void* context) override
	{
	}


	const void* GetResource(uint32_t type) const override
	{
		return nullptr;
	}


	void DisplayDetails() const override
	{
		Printf("Generic Keyboard Device\r\n");
	}

};