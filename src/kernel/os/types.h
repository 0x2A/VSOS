#pragma once

#include <stdint.h>
#include <type_traits>
#include <gfx\Types.h>
#include <string>

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
	KeyEvent_Type,
	MouseMoveEvent_Type,
	MouseButtonEvent_Type,
	PaintEvent_Type,
};

struct MessageHeader
{
	MessageType MessageType;
};

struct PaintEvent
{
	gfx::Rectangle Region;
};


/**
			 * @class KeyboardState
			 * @brief Holds the state of the keyboard
			 */
struct KeyboardState {
public:

	// Left and Right
	bool left_shift{ false };
	bool right_shift{ false };
	bool left_control{ false };
	bool right_control{ false };
	bool left_alt{ false };
	bool right_alt{ false };

	// Other Stuff
	bool caps_lock{ false };
	bool number_pad_lock{ false };
	bool scroll_lock{ false };
};


enum KeyCode : uint16_t {

	// Alphabet
	A = 'A', a = 'a',
	B = 'B', b = 'b',
	C = 'C', c = 'c',
	D = 'D', d = 'd',
	E = 'E', e = 'e',
	F = 'F', f = 'f',
	G = 'G', g = 'g',
	H = 'H', h = 'h',
	I = 'I', i = 'i',
	J = 'J', j = 'j',
	K = 'K', k = 'k',
	L = 'L', l = 'l',
	M = 'M', m = 'm',
	N = 'N', n = 'n',
	O = 'O', o = 'o',
	P = 'P', p = 'p',
	Q = 'Q', q = 'q',
	R = 'R', r = 'r',
	S = 'S', s = 's',
	T = 'T', t = 't',
	U = 'U', u = 'u',
	V = 'V', v = 'v',
	W = 'W', w = 'w',
	X = 'X', x = 'x',
	Y = 'Y', y = 'y',
	Z = 'Z', z = 'z',

	// Numbers
	zero = '0',
	one = '1',
	two = '2',
	three = '3',
	four = '4',
	five = '5',
	six = '6',
	seven = '7',
	eight = '8',
	nine = '9',

	// Symbols
	comma = ',',
	fullStop = '.',
	exclamationMark = '!',
	questionMark = '?',
	quotationMark = '\"',
	semicolon = ';',
	colon = ':',
	apostrophe = '\'',
	slantedApostrophe = '`',

	// Signs
	powerSign = '^',
	dollarSign = '$',
	percentSign = '%',
	andSign = '&',
	atSign = '@',

	// Special Characters
	underscore = '_',
	lineThing = '|',
	hash = '#',
	backslash = '\\',
	forwardSlash = '/',
	squigglyLine = '~',

	// Math Symbols
	plus = '+',
	minus = '-',
	equals = '=',
	multiply = '*',
	lessThan = '<',
	greaterThan = '>',

	// Brackets
	openBracket = '(',
	closeBracket = ')',
	openSquareBracket = '[',
	closeSquareBracket = ']',
	openCurlyBracket = '{',
	closeCurlyBracket = '}',

	// Writing
	space = ' ',
	tab = '\t',
	enter = '\n',
	backspace = '\b',

	/// OTHER CODES THAT ARE NOT CHARACTERS ///

	// Functions
	f1 = 1000,      //force it to start at 1000 so it doesn't conflict with ascii
	f2,
	f3,
	f4,
	f5,
	f6,
	f7,
	f8,
	f9,
	f10,
	f11,
	f12,

	// Top Row
	escape,
	printScreen,
	scrollLock,
	pauseBreak,

	// Arrow s
	upArrow,
	downArrow,
	leftArrow,
	rightArrow,

	// Keys Above Arrows
	insert,
	home,
	pageUp,
	deleteKey,
	end,
	pageDown,

	// Left Side
	capsLock,
	leftShift,
	leftControl,
	leftOS,                 //weird ass windows key or command on mac
	leftAlt,

	// Right Side
	rightAlt,
	functionKey,
	rightControl,
	rightShift,

	// Number Pad
	numberPadLock,
	numberPadForwardSlash,
	numberPadMultiply,
	numberPadMinus,
	numberPadPlus,
	numberPadEnter,
	numberPadZero,
	numberPadOne,
	numberPadTwo,
	numberPadThree,
	numberPadFour,
	numberPadFive,
	numberPadSix,
	numberPadSeven,
	numberPadEight,
	numberPadNine,
	numberPadFullStop,

	// Numper Pad (Non Number Lock)
	numberPadHome,
	numberPadPageDown,
	numberPadPageUp,
	numberPadEnd,
	numberPadInsert,
	numberPadUpArrow,
	numberPadDownArrow,
	numberPadLeftArrow,
	numberPadRightArrow,


	Invalid
};

/**
* @class KeyDownEvent
* @brief Event that is triggered when a key is pressed
*/
class KeyEvent {
public:
	KeyEvent(bool keyUp, KeyboardState KeyState, KeyCode keyCode) : key_code(keyCode), keyboard_state(KeyState), key_up(keyUp) {}
	KeyEvent() {}

	KeyCode key_code;
	KeyboardState keyboard_state;
	bool key_up; //true if key up event
};

struct MouseMoveEvent {
	int8_t x;
	int8_t y;
};

struct MouseButtonEvent
{
	uint8_t button;
	bool button_up;
};


struct Message
{
	MessageHeader Header;
	union
	{
		KeyEvent keyEvent;
		MouseMoveEvent mouseMoveEvent;
		MouseButtonEvent mouseButtonEvent;
		PaintEvent paintEvent;
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