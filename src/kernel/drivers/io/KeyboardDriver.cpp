#include "KeyboardDriver.h"
#include "kernel/hal/HAL.h"
#include "kernel/hal/devices/io/GenericKeyboard.h"

#define KEYBOARD_DATA_PORT		0x60
#define KEYBOARD_COMMAND_PORT	0x64
#define INTERRUPT_VECTOR		0x21

enum PS2_Commands : uint8_t
{
	ReadByte0	= 0x20,
	EnableFirstPort = 0xAE,
	DisableFirstPort = 0xAD,
	EnableScanning = 0xF4

};

KeyboardDriver::KeyboardDriver(HAL* hal)
 : Driver(new GenericKeyboard()), m_HAL(hal), m_Interpreter(nullptr)
{
}

DriverResult KeyboardDriver::Activate()
{
	

	// Wait for user to stop pressing key (this is for the start-up key eg.. hold 'F12' for boot menu or hold 'del' for bios )
	while (m_HAL->ReadPort(KEYBOARD_COMMAND_PORT & 0x1, 8))
		m_HAL->ReadPort(KEYBOARD_DATA_PORT, 8);

	// Enable keyboard interrupts
	m_HAL->WritePort(KEYBOARD_COMMAND_PORT, EnableFirstPort, 8);

	// Get the current state of the keyboard
	m_HAL->WritePort(KEYBOARD_COMMAND_PORT, 0x20, 8);
	uint8_t status = (m_HAL->ReadPort(KEYBOARD_DATA_PORT, 8) | 1) & ~0x10;

	// Reset the keyboard
	m_HAL->WritePort(KEYBOARD_COMMAND_PORT, 0x60, 8);
	m_HAL->WritePort(KEYBOARD_DATA_PORT, status, 8);

	// activate the keyboard
	//m_HAL->WritePort(KEYBOARD_DATA_PORT, EnableScanning, 8);


	interrupt_redirect_t redirect;
	redirect.type = 0x1;
	redirect.index = 1;	//PS/2 input is on pin 1
	redirect.interrupt = INTERRUPT_VECTOR;
	redirect.destination = m_HAL->CurrentCPU();
	redirect.flags = 0x0;
	redirect.mask = false;

	m_HAL->SetInterruptRedirect(&redirect);
	

	m_HAL->RegisterInterrupt(INTERRUPT_VECTOR, { KeyboardDriver::HandleInterrupt, this });
	Printf("Keyboard setup\r\n");

	return DriverResult::Success;
}

DriverResult KeyboardDriver::Deactivate()
{
	m_HAL->UnRegisterInterrupt(INTERRUPT_VECTOR);
	return DriverResult::Success;
}

DriverResult KeyboardDriver::Initialize()
{
	return DriverResult::NotImplemented;
}

DriverResult KeyboardDriver::Reset()
{
	return DriverResult::NotImplemented;
}

std::string KeyboardDriver::get_vendor_name()
{
	return "Unknown";
}

std::string KeyboardDriver::get_device_name()
{
	return "Generic Keyboard";
}

void KeyboardDriver::AddKeyEventHandler(KeyboardEventHandler* handler)
{
	m_KeyEventHandler.push_back(handler);
}

uint32_t KeyboardDriver::HandleInterrupt(void* arg)
{
	//Printf("Keyboard interrupt\r\n");
	KeyboardDriver* driver = (KeyboardDriver*)arg;

	// read the scancode from the keyboard
	uint8_t key = driver->m_HAL->ReadPort(KEYBOARD_DATA_PORT, 8);

	driver->InterpretKey(key);

	return 0;
}

void KeyboardDriver::SetKeyboardInterpreter(KeyboardInterpreter* interpreter)
{
	m_Interpreter = interpreter;
}

void KeyboardDriver::InterpretKey(uint8_t scan_code)
{
	if (m_Interpreter)
	{
		KeyEvent keyEvent;
		bool result = m_Interpreter->InterpretKey(scan_code, keyEvent);

		if(result)
		{
			for(auto handler : m_KeyEventHandler)
				handler->OnKeyEvent(keyEvent);

			//if(!keyEvent.key_up && keyEvent.key_code < f1) Printf("Keycode: %c\r\n", (uint16_t)keyEvent.key_code);
		}
	}
	else
	{	
		Printf("WARN: No keyboard interpreter registered. Keycode: 0x%x\r\n", scan_code);
	}
}

bool DefaultKeyboardInterpreter::InterpretKey(uint8_t scan_code, KeyEvent& keyEvent)
{
	// 0 is a regular key, 1 is an extended code, 2 is an extended code with e1CodeBuffer
	int keyType = 0;

	// Check if the key was released
	bool released = (scan_code & 0x80) && (m_current_extended_code_1 || (scan_code != 0xe1)) && (m_next_is_extended_code_0 || (scan_code != 0xe0));

	// Clear the released bit
	if (released)
		scan_code &= ~0x80;

	// Set the e0Code flag to true
	if (scan_code == 0xe0)
	{
		m_next_is_extended_code_0 = true;
		return false;
	}

	// If e0Code is true, set keyType to 1 and reset e0Code
	if (m_next_is_extended_code_0)
	{
		keyType = 1;
		m_next_is_extended_code_0 = false;

		// Check if the scan_code represents a shift key and return (fake shift)
		if ((KeyCodeEN_US)scan_code == KeyCodeEN_US::leftShift || (KeyCodeEN_US)scan_code == KeyCodeEN_US::rightShift)
			return false;
	}

	// If the scan_code is 0xe1, set the e1Code flag to 1 and return
	if (scan_code == 0xe1)
	{
		m_current_extended_code_1 = 1;
		return false;
	}

	// If e1Code is 1, set e1Code to 2, store the scan_code in e1CodeBuffer, and return
	if (m_current_extended_code_1 == 1)
	{
		m_current_extended_code_1 = 2;
		m_extended_code_1_buffer = scan_code;
		return false;
	}

	// If e1Code is 2, set keyType to 2, reset e1Code, and update e1CodeBuffer
	if (m_current_extended_code_1 == 2)
	{
		keyType = 2;
		m_current_extended_code_1 = 0;
		m_extended_code_1_buffer |= (((uint16_t)scan_code) << 8);
	}

	bool is_shifting = this->m_keyboard_state.left_shift || this->m_keyboard_state.right_shift;
	bool should_be_upper_case = is_shifting != this->m_keyboard_state.caps_lock;


	// TODO: Probabbly a better way to do this (investigate when adding more keyboard layouts)
	if (keyType == 0)
		switch ((KeyCodeEN_US)scan_code) {

			// First row
		case KeyCodeEN_US::escape:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::escape);
			break;

		case KeyCodeEN_US::f1:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f1);
			break;

		case KeyCodeEN_US::f2:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f2);
			break;

		case KeyCodeEN_US::f3:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f3);
			break;

		case KeyCodeEN_US::f4:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f4);
			break;

		case KeyCodeEN_US::f5:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f5);
			break;

		case KeyCodeEN_US::f6:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f6);
			break;

		case KeyCodeEN_US::f7:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f7);
			break;

		case KeyCodeEN_US::f8:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f8);
			break;

		case KeyCodeEN_US::f9:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f9);
			break;

		case KeyCodeEN_US::f10:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f10);
			break;

		case KeyCodeEN_US::f11:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f11);
			break;

		case KeyCodeEN_US::f12:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::f12);
			break;

		case KeyCodeEN_US::printScreen:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock
				? KeyCode::numberPadMultiply : KeyCode::printScreen);
			break;

		case KeyCodeEN_US::scrollLock:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::scrollLock);
			break;

			// Second row
		case KeyCodeEN_US::squigglyLine:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::squigglyLine : KeyCode::slantedApostrophe);
			break;

		case KeyCodeEN_US::one:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::exclamationMark : KeyCode::one);
			break;

		case KeyCodeEN_US::two:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::atSign : KeyCode::two);
			break;

		case KeyCodeEN_US::three:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::hash : KeyCode::three);
			break;

		case KeyCodeEN_US::four:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::dollarSign : KeyCode::four);
			break;

		case KeyCodeEN_US::five:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::percentSign : KeyCode::five);
			break;

		case KeyCodeEN_US::six:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::powerSign : KeyCode::six);
			break;

		case KeyCodeEN_US::seven:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::andSign : KeyCode::seven);
			break;

		case KeyCodeEN_US::eight:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::multiply : KeyCode::eight);
			break;

		case KeyCodeEN_US::nine:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::openBracket : KeyCode::nine);
			break;

		case KeyCodeEN_US::zero:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::closeBracket : KeyCode::zero);
			break;

		case KeyCodeEN_US::minus:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::underscore : KeyCode::minus);
			break;

		case KeyCodeEN_US::equals:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::plus : KeyCode::equals);
			break;

		case KeyCodeEN_US::backspace:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::backspace);
			break;

		case KeyCodeEN_US::insert:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock ? KeyCode::numberPadZero : KeyCode::insert);
			break;

		case KeyCodeEN_US::home:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock
				? KeyCode::numberPadSeven : KeyCode::home);
			break;

		case KeyCodeEN_US::pageUp:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock ? KeyCode::numberPadNine : KeyCode::pageUp);
			break;

		case KeyCodeEN_US::numberPadLock:

			// Ensure this is not a repeat
			if (!released) {
				this->m_keyboard_state.number_pad_lock = !this->m_keyboard_state.number_pad_lock;
			}
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::numberPadLock);
			break;

		case KeyCodeEN_US::numberPadForwardSlash:

			// Check if number pad lock is on
			if (this->m_keyboard_state.number_pad_lock) {
				keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::numberPadForwardSlash);
			}
			else {

				// Normal Forward Slash
				keyEvent = KeyEvent(released, this->m_keyboard_state,
					should_be_upper_case ? KeyCode::questionMark : KeyCode::forwardSlash);
			}
			break;

			// Number Pad Multiply is same as print screen

		case KeyCodeEN_US::numberPadMinus:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::numberPadMinus);
			break;

			// Third row
		case KeyCodeEN_US::tab:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::tab);
			break;

		case KeyCodeEN_US::Q:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::Q : KeyCode::q);
			break;

		case KeyCodeEN_US::W:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::W : KeyCode::w);
			break;

		case KeyCodeEN_US::E:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::E : KeyCode::e);
			break;

		case KeyCodeEN_US::R:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::R : KeyCode::r);
			break;

		case KeyCodeEN_US::T:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::T : KeyCode::t);
			break;

		case KeyCodeEN_US::Y:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::Y : KeyCode::y);
			break;

		case KeyCodeEN_US::U:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::U : KeyCode::u);
			break;

		case KeyCodeEN_US::I:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::I : KeyCode::i);
			break;

		case KeyCodeEN_US::O:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::O : KeyCode::o);
			break;

		case KeyCodeEN_US::P:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::P : KeyCode::p);
			break;

		case KeyCodeEN_US::openSquareBracket:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::openCurlyBracket : KeyCode::openSquareBracket);
			break;

		case KeyCodeEN_US::closeSquareBracket:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::closeCurlyBracket : KeyCode::closeSquareBracket);
			break;

		case KeyCodeEN_US::backslash:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::lineThing : KeyCode::backslash);
			break;

		case KeyCodeEN_US::deleteKey:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock
				? KeyCode::numberPadFullStop : KeyCode::deleteKey);
			break;

		case KeyCodeEN_US::end:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock ? KeyCode::numberPadOne : KeyCode::end);
			break;

		case KeyCodeEN_US::pageDown:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock
				? KeyCode::numberPadThree : KeyCode::pageDown);
			break;

			// Number pad 7 is same as home

		case KeyCodeEN_US::numberPadEight:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock
				? KeyCode::numberPadEight : KeyCode::upArrow);
			break;

			// Number pad 9 is same as page up

		case KeyCodeEN_US::numberPadPlus:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::numberPadPlus);
			break;

			// Fourth row

		case KeyCodeEN_US::capsLock:
			// Ensure this is not a repeat
			if (!released) {
				this->m_keyboard_state.caps_lock = !this->m_keyboard_state.caps_lock;
			}

			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::capsLock);
			break;

		case KeyCodeEN_US::A:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::A : KeyCode::a);
			break;

		case KeyCodeEN_US::S:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::S : KeyCode::s);
			break;

		case KeyCodeEN_US::D:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::D : KeyCode::d);
			break;

		case KeyCodeEN_US::F:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::F : KeyCode::f);
			break;

		case KeyCodeEN_US::G:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::G : KeyCode::g);
			break;

		case KeyCodeEN_US::H:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::H : KeyCode::h);
			break;

		case KeyCodeEN_US::J:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::J : KeyCode::j);
			break;

		case KeyCodeEN_US::K:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::K : KeyCode::k);
			break;

		case KeyCodeEN_US::L:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::L : KeyCode::l);
			break;

		case KeyCodeEN_US::semicolon:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::colon : KeyCode::semicolon);
			break;

		case KeyCodeEN_US::apostrophe:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::quotationMark : KeyCode::apostrophe);
			break;

		case KeyCodeEN_US::enter:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock
				? KeyCode::numberPadEnter : KeyCode::enter);
			break;

		case KeyCodeEN_US::numberPadFour:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock ? KeyCode::numberPadFour : KeyCode::leftArrow);
			break;

		case KeyCodeEN_US::numberPadFive:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::numberPadFive);
			break;

		case KeyCodeEN_US::numberPadSix:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock ? KeyCode::numberPadSix : KeyCode::rightArrow);
			break;

			// Fifth row
		case KeyCodeEN_US::leftShift:
			this->m_keyboard_state.left_shift = !this->m_keyboard_state.left_shift;
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::leftShift);
			break;

		case KeyCodeEN_US::Z:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::Z : KeyCode::z);
			break;

		case KeyCodeEN_US::X:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::X : KeyCode::x);
			break;

		case KeyCodeEN_US::C:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::C : KeyCode::c);
			break;

		case KeyCodeEN_US::V:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::V : KeyCode::v);
			break;

		case KeyCodeEN_US::B:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::B : KeyCode::b);
			break;

		case KeyCodeEN_US::N:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::N : KeyCode::n);
			break;

		case KeyCodeEN_US::M:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::M : KeyCode::m);
			break;

		case KeyCodeEN_US::comma:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::lessThan : KeyCode::comma);
			break;

		case KeyCodeEN_US::fullStop:
			keyEvent = KeyEvent(released, this->m_keyboard_state,
				should_be_upper_case ? KeyCode::greaterThan : KeyCode::fullStop);
			break;

			// Forward slash is same as number pad forward slash

		case KeyCodeEN_US::rightShift:
			// Check if this is a repeat
			if (!released) {
				this->m_keyboard_state.right_shift = !this->m_keyboard_state.right_shift;
			}

			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::rightShift);
			break;

			// Up Arrow is the same as number pad 8

			// Number pad 1 is the same as end

		case KeyCodeEN_US::numberPadTwo:
			keyEvent = KeyEvent(released, this->m_keyboard_state, this->m_keyboard_state.number_pad_lock ? KeyCode::numberPadTwo : KeyCode::downArrow);
			break;

			// Number pad 3 is the same as page down

			// Number pad enter is the same as enter

			// Sixth row
		case KeyCodeEN_US::leftControl:
			// Check if this is a repeat
			if (!released) {
				this->m_keyboard_state.left_control = !this->m_keyboard_state.left_control;
				this->m_keyboard_state.right_control = !this->m_keyboard_state.right_control;
			}

			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::leftControl);
			break;

		case KeyCodeEN_US::leftOS:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::leftOS);
			break;

		case KeyCodeEN_US::leftAlt:
			// Check if this is a repeat
			if (!released) {
				this->m_keyboard_state.left_alt = !this->m_keyboard_state.left_alt;
				this->m_keyboard_state.right_alt = !this->m_keyboard_state.right_alt;
			}

			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::leftAlt);
			break;

		case KeyCodeEN_US::space:
			keyEvent = KeyEvent(released, this->m_keyboard_state, KeyCode::space);
			break;

			// Right Alt is the same as left alt

			// Right Control is the same as left control

			// Left Arrow is the same as number pad 4

			// Down Arrow is the same as number pad 2

			// Right Arrow is the same as number pad 6

			// Number pad 0 is the same as insert

			// Number pad full stop is the same as delete

		default:
			return false;
			break;

		}

	return true;
}
