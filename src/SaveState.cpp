#include <stdlib.h>
#include <stdio.h>
#include "Config.h"
#include "imgui/imgui.h"
#include "Files.h"
#include "FileDialog.h"

void StateLoadFilenames(strref filenames);
void StateSaveFilenames(UserData& conf);
void StateLoadViews(strref conf);
void StateSaveViews(UserData& conf);
void ImGuiStateLoaded();

static const char* sSaveStateFile = "icebrolt.ini";

void LoadState()
{
	size_t size;
	uint8_t* data = LoadBinary(sSaveStateFile, size);
	if(data) {
		ConfigParse config(data, size);
		while (!config.Empty()) {
			strref name, value;
			ConfigParseType type = config.Next(&name, &value);
			if (name.same_str("Filenames") && type == CPT_Struct) {
				StateLoadFilenames(value);
			} else if (name.same_str("Views") && type == CPT_Struct) {
				StateLoadViews(value);
			} else if (name.same_str("ImGui") && type == CPT_Struct) {
				ImGui::LoadIniSettingsFromMemory(value.get(), value.get_len());
				ImGuiStateLoaded();
			}
		}
		free(data);
	}
}

void SaveState()
{
	UserData conf;
	strown<128> arg;

	conf.BeginStruct("Filenames");
	StateSaveFilenames(conf);
	conf.EndStruct();

	conf.BeginStruct(strref("Views"));
	StateSaveViews(conf);
	conf.EndStruct();

	conf.BeginStruct(strref("ImGui"));
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
