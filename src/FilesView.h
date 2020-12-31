#pragma once

#include <vector>
#ifdef __linux__
#include <linux/limits.h>
#define PATH_MAX_LEN PATH_MAX
#elif _WIN32
#define PATH_MAX_LEN _MAX_PATH
#endif

struct FVFileInfo {
	enum type {
		none,
		file,
		dir
	};
	char *name;
	size_t size;
	type fileType;
	void Free();
};

struct FVFileList {
	std::vector<FVFileInfo> files;
	char *filter;
	char *path;
	void ReadDir(const char *full_path, const char *filter = nullptr);
	void Clear();
	void InsertAlphabetically(FVFileInfo& info);

	FVFileList() : filter(nullptr), path(nullptr) {}
};

class FVFileView : protected FVFileList {
public:
	void Draw(const char *title);
	void Show(const char *folder, bool *setWhenDone = nullptr, char *pathWhenDone = nullptr, int pathWhenDoneSize = 0, const char *filter = nullptr);
	bool IsOpen() const { return open; }
	const char* GetSelectedFile() const { return selectedFile; }
	FVFileView() : open(false), selected(nullptr), pathTarget(nullptr), pathTargetSize(0), selectIndex(0xffffffff) {}
protected:
	bool open;
	bool *selected;
	char *pathTarget;
	int pathTargetSize;
	size_t selectIndex;
	char userPath[PATH_MAX_LEN];
	char userFile[PATH_MAX_LEN];
	char selectedFile[PATH_MAX_LEN];
};
