#pragma once
#include <map>
#include <string>
#include "Driver.h"
#include <list>


class DriverManager
{
public:
	DriverManager();
	~DriverManager();

	void AddDriver(Driver* driver);
	void RemoveDriver(Driver* driver);
	void OnDriverSelected(Driver*);

	std::list<Driver*> Drivers;

private:

};