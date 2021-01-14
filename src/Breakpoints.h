#pragma once

#include <inttypes.h>
#include <vector>

struct Breakpoint {
	enum Flags {
		Enabled = 0x0001,
		Stop = 0x0002,		// if clear -> tr
		Exec = 0x0004,		// traditional breakpoint
		Load = 0x0008,		// read memory at location
		Store = 0x0010,		// write memory at location
		Current = 0x0020,	// true if this breakpoint stopped exec
		Temporary = 0x0040	// goes away next time it is hit
	};

	uint32_t number;
	uint32_t flags;
	uint16_t start;
	uint16_t end;
};

void InitBreakpoints();
void ShutdownBreakpoints();
void ClearBreakpoints();
void AddBreakpoint(uint32_t number, uint32_t flags, uint16_t start, uint16_t end);
void RemoveBreakpoint(uint32_t number);
void RemoveAllBreakpoints();
size_t NumBreakpoints();
Breakpoint GetBreakpoint(size_t index);
bool BreakpointAt(uint16_t address, Breakpoint& bp);
