// Expression View

#include <stdint.h>
#include "Views.h"
#include "GLFW/glfw3.h"
#include "../struse/struse.h"
#include "WatchView.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../C64Colors.h"
#include "../Expressions.h"
#include "../Config.h"
#include "../6510.h"
#include "../Mnemonics.h"
#include "../Sym.h"
#include "../C64Colors.h"

WatchView::WatchView() : open(false), rebuildAll(false), recalcAll(false), forceEdit(false), 
  activeIndex(-1)
{
	numExpressions = 0;
	editExpression = -1;
	prevWidth = 0;
	for (int t = 0, nt = sizeof(types) / sizeof(types[0]); t < nt; ++t) {
		types[t] = WatchType::WT_NORMAL;
	}
	memset(values, 0, sizeof(values));
}

static void DrawBlueTextLine() {
	const ImVec2 p = ImGui::GetCursorScreenPos();
	ImGui::GetWindowDrawList()->AddRectFilled(
		p, ImVec2(p.x + ImGui::GetColumnWidth(), p.y + ImGui::GetTextLineHeightWithSpacing()),
		ImColor(64, 49, 141, 255));
}

void WatchView::Evaluate(int index) {
	WatchType type = WatchType::WT_NORMAL;
	strref expression = expressions[index].get_strref();
	if (expression[0] == '*') {
		type = WatchType::WT_BYTES;
		++expression;
	} else if (expression.has_prefix("dis")) {
		char c = expression[3];
		if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'))) {
			type = WatchType::WT_DISASM;
			expression += 3;
		}
	}

	rpnExp[index].set_len(BuildExpression(expression.get(), (uint8_t*)rpnExp[index].charstr(), rpnExp[index].cap()));
	types[index] = type;
	EvaluateItem(index);
}

void WatchView::EvaluateItem(int index) {
	if (index < 0 || index >= MaxExp) {
		return;
	}

	uint8_t* rpn = (uint8_t*)rpnExp[index].charstr();
	strown<64> buf;
	if (rpn && rpn[0]) {
		CPU6510 *cpu = GetCurrCPU();
		if (types[index] == WatchType::WT_NORMAL) {
			int result = EvalExpression(rpn);
			values[index] = result;
			if (result < 0) {
				buf.append('-');
				result = -result;
			}
			buf.append('$');
			if (result > 256) {
				if (result > 65536)
					buf.append_num(result, 6, 16);
				else
					buf.append_num(result, 4, 16);
			} else
				buf.append_num(result, 2, 16);
		} else if (types[index] == WatchType::WT_BYTES) {
			int addr = EvalExpression(rpn);
			buf.append('$').append_num(addr, 4, 16);
			values[index] = addr;
			int num_bytes = int(((ImGui::GetWindowWidth() - ImGui::GetColumnWidth()) - 6 * CurrFontSize()) / (3 * CurrFontSize()));
			for (int b = 0; b < num_bytes && buf.left() > 3; b++) {
				buf.append(' ');
				buf.append_num(cpu->GetByte(addr++), 2, 16);
			}
		} else {
			int addr = EvalExpression(rpn);
			int disChars = 0, branchTrg = 0;
			buf.append('$').append_num(addr, 4, 16).append(' ');
			values[index] = addr;
			Disassemble(cpu, addr, buf.charend(), buf.left(), disChars, branchTrg, true, true, true, true);
			buf.add_len(disChars);
		}
	}
	results[index].copy(buf.get_strref());
}

void WatchView::WriteConfig(UserData& config)
{
	config.AddValue(strref("open"), config.OnOff(open));
	config.BeginArray("Expressions");
	for (int e = 0; e < numExpressions; e++) {
		strown<128> arg;
		arg.append('"').append(expressions[e].get_strref()).append('"');
		config.AddValue(strref(), arg.get_strref());
	}
	config.EndArray();
}

void WatchView::ReadConfig(strref config)
{
	ConfigParse conf(config);
	while (!conf.Empty()) {
		strref name, value;
		ConfigParseType type = conf.Next(&name, &value);
		if (name.same_str("open") && type == ConfigParseType::CPT_Value) {
			open = !value.same_str("Off");
		} else if (name.same_str("Expressions") && type == ConfigParseType::CPT_Array) {
			rebuildAll = true;
			ConfigParse exp(value);
			numExpressions = 0;
			while (!exp.Empty() && numExpressions < 64) {
				strref quote = exp.ArrayElement();
				quote.trim_whitespace();
				if (quote[0] == '"') { quote += 1; }
				if (quote.get_last() == '"') { quote.clip(1); }
				expressions[numExpressions++].copy(quote);
			}
		}
	}
}

void WatchView::Draw(int index)
{
	if (!open) { return; }
	ImGuiID id = 0;
	{
		strown<64> title("Watch");
		title.append_num(index + 1, 1, 10);

		ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin(title.c_str(), &open)) {
			ImGui::End();
			return;
		}
		id = ImGui::GetCurrentWindow()->ID;
	}
	bool entered = false;
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AddressDragDrop")) {
			IM_ASSERT(payload->DataSize == sizeof(SymbolDragDrop));
			SymbolDragDrop* drop = (SymbolDragDrop*)payload->Data;
			if (numExpressions < MaxExp) {
				expressions[numExpressions].clear();
				if (drop->address < 0x10000) { expressions[numExpressions].append('*'); }
				expressions[numExpressions].append(drop->symbol).c_str();
				++numExpressions;
				rebuildAll = true;
			}
		}
		ImGui::EndDragDropTarget();
	}

	CPU6510* cpu = GetCurrCPU();
	int currWidth = -1;
	int numLines = numExpressions < MaxExp ? (numExpressions + 1) : MaxExp;
	ImGui::Columns(2, "expressionDivider", true);
	ImVec2 activeRowPos(0, 0);
	for (int i = 0; i < numLines; ++i) {
		ImVec2 cursorPos = ImGui::GetCursorPos();
		ImVec2 winPos = ImGui::GetWindowPos();
		if (i != editExpression) {
			if (ImGui::IsMouseClicked(0)) {
				ImVec2 mousePos = ImGui::GetMousePos();
				float dx = mousePos.x - winPos.x - cursorPos.x;
				float dy = mousePos.y - winPos.y - cursorPos.y;
				if (dx >= 0.0f && dx < ImGui::GetWindowWidth() && dy >= 0 && dy < CurrFontSize()) {
					activeIndex = i;
					if (dx < ImGui::GetColumnWidth()) {
						editExpression = i;
					}
				}
			}
		}
		if (rebuildAll) { Evaluate(i); } else if (recalcAll) { EvaluateItem(i); }
		if (i != editExpression) {
			if ((i & 1) == 0) { DrawBlueTextLine(); }
			ImGui::Text(expressions[i].c_str());
			if (cpu->MemoryChange()) { Evaluate(i); }
		} else {
			if (forceEdit) {
				ImGui::SetKeyboardFocusHere();
				forceEdit = false;
			}
			if (ImGui::InputTextEx("##WatchExpression", "Watch Expression", expressions[i].charstr(), expressions[i].cap(),
				ImVec2(ImGui::GetColumnWidth(), CurrFontSize()), ImGuiInputTextFlags_EnterReturnsTrue)) {
				expressions[i].set_len((strl_t)strlen(expressions[i].get()));
				Evaluate(i);
				if (i >= numExpressions) { numExpressions = i + 1; }
				editExpression = -1;
				entered = true;
			}
		}
		ImGui::NextColumn();
		if (currWidth < 0) { currWidth = (int)ImGui::GetColumnWidth(); }
		if ((i & 1) != 0) { DrawBlueTextLine(); }
		ImGui::Text(results[i].c_str());

		if (activeIndex == i /*&& GImGui->ActiveId == id*/) {
			activeRowPos = ImVec2(cursorPos.x + winPos.x, cursorPos.y + winPos.y);
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			SymbolDragDrop drag;
			drag.address = values[i];
			strovl lblStr(drag.symbol, sizeof(drag.symbol));
			lblStr.copy(expressions[i] + (expressions[i][0] == '*' ? 1 : 0)); lblStr.c_str();
			ImGui::SetDragDropPayload("AddressDragDrop", &drag, sizeof(drag));
			ImGui::Text("%s: $%04x", expressions[i].charstr() + (expressions[i][0]=='*' ? 1 : 0), values[i]);
			ImGui::EndDragDropSource();
		}
		ImGui::NextColumn();
	}
	rebuildAll = false;
	recalcAll = false;
	if (currWidth != prevWidth) {
		prevWidth = currWidth;
		recalcAll = true;
	}
	ImGui::Columns(1);

	if (activeIndex >= 0 && GImGui->NavWindow == ImGui::GetCurrentWindow()) {
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 winPos = ImGui::GetWindowPos();
		draw_list->AddRect(activeRowPos,
			ImVec2(activeRowPos.x + ImGui::GetWindowWidth() - 1.0f,
				activeRowPos.y + ImGui::GetTextLineHeightWithSpacing() - 1.0f),
			ImColor(C64_LGREEN), 0.0f, 0, 1.0f);
		if (ImGui::IsKeyPressed(GLFW_KEY_UP) && activeIndex > 0) {
			--activeIndex;
			editExpression = -1;
		} else if (ImGui::IsKeyPressed(GLFW_KEY_DOWN) && activeIndex < numExpressions) {
			++activeIndex;
			editExpression = -1;
		} else if (ImGui::IsKeyPressed(GLFW_KEY_DELETE) && activeIndex < numExpressions) {
			for (int i = activeIndex, n=numExpressions-1; i < n; ++i) {
				expressions[i] = expressions[i + 1];
				rpnExp[i] = rpnExp[i + 1];
				results[i] = results[i + 1];
			}
			--numExpressions;
			expressions[numExpressions].clear();
			rpnExp[numExpressions].clear();
			results[numExpressions].clear();
			editExpression = -1;
		} else if (ImGui::IsKeyPressed(GLFW_KEY_INSERT) && numExpressions < MaxExp) {
			for (int i = numExpressions; i > activeIndex; --i) {
				expressions[i] = expressions[i - 1];
				rpnExp[i] = rpnExp[i - 1];
				results[i] = results[i - 1];
			}
			++numExpressions;
			expressions[activeIndex].clear();
			rpnExp[activeIndex].clear();
			results[activeIndex].clear();
			editExpression = -1;
		} else if (ImGui::IsKeyPressed(GLFW_KEY_ENTER) && !entered) {
			editExpression = activeIndex;
			forceEdit = true;
		}
	}

	ImGui::End();

}

