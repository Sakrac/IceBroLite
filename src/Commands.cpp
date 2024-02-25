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
#include "Commands.h"

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

enum {
	GFX_Text,
	GFX_TextMC,
	GFX_TextEBCM,
	GFX_Bitmap,
	GFX_BitmapMC
};

const char* sazScreenModes[] = {
	"Text",
	"Text Multi-Color",
	"Text EBCM",
	"Bitmap",
	"Bitmap Multi-Color"
};

const char szBankRam[] = "bank ram";
void SendViceMonitorLine(const char* message, int size);

const char* CommandGfxSave(strref param) {

	bool wasRunning = HaltViceWait();
	SendViceMonitorLine(szBankRam, sizeof(szBankRam));

	CPU6510* cpu = GetCurrCPU();
	uint16_t vic = (3 ^ (cpu->GetByte(0xdd00) & 3)) * 0x4000;
	uint8_t d018 = cpu->GetByte(0xd018);
	uint8_t d011 = cpu->GetByte(0xd011);
	uint8_t d016 = cpu->GetByte(0xd016);
	uint16_t chars = (d018 & 0xe) * 0x400 + vic;
	uint16_t screen = (d018 >> 4) * 0x400 + vic;
	bool mc = (d016 & 0x10) ? true : false;

	strown<128> file(param);
	file.append(".txt");

	int mode = GFX_Text;
	int numChars = 256;
	int charBaseMask = 0x800;
	int colRegs = 2;

	if (d011 & 0x40) { 
		mode = GFX_TextEBCM;
		numChars = 64;
		colRegs = 5;
	} else if (d011 & 0x20) {
		mode = mc ? GFX_BitmapMC : GFX_Bitmap;
		numChars = 1000;
		charBaseMask = 0x2000;
		colRegs = 0;
	} else if (mc) { 
		mode = GFX_TextMC;
		colRegs = 3;
	}

	FILE* f;
#ifdef _WIN32
	if (fopen_s(&f, file.c_str(), "w") == 0 && f != nullptr) {
#else
	f = fopen(filename, "w");
	if (f) {
#endif
		fprintf(f, "; Info for screendump files " STRREF_FMT "\n", STRREF_ARG(param));
		fprintf(f, "mode: %s\n", sazScreenModes[mode]);
		fprintf(f, "d011: $%02x\n", d011);
		fprintf(f, "d016: $%02x\n", d016);
		fprintf(f, "d018: $%02x\n", d016);
		for (int c = 0; c < colRegs; ++c) {
			fprintf(f, "%04x: $%02x\n", 0xd020+c, cpu->GetByte(0xd020+c));
		}
		fclose(f);
	}

	file.copy(param);
	file.append(".scr");
#ifdef _WIN32
	if (fopen_s(&f, file.c_str(), "wb") == 0 && f != nullptr) {
#else
	f = fopen(filename, "wb");
	if (f) {
#endif
		fwrite(cpu->GetMem(screen), 1000, 1, f);
		fclose(f);
	}

	if (mode != GFX_Bitmap) {
		file.copy(param);
		file.append(".col");
#ifdef _WIN32
		if (fopen_s(&f, file.c_str(), "wb") == 0 && f != nullptr) {
#else
		f = fopen(filename, "wb");
		if (f) {
#endif
			uint8_t tmpCol[1000];
			memcpy(tmpCol, cpu->GetMem(0xd800), 1000);
			for (int i = 0; i < 1000; ++i) {
				tmpCol[i] &= 0x0f;
			}
			fwrite(tmpCol, 1000, 1, f);
			fclose(f);
		}
	}

	file.copy(param);
	file.append(".chr");
#ifdef _WIN32
	if (fopen_s(&f, file.c_str(), "wb") == 0 && f != nullptr) {
#else
	f = fopen(filename, "wb");
	if (f) {
#endif
		fwrite(cpu->GetMem(chars), numChars * 8, 1, f);
		fclose(f);
	}

	if (wasRunning) {
		ViceGo();
	}

	return sazScreenModes[mode];
}