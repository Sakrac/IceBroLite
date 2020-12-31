#pragma once

void InitStartFolder();
const char* GetStartFolder();
void ResetStartFolder();

void DrawFileDialog();

bool IsFileDialogOpen();
const char* ImportImageReady();
const char* LoadAnimReady();
const char* SaveAsAnimReady();
const char* SaveLevelAsReady();
const char* LoadLevelReady();
const char* LoadGrabMapReady();
const char* LoadTemplateImageReady();

void LoadTemplateDialog();
void LoadImageDialog();
void LoadAnimDialog();
void SaveAnimDialog();
void SaveLevelDialog();
void LoadLevelDialog();
void LoadGrabMapDialog();


