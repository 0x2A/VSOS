#pragma once
#include <stdint.h>
#include <gfx\Types.h>

namespace gfx
{
	class FrameBuffer;
}
class VideoDevice
{
public:

	virtual void DefineCursor(uint8_t* bitmap, uint32_t w, uint32_t h) = 0;
	virtual void UpdateRect(gfx::Rectangle rect) = 0;
	virtual void MoveCursor(uint32_t visible,
		uint32_t x,  
		uint32_t y,  
		uint32_t screenId) = 0;
	virtual uint32_t GetScreenWidth() = 0;
	virtual uint32_t GetScreenHeight() = 0;
	virtual gfx::FrameBuffer* GetFramebuffer() = 0;
	
	virtual uint32_t* AllocBuffer(uint32_t width, uint32_t height) = 0;
	virtual void FreeBuffer(uint32_t* buff) = 0;

	virtual void BlitBuffer(uint32_t* buffer, gfx::Rectangle rect) = 0;
};