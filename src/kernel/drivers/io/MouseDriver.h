#pragma once

#include "kernel/drivers/Driver.h"
#include <vector>
#include "kernel/os/types.h"

class MouseEventHandler
{
public:
	virtual void OnMouseButtonEvent(MouseButtonEvent buttonEvent) = 0;
	virtual void OnMouseMoveEvent(MouseMoveEvent buttonEvent) = 0;
};

class HAL;
class MouseDriver : public Driver
{
public:
	MouseDriver(HAL* hal);

	DriverResult Activate() override;
	DriverResult Deactivate() override;
	DriverResult Initialize() override;
	DriverResult Reset() override;


	std::string get_vendor_name() override;
	std::string get_device_name() override;

	void AddEventHandler(MouseEventHandler* handler);

	static uint32_t handle_interrupt(void* arg);

private:

	void RaiseMouseMoveEvent(MouseMoveEvent event);
	void RaiseMouseButtonEvent(MouseButtonEvent event);

	void HandleInterrupt();

	uint8_t buffer[3];
	uint8_t offset{ 2 };
	uint8_t buttons{ 0 };

	std::vector<MouseEventHandler*> m_MouseEventHandler;
	HAL* m_HAL;
};