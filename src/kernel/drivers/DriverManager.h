#pragma once
#include <map>
#include <string>
#include "Driver.h"
#include <list>


class DriverManager
{
public:
	DriverManager();

	void Initialize();
	Driver* CreateDriverForDevice(Device* device);

private:

	std::list<Driver*> m_Drivers;
};