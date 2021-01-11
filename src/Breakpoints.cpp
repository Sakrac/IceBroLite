// breakpoints are added in bulk when VICE stops
// breakpoints can be added by the debugger

#include "platform.h"
#include "Breakpoints.h"
#include "HashTable.h"

static IBMutex sBreakpointMutex;
static std::vector<Breakpoint> sBreakpoints;
static HashTable<uint16_t, uint32_t> sBreakpointLookup;	// address, VICE breakpoint index

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
	sBreakpointLookup.Clear();
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
	if ((flags & Breakpoint::Exec) && sBreakpointLookup.Value(start) == nullptr) {
		sBreakpointLookup.Insert(start, number);
	}
	IBMutexRelease(&sBreakpointMutex);
}
void RemoveBreakpoint(uint32_t number)
{
	IBMutexLock(&sBreakpointMutex);
	for (size_t i = 0; i < sBreakpoints.size(); ++i) {
		if (sBreakpoints[i].number == number) {
			if (uint32_t* num = sBreakpointLookup.Value(sBreakpoints[i].start)) {
				if (*num == number) {
					// TODO: Add remove hash table slot!
				}
			}
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

bool BreakpointAt(uint16_t address, Breakpoint& bp)
{
	if (uint32_t* number = sBreakpointLookup.Value(address)) {
		IBMutexLock(&sBreakpointMutex);
		uint32_t num = *number;
		for (size_t index = 0, n = sBreakpoints.size(); index < n; ++index) {
			if (sBreakpoints[index].number == num) {
				bp = sBreakpoints[index];
				IBMutexRelease(&sBreakpointMutex);
				return true;
			}
		}
		IBMutexRelease(&sBreakpointMutex);
	}
	return false;
}