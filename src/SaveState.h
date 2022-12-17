#pragma once

typedef struct {
	uint8_t* data;
	size_t size;
} SaveStateFile;

SaveStateFile ReadState();
void ReleaseState(SaveStateFile file);
void ParseState(SaveStateFile file);
void SaveState();
void UserSaveLayoutUpdate();
