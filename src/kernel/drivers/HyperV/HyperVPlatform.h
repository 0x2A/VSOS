#pragma once
#include <cstdint>

#include <string>
#include <bitset>
#include <intrin.h>
#include <windows/types.h>
#include "kernel/devices/hv/HyperV_def.h"
#include "kernel/os/Time.h"
#include "os.internal.h"

#include <list>

//TODO: separate into hypervplatform and hypervinfo
//platform is basically the hal
//hypervinfo will be like cpuid, just a light weight way to check features
class HyperV
{
public:
	typedef uint64_t HV_SYNIC_SINT_INDEX;

	HyperV();

	void Initialize();

	bool IsPresent()
	{
		return m_isPresent;
	}

	bool DirectSyntheticTimers() { return m_featuresEdx[19]; }

	bool AccessPartitionReferenceCounter() { return m_featuresEax[9]; }

	bool DeprecateAutoEOI() { return m_recommendationsEax[9]; }

	bool InputInXMM() { return m_featuresEdx[4]; }

	static void EOI()
	{
		__writemsr(HV_X64_MSR_EOI, 0);
	}

	nano100_t ReadTsc();
	uint64_t TscFreq()
	{
		return __readmsr(HV_X64_MSR_TSC_FREQUENCY);
	}

	static void SetSynicMessagePage(paddr_t address)
	{
		HV_INTERRUPT_PAGE_REGISTER reg;
		reg.AsUint64 = __readmsr(SIMP);
		reg.Enable = true;
		reg.BaseAddress = (address >> PageShift);

		__writemsr(SIMP, reg.AsUint64);
	}

	static void SetSynicEventPage(paddr_t address)
	{
		HV_INTERRUPT_PAGE_REGISTER reg;
		reg.AsUint64 = __readmsr(SIEFP);
		reg.Enable = true;
		reg.BaseAddress = (address >> PageShift);

		__writemsr(SIEFP, reg.AsUint64);
	}

	static void SetSintVector(uint32_t sint, uint32_t vector)
	{
		HV_SINT_REGISTER reg = { 0 };
		reg.AsUint64 = __readmsr(SINT0 + sint);
		reg.Vector = vector;
		reg.Masked = false;
		reg.AutoEOI = true;
		//reg.AutoEOI = !DeprecateAutoEOI(); //TODO
		__writemsr(SINT0 + sint, reg.AsUint64);
	}

	static void EnableSynic()
	{
		HV_SCONTROL_REGISTER reg;
		reg.AsUint64 = __readmsr(SCONTROL);
		reg.Enabled = true;
		__writemsr(SCONTROL, reg.AsUint64);
	}

	void WriteSintRegister(uint32_t index, uint64_t value)
	{
		//Assert(index < HV_SYNIC_SINT_COUNT);
		__writemsr(SINT0 + index, value);
	}

	static void SignalEom()
	{
		__writemsr(EOM, 0);
	}

	static void ProcessInterrupts(uint32_t sint, std::list<uint32_t>& channelIds, std::list<HV_MESSAGE>& queue);

	//Code: 0x005C
	//Params:
	//0 - ConnectionId (4 bytes), Padding (4 bytes) - from HvConnectPort
	//8 - MessageType (4 bytes), PayloadSize (4 bytes) - TopBit of MessageType is cleared. PayloadSize (up to 240 bytes)
	//16 Message[0] (8 bytes)
	//248 Message[29] (8 bytes)
	static HV_HYPERCALL_RESULT_VALUE
		HvPostMessage(
			__in HV_CONNECTION_ID ConnectionId,
			__in HV_MESSAGE_TYPE MessageType,
			__in UINT32 PayloadSize,
			__in_ecount(PayloadSize)
			PCVOID Message
		);

	//Code: 0x5D
	//0 - ConnectionId (4 bytes), FlagNumber (2 bytes), Reserved (2 bytes)
	static HV_HYPERCALL_RESULT_VALUE
		HvSignalEvent(
			__in HV_CONNECTION_ID ConnectionId,
			__in UINT16 FlagNumber //relative to base number for port
		);

	private:
		typedef std::bitset<std::numeric_limits<uint32_t>::digits> bitset_32;

		uint32_t m_highestLeaf;
		std::string m_vendor;
		bool m_isPresent;

		//Features
		bitset_32 m_featuresEax;//4.2.2 Partition Privilege Flags
		bitset_32 m_featuresEbx;
		bitset_32 m_featuresEdx;

		//Recommendations
		bitset_32 m_recommendationsEax;

		//Static State
		static KERNEL_PAGE_ALIGN volatile HV_REFERENCE_TSC_PAGE TscPage;

		//Per CPU State TODO
		// examine, copy to another location (work queue), set type to None, write EOI, and then process
		static KERNEL_PAGE_ALIGN volatile HV_MESSAGE SynicMessages[HV_SYNIC_SINT_COUNT];
		// examine flags, clear using LOCK AND or LOCK CMPXCHG, write EOI, process
		static KERNEL_PAGE_ALIGN volatile HV_SYNIC_EVENT_FLAGS SynicEvents[HV_SYNIC_SINT_COUNT];
		static KERNEL_PAGE_ALIGN volatile uint8_t HypercallPage[PageSize];//Code page, map as execute
		static KERNEL_PAGE_ALIGN volatile uint8_t PostMessagePage[PageSize];
};