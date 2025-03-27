#pragma once
#include "AHCI.h"


class AHCIPort
{
	friend class AHCIDriver;
public:
	AHCIPort(AHCIDriver* driver, hba_port* port, uint8_t portNumber);

	bool ConfigurePort();
	bool StartupPort();

	port_type GetType() { return m_Type; }

	bool Enable();
	bool Disable();

	// Send Identify command to port
	bool Identify();

	// Read or write sectors to device
	uint32_t TransferData(bool dirIn, uint64_t sector, uint32_t count = 1);

	// Eject drive if it is a ATAPI device
	bool Eject();

	uint8_t* GetBuffer() { return buffer; }

private:
	// Find a CMD slot which is ready for commands
	int8_t FindFreeCMDSlot();
	port_type check_port_type();

	AHCIDriver* m_Driver;
	
	volatile hba_port* m_HBAPort;
	port_type m_Type;
	uint8_t* buffer;
	uint8_t port_number;
	uint64_t bufferPhysicalAddr;
	hba_command_header* command_header; //used to save virtual address of command headers
	hba_command_table* command_table[32];
	bool isATATPI = false;
	bool useLBA48 = true;
public:
	void HandleExternalInterrupt();
};