#pragma once

#include "Device.h"
#include <map>
extern "C"
{
#include <acpi.h>
}


class LocalAPIC : public Device 
{

public:
	LocalAPIC(uint64_t addr);

	void SetProcessorAPIC(uint8_t processorID, uint8_t apicID);
	void Initialize() override;
	const void* GetResource(uint32_t type) const override;
	void DisplayDetails() const override;
	void SignalEOI();
private:

	uint32_t read(uint32_t reg);
	void write(uint32_t reg, uint32_t data);

	std::map<uint8_t, uint8_t> m_ProcessorAPICs; //maps processorID->APIC ID
	uint64_t m_Addr;

	uint64_t timer_speed_us;
};