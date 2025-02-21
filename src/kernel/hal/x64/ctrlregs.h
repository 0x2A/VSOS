#pragma once

extern "C"
{
	void _pause();
	void _ltr(uint16_t entry);
	void _sti();
	void _cli();

	cpu_flags_t DisableInterrupts();
	void RestoreFlags(cpu_flags_t flags);

	_declspec(noreturn)
	void UpdateSegments(uint16_t data_selector, uint16_t code_selector);
}
