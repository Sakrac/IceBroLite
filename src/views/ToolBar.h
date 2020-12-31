#pragma once
struct UserData;

struct ToolBar {
	bool open;

	ToolBar();
	void WriteConfig(UserData& config);
	void ReadConfig(strref config);
	void Draw();
};

