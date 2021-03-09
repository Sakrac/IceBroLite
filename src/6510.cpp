#include <cstring>
#include "struse/struse.h"
#include "6510.h"
#include "Files.h"
#include <malloc.h>

// for now support 1 CPU

static CPU6510* sp6510 = nullptr;


CPU6510::CPU6510() : space(VICEMemSpaces::MainMemory), memoryChanged(false)
{
	IBMutexInit(&memoryUpdateMutex, "CPU memory sync");
	ram = (uint8_t*)calloc(1, 64 * 1024);
}

void CPU6510::MemoryFromVICE(uint16_t start, uint16_t end, uint8_t *bytes)
{
	if (end < start) { return; }
	IBMutexLock(&memoryUpdateMutex);
	memcpy(ram + start, bytes, (size_t)end - (size_t)start + 1);
	memoryChanged = true;
	IBMutexRelease(&memoryUpdateMutex);
}
uint8_t CPU6510::GetByte(uint16_t addr)
{
	return ram[addr];
}

void CPU6510::SetByte(uint16_t addr, uint8_t byte)
{
	ram[addr] = byte;
	memoryChanged = true;
	ViceSetMemory(addr, 1, ram + addr, space);
}

void CPU6510::CopyToRAM(uint16_t address, uint8_t* data, size_t size)
{
	uint32_t bytes = 0x10000 - address;
	if (size_t(bytes) > size) { bytes = (uint32_t)size; }
	memcpy(ram + address, data, bytes);
	memoryChanged = true;
	ViceSetMemory(address, bytes, ram + address, space);
}

void CPU6510::ReadPRGToRAM(const char *filename)
{
	if (filename) {
		size_t size;
		if (uint8_t* file = LoadBinary(filename, size)) {
			if (size > 2) {
				uint16_t addr = file[0] + (((uint16_t)file[1]) << 8);
				CopyToRAM(addr, file + 2, size - 2);
				free(file);
			}
		}
	}
}
void CPU6510::SetPC(uint16_t pc)
{
	regs.PC = pc;
	ViceSetRegisters(*this, RM_PC);
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

