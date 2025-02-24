#pragma once

#include "kernel/hal/devices/Device.h"
#include <map>

class HAL;
class LocalAPIC : public Device
{
public:
	LocalAPIC(HAL* hal);
	void Initialize(void* context) override;
	void SetProcessorAPIC(uint8_t processorID, uint8_t apicID);
	const void* GetResource(uint32_t type) const override;
	void DisplayDetails() const override;

	void SignalEOI();
	void ipi(int vector);

	uint32_t id();

private:
	void CalibrateTimer();

	uint32_t read(uint32_t reg);
	void write(uint32_t reg, uint32_t data);

	uint64_t m_Addr;
	HAL* m_HAL;

	uint32_t apicCalibVal;

	uint32_t Freq;
	bool x2Apic;
};