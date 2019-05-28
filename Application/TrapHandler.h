#pragma once

#include <random>
#include <string>
#include <vector>
#include <unordered_map>
#include <Windows.h>
#include <any>
#include <filesystem>
#include "Common.h"
#include "sharedContainers.h"
#include "HashUtility.h"

namespace fs = std::filesystem;

constexpr ULONGLONG MAX_FILE_BUFFER_SIZE = 0x2FF0FFF;

class TrapHandler
{
private:
	PUCHAR Buffer;

	void FillRandContent(HANDLE file, std::size_t size);
	array<BYTE>^ getFileId(HANDLE file);
	std::wstring randFileName(const std::wstring& extension, std::vector<HANDLE>& vHandles);
	FILETIME randFileTime(const std::vector<HANDLE>& vHandles);
	std::size_t CalcFileSize(const std::vector<HANDLE>& vHandles);
	BOOLEAN TrapGenerate(const fs::path& directory);

public:
	TrapHandler() {
		Buffer = new UCHAR[MAX_FILE_BUFFER_SIZE];
		Buffer[MAX_FILE_BUFFER_SIZE - 1] = 0;
		// FIXME: check success fail throw
	}
	~TrapHandler() {
		delete[] Buffer;
	}
	// data member
	//std::unordered_set<std::wstring> dirs;

	std::wstring getFileHash(const std::wstring& filePath);
	BOOLEAN isFileTrap(const std::wstring& filePath);
	int initTraps(const std::vector<std::wstring>& dirs);
	int cleanTraps();
	int addDir(const std::wstring& dirPath);
	int remDir(const std::wstring& dirPath);


};

//static std::unordered_map<std::wstring, std::wstring> genDirTraps(const fs::path & directory);
