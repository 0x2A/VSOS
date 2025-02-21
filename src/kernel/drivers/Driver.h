#pragma once

#include <string>
#include "os.internal.h"

#include <memory>

enum class Result
{
	Success,
	Failed,
	NotImplemented
};

class Device;
class Driver
{
	friend class DriverManager;
public:
	Driver(Device& device);

	virtual const std::string GetCompatHID() = 0;
	virtual Driver* CreateInstance(Device* device) = 0;

	virtual Result Initialize() = 0;
	virtual Result Read(char* buffer, size_t length, size_t* bytesRead = nullptr) = 0;
	virtual Result Write(const char* buffer, size_t length) = 0;
	virtual Result EnumerateChildren() = 0;

protected:
	
	Device& m_device;
};