#pragma once
struct UserData;

struct BreakpointView {
	bool open;
	size_t selected_row;
	char conditionEdit[64];

	BreakpointView();
	void WriteConfig(UserData& config);
	void ReadConfig(strref config);
	void Draw();
	void SetSelected(int index);
};
