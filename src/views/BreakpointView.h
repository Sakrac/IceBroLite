#pragma once
struct UserData;

struct BreakpointView {
	bool open;
	int selected_row;

	BreakpointView();
	void WriteConfig(UserData& config);
	void ReadConfig(strref config);
	void Draw();
};
