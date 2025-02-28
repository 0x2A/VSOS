#pragma once

#include <string>
#include <list>

enum class DeviceType
{
	Unknown,
	Keyboard,
	Mouse,
	Harddrive,
	Serial,
	System,
	Other,
};


class Driver;
class Device
{
public:
	Device();

	virtual void Initialize(void* context) = 0;
	virtual const void* GetResource(uint32_t type) const = 0;
	virtual void DisplayDetails() const = 0;

	const std::string GetHid() const
	{
		return m_hid;
	}

	virtual void RegisterDriver(Driver* driver)
	{
		m_Driver = driver;
	}

	void Display() const;

	std::string Name;
	std::string Path;
	std::string Description;
	DeviceType Type;

protected:
	std::string m_hid;
	Driver* m_Driver;
	
};