#pragma once

void InitStartFolder();
const char* GetStartFolder();
void ResetStartFolder();
bool IsFileDialogOpen();
const char* LoadProgramReady();
const char* LoadListingReady();
const char* LoadKickDbgReady();
const char* LoadSymbolsReady();
const char* LoadViceCMDReady();
bool LoadViceEXEPathReady();
void LoadProgramDialog();
void LoadListingDialog();
void LoadKickDbgDialog();
void LoadSymbolsDialog();
void LoadViceCmdDialog();
void SetViceEXEPathDialog();


const char* ReloadProgramFile();
char* GetViceEXEPath();

