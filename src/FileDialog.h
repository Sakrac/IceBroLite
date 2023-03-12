#pragma once

void InitStartFolder();
bool GetCWD(char* dir, uint32_t dir_size);
const char* GetStartFolder();
void ResetStartFolder();
bool IsFileDialogOpen();
const char* LoadProgramReady();
const char* LoadListingReady();
const char* LoadKickDbgReady();
const char* LoadSymbolsReady();
const char* LoadViceCMDReady();
const char* ReadPRGToRAMReady();
bool LoadViceEXEPathReady();
void LoadProgramDialog();
void LoadListingDialog();
void LoadKickDbgDialog();
void LoadSymbolsDialog();
void LoadViceCmdDialog();
void SetViceEXEPathDialog();
void ReadPRGDialog();

const char* ReloadProgramFile();
const char* ReadPRGFile();
char* GetViceEXEPath();
void SetViceEXEPath(strref path);

