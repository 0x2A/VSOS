#pragma once

#include <stdint.h>
#include <type_traits>
#include <gfx\Types.h>
#include <string>
#include <kernel\drivers\io\KeyboardDriver.h>

typedef void* Handle;
typedef void* HThread;
typedef void* HProcess;
typedef void* HSharedMemory;
typedef void* HRingBuffer;
typedef void* HWindow;
typedef void* HFile;
typedef void* HEvent;

struct ReadOnlyBuffer
{
	const void* Data;
	size_t Length;
};

struct Buffer
{
	void* Data;
	size_t Length;

	ReadOnlyBuffer GetRef()
	{
		return { Data, Length };
	}
};

enum GenericAccess
{
	Read = (1 << 0),
	Write = (1 << 1),
	ReadWrite = Read | Write

};

inline GenericAccess operator&(GenericAccess lhs, GenericAccess rhs)
{
	return static_cast<GenericAccess>(
		static_cast<std::underlying_type<GenericAccess>::type>(lhs) &
		static_cast<std::underlying_type<GenericAccess>::type>(rhs)
		);
}

enum class StandardHandle
{
	Input,
	Output,
	Error
};

typedef size_t(*ThreadStart)(void* parameter);

//TODO(tsharpe): Mapping PDB here 1:1 with a loaded module isn't the same as 1:1 with binary. Optimize later.
constexpr size_t MaxModuleName = 16;
struct Module
{
	void* ImageBase;
	void* PDB;
	char Name[MaxModuleName];
};

constexpr size_t MaxLoadedModules = 8;
struct ProcessEnvironmentBlock
{
	uint32_t ProcessId;
	void* ImageBase;
	int argc;
	char** argv;
	void* PDB;
	Module LoadedModules[MaxLoadedModules];
	size_t ModuleIndex;
};

struct ThreadEnvironmentBlock
{
	ThreadEnvironmentBlock* SelfPointer;
	ProcessEnvironmentBlock* PEB;
	ThreadStart ThreadStart;
	Handle Handle;
	void* Arg;
	uint32_t ThreadId;
	uint32_t Error;
};

enum class MessageType
{
	KeyEvent,
	MouseEvent,
	PaintEvent,
};

struct MessageHeader
{
	MessageType MessageType;
};

struct MouseButtonState
{
	uint8_t LeftPressed : 1;
	uint8_t RightPressed : 1;
};

struct MouseEvent
{
	MouseButtonState Buttons;
	uint16_t XPosition;
	uint16_t YPosition;
};

struct PaintEvent
{
	gfx::Rectangle Region;
};

struct Message
{
	MessageHeader Header;
	union
	{
		KeyEvent KeyEvent;
		MouseEvent MouseEvent;
		PaintEvent PaintEvent;
	};
};

typedef void (*MemberFunction)(void* arg);
struct CallContext
{
	MemberFunction Handler;
	void* Context;
};

enum class WaitStatus
{
	None,
	Signaled,
	Timeout,
	Abandoned,
	BrokenPipe,
};

class Pdb;
struct KeModule
{
	std::string Name;
	void* ImageBase;
	Pdb* Pdb;
};

typedef struct
{
	struct
	{
		uint32_t  Data1;
		uint16_t  Data2;
		uint16_t  Data3;
		uint8_t   Data4[8];
	};
} guid_t;

struct PdbFunctionLookup
{
	void* Base;
	uint32_t RVA;
	size_t LineNumber;
	std::string Name;
};