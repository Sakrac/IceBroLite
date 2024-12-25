#pragma once

void FileDialogPathMenu();

void InitStartFolder();
bool GetCWD(char* dir, uint32_t dir_size);
const char* GetStartFolder();
void ResetStartFolder();
bool IsFileDialogOpen();
const char* LoadProgramReady();
const char* LoadListingReady();
const char* GetListingFilename();
const char* LoadKickDbgReady();
const char* LoadKickDbgExtraReady();
const char* GetKickDbgFile();
const char* LoadSymbolsReady();
const char* GetSymbolFilename();
const char* LoadViceCMDReady();
const char* GetViceCMDFilename();
const char* ReadPRGToRAMReady();
const char* LoadThemeReady();
const char* SaveThemeReady();
bool LoadViceEXEPathReady();
void LoadProgramDialog();
void LoadListingDialog();
void LoadKickDbgDialog();
void LoadKickDbgExtraDialog();
void LoadSymbolsDialog();
void LoadViceCmdDialog();
void LoadThemeDialog();
void SaveThemeDialog();
void SetViceEXEPathDialog();
void ReadPRGDialog();

const char* ReloadProgramFile();
const char* ReadPRGFile();
char* GetViceEXEPath();
void SetViceEXEPath(strref path);
const char* GetCustomThemePath();

