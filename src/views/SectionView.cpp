#include <inttypes.h>
#include "../imgui/imgui.h"
#include "../Sym.h"
#include "../struse/struse.h"
#include "../Config.h"
#include "Views.h"
#include "SectionView.h"

SectionView::SectionView() : open(false)
{
}

void SectionView::WriteConfig(UserData& config)
{
    config.AddValue(strref("open"), config.OnOff(open));
}

void SectionView::ReadConfig(strref config)
{
	ConfigParse conf(config);
	while (!conf.Empty()) {
		strref name, value;
		ConfigParseType type = conf.Next(&name, &value);
		if (name.same_str("open") && type == ConfigParseType::CPT_Value) {
			open = !value.same_str("Off");
		}
	}
}

#define kMaxHiddenSections 128

void SectionView::Draw()
{
    if (!open) { return; }
    ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Sections", &open)) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Hide All")) {
        HideAllSections();
    }
    ImGui::SameLine();
    if (ImGui::Button("Show All")) {
        ShowAllSections();
    }

    size_t nSections = NumSections();
    for (size_t s = 0; s < nSections; ++s) {
        const char* name = GetSectionName(s);
        uint64_t hash = strref(name).fnv1a_64();
        bool enabled = true;
        for (size_t h = 0, nh = NumHiddenSections(); h < nh; ++h) {
            if (hash == GetHiddenSection(h)) { enabled = false; break; }
        }
        if (ImGui::Checkbox(name[0] ? name : "<empty>", &enabled)) {
            HideSection(hash, !enabled);
        }
    }

    ImGui::End();
}