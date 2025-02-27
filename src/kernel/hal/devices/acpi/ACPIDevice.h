#pragma once

#include <cstdint>

extern "C"
{
#include <acpi.h>
}

#include <list>
#include "kernel/hal/devices/Device.h"

class AcpiDevice : public Device
{

public:
	AcpiDevice(const ACPI_HANDLE object);

	void Initialize(void* context) override;
	const void* GetResource(uint32_t type) const override;
	const void* GetDeviceObject(char* PathName, ACPI_OBJECT_LIST* ExternalParams);
	void DisplayDetails() const override;
	const ACPI_DEVICE_INFO* GetDeviceInfo() {return m_objectInfo;}


private:
	static ACPI_STATUS AttachResource(ACPI_RESOURCE* Resource, void* Context);
	static ACPI_STATUS DisplayResource(const ACPI_RESOURCE& Resource);

	static ACPI_STATUS AttachDMA ( ACPI_RESOURCE* Resource, void* Context);

	const ACPI_HANDLE m_acpiObject;
	std::list<ACPI_RESOURCE> m_resources;
	ACPI_DEVICE_INFO* m_objectInfo;
};