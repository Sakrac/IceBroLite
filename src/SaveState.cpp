#include <stdlib.h>
#include <stdio.h>
#include "Config.h"
#include "imgui/imgui.h"
#include "Files.h"
#include "FileDialog.h"
#include "Sym.h"
#include "SaveState.h"

void StateLoadFilenames(strref filenames);
void StateSaveFilenames(UserData& conf);
const char* GetCustomThemePath();
void StateLoadViews(strref conf);
void StateSaveViews(UserData& conf);
void SaveStateWindow(UserData& conf);
void ImGuiStateLoaded();

static const char* sSaveStateFile = "icebrolt.ini";

SaveStateFile ReadState() {
	size_t size;
	uint8_t* data = LoadBinary(sSaveStateFile, size);
	SaveStateFile ret = { data, size };
	return ret;
}

void ReleaseState(SaveStateFile file) {
	if (file.data && file.size) { free(file.data); }
}

void ParseState(SaveStateFile file)
{
	size_t size = file.size;
	uint8_t* data = file.data;
	if(data) {
		ConfigParse config(data, size);
		while (!config.Empty()) {
			strref name, value;
			ConfigParseType type = config.Next(&name, &value);
			if (name.same_str("Filenames") && type == ConfigParseType::CPT_Struct) {
				StateLoadFilenames(value);
			} else if (name.same_str("Views") && type == ConfigParseType::CPT_Struct) {
				StateLoadViews(value);
			} else if (name.same_str("HiddenSections") && type == ConfigParseType::CPT_Array) {
				StateLoadHiddenSections(value);
			} else if (name.same_str("DearImGui") && type == ConfigParseType::CPT_Struct) {
				ImGui::LoadIniSettingsFromMemory(value.get(), value.get_len());
				ImGuiStateLoaded();
			}
		}
	}
}




void SaveState()
{
	UserData conf;
	strown<128> arg;

	SaveStateWindow(conf);

	conf.BeginStruct("Filenames");
	StateSaveFilenames(conf);
	conf.EndStruct();

	conf.BeginStruct(strref("Views"));
	StateSaveViews(conf);
	conf.EndStruct();

	conf.BeginArray(strref("HiddenSections"));
	StateSaveHiddenSections(conf);
	conf.EndArray();

	conf.BeginStruct(strref("DearImGui"));
	size_t ImGuiSize;
	const char* ImGuiData = ImGui::SaveIniSettingsToMemory(&ImGuiSize);
	conf.Append(strref(ImGuiData, (strl_t)ImGuiSize));
	conf.EndStruct();

	SaveFile(sSaveStateFile, conf.start, conf.curr - conf.start);
}

static bool saveLayoutNow = false;

void UserSaveLayout()
{
	saveLayoutNow = true;
}

void UserSaveLayoutUpdate()
{
	if (saveLayoutNow) {
		saveLayoutNow = false;
		SaveState();
	}
}

