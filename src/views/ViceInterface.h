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

