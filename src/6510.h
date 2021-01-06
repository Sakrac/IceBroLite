#pragma once

#include <inttypes.h>
#include <stddef.h>

#include "ViceInterface.h"
#include "platform.h"

struct CPU6510 {

	struct Regs {
		uint8_t A, X, Y;
		uint8_t SP, FL, ZP00, ZP01;
		uint16_t PC, LIN, CYC;
		Regs() : A(0), X(0), Y(0), SP(0xff), FL(0), ZP00(0), ZP01(0x37), PC(0x0400), LIN(0), CYC(0) {}
	};

	Regs	regs;
	uint8_t *ram;
	VICEMemSpaces space;

	CPU6510();

	void MemoryFromVICE(uint16_t start, uint16_t end, uint8_t* bytes);

	uint8_t GetByte(uint16_t addr);
	void SetByte(uint16_t addr, uint8_t byte);
	bool MemoryChange() { return false; }
	void SetPC(uint16_t pc);

protected:
	IBMutex memoryUpdateMutex;
	bool memoryChanged;
};

enum StatusFlags {
	F_C = 1,
	F_Z = 2,
	F_I = 4,
	F_D = 8,
	F_B = 16,
	F_U = 32,
	F_V = 64,
	F_N = 128,
};


void CreateMainCPU();
void ShutdownMainCPU();

CPU6510* GetMainCPU();
CPU6510* GetCurrCPU();
CPU6510* GetCPU(VICEMemSpaces space);

