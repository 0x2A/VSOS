#pragma once
#include "VmBusDriver.h"
#include "HyperVRingBuffer.h"

class HyperVChannel
{
public:
	HyperVChannel(uint32_t sendSize, uint32_t receiveSize, CallContext callback);

	void Initialize(vmbus_channel_offer_channel* offerChannel, const ReadOnlyBuffer* buffer = nullptr);

	void SendPacket(const void* buffer, const size_t length, const uint64_t requestId, const vmbus_packet_type type, const uint32_t flags);

	//start read
	void* ReadPacket(const uint32_t length);
	void NextPacket(const uint32_t length);
	void StopRead();

	void SetEvent();

	void Display();


private:
	uint32_t m_sendCount;
	uint32_t m_receiveCount;
	paddr_t m_address;
	CallContext m_callback;

	HyperVRingBuffer m_inbound;
	HyperVRingBuffer m_outbound;

	uint32_t m_gpadlHandle;

	vmbus_channel_offer_channel* m_channel;

	VmBusDriver* m_vmbus;
};