#pragma once
#include "os.System.h"
#include <map>
#include "Interrupt.h"
#include "kernel/devices/acpi/ACPI.h"

#define PLATFORM_X64  defined(__x86_64__) || defined(_M_X64)


#define TIMER0_INT		0x80

// empty struct for polymorphism
#pragma pack(push, 1)
struct INTERRUPT_FRAME
{
};
#pragma pack(pop)

class Device;
class HAL
{
public:
	HAL(ConfigTables* configTables);

	void initialize();

	void SetupPaging(paddr_t root);
	void Halt();
	void SaveContext(void* context);

	void HandleInterrupt(uint8_t vector, INTERRUPT_FRAME* frame);

	void RegisterInterrupt(uint8_t vector, InterruptContext context);
	void UnRegisterInterrupt(uint8_t vector);

	void InitDevices();

	uint32_t ReadPort(uint32_t port, uint32_t width);
	void WritePort(uint32_t port, uint32_t value, uint8_t width);

	void AddDevice(Device* device);
	void SendShutdown();

private:
	void EOI();

	//Interrupts
	std::map<uint8_t, InterruptContext>* m_interruptHandlers;
	ACPI m_ACPI;
};