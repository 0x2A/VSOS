#include "AHCI.h"
#include "kernel/Kernel.h"

AHCIDriver::AHCIDriver(PCIDevice* device)
	: Driver(device)
{
	device->Type = DeviceType::Harddrive;	
}

DriverResult AHCIDriver::Activate()
{
	probe_ports(abar);
	PageTables pt;
	pt.OpenCurrent();
	auto pageCount = SizeToPages(4096);
	for (int i = 0; i < port_count; i++) {
		ahci_port* port = &ahci_ports[i];
		configure_port(port);
		port->bufferPhysicalAddr = kernel.AllocatePhysical(pageCount);
		port->buffer = (uint8_t*)kernel.DriverMapPages(port->bufferPhysicalAddr, pageCount);

		//mprotect_current(TO_KERNEL_MAP(port->buffer), 4096, PAGE_WRITE_BIT | PAGE_NX_BIT);
		memset(port->buffer, 0, 4096);
	}

	return DriverResult::Success;
}

DriverResult AHCIDriver::Deactivate()
{
	return DriverResult::NotImplemented;
}

DriverResult AHCIDriver::Initialize()
{
	PCIDevice* dev = static_cast<PCIDevice*>(m_device);
	Assert(dev);
	auto descr = dev->GetDeviceDescriptor();	
	Assert(descr.has_port_base);
	abar = (hba_memory*)kernel.MapPhysicalMemory((uint32_t)descr.bars[5].address, sizeof(hba_memory));
	Assert(abar);
	return DriverResult::Success;
}

DriverResult AHCIDriver::Reset()
{
	return DriverResult::NotImplemented;
}

std::string AHCIDriver::get_vendor_name()
{
	PCIDevice* dev = static_cast<PCIDevice*>(m_device);
	switch (dev->GetDeviceDescriptor().vendor_id)
	{
		case PCI_VEN_ID_INTEL:	//INTEL
			return "Intel Corporation";
		break;
	}
	return "Unknown";
}

std::string AHCIDriver::get_device_name()
{
	PCIDevice* dev = static_cast<PCIDevice*>(m_device);
	auto descr = dev->GetDeviceDescriptor();
	switch (descr.vendor_id)
	{
	case PCI_VEN_ID_INTEL:	//INTEL
		switch (descr.device_id)
		{
			case PCI_DEVICE_ID_INTEL_82801IR:
				return "82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode]";
			break;
		}
		break;
	}
	return "AHCI compatible controller";
}

uint8_t AHCIDriver::read_port(uint8_t port_no, uint64_t sector, uint32_t sector_count)
{
	if ((uint64_t)abar == 0) return 0;
	
	Assert(port_no < port_count);
	struct ahci_port* port = &ahci_ports[port_no];

	uint32_t sector_low = (uint32_t)sector;
	uint32_t sector_high = (uint32_t)(sector >> 32);

	//Write-clear all bits in interrupt status
	port->hba_port->interrupt_status = 0xFFFFFFFF;

	int spin = 0;
	int slot = (int)find_cmd_slot(port);
	if (slot == -1) {
		Printf("No free command slots\n");
		return 0;
	}

	//struct hba_command_header* command_header = (struct hba_command_header*)(uint64_t)(port->hba_port->command_list_base | (((uint64_t)port->hba_port->command_list_base_upper) << 32));
	hba_command_header command_header = port->command_header[slot];
	//command_header += slot;
	command_header.command_fis_length = sizeof(struct hba_command_fis) / sizeof(uint32_t);
	command_header.write = 0;
	command_header.prdt_length = (uint16_t)((sector_count - 1) >> 4) + 1;

	//struct hba_command_table* command_table = (struct hba_command_table*)(uint64_t)(command_header->command_table_base_address | ((uint64_t)command_header->command_table_base_address_upper << 32));
	hba_command_table* command_table = port->command_table[slot];
	memset(command_table, 0, sizeof(struct hba_command_table) + (command_header.prdt_length - 1) * sizeof(struct hba_prdt_entry));
	
	paddr_t buffer = port->bufferPhysicalAddr;

	int i;
	for (i = 0; i < command_header.prdt_length - 1; i++) {
		command_table->prdt_entry[i].data_base_address = (uint32_t)buffer;
		command_table->prdt_entry[i].data_base_address_upper = (uint32_t)(buffer >> 32);
		command_table->prdt_entry[i].byte_count = 8 * 1024 - 1;
		command_table->prdt_entry[i].interrupt_on_completion = 1;
		buffer = (buffer + 0x1000);
		sector_count -= 16;
	}

	command_table->prdt_entry[i].data_base_address = (uint32_t)(uint64_t)buffer;
	command_table->prdt_entry[i].data_base_address_upper = (uint32_t)((uint64_t)buffer >> 32);
	command_table->prdt_entry[i].byte_count = (sector_count << 9) - 1;
	command_table->prdt_entry[i].interrupt_on_completion = 1;

	struct hba_command_fis* command_fis = (struct hba_command_fis*)command_table->command_fis;

	command_fis->fis_type = FIS_TYPE_REG_H2D;
	command_fis->command_control = 1;
	command_fis->command = ATA_CMD_READ_DMA_EX;

	command_fis->lba0 = (uint8_t)sector_low;
	command_fis->lba1 = (uint8_t)(sector_low >> 8);
	command_fis->lba2 = (uint8_t)(sector_low >> 16);

	command_fis->device_register = 1 << 6;

	command_fis->lba3 = (uint8_t)(sector_low >> 24);
	command_fis->lba4 = (uint8_t)(sector_high);
	command_fis->lba5 = (uint8_t)(sector_high >> 8);

	command_fis->count_low = sector_count & 0xFF;
	command_fis->count_high = (sector_count >> 8);

	while (port->hba_port->task_file_data & (ATA_DEV_BUSY | ATA_DEV_DRQ) && spin < 1000000) {
		spin++;
	};
	if (spin == 1000000) {
		Printf(__FUNCTION__": Port is hung\n");
		return 0;
	}

	port->hba_port->command_issue = (1 << slot);

	while (1) {
		if ((port->hba_port->command_issue & (1 << slot)) == 0) break;
		if (port->hba_port->interrupt_status & HBA_PxIS_TFES) {
			return 0;
		}
	}

	if (port->hba_port->interrupt_status & HBA_PxIS_TFES) {
		return 0;
	}

	return 1;
}

uint8_t* AHCIDriver::get_buffer(uint8_t port_no)
{
	if ((uint64_t)abar == 0) return 0;

	return ahci_ports[port_no].buffer;
}

uint8_t AHCIDriver::get_port_count()
{
	if ((uint64_t)abar == 0) return 0;
	return port_count;
}

uint8_t AHCIDriver::identify(uint8_t port_no)
{
	if ((uint64_t)abar == 0) return 0;
	struct ahci_port* port = &ahci_ports[port_no];
	port->hba_port->interrupt_status = (uint32_t)-1;

	int spin = 0;
	int slot = (int)find_cmd_slot(port);
	if (slot == -1) {
		Printf("No free command slots\n");
		return 0;
	}
	
	//struct hba_command_header* command_header = (struct hba_command_header*)kernel.PhysicalToVirtual((uint64_t)(port->hba_port->command_list_base | (((uint64_t)port->hba_port->command_list_base_upper) << 32)));
	hba_command_header command_header = port->command_header[slot];
	//command_header += slot;
	command_header.command_fis_length = 5;
	command_header.write = 0;
	command_header.prdt_length = 1;
	command_header.prefetchable = 1;
	command_header.clear_busy_on_ok = 1;

	paddr_t buffer = port->bufferPhysicalAddr;

	//struct hba_command_table* command_table = (struct hba_command_table*)kernel.PhysicalToVirtual( (uint64_t)(command_header->command_table_base_address | ((uint64_t)command_header->command_table_base_address_upper << 32)));
	hba_command_table* command_table = port->command_table[slot];
	memset(command_table, 0, sizeof(struct hba_command_table) + (command_header.prdt_length - 1) * sizeof(struct hba_prdt_entry));
	command_table->prdt_entry[0].data_base_address_upper = (uint32_t)((uint64_t)buffer >> 32);
	command_table->prdt_entry[0].data_base_address = (uint32_t)((uint64_t)buffer);
	command_table->prdt_entry[0].byte_count = 512 - 1;
	command_table->prdt_entry[0].interrupt_on_completion = 1;

	struct hba_command_fis* command_fis = (struct hba_command_fis*) command_table->command_fis;
	memset(command_fis, 0, sizeof(struct hba_command_fis));
	command_fis->fis_type = FIS_TYPE_REG_H2D;
	command_fis->command_control = 1;
	command_fis->command = ATA_CMD_IDENTIFY;

	while (port->hba_port->task_file_data & (ATA_DEV_BUSY | ATA_DEV_DRQ) && spin < 1000000) {
		spin++;
	};
	if (spin == 1000000) {
		Printf("Port is hung\n");
		return 0;
	}

	port->hba_port->command_issue = 1;

	while (1) {
		if ((port->hba_port->command_issue & (1 << slot)) == 0) break;
		if (port->hba_port->interrupt_status & HBA_PxIS_TFES) {
			return 0;
		}
	}

	if (port->hba_port->interrupt_status & HBA_PxIS_TFES) {
		return 0;
	}

	return 1;
}

port_type AHCIDriver::check_port_type(hba_port* port)
{
	uint32_t stata_status = port->sata_status;
	uint8_t interface_power_management = (stata_status >> 8) & 0x7;
	uint8_t device_detection = stata_status & 0x7;

	if (device_detection != 0x03) return PORT_TYPE_NONE;
	if (interface_power_management != 0x01) return PORT_TYPE_NONE;

	switch (port->signature) {
	case SATA_SIG_ATAPI:
		return PORT_TYPE_SATAPI;
	case SATA_SIG_SEMB:
		return PORT_TYPE_SEMB;
	case SATA_SIG_PM:
		return PORT_TYPE_PM;
	case SATA_SIG_ATA:
		return PORT_TYPE_SATA;
	default:
		Printf("Unknown port type: 0x%x\n", port->signature);
		return PORT_TYPE_NONE;
	}
}

void AHCIDriver::probe_ports(hba_memory* abar)
{
	Printf("AHCI Probing ports\r\n");
	uint32_t ports_implemented = abar->ports_implemented;

	for (int i = 0; i < 32; i++) {
		if (ports_implemented & (1 << i)) {
			port_type pt = check_port_type(&abar->ports[i]);
			if (pt == PORT_TYPE_SATA || pt == PORT_TYPE_SATAPI) {
				ahci_ports[port_count].port_type = pt;
				ahci_ports[port_count].hba_port = &abar->ports[i];
				ahci_ports[port_count].port_number = port_count;
				Printf("Port %d is SATA/SATAPI\r\n", port_count);
				port_count++;
				
			}
		}
	}
	Printf("Found %d ports\r\n", port_count);
}

void AHCIDriver::configure_port(ahci_port* port)
{
	port_stop_command(port);

	auto pageCount = SizeToPages(1024);
	paddr_t new_base_phys = kernel.AllocatePhysical(pageCount);
	void* new_base = (uint8_t*)kernel.DriverMapPages(new_base_phys, pageCount);
	
	port->hba_port->command_list_base = (uint32_t)(uint64_t)new_base_phys;
	port->hba_port->command_list_base_upper = (uint32_t)((uint64_t)new_base_phys >> 32);
	memset(new_base, 0, 1024);

	pageCount = SizeToPages(256);
	paddr_t new_fis_base_phys = kernel.AllocatePhysical(pageCount);
	void* new_fis_base = kernel.DriverMapPages(new_fis_base_phys, pageCount);
	
	port->hba_port->fis_base_address = (uint32_t)(uint64_t)new_fis_base_phys;
	port->hba_port->fis_base_address_upper = (uint32_t)((uint64_t)new_fis_base_phys >> 32);
	memset(new_fis_base, 0, 256);

	port->command_header = (struct hba_command_header*)(new_base);
	
	for (int i = 0; i < 32; i++) {
		port->command_header[i].prdt_length = 8;

		paddr_t cmd_table_address_phys = kernel.AllocatePhysical(pageCount);
		void* cmd_table_address = kernel.DriverMapPages(cmd_table_address_phys, pageCount); 
		port->command_table[i] = (hba_command_table*)cmd_table_address;

		uint64_t address = (uint64_t)cmd_table_address_phys + ((uint64_t)(i) << 8);
		port->command_header[i].command_table_base_address = (uint32_t)address;
		port->command_header[i].command_table_base_address_upper = (uint32_t)(address >> 32);
		memset(cmd_table_address, 0, 256);

	}

	port_start_command(port);

}

void AHCIDriver::port_start_command(struct ahci_port* port)
{
	while (port->hba_port->command_status & HBA_PxCMD_CR) __nop();
	port->hba_port->command_status |= HBA_PxCMD_FRE;
	port->hba_port->command_status |= HBA_PxCMD_ST;
}

void AHCIDriver::port_stop_command(ahci_port* port)
{
	port->hba_port->command_status &= ~HBA_PxCMD_ST;
	port->hba_port->command_status &= ~HBA_PxCMD_FRE;

	// Wait until FR (bit14), CR (bit15) are cleared
	while (1) {
		if (port->hba_port->command_status & HBA_PxCMD_FR) continue;
		if (port->hba_port->command_status & HBA_PxCMD_CR) continue;
		break;
	}
}

int8_t AHCIDriver::find_cmd_slot(struct ahci_port* port)
{
	uint32_t slots = (port->hba_port->sata_active | port->hba_port->command_issue);
	int cmd_slots = (abar->host_capabilities & 0x0f00) >> 8;
	for (int i = 0; i < cmd_slots; i++) {
		if ((slots & 1) == 0) {
			return i;
		}
		slots >>= 1;
	}

	Printf("Cannot find free command list entry\n");
	return -1;
}
