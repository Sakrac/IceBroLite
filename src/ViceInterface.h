#pragma once

bool ViceConnected();
bool ViceRunning();

void ViceDisconnect();
void ViceConnect(const char* ip, uint32_t port);

void ViceBreak();
void ViceGo();
void ViceStep();
void ViceStepOver();
void ViceStepOut();
void ViceRunTo(uint16_t addr);
void ViceStartProgram(const char* loadPrg);
void ViceReset(uint8_t resetType);
void ViceRemoveBreakpoint(uint32_t number);
void ViceAddBreakpoint(uint16_t address);
void ViceRemoveBreakpointNoList(uint32_t number);

void ViceWaiting();
void ViceTickMessage();

void ViceLog(strref msg);



enum VICEMemSpaces {
	VICE_MainMemory,
	VICE_Drive8,
	VICE_Drive9,
	VICE_Drive10,
	VICE_Drive11
};

bool ViceGetMemory(uint16_t start, uint16_t end, VICEMemSpaces mem);

typedef void (*ViceLogger)(void*, const char* text, size_t len);
void ViceAddLogger(ViceLogger logger, void* user);
