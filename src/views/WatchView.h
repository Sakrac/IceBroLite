#pragma once
#include <stdint.h>

struct UserData;

struct WatchView {
	enum class WatchType : uint8_t {
		WT_NORMAL,
		WT_BYTES,
		WT_DISASM
	};

	enum {
		MaxExp = 128
	};

	strown<128> expressions[MaxExp];
	strown<128> rpnExp[MaxExp];
	strown<64> results[MaxExp];
	int numExpressions;
	int editExpression;
	int prevWidth;
	int activeIndex;
	int values[MaxExp];
	WatchType types[MaxExp];
	bool open;
	bool rebuildAll;
	bool recalcAll;
	bool forceEdit;

	WatchView();

	void Evaluate(int index);

	void EvaluateItem(int index);

	void WriteConfig(UserData& config);

	void ReadConfig(strref config);

	void Draw(int index);
};


