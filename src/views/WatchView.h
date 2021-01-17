#pragma once
struct UserData;

struct WatchView {
	enum WatchType {
		WT_NORMAL,
		WT_BYTES,
		WT_DISASM
	};

	enum {
		MaxExp = 128
	};

	int numExpressions;
	int editExpression;
	int prevWidth;
	strown<128> expressions[MaxExp];
	strown<128> rpnExp[MaxExp];
	strown<64> results[MaxExp];
	WatchType types[MaxExp];
	int values[MaxExp];
	bool open;
	bool rebuildAll;
	bool recalcAll;

	WatchView();

	void Evaluate(int index);

	void EvaluateItem(int index);

	void WriteConfig(UserData& config);

	void ReadConfig(strref config);

	void Draw(int index);
};


