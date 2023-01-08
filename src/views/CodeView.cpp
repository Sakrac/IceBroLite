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
#include "../imgui/imgui_internal.h"
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
	trackPC = false;
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
	config.AddValue(strref("trackPC"), config.OnOff(trackPC));
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
		} else if (name.same_str("trackPC") && type == ConfigParseType::CPT_Value) {
			trackPC = !value.same_str("Off");
		}
	}
}

bool CodeView::EditAssembly() {
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
		int size = Assemble(GetCurrCPU(), editAsmStr, editAsmAddr);
		if (!size) {
			editAsmAddr = -1;
			ForceKeyboardCanvas("DisAsmView");
		}
		else {
			editAsmStr[0] = 0;
			editAsmFocusRequested = true;
			editAsmAddr += size;
		}
		return true;
	}
	return false;
}

void CodeView::CodeContextMenu(CPU6510 *cpu, int index)
{
	strown<32> ctxId; ctxId.append("code").append_num(index, 1, 10).append("_ctx").c_str();
	if (ImGui::BeginPopupEx(ImGui::GetCurrentWindow()->GetID(ctxId.get()),
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {
		if (uint16_t refAddr = InstrRefAddr(cpu, contextAddr)) {
			strown<16> refAddrStr; refAddrStr.append('$').append_num(refAddr, 4, 16);
			InstrRefType refType = GetRefType(cpu, contextAddr);
			if (refType == InstrRefType::DataArray || refType == InstrRefType::DataValue) {
				for (int w = 0; w < 2; ++w) {
					strown<12> watch_name; watch_name.append("Watch ").append_num(w + 1, 1, 10);
					if (ImGui::BeginMenu(watch_name.c_str())) {
						char watchStr[32];
						for (int s = 0; s <= 17; ++s) {
							if (GetWatchRef(cpu, contextAddr, s, watchStr, sizeof(watchStr))) {
								if (ImGui::MenuItem(watchStr)) {
									AddWatch(w, watchStr);
								}
							}
						}
						ImGui::EndMenu();
					}
				}
			}
			else if (refType == InstrRefType::Code) {
				for (int c = 0; c < 4; ++c) {
					strown<16> code_name; code_name.append("Code ").append_num(c + 1, 1, 10).append("=$").append_num(refAddr, 4, 16);
					if (ImGui::MenuItem(code_name.c_str())) {
						SetCodeAddr(c, refAddr);
					}
				}
			}
		}
		ImGui::EndPopup();
	}
}

void CodeView::UpdateTrackPC(CPU6510 *cpu, int& dY, int lines)
{
	uint16_t pc = cpu->regs.PC;
	if (lastShownPC != pc || dY) {
		uint16_t a = pc;
		lastShownPCRow += dY;
		dY = 0;
		if (lastShownPCRow < 0) { lastShownPCRow = 0; }
		else if (lastShownPCRow >= lines) { lastShownPCRow = lines - 1; }

		int rows = lastShownPCRow > 0 ? lastShownPCRow : (lines / 3);
		while (rows) {
			if (const char* label = GetSymbol(a)) { --rows; }
			if (rows) {
				uint16_t an = a--;
				while (((ValidInstructionBytes(cpu, a) + a) & 0xffff) != an && (an - a) < 3) {
					--a;
				}
				--rows;
			}
		}
		SetAddr(a);
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
	ImGui::SameLine();
	ImGui::Checkbox("track PC", &trackPC);
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

	if (ImGui::IsMouseDown(ImGuiMouseButton_Middle) && !fixedAddress) {
		float dy = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle, 4.0f).y;
		if (!dragging && (dy > 3.0f || dy < -3.0f)) {
			if (mousePos.x >= curPos.x && mousePos.x < (winPos.x + winSize.x) && mousePos.y >= curPos.y && mousePos.y < (winPos.y + winSize.y) && address[0] != '=') {
				dragging = true;
				mouseDragY = 0;
				dragDiff = 0;
			}
		} else {
			float fontHgt = ImGui::GetTextLineHeightWithSpacing()-2;
			dragDiff += dy - mouseDragY;
			mouseDragY = dy;
			sY = -(int)(dragDiff / fontHgt);
			dragDiff += sY * fontHgt;
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		}
	} else {
		mouseDragY = 0.0f;
		dragging = false;
	}

	// handle scroll wheel
	if (!dragging && mousePos.x >= curPos.x && mousePos.x < (winPos.x + winSize.x) && mousePos.y >= curPos.y && mousePos.y < (winPos.y + winSize.y) && address[0]!='=') {
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

	float fontCharWidth = ImGui::GetFont()->GetCharAdvance('W');// CurrFontSize();
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

	CodeContextMenu(cpu, index);

	if (trackPC) { UpdateTrackPC(cpu, dY, lines); }

	strown<128> line;
	uint16_t read = addrValue;
	int lineNum = 0;
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
		if (read == pc && !trackPC) { lastShownPCRow = lineNum; }
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

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
				strown<32> ctxId; ctxId.append("code").append_num(index, 1, 10).append("_ctx").c_str();
				ImGui::OpenPopupEx(ImGui::GetCurrentWindow()->GetID(ctxId.get()));
				contextAddr = read;
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
		line.append(pc==read ? '>' : ' ');
		if (showAddress) { line.append_num(read, 4, 16); line.append(' '); }
		int branchTrg = -1;
		int bytes = Disassemble(cpu, read, line.end(), line.left(), chars, branchTrg, showBytes, true, showLabels, showDisAsm);
		if (editAsmAddr==read&&!editAsmDone) {
			line.pad_to(' ', 14);
			ImGui::TextUnformatted(line.get(), line.end());
			ImGui::SameLine();
			editAsmDone = EditAssembly();
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
					if (addrCursor == addrValue) {
						if (!fixedAddress) {
							uint16_t addr = addrValue-1;
							while (addr && (ValidInstructionBytes(cpu, addr)+addr)!=addrValue && (addrValue-addr)<3) {
								--addr;
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

			// draw a highlight of the current PC line
			if (GetPCHighlightStyle() && pc == read) {
				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				if (GetPCHighlightStyle() == 1) {
					draw_list->AddRect(ImVec2(linePos.x + winPos.x, linePos.y + winPos.y - ImGui::GetScrollY()),
						ImVec2(linePos.x + winPos.x + ImGui::CalcTextSize(line.c_str()).x - 2.0f,
							linePos.y + winPos.y - ImGui::GetScrollY() + ImGui::GetTextLineHeightWithSpacing() - 1.0f),
						GetPCHighlightColor(), 0.0f, 0, 1.0f);
				} else {
					draw_list->AddRectFilled(ImVec2(linePos.x + winPos.x, linePos.y + winPos.y - ImGui::GetScrollY()),
						ImVec2(linePos.x + winPos.x + ImGui::CalcTextSize(line.c_str()).x - 2.0f,
							linePos.y + winPos.y - ImGui::GetScrollY() + ImGui::GetTextLineHeightWithSpacing() - 1.0f),
						GetPCHighlightColor(), 0.0f, 0);
				}
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
	lastShownAddress = prevLineAddr;

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
