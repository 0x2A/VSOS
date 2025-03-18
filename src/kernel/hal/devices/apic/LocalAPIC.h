#pragma once

#include "kernel/hal/devices/Device.h"
#include <map>


#define APIC_TICKS_PER_SEC	100

class HAL;
class LocalAPIC : public Device
{
public:
	LocalAPIC(HAL* hal);
	void Initialize(void* context) override;
	void SetProcessorAPIC(uint8_t processorID, uint8_t apicID);
	const void* GetResource(uint32_t type) const override;
	void DisplayDetails() const override;

	void NotifyEOIRequired(int vector);
	void SignalEOI();
	int EOIPending();
	void ipi(int vector);

	const uint64_t GetAddr() { return m_Addr; }
	const uint64_t GetPhysicalAddr() { return m_PhysicalAddr; }

	uint32_t id();

private:
	void CalibrateTimer();

	uint32_t read(uint32_t reg);
	void write(uint32_t reg, uint32_t data);

	uint64_t m_Addr;
	uint64_t m_PhysicalAddr;
	HAL* m_HAL;

	uint32_t apicCalibVal;

	uint32_t Freq;
	bool x2Apic;
	int last_interrupt;
	bool eoi_required;
};