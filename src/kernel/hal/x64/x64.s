.DATA

; https://docs.microsoft.com/en-us/cpp/build/x64-software-conventions?view=vs-2019
; Keep in sync with X64_CONTEXT
Context STRUCT
	_r12 QWORD ?
	_r13 QWORD ?
	_r14 QWORD ?
	_r15 QWORD ?
	_rdi QWORD ?
	_rsi QWORD ?
	_rbx QWORD ?
	_rbp QWORD ?
	_rsp QWORD ?
	_rip QWORD ?
	_rflags QWORD ?
Context ENDS

StackReserve QWORD 20h ; Register Parameter Area