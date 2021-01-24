#include "../imgui/imgui.h"
#include "../Files.h"
#include "../struse/struse.h"
#include "../C64Colors.h"
#include "../SourceDebug.h"
#include "PreView.h"

void PreView::Draw()
{
	ImGui::SetNextWindowSize(ImVec2(740.0f, 410.0f), ImGuiCond_FirstUseEver);
	if (open && ImGui::Begin(mode == Mode::Listing ? "Review Listing" : "Source Context", &open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings)) {
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

			ImGui::Separator();
			ImGui::Text("Select source column (%d)", listingCodeColumn+1);
			if (ImGui::Button("Ok")) { open = false; ListingToSrcDebug(listingCodeColumn); }
			ImGui::SameLine();
			if( ImGui::Button("Cancel")) { open = false; }
			ImGui::Separator();

			ImVec2 winSize = ImGui::GetWindowSize();
			int chrWide = (int)(winSize.x / fontWidth) - 2;

			if (chrWide < 1) { chrWide = 1; }

			ImVec2 srcCursor = ImGui::GetCursorPos();

			ImVec2 sc = ImGui::GetCursorScreenPos();
			ImVec2 cc(sc.x + fontWidth * listingCodeColumn, sc.y);
			ImVec2 br = ImGui::GetWindowPos();
			br.x += winSize.x-fontWidth; br.y += winSize.y;

			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->AddRectFilled(cc, br, ImColor(C64_PURPLE));

			ImVec2 mouse = ImGui::GetMousePos();
			if (mouse.x > sc.x && mouse.x < br.x && mouse.y > sc.y && mouse.y < br.y) {
				int col = (int)((mouse.x - sc.x) / fontWidth);
				ImVec2 mc(sc.x + fontWidth * col, sc.y);
				dl->AddRect(mc, ImVec2(mc.x + fontWidth, br.y), ImColor(C64_WHITE));
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					listingCodeColumn = (int)((mouse.x - sc.x) / fontWidth);
				}
			}

			ImGui::SetCursorPos(srcCursor);

			while (strref line = show.line()) {
				if (ImGui::GetCursorPosY() < (winSize.y-fontHgt)) {
					ImGui::Text(STRREF_FMT, STRREF_ARG(line));
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
