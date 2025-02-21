
#include "HAL.h"

#if defined(PLATFORM_X64)
#include "x64\x64.h"
#include <intrin.h>
#include "x64\interrupt.h"
#include "kernel\kernel.h"

//defined in context.asm
extern "C" extern void _x64_save_context(void* context);
extern "C" extern void _x64_init_context(void* context, void* const entry, void* const stack);
extern "C" extern void _x64_load_context(void* context);
extern "C" extern void _x64_user_thread_start(void* context, void* teb);

void HAL::initialize()
{
	x64::SetupDescriptorTables();

	m_interruptHandlers = new std::map<uint8_t, InterruptContext>();
}

void HAL::SetupPaging(paddr_t root)
{
	if (__readcr3() != root)
	{
		__writecr3(root);
	}
}


void HAL::Wait()
{
	__halt();
}

void HAL::SaveContext(void* context)
{
	_x64_save_context(context);
}

void HAL::HandleInterrupt(uint8_t vector, INTERRUPT_FRAME* frame)
{
	X64_INTERRUPT_FRAME* x64Frame = (X64_INTERRUPT_FRAME*)frame;
	X64_INTERRUPT_VECTOR x64Vector = (X64_INTERRUPT_VECTOR)vector;

	if (x64Vector == X64_INTERRUPT_VECTOR::DoubleFault)
	{
		//KePauseSystem();
		kernel.Panic("Double Fault!\r\n");
		__halt();
	}

	//Break into kernel debugger if not user code
	//if (m_debugger.Enabled() && vector == X64_INTERRUPT_VECTOR::Breakpoint)
	{
		//m_debugger.DebuggerEvent(vector, frame);
		//return;
	}	

	const auto& it = m_interruptHandlers->find(vector);
	if (it != m_interruptHandlers->end())
	{
		InterruptContext ctx = it->second;
		ctx.Handler(ctx.Context);
		EOI();
		return;
	}

	//Show interrupt context
	Printf("ISR: 0x%x, Code: %x\n", x64Vector, x64Frame->ErrorCode);
	Printf("    RIP: 0x%016x\n", x64Frame->RIP);
	Printf("    RBP: 0x%016x\n", x64Frame->RBP);
	Printf("    RSP: 0x%016x\n", x64Frame->RSP);
	Printf("    RAX: 0x%016x\n", x64Frame->RAX);
	Printf("    RBX: 0x%016x\n", x64Frame->RBX);
	Printf("    RCX: 0x%016x\n", x64Frame->RCX);
	Printf("    RDX: 0x%016x\n", x64Frame->RDX);
	Printf("    CS: 0x%x, SS: 0x%x\n", x64Frame->CS, x64Frame->SS);


	switch (x64Vector)
	{
	case X64_INTERRUPT_VECTOR::PageFault:
		Printf("    CR2: 0x%16x\n", __readcr2());
		if (__readcr2() == 0)
			Printf("        Null pointer\n");
	}

	//Build context
	X64_CONTEXT context = {};
	context.Rip = x64Frame->RIP;
	context.Rsp = x64Frame->RSP;
	context.Rbp = x64Frame->RBP;

#if 0
	if (IsValidUserPointer((void*)frame->RIP))
	{
		//Interrupt is in userspace. Write Stack to stdout, write message, kill process.
		UserProcess& proc = m_scheduler.GetCurrentProcess();
		std::shared_ptr<UObject> uObject = proc.GetObject((Handle)StandardHandle::Output);
		if (uObject)
		{
			//Write stack
			AssertEqual(uObject->Type, UObjectType::Pipe);
			const UPipe* uPipe = (UPipe*)uObject.get();
			UserPipe& pipe = *uPipe->Pipe.get();

			//Write exception
			if (vector == X64_INTERRUPT_VECTOR::DivideError)
				pipe.Printf("Exception: Divide by zero\n");
			else if (vector == X64_INTERRUPT_VECTOR::Breakpoint)
				pipe.Printf("Exception: Breakpoint\n");
			else if (vector == X64_INTERRUPT_VECTOR::PageFault && __readcr2() == 0)
				pipe.Printf("Exception: Null pointer dereference\n");

			//Convert to unwind context
			CONTEXT ctx = { 0 };
			ctx.Rip = context.Rip;
			ctx.Rsp = context.Rsp;
			ctx.Rbp = context.Rbp;

			//Unwind stack, writing to process stdout
			StackWalk sw(&ctx);
			while (sw.HasNext())
			{
				PdbFunctionLookup lookup = {};
				Assert(IsValidUserPointer((void*)ctx.Rip));
				ResolveUserIP(ctx.Rip, lookup);

				Module* module = proc.GetModule(ctx.Rip);

				pipe.Printf("    %s::%s (%d)\n", module->Name, lookup.Name.c_str(), lookup.LineNumber);
				pipe.Printf("        IP: 0x%016x Base: 0x%016x, RVA: 0x%08x\n", ctx.Rip, lookup.Base, lookup.RVA);

				if (lookup.Base == nullptr)
					break;

				sw.Next((uintptr_t)lookup.Base);
			}
		}

		m_scheduler.KillCurrentProcess();
		return;
	}
	else
	{
#endif
		//this->ShowStack(&context);

		//Bugcheck
		Fatal("Unhandled exception");
		//}
}

	void HAL::RegisterInterrupt(uint8_t vector, InterruptContext context)
	{
		if (m_interruptHandlers->find(vector) != m_interruptHandlers->end())
			m_interruptHandlers->erase(vector);
		m_interruptHandlers->insert({ vector, context });
	}

	void HAL::EOI()
	{
		//TODO
	}



extern "C" void INTERRUPT_HANDLER(X64_INTERRUPT_VECTOR vector, X64_INTERRUPT_FRAME* frame)
{
	kernel.GetHAL()->HandleInterrupt((uint8_t)vector, frame);
}


#endif