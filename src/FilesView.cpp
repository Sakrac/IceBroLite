// attempting a home made file dialog in ImGui for a little more portability
#include "struse/struse.h"
#include "FilesView.h"
#include <string.h>
#include <malloc.h>
#include "imgui.h"

#ifdef __linux__
#include <dirent.h>
#include <sys/stat.h>
#elif _WIN32
#include <windows.h>
#include <stdio.h>
#endif

#ifdef _MSC_VER 
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

void FVFileView::Show(const char *folder, bool *setWhenDone, char *pathWhenDone, int pathWhenDoneSize, const char *filter)
{
	strovl usr(userPath, sizeof(userPath));
	usr.copy(folder);
	usr.c_str();
	ReadDir(folder, filter); 
	selected = setWhenDone;
	pathTarget = pathWhenDone;
	pathTargetSize = pathWhenDoneSize;
	open = true; 
	selectedFile[0] = 0;
}

void FVFileView::Draw(const char *title)
{
	ImGui::SetNextWindowSize(ImVec2(740.0f, 410.0f), ImGuiCond_FirstUseEver);
	if (open && ImGui::Begin(title, &open, ImGuiWindowFlags_NoDocking)) {
		if(ImGui::InputText("Path", userPath, sizeof(userPath), ImGuiInputTextFlags_EnterReturnsTrue)) {
			printf("user setting path: %s\n", userPath);
			ReadDir(userPath, filter);
		}
		if(ImGui::InputText("File", userFile, sizeof(userFile), ImGuiInputTextFlags_EnterReturnsTrue)) {
			printf("%s\n",userFile);
		}
		if( filter ) {
			strown<256> filterStr("File Types: ");
			strref filters(filter);
			bool first = true;
			while(strref filt = filters.split_token(',')) {
				strref filtType = filt.split_token(':');
				if( first ) { first = false; }
				else { filterStr.append(", "); }
				filterStr.append(filtType).append(" (").append(filt).append(")");
			}
			ImGui::Text(filterStr.c_str());
		}
		ImGui::BeginChild("Directory", ImVec2(0, 0), true);
		for (size_t i=0, n=files.size(); i<n; ++i) {
			strown<PATH_MAX_LEN> fileTxt;
			if( files[i].fileType == FVFileInfo::dir) {
				fileTxt.copy("(dir)");
			} else {
				fileTxt.append_num((uint32_t)files[i].size, 0, 10);
			}
			fileTxt.pad_to(' ', 12);
			fileTxt.append(files[i].name);

			if( ImGui::Selectable(fileTxt.c_str(), i == selectIndex, ImGuiSelectableFlags_AllowDoubleClick)) {
				if (ImGui::IsMouseDoubleClicked(0)) {
					strown<PATH_MAX_LEN> newPath(path);
					newPath.append(DIR_SEP).append(files[i].name);
					newPath.cleanup_path();

					if( files[i].fileType == FVFileInfo::dir) {
						strovl usr(userPath, sizeof(userPath));
						usr.copy(newPath);
						usr.c_str();
						userFile[0] = 0;
						ReadDir(newPath.c_str(), filter);
						ImGui::SetScrollHereY(0.0f);
						break;
					} else {
						strovl sel(selectedFile, sizeof(selectedFile));
						sel.copy(newPath);
						sel.c_str();
						if (pathTarget && pathTargetSize) {
							strovl target(pathTarget, pathTargetSize);
							target.copy(newPath);
							target.c_str();
						}
						if (selected) { *selected = true; }
						open = false;
						break;
					}
				} else {
					strovl usr(userFile, sizeof(userFile));
					usr.copy(files[i].name);
					usr.c_str();
					selectIndex = i;
				}
			}
		}
		ImGui::EndChild();
		ImGui::End();
	}
}



void FVFileInfo::Free()
{
	if( name) { free(name); name = nullptr; }
}

void FVFileList::Clear()
{
	for( std::vector<FVFileInfo>::iterator i=files.begin(); i!=files.end(); ++i) {
		i->Free();
	}
	files.clear();
}

void FVFileList::InsertAlphabetically(FVFileInfo& info)
{
	for( std::vector<FVFileInfo>::iterator i=files.begin(); i!=files.end(); ++i) {
		if(strcasecmp(info.name, i->name)<0) {
			files.insert(i, info);
			return;
		}
	}
	files.push_back(info);
}

static bool CheckFileFilter(strref name, strref filters)
{
	while(strref filt = filters.split_token(',')) {
		int c = filt.find(':');
		if( c>=0 ) { filt.skip(c+1); filt.skip_whitespace(); }
		if(name.find_wildcard(filt,0,false)) { return true; }
	}
	return false;
}

#ifdef __linux__

void FVFileList::ReadDir(const char *full_path, const char*file_filter)
{
	struct dirent *entry = nullptr;

	// if path can't be reached don't update it... 
	DIR *dp = full_path ? opendir(full_path) : nullptr;
	if( !dp) { return; }

	char* prev_path = path;
	path = strdup(full_path);
	Clear();
	if( prev_path) { free(prev_path);}

	char* prev_filter = filter;
	filter = file_filter ? strdup(file_filter) : nullptr;
	if(!prev_filter) { free(prev_filter);}

	// add a parent folder option if the current folder is not root or home
	if( !strref("/").same_str(full_path) && !strref("~/").same_str(full_path)) {
		FVFileInfo back;
		back.name = strdup("..");
		back.fileType = FVFileInfo::dir;
		back.size = 0;
		files.push_back(back);
	}

	if (dp != nullptr) {
		while ((entry = readdir(dp))) {
			if( entry->d_type == DT_DIR || entry->d_type == DT_REG) {
				FVFileInfo info;
				info.name = nullptr;
				info.size = 0;
				if( entry->d_type == DT_DIR) {
					if( strcmp(entry->d_name, ".")== 0 || strcmp(entry->d_name, "..")==0) { continue;}
					info.name = strdup(entry->d_name);
					info.fileType = FVFileInfo::dir;
				} else {
					// check against filter
					if(filter && !CheckFileFilter(strref(entry->d_name), strref(filter))) {
						continue;
					}
					info.name = strdup(entry->d_name);
					info.fileType = FVFileInfo::file;
					struct stat statbuf;
					strown<PATH_MAX_LEN> fullFile(path);
					fullFile.append('/').append(info.name);

					if (stat(fullFile.c_str(), &statbuf) == -1) {
					/* check the value of errno */
					} else {
						info.size = statbuf.st_size;
					}
				}
				InsertAlphabetically(info);
			}
		}
	}
	closedir(dp);
}

#elif _WIN32

void FVFileList::ReadDir(const char* full_path, const char* file_filter)
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	strown<MAX_PATH> szDir(full_path);
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;

	szDir.append("\\*");

	char* prev_path = path;
	path = _strdup(full_path ? full_path : "~/");
	Clear();
	if (prev_path) { free(prev_path); }

	char* prev_filter = filter;
	filter = file_filter ? _strdup(file_filter) : nullptr;
	if (!prev_filter) { free(prev_filter); }

	FVFileInfo back;
	back.name = _strdup("..");
	back.fileType = FVFileInfo::dir;
	back.size = 0;
	files.push_back(back);

	hFind = FindFirstFile(szDir.c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
	}
	do {
		FVFileInfo info;
		info.name = nullptr;
		info.size = 0;

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) { continue; }
			info.name = _strdup(ffd.cFileName);
			info.fileType = FVFileInfo::dir;
		} else {
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			// check against filter
			if(filter && !CheckFileFilter(strref(ffd.cFileName), strref(filter))) {
				continue;
			}
			info.name = _strdup(ffd.cFileName);
			info.fileType = FVFileInfo::file;
		}
		InsertAlphabetically(info);
	} while (FindNextFile(hFind, &ffd) != 0);
	FindClose(hFind);
}

#endif
