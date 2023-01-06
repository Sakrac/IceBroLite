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

bool ParseRememberCompare(strref &param_in, uint8_t *pb0, uint8_t *pb1, uint32_t *pa0, uint32_t *pa1) {
	strref param = param_in;

	strref bytes = param.split_token_trim(' ');
	uint8_t b0 = (uint8_t)ValueFromExpression(strown<256>(bytes.split_token('-')).c_str()), b1 = b0;
	if (bytes.valid()) {
		b1 = (uint8_t)ValueFromExpression(strown<256>(bytes).c_str());
	}

	uint32_t a0 = 0, a1 = 0x10000;

	char p0 = param[0];

	if (param.valid() && p0 != 't' && p0 != 'T' && p0 != 'w' && p0 != 'W') {
		strref addr1 = param.split_token_trim(' ');
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

	*pb0 = b0; *pb1 = b1; *pa0 = a0; *pa1 = a1;

	param_in = param;
	return true;
}


void CommandRemember(strref param) {
	uint8_t b0, b1;
	uint32_t a0, a1;
	if (!ParseRememberCompare(param, &b0, &b1, &a0, &a1)) { return; }

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
	if (!ParseRememberCompare(param, &b0, &b1, &a0, &a1)) { return; }
	if (CPU6510* cpu = GetCurrCPU()) {
		bool wasRunning = HaltViceWait();

		ViceLog("Matches:");
		strown<128> result;
		int found = 0;

		char bp = strref::tolower(param[0]);

		for (auto a : sRemembered) {
			if (a >= a0 && a < a1) {
				uint8_t b = cpu->GetByte(a);
				if (b >= b0 && b <= b1) {
					if (bp == 'w') { ViceAddCheckpoint(a, a, true, false, true, false); }
					else if(bp == 't') { ViceAddCheckpoint(a, a, false, false, true, false); }
					result.append_num(a, 4, 16).append(", ");
					++found;
					if ((int)(result.len()+6) >= charSpace) {
						ViceLog(result.get_strref());
						result.clear();
					}
				}
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
