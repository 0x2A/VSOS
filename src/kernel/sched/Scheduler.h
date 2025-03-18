#pragma once


#include <map>
#include <vector>
#include "KThread.h"
#include "kernel/hal/HAL.h"

class Scheduler : public TickEventHandler
{
public:
	static KThread* GetThread();
	Scheduler(HAL* hal);

	void Init();
	void Schedule();

	//Currently running threads
	KThread& GetCurrentThread();
	UserThread& GetCurrentUserThread();
	UserProcess& GetCurrentProcess();

	//General thread ops
	void AddReady(std::shared_ptr<KThread>& thread);
	void Sleep(const nano_t value);
	void KillThread(KThread& thread);
	void KillCurrentThread();
	void KillCurrentProcess();

	//Waits
	//NOTE(tsharpe): Signals were removed in favor of a simplied scheduler. This may or may not have been smart.
	WaitStatus ObjectWait(KSignalObject& object, const milli_t timeout = std::numeric_limits<milli_t>::max());
	

	void Display() const;

	bool Enabled;


	void onTimerTick(uint64_t totalTicks) override;

private:
	struct CpuContext
	{
		CpuContext() :
			SelfPointer(*this),
			Thread()
		{}

		const CpuContext& SelfPointer;
		KThread* Thread;
	};
	//Reference to clock
	HAL* m_HAL;

	//Cpu context
	CpuContext m_cpu;

	//Threads and current thread
	size_t m_threadIndex;
	std::vector<std::shared_ptr<KThread>> m_threads;

	::NO_COPY_OR_ASSIGN(Scheduler);
};