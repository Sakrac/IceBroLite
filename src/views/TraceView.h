#pragma once
struct UserData;

struct TraceView {
	bool open;
	size_t tracePointNum;
	int lastDrawnRows;
	int row;
	float mouseWheelDiff;

	TraceView();
	void WriteConfig(UserData& config);
	void ReadConfig(strref config);
	void Draw();
};
