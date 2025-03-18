#include "KThread.h"
#include "kernel/Kernel.h"

//ASM interop
extern "C"
{
	constexpr size_t KTHREAD_CONTEXT_OFF = offsetof(KThread, Context);
	static_assert(KTHREAD_CONTEXT_OFF == 8, "SystemCall.asm invalid");

	constexpr size_t KTHREAD_USERTHREAD_OFF = offsetof(KThread, UserThread);
	static_assert(KTHREAD_USERTHREAD_OFF == 16, "SystemCall.asm invalid");
}

uint32_t KThread::LastId = 0;
KThread::KThread(const ThreadStart start, void* const arg) :
	Id(++LastId),

	Context(),
	UserThread(),

	Name(),

	m_start(start),
	m_arg(arg),
	m_stack(),

	m_state(ThreadState::Ready),
	m_waitStatus(WaitStatus::None),
	m_timeout(),
	m_signal()
{
	Context = new X64_CONTEXT();
}

KThread::~KThread()
{
	//Trace();
	delete Context;
}
void KThread::Init(void* const entry)
{
	size_t s1 = offsetof(KThread, Context);
	size_t s2 = offsetof(KThread, UserThread);
	//Printf("KThread::Init offsets: 0x%x 0x%x\n", s1, s2);

	m_stack = kernel.AllocateStack(KThread::StackPages);
	m_stackPointer = MakePointer<void*>(m_stack, (KThread::StackPages << PageShift) - kernel.GetHAL()->StackReserve());

	//Printf("KThread::InitContext Id %d Start: 0x%016x, End: 0x%016x\n", Id, m_stack, m_stackPointer);
	kernel.GetHAL()->InitContext(Context, entry, m_stackPointer);
	//Printf("Context saved: 0x%16x\r\n", Context->Rip);
}

void KThread::Run()
{
	m_start(m_arg);
}

void KThread::Display() const
{
	kernel.Printf("KThread\n");
	kernel.Printf("     Id: %x\n", Id);
	kernel.Printf("   Name: %s\n", Name.c_str());
	kernel.Printf("  Start: 0x%016x\n", m_start);
	kernel.Printf("    Arg: 0x%016x\n", m_arg);
	kernel.Printf("  State: %d\n", m_state);
	kernel.Printf("WStatus: %d\n", m_waitStatus);
	kernel.Printf("Timeout: %d\n", m_timeout);
	kernel.Printf("   User: 0x%016x\n", UserThread);

	if (m_signal)
		m_signal->Display();

	//Display context

#if defined(PLATFORM_X64)
	X64_CONTEXT* ctx = (X64_CONTEXT*)Context;
	Printf("  Rbp: 0x%016x Rsp: 0x%016x Rip: 0x%016x\n", ctx->Rbp, ctx->Rsp, ctx->Rip);
#endif
	if (UserThread != nullptr)
		UserThread->Display();
}