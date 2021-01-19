#include <cstring>
#include "struse/struse.h"
#include "6510.h"
#include <malloc.h>

// for now support 1 CPU

static CPU6510* sp6510 = nullptr;


CPU6510::CPU6510() : space(VICE_MainMemory)
{
	IBMutexInit(&memoryUpdateMutex, "CPU memory sync");
	ram = (uint8_t*)calloc(1, 64 * 1024);
}

void CPU6510::MemoryFromVICE(uint16_t start, uint16_t end, uint8_t *bytes)
{
	if (end < start) { return; }
	IBMutexLock(&memoryUpdateMutex);
	memcpy(ram + start, bytes, end - start + 1);
	IBMutexRelease(&memoryUpdateMutex);
}
uint8_t CPU6510::GetByte(uint16_t addr)
{
	// 
	return ram[addr];
}

void CPU6510::SetByte(uint16_t addr, uint8_t byte)
{
	// TODO: Send byte to VICE
	ram[addr] = byte;
	ViceSetMemory(addr, 1, ram + addr, space);
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

