#pragma once
#include "LocalAPIC.h"
#include "IOAPIC.h"

class HAL;
class APIC
{
public:
	APIC(HAL* hal);
	~APIC();

	void Init();

	void disable_pic();

	void EOI();

	LocalAPIC* GetLocalAPIC() { return &m_local_apic; } 

protected:
	LocalAPIC m_local_apic;
	IOAPIC m_io_apic;


	HAL* m_HAL;
};