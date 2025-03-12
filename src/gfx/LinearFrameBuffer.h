#pragma once

#include "FrameBuffer.h"
#include "os.internal.h"

namespace gfx
{
	//This class is just a Preallocated framebuffer.
	class LinearFrameBuffer : public FrameBuffer
	{
	public:
		LinearFrameBuffer(void* const address, const size_t height, const size_t width) :
			FrameBuffer(),
			m_buffer(reinterpret_cast<Color*>(address)),
			m_height(height),
			m_width(width)
		{

		}

		virtual size_t GetHeight() const override { return m_height; }
		virtual size_t GetWidth() const override { return m_width; }
		virtual Color* GetBuffer() override { return m_buffer; }

		//Write buffer at once
		void Write(FrameBuffer& framebuffer)
		{
			const size_t bufferSize = framebuffer.GetHeight() * framebuffer.GetWidth() * sizeof(Color);
			memcpy(m_buffer, framebuffer.GetBuffer(), bufferSize);
		}

	private:
		Color* const m_buffer;
		const size_t m_height;
		const size_t m_width;

		NO_COPY_OR_ASSIGN(LinearFrameBuffer);
	};
}
