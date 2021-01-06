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

static char sLoadPrgFileName[PATH_MAX_LEN] = {};
static char sLoadLstFileName[PATH_MAX_LEN] = {};
static char sLoadDbgFileName[PATH_MAX_LEN] = {};
static char sLoadSymFileName[PATH_MAX_LEN] = {};
static char sLoadViceFileName[PATH_MAX_LEN] = {};

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
static const char sLoadKickDbgParams[] = "C64Debugger:*.dbg";
static const char sLoadSymbolsParams[] = "Symbols:*.sym";
static const char sLoadViceCmdParams[] = "Vice Commands:*.vs";
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
		filesView->Show(sFileDialogFolder, &sLoadProgramReady, sLoadPrgFileName, sizeof(sLoadPrgFileName), sLoadProgramParams);
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
		filesView->Show(sFileDialogFolder, &sLoadListingReady, sLoadLstFileName, sizeof(sLoadLstFileName), sLoadListingParams);
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
		filesView->Show(sFileDialogFolder, &sLoadKickDbgReady, sLoadDbgFileName, sizeof(sLoadDbgFileName), sLoadKickDbgParams);
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
		filesView->Show(sFileDialogFolder, &sLoadSymbolsReady, sLoadSymFileName, sizeof(sLoadSymFileName), sLoadSymbolsParams);
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
		filesView->Show(sFileDialogFolder, &sLoadViceCmdReady, sLoadViceFileName, sizeof(sLoadViceFileName), sLoadViceCmdParams);
	}
#endif
}

