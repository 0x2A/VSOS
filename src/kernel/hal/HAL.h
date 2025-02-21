#pragma once
#include "os.System.h"
#include <map>
#include "Interrupt.h"

#define PLATFORM_X64  defined(__x86_64__) || defined(_M_X64)


#define TIMER0_INT		0x80

// empty struct for polymorphism
#pragma pack(push, 1)
struct INTERRUPT_FRAME
{
};
#pragma pack(pop)

class HAL
{
public:
	void initialize();

	void SetupPaging(paddr_t root);
	void Wait();
	void SaveContext(void* context);

	void HandleInterrupt(uint8_t vector, INTERRUPT_FRAME* frame);

	void RegisterInterrupt(uint8_t vector, InterruptContext context);

private:
	void EOI();

	//Interrupts
	std::map<uint8_t, InterruptContext>* m_interruptHandlers;
};