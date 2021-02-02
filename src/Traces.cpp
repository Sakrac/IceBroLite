// Capture information from the VICE text monitor output
#include <stdint.h>
#include <vector>
#include "struse/struse.h"

// TRACE Pattern:
// ID: line starts with #, <bk num>, space, "(Trace store "<trg>")"
// next line starts with .?:<addr>" "
// contents line 0:
//  * 23: line
//	* 34, after spaces: cycle
// contents line 1:
//	* 3: pc
//	* 40: A
//	* 45: X
//	* 50: Y
//	* 55: SP
//	* 59: FLAGS
//	* 67: Stopwatch (after spaces)
//#1 (Trace store d40b)  261/$105,   3/$003
//.C:c617  9D 04 D4    STA $D404,X    - A:10 X:07 Y:00 SP:f6 ..-..I.C   26316174

struct TraceHit {
	uint32_t sw;
	uint16_t pc;
	uint16_t addr;
	uint16_t line;
	uint8_t cycle;
	uint8_t a, x, y, sp, fl;
};

struct TraceArray {
	int tpId;
	std::vector<TraceHit>* traceHits;
};

static bool sTraceStart = false;
static int sTracePointIdx = 0;
static TraceHit sTraceInfo = {};

static std::vector<TraceArray> sTraceArrays;

void AddTraceHit(int tracePoint, const TraceHit& hit)
{
	std::vector<TraceHit>* pArray = nullptr;
	for (size_t t = 0, n = sTraceArrays.size(); t < n; ++t) {
		if (sTraceArrays[t].tpId == tracePoint) {
			pArray = sTraceArrays[t].traceHits;
			break;
		}
	}
	if (!pArray) {
		TraceArray tArr = { tracePoint, new std::vector<TraceHit> };
		pArray = tArr.traceHits;
		sTraceArrays.push_back(tArr);
	}

	if (pArray) {
		pArray->push_back(hit);
	}
}


strref CaptureVICELine(strref line)
{
	strref orig = line;
	if (sTraceStart) {
		sTraceStart = false;

		if (line.get_len() >= 68 && line[0] == '.' && line[2] == ':' && line[7]==' ') {
			sTraceInfo.pc = (uint16_t)line.get_substr(3, 4).ahextoui();
			sTraceInfo.a = (uint8_t)line.get_substr(40, 2).ahextoui();
			sTraceInfo.x = (uint8_t)line.get_substr(45, 2).ahextoui();
			sTraceInfo.y = (uint8_t)line.get_substr(50, 2).ahextoui();
			sTraceInfo.sp = (uint8_t)line.get_substr(55, 2).ahextoui();
			uint8_t f = 0;
			if (line[59] != '.') { f |= 0x80; }
			if (line[60] != '.') { f |= 0x40; }
			//if (line[61] != '.') { f |= 0x20; }
			if (line[62] != '.') { f |= 0x10; }
			if (line[63] != '.') { f |= 0x08; }
			if (line[64] != '.') { f |= 0x04; }
			if (line[65] != '.') { f |= 0x02; }
			if (line[66] != '.') { f |= 0x01; }
			sTraceInfo.fl = f;
			line.skip(67);
			line.trim_whitespace();
			sTraceInfo.sw = (uint32_t)line.atoui();
			AddTraceHit(sTracePointIdx, sTraceInfo);
			return strref();
		}
	} else if (line.get_len() >= 42 && line[0] == '#' && strref::is_number(line[1])) {
		++line;
		sTracePointIdx = line.atoi_skip();
		line.skip_whitespace();
		if (line.grab_prefix("(Trace ")) {
			// don't bother with store/load/exec yet
			line.next_word_ws();
			sTraceInfo.addr = (uint16_t)line.ahextoui_skip();
			if (line.grab_char(')')) {
				line.skip_whitespace();
				if (strref::is_number(line.get_first())) {
					sTraceInfo.line = (uint16_t)line.atoi_skip();
					line.skip_to_whitespace();
					line.skip_whitespace();
					if (strref::is_number(line.get_first())) {
						sTraceInfo.line = (uint16_t)line.atoi_skip();
						sTraceStart = true;
						return strref();
					}
				}
			}
		}
	}
	return orig;
}