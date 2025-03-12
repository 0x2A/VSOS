#include "LoadingScreen.h"
#include <kernel\Kernel.h>

using namespace gfx;

LoadingScreen::LoadingScreen(gfx::FrameBuffer* frameBuffer) :
	StringPrinter(),
	m_frameBuffer(frameBuffer),
	yPos(35), xPos(0), xOffset(5)
{

}

void LoadingScreen::Initialize()
{
	//Clear frame and draw border
	m_frameBuffer->FillScreen(Background);

	m_frameBuffer->DrawFrameBorder(Border, 3);
	m_frameBuffer->DrawText({ 9, 9 }, "VSOS " KERNEL_VERSION " " KERNEL_DESCR, Foreground);
	m_frameBuffer->DrawRectangle(Border, { 0, 30, m_frameBuffer->GetWidth(), 3 });
}

void LoadingScreen::Write(const std::string& string)
{
	Point2D point = m_frameBuffer->DrawText({ xPos, yPos }, string, Colors::White, true);
	yPos = point.Y;
	xPos = point.X;
	if(xPos == 0) xPos += xOffset;

	
}

void LoadingScreen::SetFramebuffer(gfx::FrameBuffer* fb)
{
	m_frameBuffer = fb;
	xPos = yPos = 0;
}

void LoadingScreen::RemoveChar()
{
	m_frameBuffer->DrawRectangle(gfx::Colors::Black, {xPos-8, yPos, 8, 16 });
	xPos -= 8;
}
