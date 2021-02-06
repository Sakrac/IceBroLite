#pragma once
struct UserData;

struct BreakpointView {
	size_t selected_row;
	int addCheckpointType;
	char checkStartEdit[64];
	char checkEndEdit[64];
	char conditionEdit[64];
	bool open;

	BreakpointView();
	void WriteConfig(UserData& config);
	void ReadConfig(strref config);
	void Draw();
	void SetSelected(int index);
};
