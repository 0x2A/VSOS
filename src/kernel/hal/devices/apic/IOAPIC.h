#pragma once

#include "kernel/hal/devices/Device.h"
#include "kernel/hal/Interrupt.h"

class HAL;
class IOAPIC : public Device
{

public:

	#pragma pack(push, 1)
	struct Override {
		uint8_t bus;
		uint8_t source;
		uint32_t global_system_interrupt;
		uint16_t flags;
	};
	#pragma pack(pop)



	IOAPIC(HAL* hal);

	void Initialize(void* context) override;


	const void* GetResource(uint32_t type) const override;


	void DisplayDetails() const override;

	void SetRedirection(const interrupt_redirect_t* redirect);


	void RegisterOverrideEntry(uint8_t indx, Override ovr);

private:

	enum IoApicReg
	{
		ID = 0x00, //RW
		Version = 0x01, //RO
		ArbitrationID = 0x02, //RO
		FirstEntry = 0x10
	};

	enum class DeliveryMode : uint64_t
	{
		Fixed = 0b000,
		LowestPriority = 0b001,
		SMI = 0b010,
		Reserved1 = 0b011,
		NMI = 0b100,
		INIT = 0b101,
		Reserved2 = 0b110,
		ExtINT = 0b111
	};

	struct RedirectionEntry
	{
		union
		{
			struct
			{
				uint64_t InterruptVector : 8; //INTVEC - R/W
				DeliveryMode DeliveryMode : 3; //DELMOD - R/W
				uint64_t DestinationMode : 1; //DESTMOD - R/W
				uint64_t DeliveryStatus : 1; //DELIVS - RO
				uint64_t InterruptPolarity : 1; //INTPOL - R/W
				uint64_t RemoteIRR : 1; //RO
				uint64_t TriggerMode : 1; //R/W
				uint64_t InterruptMasked : 1; //R/W
				uint64_t Reserved : 39;
				uint64_t Destination : 8; //R/W
			};

			struct
			{
				uint32_t Low;
				uint32_t High;
			};

			uint64_t AsUint64;
		};
	};
	static_assert(sizeof(RedirectionEntry) == sizeof(uint64_t));

	RedirectionEntry ReadEntry(int pin);
	void WriteEntry(int pin, RedirectionEntry entry);

	uint32_t ReadReg(IoApicReg reg);
	void WriteReg(IoApicReg reg, const uint32_t value);

	HAL* m_HAL;
	uint64_t m_Addr;
	uint32_t m_GlobalIrqBase;
	uint8_t m_max_redirect_entry;
	uint8_t m_version;

	uint32_t m_override_array_size;
	Override m_override_array[0x10];
};