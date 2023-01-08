// various commands for the console view etc.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <synchapi.h>
#else
#include <unistd.h>
#endif
#include <vector>
#include "struse/struse.h"
#include "Expressions.h"
#include "6510.h"
#include "ViceInterface.h"

static std::vector<uint16_t> sRemembered;

bool HaltViceWait() {
	bool wasRunning = ViceRunning();
	if (wasRunning) {
		ViceBreak();
		for (int i = 0; i < 60; ++i) {
#ifdef _WIN32
			Sleep(1);
#else
			usleep(1000);
#endif
			if (!ViceRunning()) { break; }
		}
	}
	return wasRunning;
}

void CommandPoke(strref param) {
	strref addr = param.split_token_trim(',');
	strref byte = param;

	int addrValue = ValueFromExpression(strown<256>(addr).c_str());
	int byteValue = ValueFromExpression(strown<256>(byte).c_str());

	bool wasRunning = HaltViceWait();
	if (CPU6510* cpu = GetCurrCPU()) {
		cpu->SetByte((uint16_t)addrValue, (uint8_t)byteValue);
	}
	if (wasRunning) {
		ViceGo();
	}
}

bool ParseRememberCompare(strref &param_in, uint8_t *pb0, uint8_t *pb1, uint32_t *pa0, uint32_t *pa1, bool *pinv) {
	strref param = param_in;

	uint8_t b0 = 0, b1 = 0xff;
	uint32_t a0 = 0, a1 = 0x10000;
	bool inv = false;
	param.trim_whitespace();
	if (param.get_len()) {
		if (param[0] == '!') { ++param; inv = true; param.skip_whitespace(); }
		strref bytes = param.split_token_trim(' ');
		b0 = (uint8_t)ValueFromExpression(strown<256>(bytes.split_token('-')).c_str()), b1 = b0;
		if (bytes.valid()) {
			b1 = (uint8_t)ValueFromExpression(strown<256>(bytes).c_str());
		}

		char p0 = strref::tolower(param[0]);

		if (param.valid() && p0 != 't' && p0 != 'w' && p0 != 'c' && p0 != 'f') {
			strref addr1 = param.split_token_any_trim(strref(" -"));
			strref addr2 = param.split_token_trim(' ');

			a0 = (uint32_t)ValueFromExpression(strown<256>(addr1).c_str());
			a1 = (uint32_t)ValueFromExpression(strown<256>(addr2).c_str());

			if (a1 <= a0) {
				strown<128> errstr;
				errstr.append("Error: not a valid address range ($").append_num(a0, 4, 16).append("-$")
					.append_num(a1, 4, 16).append(")").c_str();
				ViceLog(errstr.get_strref());
				return false;
			}
		}
	}

	*pb0 = b0; *pb1 = b1; *pa0 = a0; *pa1 = a1; *pinv = inv;

	param_in = param;
	return true;
}


void CommandRemember(strref param) {
	uint8_t b0, b1;
	uint32_t a0, a1;
	bool inv = false;
	if (!ParseRememberCompare(param, &b0, &b1, &a0, &a1, &inv)) { return; }

	if (CPU6510* cpu = GetCurrCPU()) {
		bool wasRunning = HaltViceWait();

		if (sRemembered.capacity() < 512) {
			sRemembered.reserve(512);
		}

		sRemembered.clear();
		for (uint32_t a = a0; a < a1; ++a) {
			uint8_t b = cpu->GetByte(a);
			if (b >= b0 && b <= b1) {
				sRemembered.push_back(a);
			}
		}

		strown<128> resultStr;
		resultStr.append("Found ").append_num((uint32_t)sRemembered.size(), 0, 10)
			.append(" matching bytes ($").append_num(b0, 2, 16);
		if (b1 != b0) { resultStr.append("-$").append_num(b1, 2, 16); }
		resultStr.append(")");
		if (a0 > 0) { resultStr.append(" between $").append_num(a0, 4, 16)
			.append(" to $").append_num(a1, 4, 16); }
		ViceLog(resultStr.get_strref());
		if (wasRunning) { ViceGo(); }
	}
}

void  CommandForget() {
	sRemembered.clear();
}

void CommandMatch(strref param, int charSpace) {
	uint8_t b0, b1;
	uint32_t a0, a1;
	bool inv = false;
	if (!ParseRememberCompare(param, &b0, &b1, &a0, &a1, &inv)) { return; }
	if (CPU6510* cpu = GetCurrCPU()) {
		bool wasRunning = HaltViceWait();

		ViceLog("Matches:");
		strown<128> result;
		int found = 0;

		bool trc = false, wtc = false, clr = false, flt = false;
		while (strref ctrl = param.split_token_trim(' ')) {
			switch (strref::tolower(ctrl.get_first())) {
				case 't': trc = true; break;
				case 'w': wtc = true; break;
				case 'c': clr = true; break;
				case 'f': flt = true; break;
			}
		}

		if (clr) { sRemembered.clear(); }

		bool wasClr = sRemembered.size() == 0;

		if (wasClr) {
			for (uint32_t a = a0; a < a1; ++a) {
				uint8_t b = cpu->GetByte(a);
				if ((b >= b0 && b <= b1)!=inv) {
					sRemembered.push_back(a);
					++found;
				}
			}
		} else {
			size_t i = 0;
			while (i<sRemembered.size()) {
				uint16_t a = sRemembered[i];
				bool match = false;
				if (a >= a0 && a < a1) {
					uint8_t b = cpu->GetByte(a);
					if ((b >= b0 && b <= b1)!=inv) {
						if(wtc) { ViceAddCheckpoint(a, a, true, false, true, false); }
						else if(trc) { ViceAddCheckpoint(a, a, false, false, true, false); }
						result.append_num(a, 4, 16).append(", ");
						++found;
						match = true;
						if ((int)(result.len()+6) >= charSpace) {
							ViceLog(result.get_strref());
							result.clear();
						}
					}
				}
				if (match || !flt) { ++i; }
				else if (flt) { sRemembered.erase(sRemembered.begin() + i); }
			}
			if (result.get_len()) {
				ViceLog(result.get_strref());
			}
		}

		result.clear();
		result.append("Found ").append_num(found, 0, 10).append(" matching bytes ($")
			.append(" matching bytes ($").append_num(b0, 2, 16);
		if (b1 != b0) { result.append("-$").append_num(b1, 2, 16); }
		result.append(")");
		if (a0 > 0) {
			result.append(" between $").append_num(a0, 4, 16)
				.append(" to $").append_num(a1, 4, 16);
		}
		ViceLog(result.get_strref());
		if (wasRunning) { ViceGo(); }
	}
}
