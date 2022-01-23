#pragma once

struct TraceHit {
	uint32_t sw;
	uint32_t frame;
	uint16_t pc;
	uint16_t addr;
	uint16_t line;
	uint8_t cycle;
	uint8_t a, x, y, sp, fl;
};

strref CaptureVICELine(strref line);
size_t NumTracePointIds();
int GetTracePointId(size_t id);
size_t NumTraceHits(size_t id);
TraceHit GetTraceHit(int traceId, size_t index);
void ClearTrace(size_t id);

void InitTraces();
void ShutdownTraces();
