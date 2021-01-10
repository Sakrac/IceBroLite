#pragma once
struct UserData;

struct BreakpointView {
	bool open;

	BreakpointView();
	void WriteConfig(UserData& config);
	void ReadConfig(strref config);
	void Draw();
};
