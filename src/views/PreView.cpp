#include "../imgui/imgui.h"
#include "../Files.h"
#include "../struse/struse.h"
#include "PreView.h"

void PreView::Draw(const char* title)
{
	ImGui::SetNextWindowSize(ImVec2(740.0f, 410.0f), ImGuiCond_FirstUseEver);
	if (open && ImGui::Begin(title, &open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings)) {
		if (mode == Mode::Listing) {

			strref show = currFile + currFileTopOffs;


			float fontWidth = ImGui::GetFont()->GetCharAdvance('W');
			float fontHgt = ImGui::GetFont()->FontSize;

			// CONSIDER: allow moving up/down in the file
			//uint32_t numLines = (uint32_t)(ImGui::GetWindowSize().y / fontHgt);
			//uint32_t linesLeft = currFileNumLines - currFileLine;
			//
			//while (currFileLine && (numLines > 4) && (linesLeft > (numLines - 4))) {
			//	// step back 1 line
			//}
			//

			ImVec2 winSize = ImGui::GetWindowSize();
			int chrWide = (int)(winSize.x / fontWidth) - 2;

			if (chrWide < 1) { chrWide = 1; }

			while (strref line = show.line()) {
				if (ImGui::GetCursorPosY() < winSize.y) {
					strown<256> codeLine;
					if ((int)line.get_len() > chrWide) { line.clip(chrWide); }
					codeLine.copy(line);
					ImGui::Text(codeLine.c_str());
				} else {
					break;
				}
			}
		}
		ImGui::End();
	}
}

static bool IsAddress(strref num)
{
	for (int i = 0; i < 4; ++i) {
		if (!strref::is_hex(num.get_at(i))) {
			return false;
		}
	}

	return !strref::is_hex(num.get_at(4));
}

void PreView::ShowListing(strref listing)
{
	open = true;
	mode = Mode::Listing;
	currFile = listing;

	// find a likely line
	strref prev = listing, curr = listing;
	uint32_t line_index = 0;
	while (strref line = curr.line()) {
		if ((line[0] == '$' && line.get_len() > 5 && IsAddress(line + 1)) ||
			(line.get_len() > 4 && IsAddress(line))) {
			curr = prev;
			break;
		}
		++line_index;
		prev = curr;
	}
	currFileTopOffs = (uint32_t)(curr.get() - listing.get());
	currFileLine = line_index;
	currFileNumLines = listing.count_lines();
}
