#include "AHCI.h"
#include "AHCIPort.h"
#include "kernel/Kernel.h"

AHCIDriver::AHCIDriver(PCIDevice* device)
	: Driver(device)
{
	device->Type = DeviceType::Harddrive;
	memset(ahci_ports, 0, 32);
}

DriverResult AHCIDriver::Activate()
{
	PCIDevice* dev = static_cast<PCIDevice*>(m_device);

	// Enable BUS Mastering
	dev->writeBus(0x04, 0x0006);

	// Disable interrupts for now
	abar->global_host_control &= ~(1<<1);

#if 0
	// Store usefull variables before reset
	uint32_t capsBeforeReset = abar->host_capabilities & (CAP_SMPS | CAP_SSS | CAP_SPM | CAP_EMS | CAP_SXS);
	uint32_t piBeforeReset = abar->ports_implemented;

	// First set AHCI Enable bit
	abar->global_host_control |= (1 << 31);

	// Then set the HBA Reset bit
	abar->global_host_control |= (1 << 0);

	// Wait for reset completion
	int spin = 0;
	while ((abar->global_host_control & (1 << 0)) && spin < 1000000) {
		spin++;
	};
	if (spin == 1000000) {
		Printf("AHCI: Port is hung\n");
		return DriverResult::Failed;
	}

	// Set AHCI Enable bit again
	abar->global_host_control |= (1 << 31);

	// Write back stored variables
	abar->host_capabilities |= capsBeforeReset;
	abar->ports_implemented = piBeforeReset;
#endif

	probe_ports(abar);

	//clear interrupts
	abar->interrupt_status = abar->interrupt_status;

	//enable interrupts
	abar->global_host_control |= (1<<1);

	

	//Enable all found ports
	for (int i = 0; i < port_count; i++) {
		if (ahci_ports[i])
		{ 
			if(!ahci_ports[i]->StartupPort())
			{
				delete ahci_ports[i];
				ahci_ports[i] = 0;
			}	
		}
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

	kernel.GetHAL()->RegisterInterrupt( 0x20 + dev->GetDeviceDescriptor().interruptLine, { &AHCIDriver::HandleInterrupt, this });

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



AHCIPort* AHCIDriver::GetPort(uint8_t port_no)
{
	if ((uint64_t)abar == 0) return 0;
	Assert(port_no < port_count);
	return ahci_ports[port_no];
}

uint8_t AHCIDriver::get_port_count()
{
	if ((uint64_t)abar == 0) return 0;
	return port_count;
}



char AHCIDriver::ReadSector(uint16_t drive, uint64_t sector, uint8_t* buffer) const
{
	Assert(drive < 32);
	if (ahci_ports[drive])
	{
		uint32_t res = ahci_ports[drive]->TransferData(true, sector, 1);
		if (res > 0)
		{
			memcpy(buffer, ahci_ports[drive]->GetBuffer(), res);
			return 0;
		}
		return 1;
	}
}

char AHCIDriver::WriteSector(uint16_t drive, uint64_t sector, uint8_t* buffer) const
{
	Assert(drive < 32);
	if (ahci_ports[drive])
	{
		memcpy(ahci_ports[drive]->GetBuffer(), buffer, 512);

		uint8_t res = ahci_ports[drive]->TransferData(false, sector, 1);
		if (res > 0)
		{
			return 0;
		}
		return 1;
	}
	
}

bool AHCIDriver::EjectDrive(uint8_t drive)
{
	if(ahci_ports[drive])
		return ahci_ports[drive]->Eject();
}

uint32_t AHCIDriver::OnInterrupt()
{
	uint32_t interruptPending = abar->interrupt_status & abar->ports_implemented;

	if(interruptPending == 0) return 0;

	for (int i = 0; i < port_count; i++)
	{
		if (interruptPending & (1 << i))
		{
			if(ahci_ports[i])
				ahci_ports[i]->HandleExternalInterrupt();
		}
	}

	// clear pending interrupts
	abar->interrupt_status = interruptPending;
	Printf("AHCI Interrupt\r\n");
	return 0;
}

uint32_t AHCIDriver::HandleInterrupt(void* context)
{
	AHCIDriver* self = static_cast<AHCIDriver*>(context);
	return self->OnInterrupt();
}

void AHCIDriver::probe_ports(hba_memory* abar)
{
	PCIDevice* dev = static_cast<PCIDevice*>(m_device);
	Printf("AHCI Probing ports on device %s\r\n", dev->Path);
	int count = 1 + ((abar->host_capabilities) & 0x1f);
	uint32_t ports_implemented = abar->ports_implemented;

	for (int i = 0; i < count; i++) {
		if (ports_implemented & (1 << i)) {
			//port_type pt = check_port_type(&abar->ports[i]);
			//if (pt == PORT_TYPE_SATA || pt == PORT_TYPE_SATAPI) {
				ahci_ports[port_count] = new AHCIPort(this, &abar->ports[i], port_count);
				if(!ahci_ports[port_count]->ConfigurePort())
				{
					delete ahci_ports[port_count];
					ahci_ports[port_count] = 0;
				}
				else
					port_count++;
				
			//}
		}
	}
	//Printf("Found %d ports\r\n", port_count);
}



