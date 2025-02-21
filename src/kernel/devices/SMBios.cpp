#include "SMBios.h"
#include <string.h>
#include <Assert.h>
#include "kernel/Kernel.h"

SMBios::SMBios()
{
	
}

bool SMBios::Init(uint64_t ESP)
{
	//Handle unaligned addresses
	
	header = (EPS_64*)kernel.MapPhysicalMemory(ESP, sizeof(EPS_64), KernelIoStart);
	
	//header = (EPS_64*)kernel.VirtualMap(0x0, { ESP });
	
	if(memcmp(header->Anchor, "_SM3_", 5) != 0)
		return false;
	
	//Printf("SMBIOS: cs: 0x%x, l: %d, v: %d.%d, addr: 0x%16x\r\n", header->Checksum, header->EntryLength, header->MajorVersion, header->MinorVersion, header->TableAddress);
	
	SMBIOSHeader* tbl = (SMBIOSHeader*)kernel.MapPhysicalMemory(header->TableAddress, sizeof(SMBIOSHeader)*512, KernelIoStart);;

	smBiosVersion = header->MajorVersion * 10 + header->MinorVersion;
	
	while (true)
	{
		if(tbl->type == 127) break;

		//Printf("Header type: %d, len: %d\r\n", tbl->type, tbl->length);
		switch (tbl->type)
		{
		case BiosInfo:
		{	
			//Printf("Found BiosInfo\r\n");
			
			if (smBiosVersion >= 31)
			{
				smbios31_bios_information* biosInfo = (smbios31_bios_information*)tbl;

				uint64_t strBegin = (uint64_t)biosInfo + biosInfo->length;
				uint64_t ctr = strBegin;

				std::vector<std::string> strings;
				while (ctr)
				{
					char* s = (char*)ctr;
					if(s[0] == '\0') break;
					//Printf("string: %s\r\n", s);
					strings.push_back(s);
					ctr += strlen(s)+1;
				}
			}
		}
		break;
		case ProcessorInfo:
		{
			//Printf("Found ProcessorInfo:\r\n");
			cpuInfo.smbiosInfo = (smbios_cpu_info*)tbl;
			if (smBiosVersion >= 36)
			{
				cpuInfo.smbiosInfo = (smbios_cpu_info_36*)tbl;
			}
			else if(smBiosVersion >= 30)
				cpuInfo.smbiosInfo = (smbios_cpu_info_30*)tbl;
			else if(smBiosVersion >= 26)
				cpuInfo.smbiosInfo = (smbios_cpu_info_26*)tbl;

			uint64_t strBegin = (uint64_t)cpuInfo.smbiosInfo + cpuInfo.smbiosInfo->header.length;
			uint64_t ctr = strBegin;
			while (ctr)
			{
				char* s = (char*)ctr;
				if (s[0] == '\0') break;
				//Printf("string: %s\r\n", s);
				cpuInfo.strings.push_back(s);
				ctr += strlen(s) + 1;
			}

			Printf("ExternalClockSpeed: %d, max %d, curr %d\r\n", cpuInfo.smbiosInfo->external_clock, cpuInfo.smbiosInfo->max_speed, cpuInfo.smbiosInfo->current_speed);
		}
		break;
		case OEMStrings:
		{
			smbios_oem_info* oemInfo = (smbios_oem_info*)tbl;
			uint64_t strBegin = (uint64_t)oemInfo + oemInfo->header.length;
			uint64_t ctr = strBegin;
			while (ctr)
			{
				char* s = (char*)ctr;
				if (s[0] == '\0') break;
				//Printf("string: %s\r\n", s);
				//cpuInfo.strings.push_back(s);
				ctr += strlen(s) + 1;
			}
		}
		break;
		}

		uint64_t end = smbios_struct_end(tbl);
		tbl = (SMBIOSHeader*)end;
	}
	
}

uint64_t SMBios::smbios_struct_end(SMBIOSHeader* hd)
{
	char *addr = (char*)(hd) + hd->length;
	while (*addr != '\0' || *(addr + 1) != '\0') {
		addr++;
	}

	return ((uint64_t)(addr+2));
}

uint16_t SMBios::GetExtBusSpeed()
{	
	if (cpuInfo.smbiosInfo)
	{
		return cpuInfo.smbiosInfo->external_clock;
	}
}
