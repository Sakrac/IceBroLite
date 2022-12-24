#include "CodeView.h"
#include "../imgui/imgui.h"
#include "../struse/struse.h"
#include <malloc.h>
#include "Views.h"
#include "../Expressions.h"
#include "../C64Colors.h"
#include "../6510.h"
#include "../Mnemonics.h"
#include "../Breakpoints.h"
#include "../ViceInterface.h"
#include "../ImGui_Helper.h"
#include "GLFW/glfw3.h"
#include "../Image.h"
#include "../Config.h"
#include "../Sym.h"
//#include "Listing.h"
#include "../SourceDebug.h"

CodeView::CodeView() : open(false), evalAddress(false)
{
	showAddress = true;
	showDisAsm = true;
	showBytes = true;
	fixedAddress = false;
	showPCAddress = false;
	showSrc = false;
	showRefs = false;
	showLabels = true;
	editAsmFocusRequested = false;
	editAsmAddr = -1;
	mouseWheelDiff = 0.0f;
	SetAddr(0xea31);
}

void CodeView::SetAddr(uint16_t addr)
{
	addrValue = addr;
	if (!fixedAddress) {
		strovl addr(address, sizeof(address));
		addr.clear();
		addr.append('$').append_num(addrValue, 4, 16).c_str();
	}
}

void CodeView::WriteConfig(UserData& config)
{
	config.AddValue(strref("open"), config.OnOff(open));
	config.AddValue(strref("address"), strref(address));
	config.AddValue(strref("showAddress"), config.OnOff(showAddress));
	config.AddValue(strref("showBytes"), config.OnOff(showBytes));
	config.AddValue(strref("showDisAsm"), config.OnOff(showDisAsm));
	config.AddValue(strref("fixedAddress"), config.OnOff(fixedAddress));
	config.AddValue(strref("showLabels"), config.OnOff(showLabels));
	config.AddValue(strref("showSrc"), config.OnOff(showSrc));
}

void CodeView::ReadConfig(strref config)
{
	ConfigParse conf(config);
	while (!conf.Empty()) {
		strref name, value;
		ConfigParseType type = conf.Next(&name, &value);
		if (name.same_str("open")&&type==ConfigParseType::CPT_Value) {
			open = !value.same_str("Off");
		} else if (name.same_str("address")&&type== ConfigParseType::CPT_Value) {
			strovl addr(address, sizeof(address));
			addr.copy(value);
			addr.c_str();
			evalAddress = true;
		} else if (name.same_str("showAddress")&&type== ConfigParseType::CPT_Value) {
			showAddress = !value.same_str("Off");
		} else if (name.same_str("showBytes")&&type== ConfigParseType::CPT_Value) {
			showBytes = !value.same_str("Off");
		} else if (name.same_str("fixedAddress")&&type== ConfigParseType::CPT_Value) {
			fixedAddress = !value.same_str("Off");
		} else if (name.same_str("showLabels") && type == ConfigParseType::CPT_Value) {
			showLabels = !value.same_str("Off");
		} else if (name.same_str("showSrc") && type == ConfigParseType::CPT_Value) {
			showSrc = !value.same_str("Off");
		}
	}
}

#define MaxDisAsmLines 512
void CodeView::Draw(int index)
{
	if (!open) { return; }
	{
		strown<64> title;
		title.append("Code").append_num(index+1, 1, 10).append(" $").append_num(addrValue, 4, 16).append("###Code").append_num(index+1, 1, 10);
		ImGui::SetNextWindowPos(ImVec2(872, 93), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(544, 425), ImGuiCond_FirstUseEver);
		bool active = ImGui::Begin(title.c_str(), &open);

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AddressDragDrop")) {
				IM_ASSERT(payload->DataSize == sizeof(SymbolDragDrop));
				SymbolDragDrop* drop = (SymbolDragDrop*)payload->Data;
				if (drop->address < 0x10000) {
					addrValue = (uint16_t)drop->address;
					strovl addrStr(address, sizeof(address));
					addrStr.copy(drop->symbol); addrStr.c_str();
					open = true;
				}
			}
			ImGui::EndDragDropTarget();
		}

		if( !active ) {
			ImGui::End();
			return;
		}
	}


//	uint16_t addrs[MaxDisAsmLines];	// address for each line
	CPU6510* cpu = GetCurrCPU();
	const CPU6510::Regs &regs = cpu->regs;	// current registers

	// input text for address field
	if (ImGui::InputText("address", address, sizeof(address), ImGuiInputTextFlags_EnterReturnsTrue)) {
		fixedAddress = address[0]=='=';
		SetAddr(ValueFromExpression(address+(fixedAddress ? 1 : 0)));
	} else if (evalAddress||(fixedAddress && cpu->MemoryChange())) {
		SetAddr(ValueFromExpression(address+(fixedAddress ? 1 : 0)));
		evalAddress = false;
	}

	// code view options
	ImGui::Checkbox("addr", &showAddress);
	ImGui::SameLine();
	ImGui::Checkbox("bytes", &showBytes);
	ImGui::SameLine();
	ImGui::Checkbox("disAsm", &showDisAsm);
	ImGui::SameLine();
	ImGui::Checkbox("refs", &showRefs);
	ImGui::SameLine();
	ImGui::Checkbox("labels", &showLabels);
	ImGui::SameLine();
	ImGui::Checkbox("source", &showSrc);
	{	// don't overlap rightmost area
		ImVec2 content_avail = ImGui::GetContentRegionAvail();
		content_avail.x -= 8;
		ImGui::BeginChild(ImGui::GetID("codeEdit"), content_avail);
	}

	bool active = KeyboardCanvas("DisAsmView");// IsItemActive();

	ImVec2 mousePos = ImGui::GetMousePos();
	ImVec2 curPos = ImGui::GetCursorScreenPos();
	ImVec2 winPos = ImGui::GetWindowPos();
	ImVec2 winSize = ImGui::GetWindowSize();
	uint16_t pc = regs.PC;
	bool goToPC = false;
	bool setPCAtCursor = false;
	int lines = int(ImGui::GetWindowHeight() / ImGui::GetTextLineHeightWithSpacing());
	int cursorLine = -1;
	int dY = 0;
	int sY = 0;

	// handle scroll wheel
	if (mousePos.x >= curPos.x && mousePos.x < (winPos.x + winSize.x) && mousePos.y >= curPos.y && mousePos.y < (winPos.y + winSize.y) && address[0]!='=') {
		mouseWheelDiff += ImGui::GetIO().MouseWheel;
		if (mouseWheelDiff < -0.5f) { sY = 1; mouseWheelDiff += 1.0f; }
		else if (mouseWheelDiff > 0.5) { sY = -1; mouseWheelDiff -= 1.0f; }
	} else {
		mouseWheelDiff = 0.0f;
	}

	// Handle keyboard input
	if (active) {
		if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_UP)) { dY--; }
		if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_DOWN)) { dY++; }
		if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_TAB)) {
			if (ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_LEFT_SHIFT) || ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_RIGHT_SHIFT)) { setPCAtCursor = true; } else { goToPC = true; addrCursor = pc; }
		}
		if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_PAGE_UP)) { sY = -lines / 3; }
		if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_PAGE_DOWN)) { sY = lines / 3; }
		if (ImGui::IsMouseClicked(0)) {
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 winPos = ImGui::GetWindowPos();
			ImVec2 winSize = ImGui::GetWindowSize();
			if (mousePos.x >= winPos.x && mousePos.y >= winPos.y &&
				mousePos.x < (winPos.x + winSize.x) && mousePos.y < (winPos.y + winSize.y)) {
				cursorLine = int((mousePos.y - winPos.y) / ImGui::GetTextLineHeightWithSpacing());
			}
		}
	}

	strown<128> line;
	uint16_t read = addrValue;
	int lineNum = 0;
	float fontCharWidth = CurrFontSize();
	float lineHeight = ImGui::GetTextLineHeightWithSpacing()-2;

	if (sY<0) {
		uint16_t addr = addrValue;
		for (int line = 0; line<(-sY); ++line) {
			--addr;
			int len = 1;
			while (addr && InstructionBytes(cpu, addr)>len) {
				--addr;
				++len;
			}
		}
		SetAddr(addr);
	} else if (sY>0) {
		uint16_t addr = addrValue;
		for (int line = 0; line<sY; ++line) {
			addr += InstructionBytes(cpu, addr);
		}
		SetAddr(addr);
	}

	uint16_t prevLineAddr = read;
	int nextLineAddr = -1;
	bool editAsmDone = false;
	while (lineNum<lines) {
		if (const char* label = GetSymbol(read)) {
			ImGui::TextColored(C64_LGREEN, label);
			lineNum++;
		}
		ImVec2 linePos = ImGui::GetCursorPos();
		int chars = 0;
		if (lineNum==cursorLine) { addrCursor = read; }
		if (addrCursor==read && active && !editAsmDone && ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_ENTER)) {
			editAsmStr[0] = 0;
			editAsmAddr = read;
			editAsmFocusRequested = true;
		}

		int srcSpaces = 0;
		strl_t srcCol = 0;
		strref srcLine = {};
		if (showSrc) {
			srcLine = GetSourceAt(read, srcSpaces);
			srcCol = (showAddress ? 6 : 0) + (showBytes ? 9 : 0) + (showRefs ? 11 : 0) + (showLabels ? 6 : 0) + (showDisAsm ? 12 : 0) + (srcSpaces + 3) / 4;
		}

		// mouse hover
		if (mousePos.y >= (winPos.y + linePos.y) && mousePos.y <= (winPos.y + linePos.y + lineHeight) &&
			mousePos.x > linePos.x && mousePos.x < (winPos.x + winSize.x - 8)) {
			// check for drag and drop of reference
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				if (uint16_t refAddr = InstrRefAddr(cpu, read)) {
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
						SymbolDragDrop drag;
						drag.address = refAddr;
						strovl lblStr(drag.symbol, sizeof(drag.symbol));
						lblStr.sprintf("$%04x", refAddr); lblStr.c_str();
						ImGui::SetDragDropPayload("AddressDragDrop", &drag, sizeof(drag));
						strown<8> refStr; refStr.append_num(refAddr, 4, 16);
						ImGui::Text(refStr.c_str());
						ImGui::EndDragDropSource();
					}
				}
			}

			// check for custom tooltip
			uint8_t opcode = cpu->GetByte(read);
			bool disasm = false;
			uint16_t ref_addr;
			if (opcode == 0x20 || opcode == 0x4c) {
				disasm = true;
				ref_addr = cpu->GetByte(read + 1) | (((uint16_t)cpu->GetByte(read + 2)) << 8);
			} else if ((opcode | 0xe0) == 0xf0) {
				disasm = true;
				uint8_t offs = cpu->GetByte(read + 1);
				ref_addr = read + 2 + ((offs & 0x80) ? (offs - 0x100) : offs);
			}
			if (disasm) {
				strown<512> disbuf;
				size_t dsof = 0, l = 0;
				while (disbuf.left() && l < 10) {
					int branchTrg = -1;
					disbuf.sprintf_append("$%04x ", ref_addr);
					int bytes = Disassemble(cpu, ref_addr, disbuf.end(), disbuf.left(), chars, branchTrg, false, true, true, true);
					ref_addr += bytes;
					disbuf.set_len(disbuf.get_len()+chars);
					disbuf.append('\n');
					++l;
				}
				ImGui::SetNextWindowBgAlpha(0.75f);
				ImGui::BeginTooltip();
				ImGui::Text(disbuf.c_str());
				ImGui::EndTooltip();
			}
		}

		line.clear();
		line.append(regs.PC==read ? '>' : ' ');
		if (showAddress) { line.append_num(read, 4, 16); line.append(' '); }
		int branchTrg = -1;
		int bytes = Disassemble(cpu, read, line.end(), line.left(), chars, branchTrg, showBytes, true, showLabels, showDisAsm);
		if (editAsmAddr==read&&!editAsmDone) {
			line.pad_to(' ', 14);
			ImGui::TextUnformatted(line.get(), line.end());
			ImGui::SameLine();
			if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_ESCAPE)) {
				editAsmAddr = -1;
				ForceKeyboardCanvas("DisAsmView");
			}
			if (editAsmFocusRequested) {
				ImGui::SetKeyboardFocusHere();
				editAsmFocusRequested = false;
			}
			strown<32> editID("Edit Asm##");
			editID.append_num(editAsmAddr, 4, 16);
			if (ImGui::InputText(editID.c_str(), editAsmStr, sizeof(editAsmStr), ImGuiInputTextFlags_EnterReturnsTrue)) {
				int size = Assemble(cpu, editAsmStr, editAsmAddr);
				if (!size) {
					editAsmAddr = -1;
					ForceKeyboardCanvas("DisAsmView");
				} else {
					editAsmStr[0] = 0;
					editAsmFocusRequested = true;
					editAsmAddr += size;
				}
				editAsmDone = true;
			}
		} else {
			line.add_len(chars);
			if (showRefs) {
				char buf[24];
				if (InstrRef(cpu, read, buf, sizeof(buf))) { line.pad_to(' ', (showAddress ? 6 : 0) + (showBytes ? 9 : 0) + (showLabels ? 6 : 0) + (showDisAsm ? 11 : 0)).append(buf); }
			}
			if (goToPC && read==pc) { goToPC = false; } // don't recenter PC if already in view
			if (setPCAtCursor && read==addrCursor) {
				cpu->SetPC(addrCursor);
			}
			if (srcLine && line.get_len() > srcCol) { line.set_len(srcCol); }
			if (addrCursor>=read && addrCursor<(read+bytes)) {
				if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_F6)) {
					ViceRunTo(addrCursor);
				}
				ImVec2 ps = ImGui::GetCursorScreenPos();
				if (dY<0) {
					if (!lineNum) {
						if (!fixedAddress) {
							int len = 1;
							uint16_t addr = addrValue-len;
							while (addr && InstructionBytes(cpu, addr)>len) {
								--addr;
								++len;
							}
							addrValue = addr;
							addrCursor = addr;
							strovl addrStr(address, sizeof(address));
							addrStr.clear();
							addrStr.append('$').append_num(addrValue, 4, 16);
						}
					} else { addrCursor = prevLineAddr; }
				} else if (dY>0) {
					if (lineNum==(lines-1)) {
						if (!fixedAddress) { SetAddr((uint16_t)nextLineAddr); }
					}
					if (!fixedAddress||lineNum!=(lines-1)) {
						addrCursor = read+bytes;
					}
				}
				if (active) {
					ImDrawList* dl = ImGui::GetWindowDrawList();
					dl->AddRectFilled(ps, ImVec2(ps.x + ImGui::CalcTextSize(line.c_str()).x,
												 ps.y + lineHeight), ImColor(C64_PURPLE));
				}
				dY = 0;
			}
		// breakpoints
			Breakpoint bp;
			if (BreakpointAt(read, bp)) {
				ImVec2 savePos = ImGui::GetCursorPos();
				ImGui::SetCursorPos(linePos);
				DrawTexturedIcon((bp.flags & Breakpoint::Enabled) ? ViceMonIcons::VMI_BreakPoint : ViceMonIcons::VMI_DisabledBreakPoint, false, fontCharWidth);
				ImGui::SetCursorPos(savePos);
				if (active && addrCursor == read && ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_F9, false)) {
					// remove breakpoint
					ViceRemoveBreakpoint(bp.number);
				}
			} else if (active && addrCursor == read && ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_F9, false)) {
				// add exec breakpoint
				ViceAddBreakpoint(read);
			}
			// very cunningly draw code line AFTER breakpoint
			ImGui::Text(line.c_str());
			if (showSrc && srcLine) {
				ImGui::SameLine();
				ImVec2 srcPos(linePos.x + srcCol * fontCharWidth, linePos.y);
				ImGui::SetCursorPos(srcPos);
				ImGui::PushStyleColor(ImGuiCol_Text, C64_YELLOW);
				line.copy(srcLine);
				ImGui::TextUnformatted(line.c_str());
				ImGui::PopStyleColor();
			}
		}

		prevLineAddr = read;
		read = (read+bytes)&0xffff;
		if (nextLineAddr<0) { nextLineAddr = read; }
		++lineNum;
	}

	if (showPCAddress&&(regs.PC<addrValue||regs.PC>=read)&&lastShownPC!=pc) {
		goToPC = true;
	} else {
		showPCAddress = !fixedAddress && regs.PC>=addrValue && regs.PC<read && dY==0;
	}

	// if pressing tab and PC is not on screen find an address 3 lines above
	if (!fixedAddress&&(goToPC||focusPC)) {
		uint16_t addr = pc;
		for (int line = 0; line<3; ++line) {
			uint16_t addrBackup = addr;
			--addr;
			int len = 1;
			int instrLen = 1;
			while (addr) {
				instrLen = InstructionBytes(cpu, addr);
				if (instrLen>=len) { break; }
				--addr;
				++len;
			}
			if (instrLen!=len) { addr = addrBackup; break; }
		}
		SetAddr(addr);
	}
	if (focusPC) {
		addrCursor = pc;
	} else if (lastShownPC != pc && addrCursor == lastShownPC) {
		addrCursor = pc;
	} else if (read < addrValue && (addrCursor >= addrValue || addrCursor < read)) {
		// this is a valid location for the cursor so don't reset it
	} else if (addrCursor<addrValue) {
		addrCursor = addrValue;
	} else if (addrCursor>=read) {
		addrCursor = prevLineAddr;
	}
	lastShownPC = pc;
	focusPC = false;

	ImGui::EndChild();
	ImGui::End();
}
