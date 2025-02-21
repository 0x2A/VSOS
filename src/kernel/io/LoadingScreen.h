#pragma once

#include "StringPrinter.h"
#include <gfx\FrameBuffer.h>

//This method is called early on during boot (before a proper heap) and therefore
//writes to the screen directly (versus buffering in UI constructs like window or control).
class LoadingScreen : public StringPrinter
{
public:
	LoadingScreen(gfx::FrameBuffer& frameBuffer);

	void Initialize();
	virtual void Write(const std::string& string) override;
	const gfx::FrameBuffer& GetFramebuffer() const { return m_frameBuffer; }

private:
	static constexpr gfx::Color Foreground = gfx::Colors::White;
	static constexpr gfx::Color Background = gfx::Colors::Black;
	static constexpr gfx::Color Border = gfx::Colors::Blue;

	gfx::FrameBuffer& m_frameBuffer;
	size_t yPos;
	size_t xPos;
	size_t xOffset;
};