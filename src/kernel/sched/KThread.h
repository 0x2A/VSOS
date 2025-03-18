#pragma once

#include <cstdint>
#include <kernel\os\types.h>
#include <kernel\hal\x64\x64.h>
#include "UThread.h"
#include <kernel\os\Time.h>

enum class ThreadState
{
	Ready,
	Running,
	SignalWait,
	Sleeping,
	Terminated
};

class KThread
{
	friend class Scheduler;
public:
	KThread(const ThreadStart start, void* const arg);
	~KThread();

	void Init(void* const entry);
	void Run();

	void Display() const;

	const uint32_t Id;

	CPU_CONTEXT* Context;
	UserThread* UserThread;
	std::string Name;

private:
	static uint32_t LastId;
	static constexpr size_t StackPages = 8;

	const ThreadStart m_start;
	void* const m_arg;
	void* m_stack;
	void* m_stackPointer;

	//Scheduler
	ThreadState m_state;
	WaitStatus m_waitStatus;
	nano_t m_timeout;
	KSignalObject* m_signal;

	::NO_COPY_OR_ASSIGN(KThread);
};