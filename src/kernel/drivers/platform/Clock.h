#pragma once

#include "kernel/drivers/Driver.h"
#include "kernel/time.h"
#include <vector>

class TickEventHandler
{
public:
	virtual void onTimerTick(uint64_t totalTicks) = 0;
};


class HAL;
class Clock : public Driver
{

public:
	Clock(HAL* hal);

	void OnAPICTimerTick();

	DriverResult Activate() override;
	DriverResult Deactivate() override;

	DriverResult Initialize() override;
	DriverResult Reset() override;

	std::string get_vendor_name() override;
	std::string get_device_name() override;
	DeviceType get_device_type() override;

	void delay(uint32_t milliseconds);
	Time get_time();

	void RegisterTickHandler(TickEventHandler* handler);

private:

	uint8_t ReadRTC(uint8_t addr);

	uint8_t binary_representation(uint8_t number);

	uint64_t m_ticks{ 0 };
	HAL* m_HAL;

	bool m_binary;
	bool m_24_hour_clock;

	std::vector<TickEventHandler*>* m_Handlers;
};