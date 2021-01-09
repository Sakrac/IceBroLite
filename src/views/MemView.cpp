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
		if (!ImGui::Begin(title.c_str(), &open)) {
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
	}


	//	else { ImGui::Text( "Not Clicked" ); }
	ImGui::BeginChild(ImGui::GetID("hexEdit"));

	if (showHex||showText) {
		bool active = KeyboardCanvas("HexView");// IsItemActive();

		wasActive = active;

		// force font spacing
		uint32_t charWid = (uint32_t)(ImGui::GetWindowWidth()/CurrFontSize());

		uint32_t byteChars = (showHex ? 3 : 0)+(showText ? 1 : 0);
		if (showAddress) { charWid -= 5; }
		if (showHex && showText) { charWid--; }
		uint32_t spanWin = spanValue ? spanValue : charWid/byteChars;

		if (ImGui::IsMouseClicked(0)) {
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 winPos = ImGui::GetWindowPos();
			ImVec2 winSize = ImGui::GetWindowSize();
			if (mousePos.x>=winPos.x && mousePos.y>=winPos.y &&
				mousePos.x<(winPos.x+winSize.x)&&mousePos.y<(winPos.y+winSize.y)) {
				cursor[0] = int((mousePos.x-winPos.x)/ CurrFontSize());
				cursor[1] = int((mousePos.y-winPos.y)/ImGui::GetTextLineHeightWithSpacing());
				if (showAddress && cursor[0]<5) { cursor[0] = 5; }
			}
		}

		int lines = int(ImGui::GetWindowHeight()/ImGui::GetTextLineHeightWithSpacing());

		if (active) {
			int curXMin = showAddress ? 5 : 0;
			int curXMax = curXMin+(showHex ? (3*spanWin-1) : 0)+(showText ? spanWin : 0)+((showHex && showText) ? 1 : 0)-1;
			int curX = cursor[0], curY = cursor[1];
			int dX = 0, dY = 0;

			if (ImGui::IsKeyPressed(GLFW_KEY_UP)) { dY--; }
			if (ImGui::IsKeyPressed(GLFW_KEY_DOWN)) { dY++; }
			if (ImGui::IsKeyPressed(GLFW_KEY_LEFT)) { dX--; }
			if (ImGui::IsKeyPressed(GLFW_KEY_RIGHT)) { dX++; }
			if (ImGui::IsKeyPressed(GLFW_KEY_PAGE_UP)) { addrValue -= spanWin * (lines/2); }
			if (ImGui::IsKeyPressed(GLFW_KEY_PAGE_DOWN)) { addrValue += spanWin * (lines/2); }

			curX += dX; curY += dY;

			if (curX > curXMax) { curY += 1; } else if (curX < curXMin) { curY -= 1; cursor[0] = curXMax; } else { cursor[0] = curX; }
			if (curY<0) { addrValue -= spanWin; } else if (curY>=lines) { addrValue += spanWin; } else { cursor[1] = curY; }
		}

		strown<1024> line;
		uint16_t read = addrValue;
//		float leftPos = ImGui::GetCursorPosX();
		for(int lineNum = 0; lineNum < lines; ++lineNum) {
			line.clear();
			if (showAddress) { line.append_num(read, 4, 16).append(' ');  }
			if (showHex) {
				uint16_t bytes = read;
				for (uint32_t c = 0; c<spanWin; ++c) {
					line.append_num(cpu->GetByte(bytes++), 2, 16).append(' ');
				}
			}
			if (showText) {
				uint16_t chars = read;
				for (uint32_t c = 0; c<spanWin; ++c) {
					line.push_utf8(0xee00+cpu->GetByte(chars++));
				}
			}
			ImGui::Text(line.c_str());
			read += spanWin;
		}

		// keyboard
		if (active) {
			int col0 = showAddress ? 5 : 0;
			int colT = col0+(showHex ? (3*spanWin) : 0);
			// type?
			if (showHex && cursor[0]<colT) {
				uint16_t a = addrValue+(cursor[0]-col0)/3+cursor[1]*spanWin;
				int nib = (cursor[0]-col0)%3;
				uint8_t b = InputHex();
				if (b!=0xff&&nib!=2) {
					uint8_t byte = cpu->GetByte(a);
					if (nib) {
						byte = (byte&0xf0)|b;
						cursor[0] += 2;
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
			} /*else if( showText ) {
				uint16_t a = addrValue + (cursor[ 0 ] - colT) + cursor[ 1 ] * spanWin;
			}*/

			// cursor
			if (cursorTime>(0.5f*CursorFlashPeriod)) {
				const ImGuiStyle style = ImGui::GetStyle();
				ImGui::SetCursorPos(ImVec2(CurrFontSize() * cursor[0], ImGui::GetTextLineHeightWithSpacing() * cursor[1]));
				const ImVec2 p = ImGui::GetCursorScreenPos();
				ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x+ CurrFontSize(), p.y+ CurrFontSize()),
					ImColor(255, 255, 255));
				strown<16> curChr;
				if (showHex && cursor[0]<colT) {
					uint8_t b = cpu->GetByte(addrValue+(cursor[0]-col0)/3+cursor[1]*spanWin);
					int nib = (cursor[0]-col0)%3;
					if (nib<2) { curChr.append(strref::num_to_char((b>>(nib ? 0 : 4))&0xf)); }
				} else if (showText) {
					uint8_t b = cpu->GetByte(addrValue+(cursor[0]-colT)+cursor[1]*spanWin);
					curChr.push_utf8(0xee00+b);//curChr[ 0 ] = ScreenToAscii( b );
				}
				ImGui::TextColored(style.Colors[ImGuiCol_ChildBg], curChr.c_str());
			}
		}
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
		if (name.same_str("open")&&type==CPT_Value) {
			open = !value.same_str("Off");
		} else if (name.same_str("address")&&type==CPT_Value) {
			strovl addr(address, sizeof(address));
			addr.copy(value); evalAddress = true;
		} else if (name.same_str("span")&&type==CPT_Value) {
			strovl spn(span, sizeof(span));
			spn.copy(value); evalAddress = true;
		} else if (name.same_str("showAddress")&&type==CPT_Value) {
			showAddress = !value.same_str("Off");
		} else if (name.same_str("showHex")&&type==CPT_Value) {
			showHex = !value.same_str("Off");
		} else if (name.same_str("showText")&&type==CPT_Value) {
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

