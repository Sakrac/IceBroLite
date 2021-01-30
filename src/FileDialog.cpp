#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif
#include <stdio.h>
#include "Config.h"
#include "FileDialog.h"
#include "views/FilesView.h"
#include "views/Views.h"
#include "struse/struse.h"
#ifdef __linux__
#include <unistd.h>
#include <linux/limits.h>
#define PATH_MAX_LEN PATH_MAX
#define sprintf_s sprintf
#define GetCurrentDirectory(size, buf) getcwd(buf, size)
#define SetCurrentDirectory(str) chdir(str)
#else
#define PATH_MAX_LEN _MAX_PATH
#endif

//#ifdef __linux__
#define CUSTOM_FILEVIEWER
//#endif

#if defined(_WIN32) && !defined(CUSTOM_FILEVIEWER)
#define FILE_LOAD_THREAD_STACK 8192
HANDLE hThreadFileDialog = 0;
#endif


static bool sFileDialogOpen = false;
static bool sLoadProgramReady = false;
static bool sLoadListingReady = false;
static bool sLoadKickDbgReady = false;
static bool sLoadSymbolsReady = false;
static bool sLoadViceCmdReady = false;
static bool sSetViceEXEPathReady = false;

static char sLoadPrgFileName[PATH_MAX_LEN] = {};
static char sLoadLstFileName[PATH_MAX_LEN] = {};
static char sLoadDbgFileName[PATH_MAX_LEN] = {};
static char sLoadSymFileName[PATH_MAX_LEN] = {};
static char sLoadViceFileName[PATH_MAX_LEN] = {};
static char sViceEXEPath[PATH_MAX_LEN] = {};


static char sFileDialogFolder[PATH_MAX_LEN];

static char sCurrentDir[ PATH_MAX_LEN ] = {};

struct FileTypeInfo {
	const char* fileTypes;
	char* fileName;
	bool* doneFlag;
};

#ifndef CUSTOM_FILEVIEWER
#else
static const char sLoadProgramParams[] = "Prg:*.prg,D64:*.d64,Cart:*.crt";
static const char sLoadListingParams[] = "Listing:*.lst";
static const char sLoadKickDbgParams[] = "Kick Asm Debug:*.dbg";
static const char sLoadSymbolsParams[] = "Symbols:*.sym";
static const char sLoadViceCmdParams[] = "Vice Commands:*.vs";
static const char sViceEXEParams[] = "Vice EXE path:*x64*";

#endif

void InitStartFolder()
{
	if( GetCurrentDirectory( sizeof( sCurrentDir ), sCurrentDir ) != 0 ) {
		memcpy(sFileDialogFolder, sCurrentDir, sizeof(sFileDialogFolder) < sizeof(sCurrentDir) ? sizeof(sFileDialogFolder) : sizeof(sCurrentDir) );
		return;
	}
	sCurrentDir[ 0 ] = 0;
#ifdef _MSC_VER
	strcpy_s(sFileDialogFolder, "/");
#else
	strcpy(sFileDialogFolder, "/");
#endif
}

const char* GetStartFolder() { return sCurrentDir; }

void ResetStartFolder()
{
	if( sCurrentDir[ 0 ] ) {
		SetCurrentDirectory( sCurrentDir );
	}
}

bool IsFileDialogOpen() { return sFileDialogOpen; }

const char* ReloadProgramFile() { return sLoadPrgFileName[0] ? sLoadPrgFileName : nullptr; }

const char* LoadProgramReady()
{
	if (sLoadProgramReady) {
		sLoadProgramReady = false;
		return sLoadPrgFileName;
	}
	return nullptr;
}

const char* LoadListingReady()
{
	if (sLoadListingReady) {
		sLoadListingReady = false;
		return sLoadLstFileName;
	}
	return nullptr;
}

const char* LoadKickDbgReady()
{
	if (sLoadKickDbgReady) {
		sLoadKickDbgReady = false;
		return sLoadDbgFileName;
	}
	return nullptr;
}

const char* LoadSymbolsReady()
{
	if (sLoadSymbolsReady) {
		sLoadSymbolsReady = false;
		return sLoadSymFileName;
	}
	return nullptr;
}

const char* LoadViceCMDReady()
{
	if (sLoadViceCmdReady) {
		sLoadViceCmdReady = false;
		return sLoadViceFileName;
	}
	return nullptr;
}

bool LoadViceEXEPathReady()
{
	if (sSetViceEXEPathReady) {
		sSetViceEXEPathReady = false;
		return true;
	}
	return false;
}

char* GetViceEXEPath()
{
	return sViceEXEPath[0] ? sViceEXEPath : nullptr;
}


#if defined(_WIN32) && !defined(CUSTOM_FILEVIEWER)
void *FileLoadDialogThreadRun( void *param )
{
	FileTypeInfo* info = (FileTypeInfo*)param;
	OPENFILENAME ofn = {};
	ofn.lStructSize = sizeof( OPENFILENAME );
//	ofn.hInstance = GetPrgInstance();
	ofn.lpstrFile = info->fileName;
	ofn.nMaxFile = PATH_MAX_LEN;
	ofn.lpstrFilter = info->fileTypes;// "All\0*.*\0Prg\0*.prg\0Bin\0*.bin\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if( GetOpenFileName( &ofn ) != TRUE )
	{
		DWORD err = GetLastError();
		sFileDialogOpen = false;
		return nullptr;
	}
	sFileDialogOpen = false;
	*info->doneFlag = true;
	return nullptr;
}

void *FileSaveDialogThreadRun(void *param)
{
	FileTypeInfo* info = (FileTypeInfo*)param;
	OPENFILENAME ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAME);
	//	ofn.hInstance = GetPrgInstance();
	ofn.lpstrFile = info->fileName;
	ofn.nMaxFile = PATH_MAX_LEN;
	ofn.lpstrFilter = info->fileTypes;// "All\0*.*\0Prg\0*.prg\0Bin\0*.bin\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetSaveFileName(&ofn) != TRUE) {
		DWORD err = GetLastError();
		sFileDialogOpen = false;
		return nullptr;
	}
	sFileDialogOpen = false;
	*info->doneFlag = true;
	return nullptr;
}
#endif


strref StartFolder(strref prevFile)
{
	if (prevFile) {
		return prevFile.before_last('/', '\\');
	}
	return sFileDialogFolder;
}

void LoadProgramDialog()
{
	sLoadProgramReady = false;
	sFileDialogOpen = true;

#if defined(_WIN32) && !defined(CUSTOM_FILEVIEWER)
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aImportInfo,
									 0, NULL);
#else
	FVFileView* filesView = GetFileView();
	if( filesView && !filesView->IsOpen()) {
		filesView->Show(strown<PATH_MAX_LEN>(StartFolder(sLoadPrgFileName)).c_str(), &sLoadProgramReady, sLoadPrgFileName, sizeof(sLoadPrgFileName), sLoadProgramParams);
	}
#endif
}

void LoadListingDialog()
{
	sLoadListingReady = false;
	sFileDialogOpen = true;

#if defined(_WIN32) && !defined(CUSTOM_FILEVIEWER)
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aLoadTemplateInfo,
		0, NULL);
#else
	FVFileView* filesView = GetFileView();
	if (filesView && !filesView->IsOpen()) {
		filesView->Show(strown<PATH_MAX_LEN>(StartFolder(sLoadLstFileName)).c_str(), &sLoadListingReady, sLoadLstFileName, sizeof(sLoadLstFileName), sLoadListingParams);
	}
#endif
}

void LoadKickDbgDialog()
{
	sLoadKickDbgReady = false;
	sFileDialogOpen = true;

#if defined(_WIN32) && !defined(CUSTOM_FILEVIEWER)
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aLoadGrabInfo,
		0, NULL);
#else
	FVFileView* filesView = GetFileView();
	if (filesView && !filesView->IsOpen()) {
		filesView->Show(strown<PATH_MAX_LEN>(StartFolder(sLoadDbgFileName)).c_str(), &sLoadKickDbgReady, sLoadDbgFileName, sizeof(sLoadDbgFileName), sLoadKickDbgParams);
	}
#endif
}

void LoadSymbolsDialog()
{
	sLoadSymbolsReady = false;
	sFileDialogOpen = true;

#if defined(_WIN32) && !defined(CUSTOM_FILEVIEWER)
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aLoadAnimInfo,
									 0, NULL);
#else
	FVFileView* filesView = GetFileView();
	if (filesView && !filesView->IsOpen()) {
		filesView->Show(strown<PATH_MAX_LEN>(StartFolder(sLoadSymFileName)).c_str(), &sLoadSymbolsReady, sLoadSymFileName, sizeof(sLoadSymFileName), sLoadSymbolsParams);
	}
#endif
}

void LoadViceCmdDialog()
{
	sLoadViceCmdReady = false;
	sFileDialogOpen = true;

#if defined(_WIN32) && !defined(CUSTOM_FILEVIEWER)
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileSaveDialogThreadRun, &aSaveAsInfo,
									 0, NULL);
#else
	FVFileView* filesView = GetFileView();
	if (filesView && !filesView->IsOpen()) {
		filesView->Show(strown<PATH_MAX_LEN>(StartFolder(sLoadViceFileName)).c_str(), &sLoadViceCmdReady, sLoadViceFileName, sizeof(sLoadViceFileName), sLoadViceCmdParams);
	}
#endif
}

void SetViceEXEPathDialog()
{
	sSetViceEXEPathReady = false;
	sFileDialogOpen = true;

#if defined(_WIN32) && !defined(CUSTOM_FILEVIEWER)
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileSaveDialogThreadRun, &aSaveAsInfo,
									 0, NULL);
#else
	FVFileView* filesView = GetFileView();
	if (filesView && !filesView->IsOpen()) {
		filesView->Show(strown<PATH_MAX_LEN>(StartFolder(sViceEXEPath)).c_str(), &sSetViceEXEPathReady, sViceEXEPath, sizeof(sViceEXEPath), sViceEXEParams);
	}
#endif
}


void StateLoadFilenames(strref filenames)
{
	ConfigParse config(filenames);
	while (!config.Empty()) {
		strref name, value;
		ConfigParseType type = config.Next(&name, &value);
		if (type == CPT_Value) {
			if (name.same_str("Binary")) {
				strovl(sLoadPrgFileName, sizeof(sLoadPrgFileName)).append(value).c_str();
			} else if (name.same_str("Listing")) {
				strovl(sLoadLstFileName, sizeof(sLoadLstFileName)).append(value).c_str();
			} else if (name.same_str("DebugData")) {
				strovl(sLoadDbgFileName, sizeof(sLoadDbgFileName)).append(value).c_str();
			} else if (name.same_str("Symbols")) {
				strovl(sLoadSymFileName, sizeof(sLoadSymFileName)).append(value).c_str();
			} else if (name.same_str("ViceMonCommands")) {
				strovl(sLoadViceFileName, sizeof(sLoadViceFileName)).append(value).c_str();
			} else if (name.same_str("ViceExePath")) {
				strovl(sViceEXEPath, sizeof(sViceEXEPath)).append(value).c_str();
			}
		}
	}
}

void StateSaveFilenames(UserData& conf)
{
	if (sLoadPrgFileName[0]) {
		conf.AddValue("Binary", strref(sLoadPrgFileName));
	}
	if (sLoadLstFileName[0]) {
		conf.AddValue("Listing", strref(sLoadLstFileName));
	}
	if (sLoadDbgFileName[0]) {
		conf.AddValue("DebugData", strref(sLoadDbgFileName));
	}
	if (sLoadSymFileName[0]) {
		conf.AddValue("Symbols", strref(sLoadSymFileName));
	}
	if (sLoadViceFileName[0]) {
		conf.AddValue("ViceMonCommands", strref(sLoadViceFileName));
	}
	if (sViceEXEPath[0]) {
		conf.AddValue("ViceExePath", strref(sViceEXEPath));
	}

}
