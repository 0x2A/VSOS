#pragma once
#include "os.System.h"
#include <map>
#include "Interrupt.h"
#include "devices/acpi/ACPI.h"
#include "devices/apic/APIC.h"
#include "devices\CPU.h"
#include "devices\SMBios.h"
#include "devices\pci\PCIBus.h"
#include "kernel\drivers\platform\Clock.h"
#include <kernel\drivers\DriverManager.h>
#include <kernel\drivers\video\VideoDevice.h>

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

	uint64_t ReadMSR(uint32_t port);

	ACPI* GetACPI() { return &m_ACPI; }

	void RegisterCPU(uint8_t id);

	uint8_t CurrentCPU();
	uint8_t CPUCount() { return m_NumCPUs; }

	void RegisterVideoDevice(VideoDevice* dev);
	VideoDevice* GetVideoDevice();

	void ActivateDrivers();

	void SetInterruptRedirect(const interrupt_redirect_t* redirectStruct);

	Clock* GetClock() { return &m_Clock; }

	PCIBus* GetPCIController() { return &m_PCI; }

	DriverManager* driverManager;

private:
	void EOI();

	//Interrupts
	std::map<uint8_t, InterruptContext>* m_interruptHandlers;
	ACPI m_ACPI;
	APIC m_APIC;

	CPU* m_CPUS[MAX_CPUS];
	uint8_t m_NumCPUs;
	
	bool m_HasSMBIOS;
	SMBios m_SMBios;

	PCIBus m_PCI;

	ConfigTables* m_ConfigTables;

	//assume we always have a RTC (TODO: add check)
	Clock m_Clock;
	VideoDevice* m_VideoDevice;
};