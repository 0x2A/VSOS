#pragma once

#include <string>
#include <kernel\hal\devices\Device.h>
//#include "os.internal.h"

//#include <memory>

enum class DriverResult
{
	Success,
	Failed,
	NotImplemented
};

class Driver
{
	friend class DriverManager;
public:
	Driver(Device* device);

	virtual DriverResult Activate() = 0;
	virtual DriverResult Deactivate() = 0;
	virtual DriverResult Initialize() = 0;
	virtual DriverResult Reset() = 0;

	virtual std::string get_vendor_name() = 0;
	virtual std::string get_device_name() = 0;

	virtual DeviceType get_device_type();

	//virtual Result Read(char* buffer, size_t length, size_t* bytesRead = nullptr) = 0;
	//virtual Result Write(const char* buffer, size_t length) = 0;
	//virtual Result EnumerateChildren() = 0;

	
protected:
	
	Device* m_device;
};