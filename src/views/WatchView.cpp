// Expression View

#pragma once
#include <stdint.h>
#include "Views.h"
#include "../struse/struse.h"
#include "WatchView.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "../C64Colors.h"
#include "../Expressions.h"
#include "../Config.h"
#include "../6510.h"
#include "../Mnemonics.h"

WatchView::WatchView() : open(false), rebuildAll(false), recalcAll(false)
{
	numExpressions = 0;
	editExpression = -1;
	prevWidth = 0;
}

static void DrawBlueTextLine()
{
	const ImVec2 p = ImGui::GetCursorScreenPos();
	ImGui::GetWindowDrawList()->AddRectFilled(
		p, ImVec2(p.x + ImGui::GetColumnWidth(), p.y + ImGui::GetTextLineHeightWithSpacing()),
		ImColor(64, 49, 141, 255));
}

void WatchView::Evaluate(int index)
{
	WatchType type = WT_NORMAL;
	strref expression = expressions[index].get_strref();
	if (expression[0] == '*') {
		type = WT_BYTES;
		++expression;
	} else if (expression.has_prefix("dis")) {
		wchar_t c = expression[3];
		if (!((c >= L'A' && c <= 'Z') || (c >= L'0' && c <= '9') || (c >= L'a' && c <= 'z'))) {
			type = WT_DISASM;
			expression += 3;
		}
	}

	rpnExp[index].set_len(BuildExpression(expression.get(), (uint8_t*)rpnExp[index].charstr(), rpnExp[index].cap()));
	types[index] = type;
	EvaluateItem(index);
}

void WatchView::EvaluateItem(int index)
{
	if (index < 0 || index >= MaxExp)
		return;

	uint8_t* rpn = (uint8_t*)rpnExp[index].charstr();
	strown<64> buf;
	if (rpn && rpn[0]) {
		CPU6510 *cpu = GetCurrCPU();
		if (types[index] == WT_NORMAL) {
			int result = EvalExpression(rpn);
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
		} else if (types[index] == WT_BYTES) {
			int addr = EvalExpression(rpn);
			buf.append('$').append_num(addr, 4, 16);
			int num_bytes = int(((ImGui::GetWindowWidth() - ImGui::GetColumnWidth()) - 6 * CurrFontSize()) / (3 * CurrFontSize()));
			for (int b = 0; b < num_bytes && buf.left() > 3; b++) {
				buf.append(' ');
				buf.append_num(cpu->GetByte(addr++), 2, 16);
			}
		} else {
			int addr = EvalExpression(rpn);
			int disChars = 0, branchTrg = 0;
			buf.append('$').append_num(addr, 4, 16).append(' ');
			Disassemble(cpu, addr, buf.charend(), buf.left(), disChars, branchTrg, true, true, true);
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
		if (name.same_str("open") && type == CPT_Value) {
			open = !value.same_str("Off");
		} else if (name.same_str("Expressions") && type == CPT_Array) {
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
	{
		strown<64> title("Watch");
		title.append_num(index + 1, 1, 10);

		ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin(title.c_str(), &open)) {
			ImGui::End();
			return;
		}
	}
	CPU6510* cpu = GetCurrCPU();
	int currWidth = -1;
	int numLines = numExpressions < MaxExp ? (numExpressions + 1) : MaxExp;
	ImGui::Columns(2, "expressionDivider", true);
	for (int i = 0; i < numLines; ++i) {
		if (i != editExpression) {
			if (ImGui::IsMouseClicked(0)) {
				ImVec2 mousePos = ImGui::GetMousePos();
				ImVec2 winPos = ImGui::GetWindowPos();
				ImVec2 cursorPos = ImGui::GetCursorPos();
				float dx = mousePos.x - winPos.x - cursorPos.x;
				float dy = mousePos.y - winPos.y - cursorPos.y;
				if (dx >= 0.0f && dx < ImGui::GetColumnWidth() && dy >= 0 && dy < CurrFontSize()) {
					editExpression = i;
				}
			}
		}
		if (rebuildAll) { Evaluate(i); } else if (recalcAll) { EvaluateItem(i); }
		if (i != editExpression) {
			if ((i & 1) == 0) { DrawBlueTextLine(); }
			ImGui::Text(expressions[i].c_str());
			if (cpu->MemoryChange()) { Evaluate(i); }
		} else if (ImGui::InputTextEx("##WatchExpression", "Watch Expression", expressions[i].charstr(), expressions[i].cap(),
									  ImVec2(ImGui::GetColumnWidth(), CurrFontSize()), ImGuiInputTextFlags_EnterReturnsTrue)) {
			expressions[i].set_len((strl_t)strlen(expressions[i].get()));
			Evaluate(i);
			if (i >= numExpressions) { numExpressions = i + 1; }
			editExpression = -1;
		}
		ImGui::NextColumn();
		if (currWidth < 0) { currWidth = (int)ImGui::GetColumnWidth(); }
		if ((i & 1) != 0) { DrawBlueTextLine(); }
		ImGui::Text(results[i].c_str());
		ImGui::NextColumn();
	}
	rebuildAll = false;
	recalcAll = false;
	if (currWidth != prevWidth) {
		prevWidth = currWidth;
		recalcAll = true;
	}
	ImGui::Columns(1);
	ImGui::End();

}

