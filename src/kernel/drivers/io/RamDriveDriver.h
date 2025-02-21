#pragma once

#include "DiskDriver.h"
#include "kernel/devices/Device.h"

#include <memory>
#include <Ramdrive.h>

static const char* RamDriveHid = "RDRV";

class RamDriveDriver : public Driver, public DiskDriver
{
public:
	RamDriveDriver(Device& device);


	const std::string GetCompatHID() override
	{
		return RamDriveHid;
	}
	Driver* CreateInstance(Device* device) override;

	Result Initialize() override;
	Result Read(char* buffer, size_t length, size_t* bytesRead = nullptr) override;
	Result Write(const char* buffer, size_t length) override;
	Result EnumerateChildren() override;


	Result OpenFile(KFile& file, const std::string& path, const GenericAccess access) const override;


	size_t ReadFile(const KFile& handle, void* const buffer, const size_t bytesToRead) const override;
private:
	RamDrive* m_ramDrive;
};