#pragma once


#include "kernel/drivers/Driver.h"
#include <kernel\arch\x64\interrupt.h>
#include <kernel\devices\ACPIDevice.h>




class AcpiProcessor : public Driver
{

public:
	AcpiProcessor(Device& device);

	const std::string GetCompatHID() override;


	Driver* CreateInstance(Device* device) override;


	Result Initialize() override;


	Result Read(char* buffer, size_t length, size_t* bytesRead = nullptr) override;


	Result Write(const char* buffer, size_t length) override;


	Result EnumerateChildren() override;

	void SetLocalAPIC(ACPI_MADT_LOCAL_APIC* lapictable);

private:
	uint8_t cpuID;
	ACPI_MADT_LOCAL_APIC* m_LocalAPIC;
};