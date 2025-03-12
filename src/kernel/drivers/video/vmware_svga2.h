#pragma once

#include <stdint.h>
#include "kernel/drivers/Driver.h"
#include <kernel\hal\devices\pci\PCIDevice.h>
#include <gfx\LinearFrameBuffer.h>
#include "VideoDevice.h"

#define SVGA_IO_MUL 1
#define SVGA_INDEX_PORT 0
#define SVGA_VALUE_PORT 1

#define SVGA_REG_ID 0
#define SVGA_REG_ENABLE 1
#define SVGA_REG_WIDTH 2
#define SVGA_REG_HEIGHT 3
#define SVGA_REG_MAX_WIDTH 4
#define SVGA_REG_MAX_HEIGHT 5
#define SVGA_REG_BITS_PER_PIXEL 7
#define SVGA_REG_BYTES_PER_LINE 12
#define SVGA_REG_FB_START 13
#define SVGA_REG_FB_OFFSET 14	// offset in the frame buffer to the visible pixel data
#define SVGA_REG_VRAM_SIZE 15
#define SVGA_REG_FB_SIZE 16
#define SVGA_REG_CAPABILITIES 17
#define SVGA_REG_FIFO_START 18
#define SVGA_REG_FIFO_SIZE 19
#define SVGA_REG_CONFIG_DONE 20 // flag to enable FIFO operation
#define SVGA_REG_SYNC 21 // flag set by the driver to flush FIFO changes
#define SVGA_REG_BUSY 22 // flag set by the FIFO when it's processed


#define SVGA_CMD_UPDATE 1 // Update the screen; Operands: startX, startY, width, height
#define SVGA_CMD_RECT_COPY 3 // Copy a rectangle from one area of the visible frame buffer to another; Operands: srcX, srcY, destX, destY, width, height(Requires SVGA_CAP_RECT_COPY)
#define SVGA_CMD_DEFINE_CURSOR 19
#define SVGA_CMD_DEFINE_ALPHA_CURSOR 22
#define SVGA_CMD_UPDATE_VERBOSE 25 //->Just like SVGA_CMD_UPDATE but adds another operand named reason
#define SVGA_CMD_FRONT_ROP_FILL 29
#define SVGA_CMD_FENCE 30
#define SVGA_CMD_ESCAPE 33
#define SVGA_CMD_DEFINE_SCREEN 34
#define SVGA_CMD_DESTROY_SCREEN 35
#define SVGA_CMD_DEFINE_GMRFB 36
#define SVGA_CMD_BLIT_GMRFB_TO_SCREEN 37
#define SVGA_CMD_BLIT_SCREEN_TO_GMRFB 38
#define SVGA_CMD_ANNOTATION_FILL 39
#define SVGA_CMD_ANNOTATION_COPY 40
#define SVGA_CMD_DEFINE_GMR2 41
#define SVGA_CMD_REMAP_GMR2 42

/*
 * FIFO register indices.
 *
 * The FIFO is a chunk of device memory mapped into guest physmem.  It
 * is always treated as 32-bit words.
 *
 * The guest driver gets to decide how to partition it between
 * - FIFO registers (there are always at least 4, specifying where the
 *   following data area is and how much data it contains; there may be
 *   more registers following these, depending on the FIFO protocol
 *   version in use)
 * - FIFO data, written by the guest and slurped out by the VMX.
 * These indices are 32-bit word offsets into the FIFO.
 */

enum {
    /*
     * Block 1 (basic registers): The originally defined FIFO registers.
     * These exist and are valid for all versions of the FIFO protocol.
     */

    SVGA_FIFO_MIN = 0,
    SVGA_FIFO_MAX,       /* The distance from MIN to MAX must be at least 10K */
    SVGA_FIFO_NEXT_CMD,
    SVGA_FIFO_STOP,

    /*
     * Block 2 (extended registers): Mandatory registers for the extended
     * FIFO.  These exist if the SVGA caps register includes
     * SVGA_CAP_EXTENDED_FIFO; some of them are valid only if their
     * associated capability bit is enabled.
     *
     * Note that when originally defined, SVGA_CAP_EXTENDED_FIFO implied
     * support only for (FIFO registers) CAPABILITIES, FLAGS, and FENCE.
     * This means that the guest has to test individually (in most cases
     * using FIFO caps) for the presence of registers after this; the VMX
     * can define "extended FIFO" to mean whatever it wants, and currently
     * won't enable it unless there's room for that set and much more.
     */

    SVGA_FIFO_CAPABILITIES = 4,
    SVGA_FIFO_FLAGS,
    // Valid with SVGA_FIFO_CAP_FENCE:
    SVGA_FIFO_FENCE,

    /*
     * Block 3a (optional extended registers): Additional registers for the
     * extended FIFO, whose presence isn't actually implied by
     * SVGA_CAP_EXTENDED_FIFO; these exist if SVGA_FIFO_MIN is high enough to
     * leave room for them.
     *
     * These in block 3a, the VMX currently considers mandatory for the
     * extended FIFO.
     */

     // Valid if exists (i.e. if extended FIFO enabled):
    SVGA_FIFO_3D_HWVERSION,       /* See SVGA3dHardwareVersion in svga3d_reg.h */
    // Valid with SVGA_FIFO_CAP_PITCHLOCK:
    SVGA_FIFO_PITCHLOCK,

    // Valid with SVGA_FIFO_CAP_CURSOR_BYPASS_3:
    SVGA_FIFO_CURSOR_ON,          /* Cursor bypass 3 show/hide register */
    SVGA_FIFO_CURSOR_X,           /* Cursor bypass 3 x register */
    SVGA_FIFO_CURSOR_Y,           /* Cursor bypass 3 y register */
    SVGA_FIFO_CURSOR_COUNT,       /* Incremented when any of the other 3 change */
    SVGA_FIFO_CURSOR_LAST_UPDATED,/* Last time the host updated the cursor */

    // Valid with SVGA_FIFO_CAP_RESERVE:
    SVGA_FIFO_RESERVED,           /* Bytes past NEXT_CMD with real contents */

    /*
     * Valid with SVGA_FIFO_CAP_SCREEN_OBJECT or SVGA_FIFO_CAP_SCREEN_OBJECT_2:
     *
     * By default this is SVGA_ID_INVALID, to indicate that the cursor
     * coordinates are specified relative to the virtual root. If this
     * is set to a specific screen ID, cursor position is reinterpreted
     * as a signed offset relative to that screen's origin.
     */
    SVGA_FIFO_CURSOR_SCREEN_ID,

    /*
     * Valid with SVGA_FIFO_CAP_DEAD
     *
     * An arbitrary value written by the host, drivers should not use it.
     */
    SVGA_FIFO_DEAD,

    /*
     * Valid with SVGA_FIFO_CAP_3D_HWVERSION_REVISED:
     *
     * Contains 3D HWVERSION (see SVGA3dHardwareVersion in svga3d_reg.h)
     * on platforms that can enforce graphics resource limits.
     */
    SVGA_FIFO_3D_HWVERSION_REVISED,

    /*
     * XXX: The gap here, up until SVGA_FIFO_3D_CAPS, can be used for new
     * registers, but this must be done carefully and with judicious use of
     * capability bits, since comparisons based on SVGA_FIFO_MIN aren't
     * enough to tell you whether the register exists: we've shipped drivers
     * and products that used SVGA_FIFO_3D_CAPS but didn't know about some of
     * the earlier ones.  The actual order of introduction was:
     * - PITCHLOCK
     * - 3D_CAPS
     * - CURSOR_* (cursor bypass 3)
     * - RESERVED
     * So, code that wants to know whether it can use any of the
     * aforementioned registers, or anything else added after PITCHLOCK and
     * before 3D_CAPS, needs to reason about something other than
     * SVGA_FIFO_MIN.
     */

     /*
      * 3D caps block space; valid with 3D hardware version >=
      * SVGA3D_HWVERSION_WS6_B1.
      */
    SVGA_FIFO_3D_CAPS = 32,
    SVGA_FIFO_3D_CAPS_LAST = 32 + 255,

    /*
     * End of VMX's current definition of "extended-FIFO registers".
     * Registers before here are always enabled/disabled as a block; either
     * the extended FIFO is enabled and includes all preceding registers, or
     * it's disabled entirely.
     *
     * Block 3b (truly optional extended registers): Additional registers for
     * the extended FIFO, which the VMX already knows how to enable and
     * disable with correct granularity.
     *
     * Registers after here exist if and only if the guest SVGA driver
     * sets SVGA_FIFO_MIN high enough to leave room for them.
     */

     // Valid if register exists:
    SVGA_FIFO_GUEST_3D_HWVERSION, /* Guest driver's 3D version */
    SVGA_FIFO_FENCE_GOAL,         /* Matching target for SVGA_IRQFLAG_FENCE_GOAL */
    SVGA_FIFO_BUSY,               /* See "FIFO Synchronization Registers" */

    /*
     * Always keep this last.  This defines the maximum number of
     * registers we know about.  At power-on, this value is placed in
     * the SVGA_REG_MEM_REGS register, and we expect the guest driver
     * to allocate this much space in FIFO memory for registers.
     */
    SVGA_FIFO_NUM_REGS
};

#define SVGA_MAGIC         0x900000UL
#define SVGA_MAKE_ID(ver)  (SVGA_MAGIC << 8 | (ver))

/* Version 2 let the address of the frame buffer be unsigned on Win32 */
#define SVGA_VERSION_2     2
#define SVGA_ID_2          SVGA_MAKE_ID(SVGA_VERSION_2)

/* Version 1 has new registers starting with SVGA_REG_CAPABILITIES so
   PALETTE_BASE has moved */
#define SVGA_VERSION_1     1
#define SVGA_ID_1          SVGA_MAKE_ID(SVGA_VERSION_1)

   /* Version 0 is the initial version */
#define SVGA_VERSION_0     0
#define SVGA_ID_0          SVGA_MAKE_ID(SVGA_VERSION_0)



/*
 *  Capabilities
 *
 *  Note the holes in the bitfield. Missing bits have been deprecated,
 *  and must not be reused. Those capabilities will never be reported
 *  by new versions of the SVGA device.
 *
 * SVGA_CAP_GMR2 --
 *    Provides asynchronous commands to define and remap guest memory
 *    regions.  Adds device registers SVGA_REG_GMRS_MAX_PAGES and
 *    SVGA_REG_MEMORY_SIZE.
 *
 * SVGA_CAP_SCREEN_OBJECT_2 --
 *    Allow screen object support, and require backing stores from the
 *    guest for each screen object.
 */

#define SVGA_CAP_NONE               0x00000000
#define SVGA_CAP_RECT_COPY          0x00000002
#define SVGA_CAP_CURSOR             0x00000020
#define SVGA_CAP_CURSOR_BYPASS      0x00000040   // Legacy (Use Cursor Bypass 3 instead)
#define SVGA_CAP_CURSOR_BYPASS_2    0x00000080   // Legacy (Use Cursor Bypass 3 instead)
#define SVGA_CAP_8BIT_EMULATION     0x00000100
#define SVGA_CAP_ALPHA_CURSOR       0x00000200
#define SVGA_CAP_3D                 0x00004000
#define SVGA_CAP_EXTENDED_FIFO      0x00008000
#define SVGA_CAP_MULTIMON           0x00010000   // Legacy multi-monitor support
#define SVGA_CAP_PITCHLOCK          0x00020000
#define SVGA_CAP_IRQMASK            0x00040000
#define SVGA_CAP_DISPLAY_TOPOLOGY   0x00080000   // Legacy multi-monitor support
#define SVGA_CAP_GMR                0x00100000
#define SVGA_CAP_TRACES             0x00200000
#define SVGA_CAP_GMR2               0x00400000
#define SVGA_CAP_SCREEN_OBJECT_2    0x00800000


 /*
  * FIFO Capabilities
  *
  *      Fence -- Fence register and command are supported
  *      Accel Front -- Front buffer only commands are supported
  *      Pitch Lock -- Pitch lock register is supported
  *      Video -- SVGA Video overlay units are supported
  *      Escape -- Escape command is supported
  *
  * XXX: Add longer descriptions for each capability, including a list
  *      of the new features that each capability provides.
  *
  * SVGA_FIFO_CAP_SCREEN_OBJECT --
  *
  *    Provides dynamic multi-screen rendering, for improved Unity and
  *    multi-monitor modes. With Screen Object, the guest can
  *    dynamically create and destroy 'screens', which can represent
  *    Unity windows or virtual monitors. Screen Object also provides
  *    strong guarantees that DMA operations happen only when
  *    guest-initiated. Screen Object deprecates the BAR1 guest
  *    framebuffer (GFB) and all commands that work only with the GFB.
  *
  *    New registers:
  *       FIFO_CURSOR_SCREEN_ID, VIDEO_DATA_GMRID, VIDEO_DST_SCREEN_ID
  *
  *    New 2D commands:
  *       DEFINE_SCREEN, DESTROY_SCREEN, DEFINE_GMRFB, BLIT_GMRFB_TO_SCREEN,
  *       BLIT_SCREEN_TO_GMRFB, ANNOTATION_FILL, ANNOTATION_COPY
  *
  *    New 3D commands:
  *       BLIT_SURFACE_TO_SCREEN
  *
  *    New guarantees:
  *
  *       - The host will not read or write guest memory, including the GFB,
  *         except when explicitly initiated by a DMA command.
  *
  *       - All DMA, including legacy DMA like UPDATE and PRESENT_READBACK,
  *         is guaranteed to complete before any subsequent FENCEs.
  *
  *       - All legacy commands which affect a Screen (UPDATE, PRESENT,
  *         PRESENT_READBACK) as well as new Screen blit commands will
  *         all behave consistently as blits, and memory will be read
  *         or written in FIFO order.
  *
  *         For example, if you PRESENT from one SVGA3D surface to multiple
  *         places on the screen, the data copied will always be from the
  *         SVGA3D surface at the time the PRESENT was issued in the FIFO.
  *         This was not necessarily true on devices without Screen Object.
  *
  *         This means that on devices that support Screen Object, the
  *         PRESENT_READBACK command should not be necessary unless you
  *         actually want to read back the results of 3D rendering into
  *         system memory. (And for that, the BLIT_SCREEN_TO_GMRFB
  *         command provides a strict superset of functionality.)
  *
  *       - When a screen is resized, either using Screen Object commands or
  *         legacy multimon registers, its contents are preserved.
  *
  * SVGA_FIFO_CAP_GMR2 --
  *
  *    Provides new commands to define and remap guest memory regions (GMR).
  *
  *    New 2D commands:
  *       DEFINE_GMR2, REMAP_GMR2.
  *
  * SVGA_FIFO_CAP_3D_HWVERSION_REVISED --
  *
  *    Indicates new register SVGA_FIFO_3D_HWVERSION_REVISED exists.
  *    This register may replace SVGA_FIFO_3D_HWVERSION on platforms
  *    that enforce graphics resource limits.  This allows the platform
  *    to clear SVGA_FIFO_3D_HWVERSION and disable 3D in legacy guest
  *    drivers that do not limit their resources.
  *
  *    Note this is an alias to SVGA_FIFO_CAP_GMR2 because these indicators
  *    are codependent (and thus we use a single capability bit).
  *
  * SVGA_FIFO_CAP_SCREEN_OBJECT_2 --
  *
  *    Modifies the DEFINE_SCREEN command to include a guest provided
  *    backing store in GMR memory and the bytesPerLine for the backing
  *    store.  This capability requires the use of a backing store when
  *    creating screen objects.  However if SVGA_FIFO_CAP_SCREEN_OBJECT
  *    is present then backing stores are optional.
  *
  * SVGA_FIFO_CAP_DEAD --
  *
  *    Drivers should not use this cap bit.  This cap bit can not be
  *    reused since some hosts already expose it.
  */

#define SVGA_FIFO_CAP_NONE                  0
#define SVGA_FIFO_CAP_FENCE             (1<<0)
#define SVGA_FIFO_CAP_ACCELFRONT        (1<<1)
#define SVGA_FIFO_CAP_PITCHLOCK         (1<<2)
#define SVGA_FIFO_CAP_VIDEO             (1<<3)
#define SVGA_FIFO_CAP_CURSOR_BYPASS_3   (1<<4)
#define SVGA_FIFO_CAP_ESCAPE            (1<<5)
#define SVGA_FIFO_CAP_RESERVE           (1<<6)
#define SVGA_FIFO_CAP_SCREEN_OBJECT     (1<<7)
#define SVGA_FIFO_CAP_GMR2              (1<<8)
#define SVGA_FIFO_CAP_3D_HWVERSION_REVISED  SVGA_FIFO_CAP_GMR2
#define SVGA_FIFO_CAP_SCREEN_OBJECT_2   (1<<9)
#define SVGA_FIFO_CAP_DEAD              (1<<10)

 /*
  * 3D Hardware Version
  *
  *   The hardware version is stored in the SVGA_FIFO_3D_HWVERSION fifo
  *   register.   Is set by the host and read by the guest.  This lets
  *   us make new guest drivers which are backwards-compatible with old
  *   SVGA hardware revisions.  It does not let us support old guest
  *   drivers.  Good enough for now.
  *
  */

#define SVGA3D_MAKE_HWVERSION(major, minor)      (((major) << 16) | ((minor) & 0xFF))
#define SVGA3D_MAJOR_HWVERSION(version)          ((version) >> 16)
#define SVGA3D_MINOR_HWVERSION(version)          ((version) & 0xFF)

typedef enum {
    SVGA3D_HWVERSION_WS5_RC1 = SVGA3D_MAKE_HWVERSION(0, 1),
    SVGA3D_HWVERSION_WS5_RC2 = SVGA3D_MAKE_HWVERSION(0, 2),
    SVGA3D_HWVERSION_WS51_RC1 = SVGA3D_MAKE_HWVERSION(0, 3),
    SVGA3D_HWVERSION_WS6_B1 = SVGA3D_MAKE_HWVERSION(1, 1),
    SVGA3D_HWVERSION_FUSION_11 = SVGA3D_MAKE_HWVERSION(1, 4),
    SVGA3D_HWVERSION_WS65_B1 = SVGA3D_MAKE_HWVERSION(2, 0),
    SVGA3D_HWVERSION_WS8_B1 = SVGA3D_MAKE_HWVERSION(2, 1),
    SVGA3D_HWVERSION_CURRENT = SVGA3D_HWVERSION_WS8_B1,
} SVGA3dHardwareVersion;


#define SVGA_VIDEO_COLORKEY_MASK             0x00ffffff

#define SVGA_ESCAPE_VMWARE_VIDEO             0x00020000

#define SVGA_ESCAPE_VMWARE_VIDEO_SET_REGS    0x00020001
/* FIFO escape layout:
 * Type, Stream Id, (Register Id, Value) pairs */

#define SVGA_ESCAPE_VMWARE_VIDEO_FLUSH       0x00020002
 /* FIFO escape layout:
  * Type, Stream Id */

/*
 * Namespace IDs for the escape command
 */

#define SVGA_ESCAPE_NSID_VMWARE 0x00000000
#define SVGA_ESCAPE_NSID_DEVEL  0xFFFFFFFF

typedef
struct SVGAEscapeVideoFlush {
	uint32_t cmdType;
	uint32_t streamId;
} SVGAEscapeVideoFlush;

typedef
struct {
    uint32_t id;             // Reserved, must be zero.
    uint32_t hotspotX;
    uint32_t hotspotY;
    uint32_t width;
    uint32_t height;
    uint32_t andMaskDepth;   // Value must be 1 or equal to BITS_PER_PIXEL
    uint32_t xorMaskDepth;   // Value must be 1 or equal to BITS_PER_PIXEL
    /*
     * Followed by scanline data for AND mask, then XOR mask.
     * Each scanline is padded to a 32-bit boundary.
    */
} SVGAFifoCmdDefineCursor;

typedef
struct {
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
} SVGAFifoCmdUpdate;

class HAL;
class VMware_SVGA2 : public Driver, public VideoDevice
{

public:
	VMware_SVGA2(PCIDevice* device, HAL* hal);

	DriverResult Activate() override;

	DriverResult Deactivate() override;
	DriverResult Initialize() override;
	DriverResult Reset() override;


	std::string get_vendor_name() override;
	std::string get_device_name() override;


	void SetResolution(uint16_t w, uint16_t h);

    void DefineCursor(uint8_t* bitmap, uint32_t w, uint32_t h) override;
    void UpdateRect(gfx::Rectangle rect) override;
    void MoveCursor(uint32_t visible, uint32_t x, uint32_t y, uint32_t screenId) override;
    

    uint32_t GetScreenWidth() override;
    uint32_t GetScreenHeight() override;

    gfx::FrameBuffer* GetFramebuffer() override;

    uint32_t* AllocBuffer(uint32_t width, uint32_t height) override;
    void FreeBuffer(uint32_t* buff) override;


    void BlitBuffer(uint32_t* buffer, gfx::Rectangle rect) override;

private:

	void WriteReg(int reg, int val);
	uint32_t ReadReg(int reg);

	bool HasFIFOCap(int cap);
	bool IsFIFORegValid(int reg);

	void* FIFOReserve(uint32_t bytes);

    void Panic(const char* str);

    /*
     *-----------------------------------------------------------------------------
     *
     * FIFOFull --
     *
     *      This function is called repeatedly as long as the FIFO has too
     *      little free space for us to continue.
     *
     *      The simplest implementation of this function is a no-op.  This
     *      will just burn guest CPU until space is available. (That's a
     *      bad idea, since the host probably needs that CPU in order to
     *      make progress on emptying the FIFO.)
     *
     *      A better implementation would sleep until a FIFO progress
     *      interrupt occurs. Depending on the OS you're writing drivers
     *      for, this may deschedule the calling task or it may simply put
     *      the CPU to sleep.
     *
     * Results:
     *      None.
     *
     * Side effects:
     *      None.
     *
     *-----------------------------------------------------------------------------
     */

    void FIFOFull(void);

    /*
     *-----------------------------------------------------------------------------
     *
     *  FIFOReserveEscape --
     *
     *      This is a convenience wrapper around SVGA_FIFOReserve, which
     *      prefixes the reserved memory block with an ESCAPE command header.
     *
     *      ESCAPE commands are a way of encoding extensible and
     *      variable-length packets within the basic FIFO protocol
     *      itself. ESCAPEs are used for some SVGA device functionality,
     *      like video overlays, for VMware's internal debugging tools,
     *      and for communicating with third party code that can load into
     *      the SVGA device.
     *
     * Results:
     *      Always returns a pointer to 'bytes' bytes of reserved space in the FIFO.
     *
     * Side effects:
     *      Begins a FIFO command, reserves space in the FIFO. Writes a
     *      3-word header into the FIFO.  May block (in SVGAFIFOFull) if
     *      the FIFO is full.
     *
     *-----------------------------------------------------------------------------
     */

    void* FIFOReserveEscape(uint32_t nsid,   // IN
            uint32_t bytes);  // IN

    /*
     *-----------------------------------------------------------------------------
     *
     * FIFOCommitAll --
     *
     *      This is a convenience wrapper for SVGA_FIFOCommit(), which
     *      always commits the last reserved block in its entirety.
     *
     * Results:
     *      None.
     *
     * Side effects:
     *      SVGA_FIFOCommit.
     *
     *-----------------------------------------------------------------------------
     */
    void FIFOCommitAll(void);

    /*
     *-----------------------------------------------------------------------------
     *
     * SVGA_FIFOCommit --
     *
     *      Commit a block of FIFO data which was placed in the buffer
     *      returned by SVGA_FIFOReserve. Every Reserve must be paired
     *      with exactly one Commit, but the sizes don't have to match.
     *      The caller is free to commit less space than they
     *      reserved. This can be used if the command size isn't known in
     *      advance, but it is reasonable to make a worst-case estimate.
     *
     *      The commit size does not have to match the size of a single
     *      FIFO command. This can be used to write a partial command, or
     *      to write multiple commands at once.
     *
     * Results:
     *      None.
     *
     * Side effects:
     *      None.
     *
     *-----------------------------------------------------------------------------
     */

    void FIFOCommit(uint32_t bytes);  // IN

	void* FIFOReserveCmd(uint32_t type,   // IN
			uint32_t bytes);  // IN

private:
	uint32_t m_IObaseAddr;
	uint64_t m_FIFObaseAddr;
    uint64_t m_FBAddr;

	HAL* m_HAL;

	uint32_t bytesPerLine, w, h;
	size_t m_VRamSize;
	size_t m_FBSize;
	size_t m_FifoSize;
    	

	uint32_t     deviceVersionId;
	uint32_t     capabilities;

	struct {
		uint32_t  reservedSize;
		bool    usingBounceBuffer;
		uint8_t   bounceBuffer[1024 * 1024];
		uint32_t  nextFence;
	} fifo;


	gfx::LinearFrameBuffer* m_Framebuffer;

};