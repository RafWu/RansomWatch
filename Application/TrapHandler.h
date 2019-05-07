#pragma once

#include <random>
#include <string>
#include <vector>
#include <unordered_map>
#include <Windows.h>
#include <any>
#include <filesystem>
#include "Common.h"
#include "PythonEmbed.h"
#include <bcrypt.h>

namespace fs = std::filesystem;

namespace TrapHandler
{
	// data member
		//std::unordered_set<std::wstring> dirs;

	std::wstring getFileHash(const std::wstring& filePath);
	BOOLEAN isFileTrap(const std::wstring& filePath);
	int initTraps(const std::vector<std::wstring>& dirs);
	int cleanTraps();
	int addDir(const std::wstring& dirPath);
	int remDir(const std::wstring& dirPath);


};
