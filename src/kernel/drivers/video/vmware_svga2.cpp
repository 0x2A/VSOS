#include "vmware_svga2.h"
#include <Assert.h>
#include "kernel/hal/HAL.h"
#include "kernel/Kernel.h"


#define PREFFERED_WIDTH 1440
#define PREFFERED_HEIGHT 900

VMware_SVGA2::VMware_SVGA2(PCIDevice* device, HAL* hal)
	: Driver(device), m_HAL(hal), m_Framebuffer(nullptr)
{
	m_device->Type = DeviceType::Video;
	m_device->Name = "SVGA-II";
	m_device->Description = "VMWare SVGA-II Graphics device";
}

DriverResult VMware_SVGA2::Activate()
{
	/*
	* Initialize the command FIFO. The beginning of FIFO memory is
	* used for an additional set of registers, the "FIFO registers".
	* These are higher-performance memory mapped registers which
	* happen to live in the same space as the FIFO. The driver is
	* responsible for allocating space for these registers, according
	* to the maximum number of registers supported by this driver
	* release.
	*/

	uint32_t* fifoMem = (uint32_t*)m_FIFObaseAddr;
	fifoMem[SVGA_FIFO_MIN] = SVGA_FIFO_NUM_REGS * sizeof(uint32_t);
	fifoMem[SVGA_FIFO_MAX] = m_FifoSize;
	fifoMem[SVGA_FIFO_NEXT_CMD] = fifoMem[SVGA_FIFO_MIN];
	fifoMem[SVGA_FIFO_STOP] = fifoMem[SVGA_FIFO_MIN];

	Printf("Success!\r\n");
	/*
	* Prep work for 3D version negotiation. See SVGA3D_Init for
	* details, but we have to give the host our 3D protocol version
	* before enabling the FIFO.
	*/

	/*
	* Prep work for 3D version negotiation. See SVGA3D_Init for
	* details, but we have to give the host our 3D protocol version
	* before enabling the FIFO.
	*/

	/*if (HasFIFOCap(SVGA_CAP_EXTENDED_FIFO) &&
		IsFIFORegValid(SVGA_FIFO_GUEST_3D_HWVERSION)) {

		fifoMem[SVGA_FIFO_GUEST_3D_HWVERSION] = SVGA3D_HWVERSION_CURRENT;
	}*/

	/*
	 * Enable the SVGA device and FIFO.
	 */

	WriteReg(SVGA_REG_ENABLE, TRUE);
	WriteReg(SVGA_REG_CONFIG_DONE, TRUE);

	/*
	 * Now that the FIFO is initialized, we can do an IRQ sanity check.
	 * This makes sure that the VM's chipset and our own IRQ code
	 * works. Better to find out now if something's wrong, than to
	 * deadlock later.
	 *
	 * This inserts a FIFO fence, does a legacy sync to drain the FIFO,
	 * then ensures that we received all applicable interrupts.
	 */

#if 0
	if (gSVGA.capabilities & SVGA_CAP_IRQMASK) {

		SVGA_WriteReg(SVGA_REG_IRQMASK, SVGA_IRQFLAG_ANY_FENCE);
		SVGA_ClearIRQ();

		SVGA_InsertFence();

		SVGA_WriteReg(SVGA_REG_SYNC, 1);
		while (SVGA_ReadReg(SVGA_REG_BUSY) != FALSE);

		SVGA_WriteReg(SVGA_REG_IRQMASK, 0);

		/* Check whether the interrupt occurred without blocking. */
		if ((gSVGA.irq.pending & SVGA_IRQFLAG_ANY_FENCE) == 0) {
			SVGA_Panic("SVGA IRQ appears to be present but broken.");
		}

		SVGA_WaitForIRQ();
	}
#endif

	SetResolution(PREFFERED_WIDTH, PREFFERED_HEIGHT);

//	FlushScreen();

	m_HAL->RegisterVideoDevice(this);
	Printf("Success!");
	return DriverResult::Success;
}

DriverResult VMware_SVGA2::Deactivate()
{
	WriteReg(SVGA_REG_ENABLE, 0);

	return DriverResult::Success;
}

DriverResult VMware_SVGA2::Initialize()
{
	((PCIDevice*)m_device)->SetMemEnabled(true);
	auto bar0 = ((PCIDevice*)m_device)->GetBAR(0);
	auto bar1 = ((PCIDevice*)m_device)->GetBAR(1);
	auto bar2 = ((PCIDevice*)m_device)->GetBAR(2);

	this->fifo.reservedSize = 0;
	this->fifo.nextFence = 0;

	memset(fifo.bounceBuffer, 0, 1024*1024 );

	m_IObaseAddr = (uint32_t)bar0.address;
	m_FBAddr = (uint32_t)bar1.address;
	m_FIFObaseAddr = (uint32_t)bar2.address;

	/*
	* Version negotiation:
	*
	*   1. Write to SVGA_REG_ID the maximum ID supported by this driver.
	*   2. Read from SVGA_REG_ID
	*      a. If we read back the same value, this ID is supported. We're done.
	*      b. If not, decrement the ID and repeat.
	*/

	deviceVersionId = SVGA_ID_2;
	do {
		WriteReg(SVGA_REG_ID, deviceVersionId);
		if (ReadReg(SVGA_REG_ID) == deviceVersionId) {
			break;
		}
		else {
			deviceVersionId--;
		}
	} while (deviceVersionId >= SVGA_ID_0);

	/*
	* We must determine the FIFO and FB size after version
	* negotiation, since the default version (SVGA_ID_0)
	* does not support the FIFO buffer at all.
	*/

	m_VRamSize = ReadReg(SVGA_REG_VRAM_SIZE);
	m_FBSize = ReadReg(SVGA_REG_FB_SIZE);
	m_FifoSize = ReadReg(SVGA_REG_FIFO_SIZE);

	m_FBAddr = (uint64_t)kernel.DriverMapPages(m_FBAddr, SizeToPages(m_FBSize));
	m_FIFObaseAddr = (uint64_t)kernel.DriverMapPages(m_FIFObaseAddr, SizeToPages(m_FifoSize));

	/*
	* Sanity-check the FIFO and framebuffer sizes.
	* These are arbitrary values.
	*/
	Printf("FifoSize: 0x%08x\r\n", m_FifoSize);
	if (m_FBSize < 0x10000) {
		kernel.Panic("FB size very small, probably incorrect.");
	}
	if (m_FifoSize < 0x5000) {
		kernel.Panic("FIFO size very small, probably incorrect.\r\n");
	}

	/*
	* If the device is new enough to support capability flags, get the
	* capabilities register.
	*/
	if (deviceVersionId >= SVGA_ID_1) {
		capabilities = ReadReg(SVGA_REG_CAPABILITIES);
	}


	Printf("vmware io base: 0x%016x, fb: 0x%016x, fifo: 0x%016x\r\n", m_IObaseAddr, m_FBAddr, m_FIFObaseAddr);
	// TODO: Enable interrupt support

#if 0
	if (gSVGA.capabilities & SVGA_CAP_IRQMASK) {
		uint8 irq = PCI_ConfigRead8(&gSVGA.pciAddr, offsetof(PCIConfigSpace, intrLine));

		/* Start out with all SVGA IRQs masked */
		SVGA_WriteReg(SVGA_REG_IRQMASK, 0);

		/* Clear all pending IRQs stored by the device */
		IO_Out32(gSVGA.ioBase + SVGA_IRQSTATUS_PORT, 0xFF);

		/* Clear all pending IRQs stored by us */
		SVGA_ClearIRQ();

		/* Enable the IRQ */
		Intr_SetHandler(IRQ_VECTOR(irq), SVGAInterruptHandler);
		Intr_SetMask(irq, TRUE);
	}

#endif

	return DriverResult::Success;
}

DriverResult VMware_SVGA2::Reset()
{
	return DriverResult::NotImplemented;
}

std::string VMware_SVGA2::get_vendor_name()
{
	return "VMWare";
}

std::string VMware_SVGA2::get_device_name()
{
	return "SVGA-II";
}

void VMware_SVGA2::SetResolution(uint16_t w, uint16_t h)
{
	//WriteReg(SVGA_REG_ENABLE, 0);
	//WriteReg(SVGA_REG_ID, 0);
	WriteReg(SVGA_REG_WIDTH, w);
	WriteReg(SVGA_REG_HEIGHT, h);
	WriteReg(SVGA_REG_BITS_PER_PIXEL, 32);
	WriteReg(SVGA_REG_ENABLE, 1);

	uint32_t bpl = ReadReg(SVGA_REG_BYTES_PER_LINE);
	this->w = w;
	this->h = h;
	this->bytesPerLine = bpl;
	//m_FBMemSize = ReadReg(15);

	if(m_Framebuffer)
		delete m_Framebuffer;
	m_Framebuffer = new gfx::LinearFrameBuffer((void*)m_FBAddr, h, w);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_BeginDefineCursor --
 *
 *      Begin an SVGA_CMD_DEFINE_CURSOR command. This copies the command header
 *      into the FIFO, and reserves enough space for the cursor image itself.
 *      We return pointers to FIFO memory where the AND/XOR masks can be written.
 *      When finished, the caller must invoke SVGA_FIFOCommitAll().
 *
 * Results:
 *      Returns pointers to memory where the caller can store the AND/XOR masks.
 *
 * Side effects:
 *      Reserves space in the FIFO.
 *
 *-----------------------------------------------------------------------------
 */
void VMware_SVGA2::DefineCursor(uint8_t* bitmap, uint32_t w, uint32_t h)
{
	//throw std::logic_error("The method or operation is not implemented.");
	SVGAFifoCmdDefineCursor cursor;
	cursor.id = 0;
	cursor.hotspotX = 1;
	cursor.hotspotY = 1;
	cursor.width = w;
	cursor.height = h;
	cursor.andMaskDepth = 1;
	cursor.xorMaskDepth = 1;

	void *andData, *xorData;
	
	uint32_t andPitch = ((cursor.andMaskDepth * cursor.width + 31) >> 5) << 2;
	uint32_t andSize = andPitch * cursor.height;
	uint32_t xorPitch = ((cursor.xorMaskDepth * cursor.width + 31) >> 5) << 2;
	uint32_t xorSize = xorPitch * cursor.height;

	SVGAFifoCmdDefineCursor* cmd = (SVGAFifoCmdDefineCursor*)FIFOReserveCmd(SVGA_CMD_DEFINE_CURSOR,
		sizeof * cmd + andSize + xorSize);
	*cmd = cursor;
	andData = (void*) (cmd + 1);
	xorData = (void*) (andSize + (uint8_t*) andData);

	memcpy(andData, bitmap, andSize);

	FIFOCommitAll();
}

void VMware_SVGA2::UpdateRect(gfx::Rectangle rect)
{
	SVGAFifoCmdUpdate* cmd = (SVGAFifoCmdUpdate *)FIFOReserveCmd(SVGA_CMD_UPDATE, sizeof * cmd);
	cmd->x = rect.X;
	cmd->y = rect.Y;
	cmd->width = rect.Width;
	cmd->height = rect.Height;
	FIFOCommitAll();
}

void VMware_SVGA2::MoveCursor(uint32_t visible, uint32_t x, uint32_t y, uint32_t screenId)
{
	uint32_t* fifoMem = (uint32_t*)m_FIFObaseAddr;
	if (HasFIFOCap(SVGA_FIFO_CAP_SCREEN_OBJECT)) {
		fifoMem[SVGA_FIFO_CURSOR_SCREEN_ID] = screenId;
	}

	if (HasFIFOCap(SVGA_FIFO_CAP_CURSOR_BYPASS_3)) {
		fifoMem[SVGA_FIFO_CURSOR_ON] = visible;
		fifoMem[SVGA_FIFO_CURSOR_X] = x;
		fifoMem[SVGA_FIFO_CURSOR_Y] = y;
		fifoMem[SVGA_FIFO_CURSOR_COUNT]++;
	}
}

uint32_t VMware_SVGA2::GetScreenWidth()
{
	return m_Framebuffer->GetWidth();
}

uint32_t VMware_SVGA2::GetScreenHeight()
{
	return m_Framebuffer->GetHeight();
}

gfx::FrameBuffer* VMware_SVGA2::GetFramebuffer()
{
	return m_Framebuffer;
}

uint32_t* VMware_SVGA2::AllocBuffer(uint32_t width, uint32_t height)
{
	return new uint32_t[width*height];

	//TODO: Implement SVGA GMR functionality for hardware accelerated blitting!
}

void VMware_SVGA2::FreeBuffer(uint32_t* buff)
{
	delete[] buff;
	//TODO: Implement SVGA GMR functionality for hardware accelerated blitting!
}

void VMware_SVGA2::BlitBuffer(uint32_t* buffer, gfx::Rectangle rect)
{
	m_Framebuffer->WriteFrame(rect, buffer);
}

void VMware_SVGA2::WriteReg(int reg, int val)
{
	m_HAL->WritePort(SVGA_IO_MUL * SVGA_INDEX_PORT + m_IObaseAddr, reg, 32);
	m_HAL->WritePort(SVGA_IO_MUL * SVGA_VALUE_PORT + m_IObaseAddr, val, 32);
}

uint32_t VMware_SVGA2::ReadReg(int reg)
{
	m_HAL->WritePort(SVGA_IO_MUL * SVGA_INDEX_PORT + m_IObaseAddr, reg, 32);
	return m_HAL->ReadPort(SVGA_IO_MUL * SVGA_VALUE_PORT + m_IObaseAddr, 32);
}

bool VMware_SVGA2::HasFIFOCap(int cap)
{
	return (((uint32_t*)m_FIFObaseAddr)[SVGA_FIFO_CAPABILITIES] & cap) != 0;
}

bool VMware_SVGA2::IsFIFORegValid(int reg)
{
	return ((uint32_t*)m_FIFObaseAddr)[SVGA_FIFO_MIN] > (reg << 2);
}

void* VMware_SVGA2::FIFOReserve(uint32_t bytes)
{
	volatile uint32_t* fifo = (uint32_t*)m_FIFObaseAddr;
	uint32_t max = fifo[SVGA_FIFO_MAX];
	uint32_t min = fifo[SVGA_FIFO_MIN];
	uint32_t nextCmd = fifo[SVGA_FIFO_NEXT_CMD];
	bool reserveable = HasFIFOCap(SVGA_FIFO_CAP_RESERVE);

	/*
	* This example implementation uses only a statically allocated
	* buffer.  If you want to support arbitrarily large commands,
	* dynamically allocate a buffer if and only if it's necessary.
	*/

	if (bytes > sizeof(this->fifo.bounceBuffer) ||
		bytes > (max - min)) {
		
		Panic("FIFO command too large");
	}

	if (bytes % sizeof(uint32_t)) {
		Panic("FIFO command length not 32-bit aligned");
	}

	if (this->fifo.reservedSize != 0) {
		Panic("FIFOReserve before FIFOCommit");
	}

	this->fifo.reservedSize = bytes;

	while (1) {
		uint32_t stop = fifo[SVGA_FIFO_STOP];
		bool reserveInPlace = FALSE;
		bool needBounce = FALSE;

		/*
		 * Find a strategy for dealing with "bytes" of data:
		 * - reserve in place, if there's room and the FIFO supports it
		 * - reserve in bounce buffer, if there's room in FIFO but not
		 *   contiguous or FIFO can't safely handle reservations
		 * - otherwise, sync the FIFO and try again.
		 */

		if (nextCmd >= stop) {
			/* There is no valid FIFO data between nextCmd and max */

			if (nextCmd + bytes < max ||
				(nextCmd + bytes == max && stop > min)) {
				/*
				 * Fastest path 1: There is already enough contiguous space
				 * between nextCmd and max (the end of the buffer).
				 *
				 * Note the edge case: If the "<" path succeeds, we can
				 * quickly return without performing any other tests. If
				 * we end up on the "==" path, we're writing exactly up to
				 * the top of the FIFO and we still need to make sure that
				 * there is at least one unused DWORD at the bottom, in
				 * order to be sure we don't fill the FIFO entirely.
				 *
				 * If the "==" test succeeds, but stop <= min (the FIFO
				 * would be completely full if we were to reserve this
				 * much space) we'll end up hitting the FIFOFull path below.
				 */
				reserveInPlace = TRUE;
			}
			else if ((max - nextCmd) + (stop - min) <= bytes) {
				/*
				 * We have to split the FIFO command into two pieces,
				 * but there still isn't enough total free space in
				 * the FIFO to store it.
				 *
				 * Note the "<=". We need to keep at least one DWORD
				 * of the FIFO free at all times, or we won't be able
				 * to tell the difference between full and empty.
				 */
				FIFOFull();
			}
			else {
				/*
				 * Data fits in FIFO but only if we split it.
				 * Need to bounce to guarantee contiguous buffer.
				 */
				needBounce = TRUE;
			}

		}
		else {
			/* There is FIFO data between nextCmd and max */

			if (nextCmd + bytes < stop) {
				/*
				 * Fastest path 2: There is already enough contiguous space
				 * between nextCmd and stop.
				 */
				reserveInPlace = TRUE;
			}
			else {
				/*
				 * There isn't enough room between nextCmd and stop.
				 * The FIFO is too full to accept this command.
				 */
				FIFOFull();
			}
		}

		/*
		 * If we decided we can write directly to the FIFO, make sure
		 * the VMX can safely support this.
		 */
		if (reserveInPlace) {
			if (reserveable || bytes <= sizeof(uint32_t)) {
				this->fifo.usingBounceBuffer = FALSE;
				if (reserveable) {
					fifo[SVGA_FIFO_RESERVED] = bytes;
				}
				return nextCmd + (uint8_t*)fifo;
			}
			else {
				/*
				 * Need to bounce because we can't trust the VMX to safely
				 * handle uncommitted data in FIFO.
				 */
				needBounce = TRUE;
			}
		}

		/*
		 * If we reach here, either we found a full FIFO, called
		 * SVGAFIFOFull to make more room, and want to try again, or we
		 * decided to use a bounce buffer instead.
		 */
		if (needBounce) {
			this->fifo.usingBounceBuffer = TRUE;
			return this->fifo.bounceBuffer;
		}
	} /* while (1) */
}

void VMware_SVGA2::Panic(const char* str)
{
	Deactivate();

	Printf(str);
	while (true) __halt();
}

void VMware_SVGA2::FIFOFull(void)
{
	//TODO
}

void* VMware_SVGA2::FIFOReserveEscape(uint32_t nsid, /* IN */ uint32_t bytes)
{
	uint32_t paddedBytes = (bytes + 3) & ~3UL;

	#pragma push(pack, 1)
	struct _hdr_{
		uint32_t cmd;
		uint32_t nsid;
		uint32_t size;
	};
	#pragma pop(pack)
	
	_hdr_ *header = (_hdr_*)FIFOReserve(paddedBytes + sizeof(_hdr_));

	header->cmd = SVGA_CMD_ESCAPE;
	header->nsid = nsid;
	header->size = bytes;

	return header + 1;
}

void VMware_SVGA2::FIFOCommitAll(void)
{
	FIFOCommit(this->fifo.reservedSize);
}

void VMware_SVGA2::FIFOCommit(uint32_t bytes)
{
	volatile uint32_t* fifo = (uint32_t*)m_FIFObaseAddr;
	uint32_t nextCmd = fifo[SVGA_FIFO_NEXT_CMD];
	uint32_t max = fifo[SVGA_FIFO_MAX];
	uint32_t min = fifo[SVGA_FIFO_MIN];
	bool reserveable = HasFIFOCap(SVGA_FIFO_CAP_RESERVE);

	if (this->fifo.reservedSize == 0) {
		Panic("FIFOCommit before FIFOReserve");
	}
	this->fifo.reservedSize = 0;

	if (this->fifo.usingBounceBuffer) {
		/*
		 * Slow paths: copy out of a bounce buffer.
		 */
		uint8_t* buffer = this->fifo.bounceBuffer;

		if (reserveable) {
			/*
			 * Slow path: bulk copy out of a bounce buffer in two chunks.
			 *
			 * Note that the second chunk may be zero-length if the reserved
			 * size was large enough to wrap around but the commit size was
			 * small enough that everything fit contiguously into the FIFO.
			 *
			 * Note also that we didn't need to tell the FIFO about the
			 * reservation in the bounce buffer, but we do need to tell it
			 * about the data we're bouncing from there into the FIFO.
			 */

			uint32_t chunkSize = MIN(bytes, max - nextCmd);
			fifo[SVGA_FIFO_RESERVED] = bytes;
			memcpy(nextCmd + (uint8_t*)fifo, buffer, chunkSize);
			memcpy(min + (uint8_t*)fifo, buffer + chunkSize, bytes - chunkSize);

		}
		else {
			/*
			 * Slowest path: copy one dword at a time, updating NEXT_CMD as
			 * we go, so that we bound how much data the guest has written
			 * and the host doesn't know to checkpoint.
			 */

			uint32_t* dword = (uint32_t*)buffer;

			while (bytes > 0) {
				fifo[nextCmd / sizeof * dword] = *dword++;
				nextCmd += sizeof * dword;
				if (nextCmd == max) {
					nextCmd = min;
				}
				fifo[SVGA_FIFO_NEXT_CMD] = nextCmd;
				bytes -= sizeof * dword;
			}
		}
	}

	/*
	 * Atomically update NEXT_CMD, if we didn't already
	 */
	if (!this->fifo.usingBounceBuffer || reserveable) {
		nextCmd += bytes;
		if (nextCmd >= max) {
			nextCmd -= max - min;
		}
		fifo[SVGA_FIFO_NEXT_CMD] = nextCmd;
	}

	/*
	 * Clear the reservation in the FIFO.
	 */
	if (reserveable) {
		fifo[SVGA_FIFO_RESERVED] = 0;
	}
}

void* VMware_SVGA2::FIFOReserveCmd(uint32_t type, /* IN */ uint32_t bytes)
{
	uint32_t* cmd = (uint32_t*)FIFOReserve(bytes + sizeof type);
	cmd[0] = type;
	return cmd + 1;
}

