#include "cpu.h"
#include "kernel/hal/devices/apic/LocalAPIC.h"

CPU::CPU(LocalAPIC* apic)
: m_APIC(apic)
{

}

uint8_t CPU::cpunum()
{
	return m_APIC->id();
}

