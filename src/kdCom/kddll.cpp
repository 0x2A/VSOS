
#include <ntdef.h>
#include <rtltypes.h>
#include <windbgkd.h>
#include "kdcom.h"

/* GLOBALS ********************************************************************/

ULONG CurrentPacketId = INITIAL_PACKET_ID | SYNC_PACKET_ID;
ULONG RemotePacketId = INITIAL_PACKET_ID;


/* PRIVATE FUNCTIONS **********************************************************/

/******************************************************************************
 * \name KdpCalculateChecksum
 * \brief Calculates the checksum for the packet data.
 * \param Buffer Pointer to the packet data.
 * \param Length Length of data in bytes.
 * \return The calculated checksum.
 * \sa http://www.vista-xp.co.uk/forums/technical-reference-library/2540-basics-debugging.html
 */
ULONG
NTAPI
KdpCalculateChecksum(
	IN PVOID Buffer,
	IN ULONG Length)
{
	PUCHAR ByteBuffer = (PUCHAR)Buffer;
	ULONG Checksum = 0;

	while (Length-- > 0)
	{
		Checksum += (ULONG)*ByteBuffer++;
	}
	return Checksum;
}

VOID
NTAPI
KdpSendControlPacket(
	IN USHORT PacketType,
	IN ULONG PacketId OPTIONAL)
{
	KD_PACKET Packet;

	Packet.PacketLeader = CONTROL_PACKET_LEADER;
	Packet.PacketId = PacketId;
	Packet.ByteCount = 0;
	Packet.Checksum = 0;
	Packet.PacketType = PacketType;

	KdpSendBuffer(&Packet, sizeof(KD_PACKET));
}




/* PUBLIC FUNCTIONS ***********************************************************/

/******************************************************************************
 * \name KdReceivePacket
 * \brief Receive a packet from the KD port.
 * \param [in] PacketType Describes the type of the packet to receive.
 *        This can be one of the PACKET_TYPE_ constants.
 * \param [out] MessageHeader Pointer to a STRING structure for the header.
 * \param [out] MessageData Pointer to a STRING structure for the data.
 * \return KdPacketReceived if successful, KdPacketTimedOut if the receive
 *         timed out, KdPacketNeedsResend to signal that the last packet needs
 *         to be sent again.
 * \note If PacketType is PACKET_TYPE_KD_POLL_BREAKIN, the function doesn't
 *       wait for any data, but returns KdPacketTimedOut instantly if no breakin
 *       packet byte is received.
 * \sa http://www.nynaeve.net/?p=169
 */
KDP_STATUS
NTAPI
KdReceivePacket(
	IN ULONG PacketType,
	OUT PSTRING MessageHeader,
	OUT PSTRING MessageData,
	OUT PULONG DataLength,
	IN OUT PKD_CONTEXT KdContext)
{
	UCHAR Byte = 0;
	KDP_STATUS KdStatus;
	KD_PACKET Packet;
	ULONG Checksum;

	/* Special handling for breakin packet */
	if (PacketType == PACKET_TYPE_KD_POLL_BREAKIN)
	{
		return KdpPollBreakIn();
	}

	for (;;)
	{
		/* Step 1 - Read PacketLeader */
		KdStatus = KdpReceivePacketLeader(&Packet.PacketLeader);
		if (KdStatus != KDP_PACKET_RECEIVED)
		{
			/* Check if we got a breakin  */
			if (KdStatus == KDP_PACKET_RESEND)
			{
				KdContext->KdpControlCPending = TRUE;
			}
			return KdStatus;
		}

		/* Step 2 - Read PacketType */
		KdStatus = KdpReceiveBuffer(&Packet.PacketType, sizeof(USHORT));
		if (KdStatus != KDP_PACKET_RECEIVED)
		{
			/* Didn't receive a PacketType. */
			return KdStatus;
		}

		/* Check if we got a resend packet */
		if (Packet.PacketLeader == CONTROL_PACKET_LEADER &&
			Packet.PacketType == PACKET_TYPE_KD_RESEND)
		{
			return KDP_PACKET_RESEND;
		}

		/* Step 3 - Read ByteCount */
		KdStatus = KdpReceiveBuffer(&Packet.ByteCount, sizeof(USHORT));
		if (KdStatus != KDP_PACKET_RECEIVED)
		{
			/* Didn't receive ByteCount. */
			return KdStatus;
		}

		/* Step 4 - Read PacketId */
		KdStatus = KdpReceiveBuffer(&Packet.PacketId, sizeof(ULONG));
		if (KdStatus != KDP_PACKET_RECEIVED)
		{
			/* Didn't receive PacketId. */
			return KdStatus;
		}

		/*
				if (Packet.PacketId != ExpectedPacketId)
				{
					// Ask for a resend!
					continue;
				}
		*/

		/* Step 5 - Read Checksum */
		KdStatus = KdpReceiveBuffer(&Packet.Checksum, sizeof(ULONG));
		if (KdStatus != KDP_PACKET_RECEIVED)
		{
			/* Didn't receive Checksum. */
			return KdStatus;
		}

		/* Step 6 - Handle control packets */
		if (Packet.PacketLeader == CONTROL_PACKET_LEADER)
		{
			switch (Packet.PacketType)
			{
			case PACKET_TYPE_KD_ACKNOWLEDGE:
				/* Are we waiting for an ACK packet? */
				if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE &&
					Packet.PacketId == (CurrentPacketId & ~SYNC_PACKET_ID))
				{
					/* Remote acknowledges the last packet */
					CurrentPacketId ^= 1;
					return KDP_PACKET_RECEIVED;
				}
				/* That's not what we were waiting for, start over */
				continue;

			case PACKET_TYPE_KD_RESET:
				KDDBGPRINT("KdReceivePacket - got PACKET_TYPE_KD_RESET\n");
				CurrentPacketId = INITIAL_PACKET_ID;
				RemotePacketId = INITIAL_PACKET_ID;
				KdpSendControlPacket(PACKET_TYPE_KD_RESET, 0);
				/* Fall through */

			case PACKET_TYPE_KD_RESEND:
				KDDBGPRINT("KdReceivePacket - got PACKET_TYPE_KD_RESEND\n");
				/* Remote wants us to resend the last packet */
				return KDP_PACKET_RESEND;

			default:
				KDDBGPRINT("KdReceivePacket - got unknown control packet\n");
				/* We got an invalid packet, ignore it and start over */
				continue;
			}
		}

		/* Did we wait for an ack packet? */
		if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
		{
			/* We received something different */
			KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
			CurrentPacketId ^= 1;
			return KDP_PACKET_RECEIVED;
		}

		/* Get size of the message header */
		MessageHeader->Length = MessageHeader->MaximumLength;

		/* Packet smaller than expected or too big? */
		if (Packet.ByteCount < MessageHeader->Length ||
			Packet.ByteCount > PACKET_MAX_SIZE)
		{
			KDDBGPRINT("KdReceivePacket - too few data (%d) for type %d\n",
				Packet.ByteCount, MessageHeader->Length);
			MessageHeader->Length = Packet.ByteCount;
			KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
			continue;
		}

		//KDDBGPRINT("KdReceivePacket - got normal PacketType, Buffer = %p\n", MessageHeader->Buffer);

		/* Receive the message header data */
		KdStatus = KdpReceiveBuffer(MessageHeader->Buffer,
			MessageHeader->Length);
		if (KdStatus != KDP_PACKET_RECEIVED)
		{
			/* Didn't receive data. Packet needs to be resent. */
			KDDBGPRINT("KdReceivePacket - Didn't receive message header data.\n");
			KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
			continue;
		}

		//KDDBGPRINT("KdReceivePacket - got normal PacketType 3\n");

		/* Calculate checksum for the header data */
		Checksum = KdpCalculateChecksum(MessageHeader->Buffer,
			MessageHeader->Length);

		/* Calculate the length of the message data */
		*DataLength = Packet.ByteCount - MessageHeader->Length;

		/* Shall we receive message data? */
		if (MessageData)
		{
			/* Set the length of the message data */
			MessageData->Length = (USHORT)*DataLength;

			/* Do we have data? */
			if (MessageData->Length)
			{
				KDDBGPRINT("KdReceivePacket - got data\n");

				/* Receive the message data */
				KdStatus = KdpReceiveBuffer(MessageData->Buffer,
					MessageData->Length);
				if (KdStatus != KDP_PACKET_RECEIVED)
				{
					/* Didn't receive data. Start over. */
					KDDBGPRINT("KdReceivePacket - Didn't receive message data.\n");
					KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
					continue;
				}

				/* Add cheksum for message data */
				Checksum += KdpCalculateChecksum(MessageData->Buffer,
					MessageData->Length);
			}
		}

		/* We must receive a PACKET_TRAILING_BYTE now */
		KdStatus = KdpReceiveBuffer(&Byte, sizeof(UCHAR));
		if (KdStatus != KDP_PACKET_RECEIVED || Byte != PACKET_TRAILING_BYTE)
		{
			KDDBGPRINT("KdReceivePacket - wrong trailing byte (0x%x), status 0x%x\n", Byte, KdStatus);
			KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
			continue;
		}

		/* Compare checksum */
		if (Packet.Checksum != Checksum)
		{
			KDDBGPRINT("KdReceivePacket - wrong cheksum, got %x, calculated %x\n",
				Packet.Checksum, Checksum);
			KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
			continue;
		}

		/* Acknowledge the received packet */
		KdpSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE, Packet.PacketId);

		/* Check if the received PacketId is ok */
		if (Packet.PacketId != RemotePacketId)
		{
			/* Continue with next packet */
			continue;
		}

		/* Did we get the right packet type? */
		if (PacketType == Packet.PacketType)
		{
			/* Yes, return success */
			//KDDBGPRINT("KdReceivePacket - all ok\n");
			RemotePacketId ^= 1;
			return KDP_PACKET_RECEIVED;
		}

		/* We received something different, ignore it. */
		KDDBGPRINT("KdReceivePacket - wrong PacketType\n");
	}

	return KDP_PACKET_RECEIVED;
}


VOID
NTAPI
KdSendPacket(
	IN ULONG PacketType,
	IN PSTRING MessageHeader,
	IN PSTRING MessageData,
	IN OUT PKD_CONTEXT KdContext)
{
	
}