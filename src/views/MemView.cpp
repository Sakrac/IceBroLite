#include "MemView.h"
#include "../imgui/imgui.h"
#include "../struse/struse.h"
#include <malloc.h>
#include "Views.h"
#include "../6510.h"
#include "../Expressions.h"
//#include "ViceConnect.h"
#include "../Config.h"
#include "../ImGui_Helper.h"
#include "../imgui/imgui_internal.h"
#include "../Sym.h"
#include "GLFW/glfw3.h"

MemView::MemView() : fixedAddress(false), open(false), evalAddress(false)
{
	SetAddr(0x400);

	spanValue = 0;
	span[0] = 0;
	address[0] = 0;

	cursor[0] = 6;
	cursor[1] = 5;

	showAddress = true;
	showHex = true;
	showText = true;

	cursorTime = 0.0f;
	mouseWheelDiff = 0.0f;
}

#define CursorFlashPeriod 64.0f/50.0f

uint8_t ScreenToAscii(uint8_t s)
{
	if (s==0) { return '@'; }
	if (s<=0x1a) { return s+'a'-1; }
	if (s==0x1b) { return '['; }
	if (s==0x1e) { return ']'; }
	if (s>=0x61&&s<=0x7a) { return s-0x20; }
	if (s<0x20||s > 0x40) { return '.'; }
	return s;
}

void MemView::SetAddr(uint16_t addr)
{
	addrValue = addr;
	if (!fixedAddress) {
		strovl addr(address, sizeof(address));
		addr.clear();
		addr.append('$').append_num(addrValue, 4, 16).c_str();
	}
}

void MemView::Draw(int index)
{
	CPU6510* cpu = GetCurrCPU();
	if (!open) { return; }
	{
		strown<64> title("Mem");
		title.append_num(index+1, 1, 10);

		ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
		bool active = ImGui::Begin(title.c_str(), &open);

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AddressDragDrop")) {
				IM_ASSERT(payload->DataSize == sizeof(SymbolDragDrop));
				SymbolDragDrop* drop = (SymbolDragDrop*)payload->Data;
				if (drop->address < 0x10000) {
					addrValue = drop->address;
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

	cursorTime += ImGui::GetIO().DeltaTime;
	if (cursorTime>=CursorFlashPeriod) { cursorTime -= CursorFlashPeriod; }
	{
		strown<64> field("address##");
		field.append_num(index+1, 1, 10);
		ImGui::Columns(2, "memViewCokumns", false);  // 3-ways, no border
		if (ImGui::InputText(field.c_str(), address, sizeof(address), ImGuiInputTextFlags_EnterReturnsTrue)) {
			fixedAddress = address[0]=='=';
			SetAddr(ValueFromExpression(address+(fixedAddress ? 1 : 0)));
		}
		ImGui::NextColumn();

		field.copy("span");
		field.append_num(index+1, 1, 10);
		if (ImGui::InputText(field.c_str(), span, sizeof(span))) {
			spanValue = ValueFromExpression(span);
			if (spanValue>256) { spanValue = 256; }
		}
	}

	if (evalAddress||(fixedAddress && cpu->MemoryChange())) {
		SetAddr(ValueFromExpression(address+(fixedAddress ? 1 : 0)));
		spanValue = span ? ValueFromExpression(span) : 0;
		evalAddress = false;
	}

	strown<32> ctxId; ctxId.append("code").append_num(index, 1, 10).append("_ctx").c_str();
	if (ImGui::BeginPopupEx(ImGui::GetCurrentWindow()->GetID(ctxId.get()),
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {
		if (ImGui::MenuItem("Lowercase", NULL, textLowercase)) {
			textLowercase = !textLowercase;
		}
		ImGui::EndPopup();
	}

	int petsciiFont = PetsciiFont();

	ImGui::Columns(1);
	{
		strown<16> addrStr;
		addrStr.append_num(addrValue, 4, 16);
		ImGui::TextUnformatted(addrStr.c_str());
		ImGui::SameLine();
		ImGui::Checkbox("addr", &showAddress);
		ImGui::SameLine();
		ImGui::Checkbox("hex", &showHex);
		ImGui::SameLine();
		ImGui::Checkbox("text", &showText);
		ImGui::SameLine();
		ImGui::Checkbox("case", &textLowercase);
	}
	ImGui::BeginChild(ImGui::GetID("hexEdit"));

	uint32_t prevAddrValue = addrValue;

	if (showHex||showText) {
		bool active = KeyboardCanvas("HexView");// IsItemActive();

		wasActive = active;

		// force font spacing
		float fontWidth = ImGui::GetFont()->GetCharAdvance('D');
		float fontHgt = ImGui::GetFont()->FontSize;
		uint32_t charWid = (uint32_t)(ImGui::GetWindowWidth()/fontWidth);

		uint32_t byteChars = (showHex ? 3 : 0)+(showText ? 1 : 0);
		if (showAddress) { charWid -= 5; }
		if (showHex && showText) { charWid--; }
		uint32_t spanWin = spanValue ? spanValue : charWid/byteChars;
		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 curPos = ImGui::GetCursorScreenPos();
		ImVec2 winPos = ImGui::GetWindowPos();
		ImVec2 winSize = ImGui::GetWindowSize();

		if (ImGui::IsMouseDown(ImGuiMouseButton_Middle) && !fixedAddress) {
			float dy = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle, 4.0f).y;
			if (!dragging) {
				if (mousePos.x >= curPos.x && mousePos.x < (winPos.x + winSize.x) && mousePos.y >= curPos.y && mousePos.y < (winPos.y + winSize.y) && address[0] != '=') {
					if (dy > 3.0f || dy < -3.0f) {
						dragging = true;
						mouseDragY = 0;
						dragDiff = 0;
					}
				}
			} else {
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				dragDiff += dy - mouseDragY;
				mouseDragY = dy;
				while (dragDiff < -fontHgt) {
					dragDiff += fontHgt;
					addrValue += spanWin;
				}
				while (dragDiff > fontHgt) {
					dragDiff -= fontHgt;
					addrValue -= spanWin;
				}
			}
		} else if (dragging) {
			mouseDragY = 0.0f;
			dragging = false;
			ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		}



		// handle scroll wheel
		if (mousePos.x >= curPos.x && mousePos.x < (winPos.x + winSize.x) && mousePos.y >= curPos.y && mousePos.y < (winPos.y + winSize.y) && address[0] != '=') {
			mouseWheelDiff += ImGui::GetIO().MouseWheel;
			if (mouseWheelDiff < -0.5f) { addrValue += spanWin; mouseWheelDiff += 1.0f; } 
			else if (mouseWheelDiff > 0.5) { addrValue -= spanWin; mouseWheelDiff -= 1.0f; }
		} else {
			mouseWheelDiff = 0.0f;
		}

		if (ImGui::IsMouseClicked(0)) {
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 winPos = ImGui::GetWindowPos();
			ImVec2 winSize = ImGui::GetWindowSize();
			if (mousePos.x>=winPos.x && mousePos.y>=winPos.y &&
				mousePos.x<(winPos.x+winSize.x)&&mousePos.y<(winPos.y+winSize.y)) {
				float mx = mousePos.x - winPos.x;
				if (showAddress) { mx -= fontWidth * 5; }
				int byte = (int)((mx + 0.5f * fontWidth) / (3.0f * fontWidth));
				if (byte >= 0 && (uint32_t)byte < spanWin) {
					int nib = (int)((mx - (byte * 3.0f * fontWidth)) / fontWidth);
					cursor[0] = byte * 2 + nib;
					cursor[1] = int((mousePos.y - winPos.y) / ImGui::GetTextLineHeightWithSpacing());
				}
			}
		}

		int lines = int(ImGui::GetWindowHeight()/ImGui::GetTextLineHeightWithSpacing());

		if (active) {
			int curX = cursor[0], curY = cursor[1];
			int dX = 0, dY = 0;

			if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_UP)) { dY--; }
			if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_DOWN)) { dY++; }
			if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_LEFT)) { dX--; }
			if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_RIGHT)) { dX++; }
			if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_PAGE_UP)) { addrValue -= spanWin * (lines/2); }
			if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_PAGE_DOWN)) { addrValue += spanWin * (lines/2); }

			if (showHex) {
				curX += dX; curY += dY;
				if (curX >= int(spanWin * 2)) { curY += 1; curX = 0; } else if (curX < 0) { curY -= 1; cursor[0] = spanWin * 2 - 1; } else { cursor[0] = curX; }
				if (curY < 0) { addrValue -= spanWin; } else if (curY >= lines) { addrValue += spanWin; } else { cursor[1] = curY; }
			} else if( dY ) {
				addrValue += dY * spanWin;
			}
		}

		strown<1024> line;
		uint16_t read = addrValue;
		for(int lineNum = 0; lineNum < lines; ++lineNum) {
			line.clear();
			if (showAddress) { line.append_num(read, 4, 16).append(' ');  }
			if (showHex) {
				uint16_t bytes = read;
				for (uint32_t c = 0; c<spanWin; ++c) {
					line.append_num(cpu->GetByte(bytes++), 2, 16).append(' ');
				}
			}
			if (showText && petsciiFont<0) {
				uint16_t chars = read;
				uint32_t base = textLowercase ? (uint32_t)0xee00 : (uint32_t)0xef00;
				for (uint32_t c = 0; c<spanWin; ++c) {
					uint32_t code = base + cpu->GetByte(chars++);
					line.push_utf8(code);
				}
			}
			ImGui::Text(line.c_str());
			if (showText && petsciiFont >= 0) {
				float yPos = ImGui::GetCursorPosY();
				ImGui::SameLine();
				ImGui::SetCursorPosX(line.len()* fontWidth);
				ViewPushFont(petsciiFont);
				line.clear();
				uint16_t chars = read;
				for (uint32_t c = 0; c < spanWin; ++c) {
					line.push_utf8((textLowercase ? 0xee00 : 0xef00) + cpu->GetByte(chars++));
				}
				ImGui::Text(line.c_str());
				ImGui::PopFont();
				ImGui::SetCursorPosY(yPos);
			}


			read += spanWin;
		}

		// keyboard
		if (active && showHex) {
			int col0 = 0;
			int colT = spanWin * 2;
			if (showHex && cursor[0]<colT) {
				uint16_t a = addrValue + cursor[0]/2 + cursor[1]*spanWin;
				int nib = cursor[0] & 1;
				uint8_t b = InputHex();
				if (b!=0xff) {
					uint8_t byte = cpu->GetByte(a);
					if (nib) {
						byte = (byte&0xf0)|b;
						++cursor[0];
						if (cursor[0]>=colT) {
							cursor[0] = col0;
							++cursor[1];
						}
					} else {
						byte = (byte&0xf)|(b<<4);
						++cursor[0];
					}
					cpu->SetByte(a, byte);
				}
			}
			// cursor
			if (cursorTime>(0.5f*CursorFlashPeriod)) {
				const ImGuiStyle style = ImGui::GetStyle();

				int cx = (cursor[0] & 1) + (cursor[0] / 2) * 3 + (showAddress ? 5 : 0);

				ImGui::SetCursorPos(ImVec2(fontWidth * cx, ImGui::GetTextLineHeightWithSpacing() * cursor[1]));
				const ImVec2 p = ImGui::GetCursorScreenPos();
				ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x+fontWidth, p.y+fontHgt),
					ImColor(255, 255, 255));
				strown<16> curChr;
				if (showHex && cursor[0]<colT) {
					uint8_t b = cpu->GetByte(addrValue+cursor[0]/2+cursor[1]*spanWin);
					int nib = (cursor[0]-col0)&1;
					curChr.append(strref::num_to_char((b>>(nib ? 0 : 4))&0xf));
				} else if (showText) {
					uint8_t b = cpu->GetByte(addrValue+(cursor[0]-colT)+cursor[1]*spanWin);
					curChr.push_utf8(0xee00+b);//curChr[ 0 ] = ScreenToAscii( b );
				}
				ImGui::TextColored(style.Colors[ImGuiCol_ChildBg], curChr.c_str());
			}
		}
	}
	if (addrValue != prevAddrValue) {
		strovl addrStr(address, (strl_t)sizeof(address));
		addrStr.append('$').append_num(addrValue, 4, 16).c_str();
	}
	ImGui::EndChild();

	ImGui::End();
}

void MemView::WriteConfig(UserData& config)
{
	config.AddValue(strref("open"), config.OnOff(open));
	config.AddValue(strref("address"), strref(address));
	config.AddValue(strref("span"), strref(span));
	config.AddValue(strref("showAddress"), config.OnOff(showAddress));
	config.AddValue(strref("showHex"), config.OnOff(showHex));
	config.AddValue(strref("showText"), config.OnOff(showText));
}

void MemView::ReadConfig(strref config)
{
	ConfigParse conf(config);
	while (!conf.Empty()) {
		strref name, value;
		ConfigParseType type = conf.Next(&name, &value);
		if (name.same_str("open")&&type== ConfigParseType::CPT_Value) {
			open = !value.same_str("Off");
		} else if (name.same_str("address")&&type== ConfigParseType::CPT_Value) {
			strovl addr(address, sizeof(address));
			addr.copy(value); addr.c_str(); evalAddress = true;
		} else if (name.same_str("span")&&type== ConfigParseType::CPT_Value) {
			strovl spn(span, sizeof(span));
			spn.copy(value); spn.c_str(); evalAddress = true;
		} else if (name.same_str("showAddress")&&type== ConfigParseType::CPT_Value) {
			showAddress = !value.same_str("Off");
		} else if (name.same_str("showHex")&&type== ConfigParseType::CPT_Value) {
			showHex = !value.same_str("Off");
		} else if (name.same_str("showText")&&type== ConfigParseType::CPT_Value) {
			showText = !value.same_str("Off");
		}
	}
}




// if I lose track of keyboardcanvas
/*
imgui.h (460):
IMGUI_API bool          KeyboardCanvas(const char* label);

imgui_widgets.cpp (2839):
bool ImGui::KeyboardCanvas( const char* label )
{
	ImGuiWindow* window = GetCurrentWindow();
	if( window->SkipItems )
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiIO& io = g.IO;

	const ImGuiID id = window->GetID( label );

	const bool hovered = IsWindowHovered();
	const bool focus_requested = FocusableItemRegister( window, id );    // Using completion callback disable keyboard tabbing

	const bool user_clicked = hovered && io.MouseClicked[ 0 ];
	const bool user_nav_input_start = (g.ActiveId != id) && ((g.NavInputId == id) || (g.NavActivateId == id && g.NavInputSource == ImGuiInputSource_NavKeyboard));

	if( focus_requested || user_clicked || user_nav_input_start )
	{
		if( g.ActiveId != id )
		{
			SetActiveID( id, window );
			SetFocusID( id, window );
			FocusWindow( window );
		}
	}
	else if( g.ActiveId == id && io.MouseClicked[ 0 ] )
		ClearActiveID();

	return g.ActiveId == id;
}
*/

