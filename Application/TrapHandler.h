#pragma once

#include <random>
#include <string>
#include <vector>
#include <any>
#include <filesystem>
#include "Common.h"
#include "Traps.h"
#include <bcrypt.h>
#include <vcclr.h>
#include <unordered_map>

namespace fs = std::filesystem;

constexpr ULONGLONG MAX_FILE_BUFFER_SIZE = 0x2FF0FFF;

class TrapHandler
{
private:
	PUCHAR Buffer;

	void FillRandContent(HANDLE file, std::size_t size);
	FileId getFileId(HANDLE file);
	std::wstring randFileName(const std::wstring& extension, std::vector<HANDLE>& vHandles);
	FILETIME randFileTime(const std::vector<HANDLE>& vHandles);
	std::size_t CalcFileSize(const std::vector<HANDLE>& vHandles);
	BOOLEAN TrapGenerate(const fs::directory_entry& directory);

public:
	TrapHandler() {
		Buffer = new UCHAR[MAX_FILE_BUFFER_SIZE];
		Buffer[MAX_FILE_BUFFER_SIZE - 1] = 0;
		// FIXME: check success fail throw
	}
	~TrapHandler() {
		delete[] Buffer;
	}

	int initDirTraps(System::String ^ Path);
	int remDirTraps(System::String^ Path);
	int cleanTraps();
private: int addDir(const fs::directory_entry& dirPath);
private: int remDir(const fs::directory_entry& dirPath);


};