#include "AHCIPort.h"
#include <Assert.h>
#include <intrin.h>
#include <OS.System.h>
#include "kernel/Kernel.h"
#include <kernel\io\disk\Disk.h>

AHCIPort::AHCIPort(AHCIDriver* driver, hba_port* port, uint8_t portNumber)
	: m_Driver(driver), m_HBAPort(port), port_number(portNumber)
{
}

bool AHCIPort::ConfigurePort()
{
	if(!Disable())
		return false;

	m_Type = check_port_type();
	if(m_Type == PORT_TYPE_NONE) return false;

	auto pageCount = SizeToPages(1024);
	paddr_t new_base_phys = kernel.AllocatePhysical(pageCount);
	void* new_base = (uint8_t*)kernel.DriverMapPages(new_base_phys, pageCount);

	m_HBAPort->command_list_base = (uint32_t)(uint64_t)new_base_phys;
	m_HBAPort->command_list_base_upper = (uint32_t)((uint64_t)new_base_phys >> 32);
	memset(new_base, 0, 1024);

	pageCount = SizeToPages(256);
	paddr_t new_fis_base_phys = kernel.AllocatePhysical(pageCount);
	void* new_fis_base = kernel.DriverMapPages(new_fis_base_phys, pageCount);

	m_HBAPort->fis_base_address = (uint32_t)(uint64_t)new_fis_base_phys;
	m_HBAPort->fis_base_address_upper = (uint32_t)((uint64_t)new_fis_base_phys >> 32);
	memset(new_fis_base, 0, 256);

	command_header = (struct hba_command_header*)(new_base);

	for (int i = 0; i < 32; i++) {
		command_header[i].prdt_length = 8;

		paddr_t cmd_table_address_phys = kernel.AllocatePhysical(pageCount);
		void* cmd_table_address = kernel.DriverMapPages(cmd_table_address_phys, pageCount);
		command_table[i] = (hba_command_table*)cmd_table_address;

		uint64_t address = (uint64_t)cmd_table_address_phys + ((uint64_t)(i) << 8);
		command_header[i].command_table_base_address = (uint32_t)address;
		command_header[i].command_table_base_address_upper = (uint32_t)(address >> 32);
		memset(cmd_table_address, 0, 256);

	}

	// Power and Spin up device
	m_HBAPort->command_status |= HBA_PxCMD_POD | HBA_PxCMD_SUD;
	
	// Activate link
	//writeRegister(AHCI_PORTREG_CMDANDSTATUS, (readRegister(AHCI_PORTREG_CMDANDSTATUS) & ~PORT_CMD_ICC_MASK) | PORT_CMD_ICC_ACTIVE);

	// Enable FIS receive (enabled when fb set, only to be disabled when unset)
	//writeRegister(AHCI_PORTREG_CMDANDSTATUS, readRegister(AHCI_PORTREG_CMDANDSTATUS) | PORT_CMD_FRE);


	return true;
}

bool AHCIPort::StartupPort()
{
	constexpr auto pageCount = SizeToPages(4096);
	bufferPhysicalAddr = kernel.AllocatePhysical(pageCount);
	buffer = (uint8_t*)kernel.DriverMapPages(bufferPhysicalAddr, pageCount);
	memset(buffer, 0, 4096);

	if (this->Enable() == false)
		return false;

	// Enable interrupts
	m_HBAPort->interrupt_enable = PORT_INT_MASK;

	//Printf("AHCI: Port %d is of type %d\r\n", port_number, m_Type);

	if (m_Type != PORT_TYPE_NONE)
	{
		isATATPI = m_Type == PORT_TYPE_SATAPI;

		// Now we can send a Identify command to the device
		// Data will be the same as with the IDE controller
		if (!this->Identify())
			return false;

		uint32_t diskSize = 0;
		char diskModel[41];

		// Extract Command set from buffer
		uint32_t commandSet = *((uint32_t*)&buffer[ATA_IDENT_COMMANDSETS]);
		// Get Size:
		if (commandSet & (1 << 26)) {
			// Device uses 48-Bit Addressing:
			diskSize = *((uint32_t*)&buffer[ATA_IDENT_MAX_LBA_EXT]);
			this->useLBA48 = true;
		}
		else {
			// Device uses CHS or 28-bit Addressing:
			diskSize = *((uint32_t*)&buffer[ATA_IDENT_MAX_LBA]);
			this->useLBA48 = false;
		}

		// String indicates model of device
		uint8_t* strPtr = (uint8_t*)buffer;
		for (int k = 0; k < 40; k += 2) {
			diskModel[k] = strPtr[ATA_IDENT_MODEL + k + 1];
			diskModel[k + 1] = strPtr[ATA_IDENT_MODEL + k];
		}
		diskModel[40] = 0; // Terminate String.

		Printf("AHCI: Found %s drive %s\r\n", this->isATATPI ? "ATAPI" : "ATA", diskModel);
		uint32_t sectSize = this->isATATPI ? 2048 : 512;
		
		//Create Disk Object
		Disk* disk = new Disk(m_Driver, port_number, isATATPI ? CDROM : HardDisk, (uint64_t)(diskSize / 2U) * (uint64_t)1024, diskSize / 2 / sectSize, sectSize);

		// Create Identifier
		int strLen = 40;
		while (diskModel[strLen - 1] == ' ' && strLen > 1)
			strLen--;
		disk->identifier = new char[strLen + 1];

		memcpy(disk->identifier, diskModel, strLen);
		disk->identifier[strLen] = '\0';

		kernel.GetDiskManager()->AddDisk(disk);
	}

	return true;
}

bool AHCIPort::Enable()
{
	while (m_HBAPort->command_status & HBA_PxCMD_CR) __nop();
	m_HBAPort->command_status |= HBA_PxCMD_FRE;
	m_HBAPort->command_status |= HBA_PxCMD_ST;

	return true;
}

bool AHCIPort::Disable()
{
	m_HBAPort->command_status &= ~HBA_PxCMD_ST;
	m_HBAPort->command_status &= ~HBA_PxCMD_FRE;

	// Wait until FR (bit14), CR (bit15) are cleared
	int i=0;
	while (i < 10000000) {
		i++;
		if (m_HBAPort->command_status & HBA_PxCMD_FR) continue;
		if (m_HBAPort->command_status & HBA_PxCMD_CR) continue;
		return true;
	}
	return false;
}

bool AHCIPort::Identify()
{
	
	m_HBAPort->interrupt_status = (uint32_t)-1;

	int spin = 0;
	int slot = (int)FindFreeCMDSlot();
	if (slot == -1) {
		Printf("No free command slots\n");
		return false;
	}

	//struct hba_command_header* command_header = (struct hba_command_header*)kernel.PhysicalToVirtual((uint64_t)(port->hba_port->command_list_base | (((uint64_t)port->hba_port->command_list_base_upper) << 32)));
	hba_command_header ch = this->command_header[slot];
	//command_header += slot;
	ch.command_fis_length = 5;
	ch.write = 0;
	ch.prdt_length = 1;
	ch.prefetchable = 1;
	ch.clear_busy_on_ok = 1;

	paddr_t buffer = bufferPhysicalAddr;

	//struct hba_command_table* command_table = (struct hba_command_table*)kernel.PhysicalToVirtual( (uint64_t)(command_header->command_table_base_address | ((uint64_t)command_header->command_table_base_address_upper << 32)));
	hba_command_table* ct = command_table[slot];
	memset(ct, 0, sizeof(struct hba_command_table) + (ch.prdt_length - 1) * sizeof(struct hba_prdt_entry));
	ct->prdt_entry[0].data_base_address_upper = (uint32_t)((uint64_t)buffer >> 32);
	ct->prdt_entry[0].data_base_address = (uint32_t)((uint64_t)buffer);
	ct->prdt_entry[0].byte_count = 512 - 1;
	ct->prdt_entry[0].interrupt_on_completion = 1;

	struct hba_command_fis* command_fis = (struct hba_command_fis*)ct->command_fis;
	memset(command_fis, 0, sizeof(struct hba_command_fis));
	command_fis->fis_type = FIS_TYPE_REG_H2D;
	command_fis->command_control = 1;
	command_fis->command = isATATPI ? ATA_CMD_IDENTIFY_PACKET : ATA_CMD_IDENTIFY;

	while (m_HBAPort->task_file_data & (ATA_DEV_BUSY | ATA_DEV_DRQ) && spin < 1000000) {
		spin++;
	};
	if (spin == 1000000) {
		Printf("AHCI: Port is hung\n");
		return false;
	}

	m_HBAPort->command_issue = 1;

	while (1) {
		if ((m_HBAPort->command_issue & (1 << slot)) == 0) break;
		if (m_HBAPort->interrupt_status & HBA_PxIS_TFES) {
			return false;
		}
	}

	if (m_HBAPort->interrupt_status & HBA_PxIS_TFES) {
		return false;
	}

	return true;
}

uint32_t AHCIPort::TransferData(bool dirIn, uint64_t sector, uint32_t count /*= 1*/)
{
	//if ((uint64_t)abar == 0) return 0;

	uint32_t count2 = count;
	//Assert(port_no < port_count);
	//struct ahci_port* port = &ahci_ports[port_no];
	//Assert(count < (isATATPI ? 2 : 8));
	uint32_t sector_low = (uint32_t)sector;
	uint32_t sector_high = (uint32_t)(sector >> 32);

	//Write-clear all bits in interrupt status
	m_HBAPort->interrupt_status = 0xFFFFFFFF;

	int spin = 0;
	int slot = (int)FindFreeCMDSlot();
	if (slot == -1) {
		Printf("No free command slots\n");
		return 0;
	}

	//struct hba_command_header* command_header = (struct hba_command_header*)(uint64_t)(port->hba_port->command_list_base | (((uint64_t)port->hba_port->command_list_base_upper) << 32));
	hba_command_header ch = command_header[slot];
	//command_header += slot;
	ch.command_fis_length = sizeof(struct hba_command_fis) / sizeof(uint32_t);
	ch.write = 0;
	ch.prdt_length = (uint16_t)((count - 1) >> 4) + 1;

	//struct hba_command_table* command_table = (struct hba_command_table*)(uint64_t)(command_header->command_table_base_address | ((uint64_t)command_header->command_table_base_address_upper << 32));
	hba_command_table* ct = command_table[slot];
	memset(ct, 0, sizeof(struct hba_command_table) + (ch.prdt_length - 1) * sizeof(struct hba_prdt_entry));

	paddr_t buffer = bufferPhysicalAddr;

	int i;
	for (i = 0; i < ch.prdt_length - 1; i++) {
		ct->prdt_entry[i].data_base_address = (uint32_t)buffer;
		ct->prdt_entry[i].data_base_address_upper = (uint32_t)(buffer >> 32);
		ct->prdt_entry[i].byte_count = 8 * 1024 - 1;
		ct->prdt_entry[i].interrupt_on_completion = 1;
		buffer = (buffer + 0x1000);
		count -= 16;
	}

	ct->prdt_entry[i].data_base_address = (uint32_t)(uint64_t)buffer;
	ct->prdt_entry[i].data_base_address_upper = (uint32_t)((uint64_t)buffer >> 32);
	ct->prdt_entry[i].byte_count = (count << 9) - 1;
	ct->prdt_entry[i].interrupt_on_completion = 1;

	struct hba_command_fis* command_fis = (struct hba_command_fis*)ct->command_fis;

	command_fis->fis_type = FIS_TYPE_REG_H2D;
	command_fis->command_control = 1;

	if (isATATPI)
	{
		command_fis->command = ATA_CMD_PACKET;
		command_fis->feature_low = 1;
		command_fis->feature_high = 0;
		command_fis->count_low = count & 0xFF;
		command_fis->count_high = (count >> 8);

		ch.atapi = 1;

		ct->atapi_command[0] = ATAPI_READ_CMD;
		ct->atapi_command[1] = 0x0;
		ct->atapi_command[2] = (sector_high >> 8) & 0xFF;
		ct->atapi_command[3] = (sector_high);
		ct->atapi_command[4] = (sector_low >> 24) & 0xFF;
		ct->atapi_command[5] = (sector_low >> 16) & 0xFF;
		ct->atapi_command[6] = (sector_low >> 8) & 0xFF;
		ct->atapi_command[7] = (sector_low) & 0xFF;
		ct->atapi_command[8] = 0;
		ct->atapi_command[9] = count;
		ct->atapi_command[10] = 0;
		ct->atapi_command[11] = 0;
		
	}
	else
	{
		command_fis->command = dirIn ? ATA_CMD_READ_DMA_EX : ATA_CMD_WRITE_DMA_EX;

		command_fis->lba0 = (uint8_t)sector_low;
		command_fis->lba1 = (uint8_t)(sector_low >> 8);
		command_fis->lba2 = (uint8_t)(sector_low >> 16);

		command_fis->device_register = 1 << 6;

		command_fis->lba3 = (uint8_t)(sector_low >> 24);
		command_fis->lba4 = (uint8_t)(sector_high);
		command_fis->lba5 = (uint8_t)(sector_high >> 8);

		command_fis->count_low = count & 0xFF;
		command_fis->count_high = (count >> 8);
	}
	

	while (m_HBAPort->task_file_data & (ATA_DEV_BUSY | ATA_DEV_DRQ) && spin < 1000000) {
		spin++;
	};
	if (spin == 1000000) {
		Printf(__FUNCTION__": Port is hung\n");
		return 0;
	}

	m_HBAPort->command_issue = (1 << slot);

	while (1) {
		if ((m_HBAPort->command_issue & (1 << slot)) == 0) break;
		if (m_HBAPort->interrupt_status & HBA_PxIS_TFES) {
			return 0;
		}
	}

	if (m_HBAPort->interrupt_status & HBA_PxIS_TFES) {
		return 0;
	}

	return (count2 * (isATATPI ? 2048 : 512));
}

bool AHCIPort::Eject()
{
	if(!isATATPI) return false;

	//TODO

	return true;
}

int8_t AHCIPort::FindFreeCMDSlot()
{
	uint32_t slots = (m_HBAPort->sata_active | m_HBAPort->command_issue);
	int cmd_slots = (m_Driver->abar->host_capabilities & 0x0f00) >> 8;
	for (int i = 0; i < cmd_slots; i++) {
		if ((slots & 1) == 0) {
			return i;
		}
		slots >>= 1;
	}

	Printf("Cannot find free command list entry\n");
	return -1;
}

port_type AHCIPort::check_port_type()
{
	uint32_t stata_status = m_HBAPort->sata_status;
	uint8_t interface_power_management = (stata_status >> 8) & 0x7;
	uint8_t device_detection = stata_status & 0x7;

	if (device_detection != 0x03) return PORT_TYPE_NONE;
	if (interface_power_management != 0x01) return PORT_TYPE_NONE;

	switch (m_HBAPort->signature) {
	case SATA_SIG_ATAPI:
		return PORT_TYPE_SATAPI;
	case SATA_SIG_SEMB:
		return PORT_TYPE_SEMB;
	case SATA_SIG_PM:
		return PORT_TYPE_PM;
	case SATA_SIG_ATA:
		return PORT_TYPE_SATA;
	default:
		Printf("Unknown port type: 0x%x\n", m_HBAPort->signature);
		return PORT_TYPE_NONE;
	}
}

void AHCIPort::HandleExternalInterrupt()
{
	uint32_t status = m_HBAPort->interrupt_status;

	//TODO

	m_HBAPort->interrupt_status = status;
}
