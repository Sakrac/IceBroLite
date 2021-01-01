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

void ViceWaiting();

enum VICEMemSpaces {
	VICE_MainMemory,
	VICE_Drive8,
	VICE_Drive9,
	VICE_Drive10,
	VICE_Drive11
};

bool ViceGetMemory(uint16_t start, uint16_t end, VICEMemSpaces mem);
