#pragma once

#include "Kernel/objects/KFile.h"
#include <string>
#include <memory>
#include <kernel\drivers\Driver.h>

class DiskDriver
{
public:
	virtual char ReadSector(uint16_t drive, uint64_t sector, uint8_t* buffer) const = 0;
	virtual char WriteSector(uint16_t drive, uint64_t sector, uint8_t* buffer) const = 0;
	virtual bool EjectDrive(uint8_t drive) = 0;
};