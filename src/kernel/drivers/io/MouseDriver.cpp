#include "MouseDriver.h"
#include "kernel/hal/HAL.h"
#include "kernel/hal/devices/io/GenericMouse.h"
#include <Assert.h>


#define MOUSE_DATA_PORT		0x60
#define MOUSE_COMMAND_PORT	0x64
#define MOUSE_INTERRUPT_VEC	0x2C

MouseDriver::MouseDriver(HAL* hal)
: m_HAL(hal), Driver(new GenericMouse())
{
}

DriverResult MouseDriver::Activate()
{
	//  Get the current state of the mouse
	m_HAL->WritePort(MOUSE_COMMAND_PORT, 0x20, 8);
	uint8_t status = (m_HAL->ReadPort(MOUSE_DATA_PORT, 8) | 2);

	// write the new state
	m_HAL->WritePort(MOUSE_COMMAND_PORT, 0x60, 8);
	m_HAL->WritePort(MOUSE_DATA_PORT, status, 8);

	// Tell the PIC to start listening to the mouse
	m_HAL->WritePort(MOUSE_COMMAND_PORT, 0xAB, 8);

	// activate the mouse
	m_HAL->WritePort(MOUSE_COMMAND_PORT, 0xD4, 8);
	m_HAL->WritePort(MOUSE_DATA_PORT, 0xF4, 8);
	m_HAL->ReadPort(MOUSE_DATA_PORT, 8);

	interrupt_redirect_t mouseRedirect;
	mouseRedirect.type = 0xC;
	mouseRedirect.index = 0xC;	//INT12 is PS2 Mouse
	mouseRedirect.interrupt = MOUSE_INTERRUPT_VEC;
	mouseRedirect.destination = 0x00;
	mouseRedirect.flags = 0x00;
	mouseRedirect.mask = false;

	m_HAL->SetInterruptRedirect(&mouseRedirect);

	m_HAL->RegisterInterrupt(MOUSE_INTERRUPT_VEC, { MouseDriver::handle_interrupt, this });

	return DriverResult::Success;	
}

DriverResult MouseDriver::Deactivate()
{
	return DriverResult::NotImplemented;
}

DriverResult MouseDriver::Initialize()
{
	if (m_device) m_device->Initialize(this);
	return DriverResult::Success;
}

DriverResult MouseDriver::Reset()
{
	return DriverResult::NotImplemented;
}

std::string MouseDriver::get_vendor_name()
{
	return "unknown";
}

std::string MouseDriver::get_device_name()
{
	return m_device->Name;
}

void MouseDriver::AddEventHandler(MouseEventHandler* handler)
{
	m_MouseEventHandler.push_back(handler);
}

uint32_t MouseDriver::handle_interrupt(void* arg)
{
	MouseDriver* driver = (MouseDriver*)arg;

	if(driver)
		driver->HandleInterrupt();

	return 0;
}

void MouseDriver::RaiseMouseMoveEvent(MouseMoveEvent event)
{
	for(auto t : m_MouseEventHandler)
		t->OnMouseMoveEvent(event);
}

void MouseDriver::RaiseMouseButtonEvent(MouseButtonEvent event)
{
	for (auto t : m_MouseEventHandler)
		t->OnMouseButtonEvent(event);
}

void MouseDriver::HandleInterrupt()
{

	//Only if the 6th bit of data is one then there is data to handle
	uint8_t status = m_HAL->ReadPort(MOUSE_COMMAND_PORT, 8);
	if (!(status & 0x20))
		return;

	// read the data and store it in the buffer
	buffer[offset] = m_HAL->ReadPort(MOUSE_DATA_PORT, 8);
	offset = (offset + 1) % 3;

	// If the mouse data transmission is incomplete (3rd piece of data isn't through)
	if (offset != 0)
		return;

	// If the mouse is moved (y-axis is inverted)
	if (buffer[1] != 0 || buffer[2] != 0)
	{
		//convert 9 bit values to correct signed int8
		int8_t relX = buffer[0] - ((buffer[2] << 4) & 0x100);
		int8_t relY = buffer[1] - ((buffer[2] << 3) & 0x100);
		
		RaiseMouseMoveEvent({relX, (int8_t)-relY});
	}

	for (int i = 0; i < 3; ++i) {

		// Check if the button state has changed
		if ((buffer[2] & (0x1 << i)) != (buttons & (0x1 << i)))
		{
			// Check if the button is up or down
			if (buttons & (0x1 << i))
				RaiseMouseButtonEvent({(uint8_t)(i + 1), true });
			else
				RaiseMouseButtonEvent({ (uint8_t)(i + 1), false });

		}
	}

	// Update the buttons
	buttons = buffer[0];
}
