#include "DriverManager.h"
#include <Assert.h>

DriverManager::DriverManager()
	: Drivers()
{

}


DriverManager::~DriverManager()
{
	// Remove any drivers that are still attached
	while (!Drivers.empty())
		RemoveDriver(*Drivers.begin());
}

void DriverManager::AddDriver(Driver* driver)
{
	Drivers.push_back(driver);
}

void DriverManager::RemoveDriver(Driver* driver)
{
	driver->Deactivate();

	Drivers.remove(driver);

}

void DriverManager::OnDriverSelected(Driver* driver)
{
	AddDriver(driver);
}
