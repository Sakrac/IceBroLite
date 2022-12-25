#pragma once

struct CPU6510;

enum AddressModes {
	// address mode bit index

	// 6502

	AM_ZP_REL_X,	// 0 ($12,x)
	AM_ZP,			// 1 $12
	AM_IMM,			// 2 #$12
	AM_ABS,			// 3 $1234
	AM_ZP_Y_REL,	// 4 ($12),y
	AM_ZP_X,		// 5 $12,x
	AM_ABS_Y,		// 6 $1234,y
	AM_ABS_X,		// 7 $1234,x
	AM_REL,			// 8 ($1234)
	AM_ACC,			// 9 A
	AM_NON,			// a
	AM_BRANCH,		// b $1234
	AM_ZP_REL_Y,	// c
	AM_ZP_Y,		// d
	AM_COUNT,
};

enum class InstrRefType : uint8_t {
	None,
	Flags,
	Register,
	DataValue,
	DataArray,
	Code,
};


int Disassemble(CPU6510* cpu, uint16_t addr, char* dest, int left, int& chars, int& branchTrg, bool showBytes, bool illegals, bool showLabels, bool showDis);
int Assemble(CPU6510* cpu, char* cmd, uint16_t addr);
bool GetWatchRef(CPU6510* cpu, uint16_t addr, int style, char* buf, size_t bufCap);
InstrRefType GetRefType(CPU6510* cpu, uint16_t addr);
uint16_t InstrRefAddr(CPU6510* cpu, uint16_t addr);
int InstrRef(CPU6510* cpu, uint16_t pc, char* buf, size_t bufSize);
int InstructionBytes(CPU6510* cpu, uint16_t addr, bool illegals = true);
