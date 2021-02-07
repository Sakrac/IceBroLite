#pragma once
struct UserData;

struct TraceView {
	size_t tracePointNum;
	int lastDrawnRows;
	int row;
	float mouseWheelDiff;
	float mouseYLast;
	bool open;
	bool mouseDrag;

	TraceView();
	void WriteConfig(UserData& config);
	void ReadConfig(strref config);
	void Draw();
};
