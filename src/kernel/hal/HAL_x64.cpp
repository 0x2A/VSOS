
#include "HAL.h"

#if defined(PLATFORM_X64)
#include "x64\x64.h"
#include <intrin.h>

//defined in context.asm
extern "C" extern void _x64_save_context(void* context);
extern "C" extern void _x64_init_context(void* context, void* const entry, void* const stack);
extern "C" extern void _x64_load_context(void* context);
extern "C" extern void _x64_user_thread_start(void* context, void* teb);

void HAL::initialize()
{
	x64::SetupDescriptorTables();

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

#endif