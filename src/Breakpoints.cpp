// breakpoints are added in bulk when VICE stops
// breakpoints can be added by the debugger

#include <stdlib.h>
#include "platform.h"
#include "Breakpoints.h"
#include "HashTable.h"
#include "struse/struse.h"
#include "ViceInterface.h"

#ifndef _WIN32
#define _strdup strdup
#endif


static IBMutex sBreakpointMutex;
static std::vector<Breakpoint> sBreakpoints;
static std::vector<uint32_t> sCurrentBreakpoints;
static HashTable<uint16_t, uint32_t> sBreakpointLookup;	// address, VICE breakpoint index

void InitBreakpoints()
{
	IBMutexInit(&sBreakpointMutex, "Breakpoints");
}

void ShutdownBreakpoints()
{
	sBreakpoints.clear();
	ClearBreapointsHit();
	IBMutexDestroy(&sBreakpointMutex);
}

void ClearBreakpoints()
{
	IBMutexLock(&sBreakpointMutex);
	sBreakpoints.clear();
	sBreakpointLookup.Clear();
	IBMutexRelease(&sBreakpointMutex);
}

bool BreakpointCurrent(uint32_t number)
{
	for (size_t i = 0, n = sCurrentBreakpoints.size(); i < n; ++i) {
		if (sCurrentBreakpoints[i] == number) {
			return true;
		}
	}
	return false;
}

void SetBreakpointHit(uint32_t number)
{
	if (!BreakpointCurrent(number)) {
		sCurrentBreakpoints.push_back(number);
	}
}

void ClearBreapointsHit()
{
	sCurrentBreakpoints.clear();
}

void AddBreakpoint(uint32_t number, uint32_t flags, uint16_t start, uint16_t end, const char* condition)
{
	// breakpoints with multiple settings are duplicated so merge them by number
	IBMutexLock(&sBreakpointMutex);
	for (size_t i = 0; i < sBreakpoints.size(); ++i) {
		if (sBreakpoints[i].number == number) {
			sBreakpoints[i].flags |= flags;
			if (sBreakpoints[i].condition) {
				free((void*)sBreakpoints[i].condition);
				sBreakpoints[i].condition = nullptr;
			}
			if (condition) { sBreakpoints[i].condition = _strdup(condition);	}
			IBMutexRelease(&sBreakpointMutex);
			return;
		}
	}
	Breakpoint bp = { number, flags, start, end, nullptr };
	if (condition) { bp.condition = _strdup(condition); }
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

void RemoveAllBreakpoints()
{
	IBMutexLock(&sBreakpointMutex);
	for (size_t i = 0; i < sBreakpoints.size(); ++i) {
		ViceRemoveBreakpointNoList(sBreakpoints[i].number);
		if (sBreakpoints[i].condition) { free((void*)sBreakpoints[i].condition); }
	}
	sBreakpoints.clear();
	sBreakpointLookup.Clear();
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
	IBMutexLock(&sBreakpointMutex);
	if (uint32_t* number = sBreakpointLookup.Value(address)) {
		uint32_t num = *number;
		for (size_t index = 0, n = sBreakpoints.size(); index < n; ++index) {
			if (sBreakpoints[index].number == num) {
				bp = sBreakpoints[index];
				IBMutexRelease(&sBreakpointMutex);
				return true;
			}
		}
	}
	IBMutexRelease(&sBreakpointMutex);
	return false;
}