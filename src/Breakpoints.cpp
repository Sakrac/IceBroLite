// breakpoints are added in bulk when VICE stops
// breakpoints can be added by the debugger

#include "platform.h"
#include "Breakpoints.h"

static IBMutex sBreakpointMutex;
static std::vector<Breakpoint> sBreakpoints;


void InitBreakpoints()
{
	IBMutexInit(&sBreakpointMutex, "Breakpoints");
}

void ShutdownBreakpoints()
{
	sBreakpoints.clear();
	IBMutexDestroy(&sBreakpointMutex);
}

void ClearBreakpoints()
{
	IBMutexLock(&sBreakpointMutex);
	sBreakpoints.clear();
	IBMutexRelease(&sBreakpointMutex);
}

void AddBreakpoint(uint32_t number, uint32_t flags, uint16_t start, uint16_t end)
{
	// breakpoints with multiple settings are duplicated so merge them by number
	IBMutexLock(&sBreakpointMutex);
	for (size_t i = 0; i < sBreakpoints.size(); ++i) {
		if (sBreakpoints[i].number == number) {
			sBreakpoints[i].flags |= flags;
			IBMutexRelease(&sBreakpointMutex);
			return;
		}
	}
	Breakpoint bp = { number, flags, start, end };
	sBreakpoints.push_back(bp);
	IBMutexRelease(&sBreakpointMutex);
}
void RemoveBreakpoint(uint32_t number)
{
	IBMutexLock(&sBreakpointMutex);
	for (size_t i = 0; i < sBreakpoints.size(); ++i) {
		if (sBreakpoints[i].number == number) {
			sBreakpoints.erase(sBreakpoints.begin() + i);
			break;
		}
	}
	IBMutexRelease(&sBreakpointMutex);
}

size_t NumBreakpoints()
{
	return sBreakpoints.size();
}
Breakpoint GetBreakpoint(size_t index)
{
	Breakpoint r = { 0xffffffff, 0, 0, 0 };
	IBMutexLock(&sBreakpointMutex);
	if (index < sBreakpoints.size()) {
		r = sBreakpoints[index];
	}
	IBMutexRelease(&sBreakpointMutex);
	return r;
}

