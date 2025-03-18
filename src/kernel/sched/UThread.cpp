#include "UThread.h"

#if defined(PLATFORM_X64)
#include "kernel/hal/x64/x64.h"
#endif

void UserThread::Display()
{
	Printf("UserThread\n");
	Printf("     Id: %d\n", Id);
	Printf("  m_teb: 0x%016x\n", m_teb);
	Printf("Context:\n");
#if defined(PLATFORM_X64)
	X64_CONTEXT* ctx = static_cast<X64_CONTEXT*>(&m_context);
	Printf("  Rsp: 0x%016x \n", ctx->Rsp);
	Printf("  Rip: 0x%016x\n", ctx->Rip);
#endif
}
