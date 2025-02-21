#pragma once

#include "Kernel/obj/KFile.h"
#include <string>
#include <memory>
#include <kernel\drivers\Driver.h>

class DiskDriver
{
public:
	virtual Result OpenFile(KFile& file, const std::string& path, const GenericAccess access) const = 0;
	virtual size_t ReadFile(const KFile& handle, void* const buffer, const size_t bytesToRead) const = 0;
};