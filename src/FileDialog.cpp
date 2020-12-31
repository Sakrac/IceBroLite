#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif
#include <stdio.h>
#include "Config.h"
#include "FileDialog.h"
#include "FilesView.h"
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

#ifdef _WIN32
#define FILE_LOAD_THREAD_STACK 8192
HANDLE hThreadFileDialog = 0;
#endif

#ifdef __linux__
#define CUSTOM_FILEVIEWER
#endif

static bool sFileDialogOpen = false;
static bool sImportImageReady = false;
static bool sLoadAnimReady = false;
static bool sSaveAsAnimReady = false;
static bool sSaveAsLevelReady = false;
static bool sLoadLevelReady = false;
static bool sLoadGrabMapReady = false;
static bool sLoadTemplateImageReady = false;

static char sImportImageFile[PATH_MAX_LEN] = {};
static char sLoadGrabFile[PATH_MAX_LEN] = {};
static char sLoadAnimFile[PATH_MAX_LEN] = {};
static char sSaveLevelFile[PATH_MAX_LEN] = {};
static char sLoadLevelFile[PATH_MAX_LEN] = {};
static char sLoadTemplateFile[PATH_MAX_LEN] = {};

static char sFileDialogFolder[PATH_MAX_LEN];

static FVFileView filesView;
static char sCurrentDir[ PATH_MAX_LEN ] = {};

struct FileTypeInfo {
	const char* fileTypes;
	char* fileName;
	bool* doneFlag;
};

#ifndef CUSTOM_FILEVIEWER
static FileTypeInfo aImportInfo = { "Png\0*.png\0BMP\0*.bmp\0TGA\0*.tga\0", sImportImageFile, &sImportImageReady };
static FileTypeInfo aLoadAnimInfo = { "Animation\0*.can\0", sLoadAnimFile, &sLoadAnimReady };
static FileTypeInfo aSaveAsInfo = { "Animation\0*.can\0", sLoadAnimFile, &sSaveAsAnimReady };
static FileTypeInfo aSaveLevelAsInfo = { "Level\0*.txt\0", sSaveLevelFile, &sSaveAsLevelReady };
static FileTypeInfo aLoadLevelInfo = { "Level\0*.txt\0", sLoadLevelFile, &sLoadLevelReady };
static FileTypeInfo aLoadGrabInfo = { "GrabMap\0*.png\0*.bmp\0*.tga\0", sLoadGrabFile, &sLoadGrabMapReady };
static FileTypeInfo aLoadTemplateInfo = { "Template\0*.txt\0", sLoadTemplateFile, &sLoadTemplateImageReady };
#else
const char aImportInfo[] = "Png:*.png,BMP:*.bmp,TGA:*.tga";
const char aLoadAnimInfo[] = "Animation:*.can";
const char aSaveAsInfo[] = "Level:*.txt";
const char aSaveLevelAsInfo[] = "Level:*.txt";
const char aLoadLevelInfo[] = "Level:*.txt";
const char aLoadGrabInfo[] = "Png:*.png,BMP:*.bmp,TGA:*.tga";
const char aLoadTemplateInfo[] = "Template:*.txt";
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

const char* ImportImageReady()
{
	if (sImportImageReady) {
		sImportImageReady = false;
		strovl savePath(sFileDialogFolder, sizeof(sFileDialogFolder));
		savePath.copy(strref(sImportImageFile).before_last(DIR_SEP));
		savePath.c_str();
		return sImportImageFile;
	}
	return nullptr;
}

void DrawFileDialog()
{
#ifdef CUSTOM_FILEVIEWER
	filesView.Draw("Select File");
#endif
}

const char* LoadGrabMapReady()
{
	if (sLoadGrabMapReady) {
		sLoadGrabMapReady = false;
		strovl savePath(sFileDialogFolder, sizeof(sFileDialogFolder));
		savePath.copy(strref(sLoadGrabFile).before_last(DIR_SEP));
		savePath.c_str();
		return sLoadGrabFile;
	}
	return nullptr;
}

const char* LoadTemplateImageReady()
{
	if (sLoadTemplateImageReady) {
		sLoadTemplateImageReady = false;
		strovl savePath(sFileDialogFolder, sizeof(sFileDialogFolder));
		savePath.copy(strref(sLoadTemplateFile).before_last(DIR_SEP));
		savePath.c_str();
		return sLoadTemplateFile;
	}
	return nullptr;
}

const char* LoadAnimReady()
{
	if (sLoadAnimReady) {
		sLoadAnimReady = false;
		strovl savePath(sFileDialogFolder, sizeof(sFileDialogFolder));
		savePath.copy(strref(sLoadAnimFile).before_last(DIR_SEP));
		savePath.c_str();
		return sLoadAnimFile;
	}
	return nullptr;
}

const char* SaveAsAnimReady()
{
	if (sSaveAsAnimReady) {
		sSaveAsAnimReady = false;
		strovl savePath(sFileDialogFolder, sizeof(sFileDialogFolder));
		savePath.copy(strref(sLoadAnimFile).before_last(DIR_SEP));
		savePath.c_str();
		return sLoadAnimFile;
	}
	return nullptr;
}

const char* SaveLevelAsReady()
{
	if (sSaveAsLevelReady) {
		sSaveAsLevelReady = false;
		strovl savePath(sFileDialogFolder, sizeof(sFileDialogFolder));
		savePath.copy(strref(sSaveLevelFile).before_last(DIR_SEP));
		savePath.c_str();
		return sSaveLevelFile;
	}
	return nullptr;
}

const char* LoadLevelReady()
{
	if (sLoadLevelReady) {
		sLoadLevelReady = false;
		strovl savePath(sFileDialogFolder, sizeof(sFileDialogFolder));
		savePath.copy(strref(sLoadLevelFile).before_last(DIR_SEP));
		savePath.c_str();
		return sLoadLevelFile;
	}
	return nullptr;
}

#ifdef _WIN32
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

void LoadImageDialog()
{
	sImportImageReady = false;
	sFileDialogOpen = true;

#ifdef _WIN32
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aImportInfo,
									 0, NULL);
#else
	if( !filesView.IsOpen()) {
		filesView.Show(sFileDialogFolder, &sImportImageReady, sImportImageFile, sizeof(sImportImageFile), aImportInfo);
	}
#endif
}

void LoadTemplateDialog()
{
	sLoadTemplateImageReady = false;
	sFileDialogOpen = true;

#ifdef _WIN32
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aLoadTemplateInfo,
		0, NULL);
#else
	if( !filesView.IsOpen()) {
		filesView.Show(sFileDialogFolder, &sLoadTemplateImageReady, sLoadTemplateFile, sizeof(sLoadTemplateFile));
	}
#endif
}

void LoadGrabMapDialog()
{
	sLoadGrabMapReady = false;
	sFileDialogOpen = true;

#ifdef _WIN32
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aLoadGrabInfo,
		0, NULL);
#else
	if( !filesView.IsOpen()) {
		filesView.Show(sFileDialogFolder, &sLoadGrabMapReady, sLoadGrabFile, sizeof(sLoadGrabFile), aLoadGrabInfo);
	}
#endif
}

void LoadAnimDialog()
{
	sLoadAnimReady = false;
	sFileDialogOpen = true;

#ifdef _WIN32
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aLoadAnimInfo,
									 0, NULL);
#endif
}

void SaveAnimDialog()
{
	sLoadAnimReady = false;
	sFileDialogOpen = true;

#ifdef _WIN32
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileSaveDialogThreadRun, &aSaveAsInfo,
									 0, NULL);
#endif
}


void SaveLevelDialog()
{
	sSaveAsLevelReady = false;
	sFileDialogOpen = true;

#ifdef _WIN32
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileSaveDialogThreadRun, &aSaveLevelAsInfo,
									 0, NULL);
#else
	if( !filesView.IsOpen()) {
		filesView.Show(sFileDialogFolder, &sSaveAsLevelReady, sSaveLevelFile, sizeof(sSaveLevelFile), aSaveLevelAsInfo);
	}
#endif
}

void LoadLevelDialog()
{
	sLoadLevelReady = false;
	sFileDialogOpen = true;

#ifdef _WIN32
	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aLoadLevelInfo,
		0, NULL);
#else
	if( !filesView.IsOpen()) {
		filesView.Show(sFileDialogFolder, &sLoadLevelReady, sLoadLevelFile, sizeof(sLoadLevelFile), aLoadLevelInfo);
	}
#endif
}

