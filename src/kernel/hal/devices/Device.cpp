#include "Device.h"
#include "Assert.h"

Device::Device() :
	Path(),
	Name(),
	Description(),
	Type(DeviceType::Unknown),
	m_hid()
{

}

void Device::Display() const
{
	Printf("%s\n", this->Path);
	Printf("    Name: %s\n", this->Name.c_str());
	Printf("    Desc: %s\n", this->Description.c_str());
	Printf("    HID : %s\n", this->GetHid());

	//this->DisplayDetails();

}
