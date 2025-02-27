#include "Clock.h"
#include <kernel\hal\devices\apic\LocalAPIC.h>
#include "kernel\hal\HAL.h"
#include <intrin.h>
#include <kernel\hal\x64\interrupt.h>
#include <Assert.h>

#define RTC_COMMAND_PORT	0x70
#define RTC_DATA_PORT		0x71

//#define MS_PER_TICK			(1000/APIC_TICKS_PER_SEC)

const uint32_t msPerTick = 1000 / APIC_TICKS_PER_SEC;

uint32_t OnTimer0Interrupt(void* arg)
{
	Clock* c = (Clock*)arg;
	c->OnAPICTimerTick();

	return 0;
}

Clock::Clock(HAL* hal)
: m_HAL(hal), Driver(nullptr)
{
}

void Clock::OnAPICTimerTick()
{
	// Increment the number of ticks and decrement the number of ticks until the next event
	m_ticks++;
	
	for(auto handler = m_Handlers->begin(); handler != m_Handlers->end(); handler++)
		(*handler)->onTimerTick(m_ticks);

}


DriverResult Clock::Activate()
{
	m_Handlers = new std::vector<TickEventHandler*>(); //need this on heap so stack isn't overflown

	m_HAL->RegisterInterrupt((uint8_t)X64_INTERRUPT_VECTOR::Timer0, { OnTimer0Interrupt, this });

	// read the status register
	uint8_t status = ReadRTC(0xB);

	// Set the clock information
	m_24_hour_clock = status & 0x02;
	m_binary = status & 0x04;

	Printf("Clock activated\r\n");
	return DriverResult::Success;
}

DriverResult Clock::Deactivate()
{
	m_HAL->UnRegisterInterrupt((uint8_t)X64_INTERRUPT_VECTOR::Timer0);

	delete m_Handlers;
	return DriverResult::Success;
}

DriverResult Clock::Initialize()
{
	return DriverResult::NotImplemented;
}

DriverResult Clock::Reset()
{
	return DriverResult::NotImplemented;
}

std::string Clock::get_vendor_name()
{
	return "Generic";
}

std::string Clock::get_device_name()
{
	return "RTC";
}

DeviceType Clock::get_device_type()
{
	return DeviceType::System;
}

void Clock::delay(uint32_t milliseconds)
{
	// Round the number of milliseconds to the nearest MS_PER_TICK
	uint64_t rounded_milliseconds = ((milliseconds + (msPerTick-1)) / msPerTick);

	// Calculate the number of ticks until the delay is over
	uint64_t ticks_until_delay_is_over = m_ticks + rounded_milliseconds;

	// Wait until the number of ticks is equal to the number of ticks until the delay is over
	while (m_ticks < ticks_until_delay_is_over)
		__nop();
}

Time Clock::get_time()
{
	// Wait for the clock to be ready
	while (ReadRTC(0xA) & 0x80)
		__nop();

	Time time{};

	// read the time from the hardware clock
	time.year = binary_representation(ReadRTC(0x9)) + 2000;
	time.month = binary_representation(ReadRTC(0x8));
	time.day = binary_representation(ReadRTC(0x7));
	time.hour = binary_representation(ReadRTC(0x4));
	time.minute = binary_representation(ReadRTC(0x2));
	time.second = binary_representation(ReadRTC(0x0));

	// If the clock is using 12hr format and PM is set then add 12 to the hour
	if (!m_24_hour_clock && (time.hour & 0x80) != 0)
		time.hour = ((time.hour & 0x7F) + 12) % 24;


	return time;
}

void Clock::RegisterTickHandler(TickEventHandler* handler)
{
	m_Handlers->push_back(handler);
}

uint8_t Clock::ReadRTC(uint8_t addr)
{
	// Send the address to the hardware clock
	m_HAL->WritePort(RTC_COMMAND_PORT, addr, 8);

	// read the value from the hardware clock
	return (uint8_t)m_HAL->ReadPort(RTC_DATA_PORT, 8);
}

uint8_t Clock::binary_representation(uint8_t number)
{
	// If the binary coded decimal representation is not used, return the number
	if (m_binary)
		return number;

	// Otherwise, return the binary representation
	return ((number / 16) * 10) + (number & 0x0f);
}
