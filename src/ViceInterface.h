#pragma once

struct CPU6510;

enum class VICEMemSpaces {
	MainMemory,
	Drive8,
	Drive9,
	Drive10,
	Drive11
};

enum class VICEEmuType {
  C64,
  Vic20,
  Plus4
};

VICEEmuType ViceGetEmuType();
void ViceSetEmuType(VICEEmuType value);

bool ViceConnected();
bool ViceRunning();

void ViceDisconnect();
void ViceConnect(const char* ip, uint32_t port);
void ViceQuit();

void ViceBreak();
void ViceGo();
void ViceStep();
void ViceStepOver();
void ViceStepOut();
void ViceRunTo(uint16_t addr);
bool ViceGetMemory(uint16_t start, uint16_t end, VICEMemSpaces mem);
bool ViceSetMemory(uint16_t start, uint16_t len, uint8_t* bytes, VICEMemSpaces mem);
bool ViceSetRegisters(const CPU6510& cpu, uint32_t regMask);
void ViceStartProgram(const char* loadPrg);
void ViceReset(uint8_t resetType);
void ViceRemoveBreakpoint(uint32_t number);
void ViceToggleBreakpoint(uint32_t number, bool enable);
void ViceAddCheckpoint(uint16_t start, uint16_t end, bool stop, bool load, bool store, bool exec);
void ViceAddBreakpoint(uint16_t address);
void ViceSetCondition(int checkPoint, strref condition);
void ViceRemoveBreakpointNoList(uint32_t number);

void ViceWaiting();
void ViceTickMessage();

void ViceLog(strref msg);
typedef void (*ViceLogger)(void*, const char* text, size_t len);
void ViceAddLogger(ViceLogger logger, void* user);
