#include "StackWalk.h"

StackWalk::StackWalk(PCONTEXT context, PKNONVOLATILE_CONTEXT_POINTERS contextPointers /*= nullptr*/) :
	m_context(context),
	m_contextPointers(contextPointers)
{

}

bool StackWalk::HasNext()
{
	return m_context->Rip != NULL;
}

//OOPStackUnwinderAMD64::Unwind
//https://github.com/dotnet/runtime/blob/main/src/coreclr/unwinder/amd64/unwinder_amd64.cpp
PCONTEXT StackWalk::Next(const uintptr_t imageBase)
{
	//TODO
	/*const PRUNTIME_FUNCTION functionEntry = RuntimeSupport::LookupFunctionEntry(m_context->Rip, imageBase, nullptr);
	if (!functionEntry)
	{
		//Pop IP off stack
		Printf("Got from stack\n");
		m_context->Rip = *(DWORD64*)m_context->Rsp;
		m_context->Rsp += sizeof(DWORD64);
		return m_context;
	}
	RuntimeSupport::VirtualUnwind(UNW_FLAG_NHANDLER, imageBase, m_context->Rip, functionEntry, m_context, m_contextPointers);*/
	return m_context;
}

