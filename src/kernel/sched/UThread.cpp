#include "UThread.h"

void UserThread::Display()
{
	Printf("UserThread\n");
	Printf("     Id: %d\n", Id);
	Printf("  m_teb: 0x%016x\n", m_teb);
	Printf("Context:\n");
	Printf("  Rsp: 0x%016x \n", m_context.Rsp);
	Printf("  Rip: 0x%016x\n", m_context.Rip);
}
