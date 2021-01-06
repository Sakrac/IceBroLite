#include <cstring>
#include "6510.h"
#include <malloc.h>

// for now support 1 CPU

static CPU6510* sp6510 = nullptr;


CPU6510::CPU6510() : space(VICE_MainMemory), memoryRequestsPending(0), memoryFlush(false)
{
	IBMutexInit(&memoryUpdateMutex, "CPU memory sync");
	ram = (uint8_t*)malloc(64 * 1024);
}

void CPU6510::RefreshMemory()
{
	IBMutexLock(&memoryUpdateMutex);
	if (memoryFlush) { currentMem.numRanges = 0; memoryFlush = false; }
	if (memoryRequestsPending == 0) {
		MemRanges delta;
		delta.Delta(currentMem, requestedMem);

		for (size_t i = 0; i < delta.numRanges; ++i) {
			if (ViceGetMemory(delta.start[i], delta.end[i], space)) {
				++memoryRequestsPending;
			}
		}
	} else {
		ViceWaiting();
	}
	IBMutexRelease(&memoryUpdateMutex);
}

void CPU6510::MemoryFromVICE(uint16_t start, uint16_t end, uint8_t *bytes)
{
	IBMutexLock(&memoryUpdateMutex);
	if (memoryRequestsPending) { --memoryRequestsPending; }
	currentMem.Add(start, end);
	memcpy(ram + start, bytes, end - start + 1);
	IBMutexRelease(&memoryUpdateMutex);
}

uint8_t CPU6510::GetByte(uint16_t addr)
{
	// 
	requestedMem.Add(addr);
	return ram[addr];
}

void CPU6510::SetByte(uint16_t addr, uint8_t byte)
{
	// TODO: Send byte to VICE
	ram[addr] = byte;
}

void CPU6510::SetPC(uint16_t pc)
{
	// TODO: Set PC in Vice
	regs.PC = pc;
}

void CreateMainCPU()
{
	if (sp6510 == nullptr) {
		sp6510 = new CPU6510;
	}
}

void ShutdownMainCPU()
{
	if (sp6510 != nullptr) {
		delete sp6510;
		sp6510 = nullptr;
	}
}

CPU6510* GetMainCPU()
{
	return sp6510;
}

CPU6510* GetCurrCPU()
{
	return sp6510;
}

CPU6510* GetCPU(VICEMemSpaces space)
{
	return sp6510;
}

void MemRanges::Insert(size_t slot)
{
	for (size_t i = numRanges; i > slot; --i) {
		start[i] = start[i - 1];
		end[i] = end[i - 1];
	}
	++numRanges;
}

void MemRanges::Remove(size_t slot)
{
	for (size_t i = slot, n = numRanges - 1; i < n; ++i) {
		start[i] = start[i + 1];
		end[i] = end[i + 1];
	}
	--numRanges;
}

void MemRanges::Delta(const MemRanges& orig, const MemRanges& next)
{
	numRanges = 0;
	size_t on = orig.numRanges, nn = next.numRanges;
	size_t di = 0;

	// anything in next that is not in orig
	for (size_t ni = 0; ni < nn; ++ni) {
		if (!orig.Exists(next.start[ni], next.end[ni]))
		{
			uint16_t s = next.start[ni];
			uint16_t e = next.end[ni];
			bool done = false;
			for (size_t oi = 0; oi < on && !done; ++oi) {
				if (s < orig.start[oi] && e >= orig.start[oi]) {
					start[di] = s;
					end[di] = orig.start[oi] - 1;
					++di;
					if (e <= orig.end[oi]) { done = true;  break; }
					else { s = orig.end[oi] + 1; }
				}
				if (!done && s <= orig.end[oi] && e > orig.end[oi]) {
					start[di] = orig.end[oi] + 1;
					end[di] = e;
					++di;
					done = true;
				}
			}
			if (!done) {
				start[di] = s;
				end[di] = e;
				++di;
			}
		}
	}
	numRanges = di;
}

bool MemRanges::Exists(uint16_t a0, uint16_t a1) const
{
	// address already exists?
	size_t i = 0, n = numRanges;
	for (; i < n; ++i) {
		if (a0 >= start[i] && a1 <= end[i]) { return true; }
		//else if (a0 > end[i]) { return false; }
	}
	return false;
}

void MemRanges::Add(uint16_t a0, uint16_t a1)
{
	size_t i = 0, c = numRanges;
	for (; i < c; ++i) {
		if (a0 < end[i]) { break; }
		if (a0 >= start[i] && a1 <= end[i]) { return; }
	}

	if (i < c) { // a0 < end[i]
		if (a0 < start[i]) {
			if (a1 >= start[i] || (start[i] - a1) < 64) {
				start[i] = a0;
				if (a1 > end[i]) { end[i] = a1; }
			} else if (i && (a0-end[i-1])<64) {
				end[i - 1] = a1;
			} else {
				Insert(i);
				start[i] = a0;
				end[i] = a1;
			}
		} else {
			if (end[i] < a1) { end[i] = a1; }
		}
	} else if (i) {
		size_t l = c - 1;
		if ((a0 - end[l]) < 64) {
			end[l] = a1;
		} else {
			if (numRanges < MaxRanges) {
				numRanges++;
				start[i] = a0;
				end[i] = a1;
			} else {
				end[i - 1] = a1;
			}
		}
	} else {
		start[0] = a0;
		end[0] = a1;
		numRanges = 1;
	}
}

void MemRanges::Merge(MemRanges& delta)
{
	for (size_t i = 0, n = delta.numRanges; i < n; ++i) {
		Add(delta.start[i], delta.end[i]);
	}
}



void MemRanges::Add(uint16_t addr)
{
	// address already exists?
	size_t i = 0, n = numRanges;
	for (; i < n; ++i) {
		if (addr >= start[i] && addr <= end[i]) { return; }
		if (addr < end[i]) { break; }
	}

	// slot i is higher than i, check if merge with neighbours
	if (i < n) {
		if ((start[i] - addr) < 64) {
			start[i] = addr;
			if (i && (addr - end[i - 1] < 64)) {
				end[i - 1] = end[i];
				Remove(i);
			}
			return;
		} else if( i && (addr-end[i-1]) < 64) {
			end[i - 1] = addr;
			return;
		} else {
			Insert(i);
			start[i] = addr;
			end[i] = addr;
			return;
		}
	} else if(i) {
		if ((addr - end[i - 1]) < 64) {
			end[i - 1] = addr;
			return;
		}
	}
	if ((i + 1) < n) {
		if ((start[i + 1] - addr) < 64) {
			start[i + 1] = addr;
			return;
		}
	}

	if (numRanges >= MaxRanges) {
		start[0] = 0; end[9] = 0xffff; numRanges = 1;
		return;
	}

	Insert(i);
	start[i] = addr;
	end[i] = addr;
}

