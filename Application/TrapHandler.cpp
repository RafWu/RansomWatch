#include "TrapHandler.h"

static std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>> dirPFilesHash;
static std::random_device rd;
static std::mt19937 rng(rd());
std::uniform_int_distribution<WORD> uniMinSec(0, 59);
std::uniform_int_distribution<WORD> uniMilli(0, 999);

// TODO: maybe change return value to enum/define
// I use rng in the function to speed things up. it would be unneccesary
// and wasteful to create an rng for every honeypot we want to change.
/*static bool changeFileDate(const WORD day, const WORD month, const WORD year, const LPCWSTR& filePath, std::mt19937& rng) {
	// create uniform distribution number generator


	// set to desired time + randomize mins, secs and millisecs
	FILETIME filetime;
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	systime.wDay = day;
	systime.wMonth = month;
	systime.wYear = year;
	systime.wMinute = uniMinSec(rng);
	systime.wMilliseconds = uniMilli(rng);
	systime.wSecond = uniMinSec(rng);

	// convert SYSTIME to FILETIME
	if (!SystemTimeToFileTime(&systime, &filetime)) {
		return false;
	}

	// open file and return a file handler
	HANDLE hFile = CreateFile(filePath, // open filePath
		GENERIC_WRITE,                    // open for writing
		0,                                // do not share
		nullptr,                          // no security
		OPEN_EXISTING,                    // existing file only
		FILE_ATTRIBUTE_NORMAL,            // normal file
		nullptr);                         // no attr. template

	  // check for failue. for example, if the file does not exist it would fail
	if (hFile == INVALID_HANDLE_VALUE) {
		return false;
	}

	// set to new time
	if (!SetFileTime(hFile, &filetime, &filetime, &filetime)) {
		return false;
	}

	CloseHandle(hFile);
	return true;
}*/

void TrapHandler::FillRandContent(HANDLE file, std::size_t size) {
	BCryptGenRandom(nullptr, Buffer, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	Buffer[size - 1] = 0;
	WriteFile(file, Buffer, size, nullptr, nullptr);	
	/*std::size_t tempSize = 0;
	static std::uniform_int_distribution<USHORT> RangeRand(1, 255);
	UCHAR buffer[4096];
	for (; tempSize + 4096 < size - 1; tempSize += 4096) {
		for (DWORD i = 0; i < 4096; i++) {
			buffer[i] = RangeRand(rng);
		}
		WriteFile(file, buffer, 4096, nullptr, nullptr);
		SetFilePointer(file, 0, NULL, FILE_END);
	}
	DWORD it;
	for (it = 0; it < size - tempSize - 1; it++) {
		buffer[it] = RangeRand(rng);
	}
	buffer[it] = '\0';
	WriteFile(file, buffer, it + 1, nullptr, nullptr);	*/


}

array<BYTE>^ TrapHandler::getFileId(HANDLE file) {
	FILE_ID_INFO idInfo;
	array<BYTE>^ retArray = nullptr;
	// open file and return a file handler
	// get file id info from file handle, store into idInfo which has 2 fields
	// field 1 is volume serial number
	// field 2 is unique file id stored in struct FILE_ID_128 which is a char array -  BYTE Identifier[16]
	if (!GetFileInformationByHandleEx(file, FileIdInfo, &idInfo, sizeof(idInfo))) {
		DBOUT("GetFileId failed" << std::endl);
	}
	retArray = gcnew array<BYTE>(FILE_OBJECT_ID_SIZE);
	if (retArray == nullptr) {
		DBOUT("Alloc file id failed" << std::endl);
	}
	else {

		const FILE_ID_128& idStruct = idInfo.FileId;
		for (ULONG i = 0; i < FILE_OBJECT_ID_SIZE; i++) {
			retArray[i] = idStruct.Identifier[i];
		}
	}
	return retArray;
}

// FIXME: add throws
std::wstring TrapHandler::randFileName(const std::wstring& extension, std::vector<HANDLE>& vHandles) {
	std::wstring newName;
	static std::wstring letters(L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
	static std::uniform_int_distribution<WORD> RangeNameRand(5, 14);
	static std::uniform_int_distribution<WORD> LettersRand(0, letters.size());
	WORD size = RangeNameRand(rng);
	newName.resize(size + 1 + extension.size());
	for (WORD i = 0; i < size; i++) {
		newName[i] = letters[LettersRand(rng)];
	}
	newName[size] = L'.';
	for (WORD i = 0; i < extension.size(); i++) {
		newName[i + size + 1] = extension[i];
	}
	return newName;
}

// FIXME: add throws
FILETIME TrapHandler::randFileTime(const std::vector<HANDLE>& vHandles ) {
	FILETIME time;
	std::vector<FILETIME> times;
	for (HANDLE fHandle : vHandles) {
		if (fHandle == INVALID_HANDLE_VALUE) {
			continue;
		}
		LONGLONG res;
		if (GetFileTime(fHandle, &time, nullptr, nullptr)) {
			times.push_back(time);
		}
	}
	if (times.empty()) return time; // FIXME debug msg
	if (times.size() == 1) return times.at(0);
	std::uniform_int_distribution<WORD> DateRand(0, times.size() - 1);
	return times.at(DateRand(rng));
}

std::size_t TrapHandler::CalcFileSize(const std::vector<HANDLE>& vHandles) {
	std::size_t minSize = MAXULONGLONG;
	std::size_t maxSize = 0;
	static std::uniform_int_distribution<std::size_t> randAddition(0, 100);

	for (HANDLE fHandle : vHandles) {
		if (fHandle == INVALID_HANDLE_VALUE) {
			continue;
		}

		std::size_t fileSize;
		if (GetFileSizeEx(fHandle, (PLARGE_INTEGER)&fileSize)) {
			if (fileSize > maxSize) {
				maxSize = fileSize;
			}

			if (fileSize < minSize) {
				minSize = fileSize;
			}
		}
	}

	if (minSize > maxSize) {
		// should not happen, unless we couldn't get any file size
		return 0;
	}

	std::size_t retval = 0;
	if (minSize == maxSize) {
		// when we got only one file size. in that case, we create a file with
		// the same size + some random size addition
		retval = (maxSize > 0x2FEFFFF) ? 0x2FEFFFF : maxSize + std::uniform_int_distribution<std::size_t>(0, 50)(rng);
		retval += randAddition(rng);
		return retval;
	}

	// limit file size to ~50MB
	try {
		std::uniform_int_distribution<std::size_t> sizeRand((minSize >= 0x2FEFFFF) ? 0x2FEFF00  : minSize, (maxSize > 0x2FEFFFF) ? 0x2FEFFFF : maxSize);
		retval = sizeRand(rng) + randAddition(rng);
	}
	catch (std::exception& e) {
		// TODO: debug. it shouldn't throw normally,
		// but if it does we don't want our application to crash because of it
	}
	return retval;

}

BOOLEAN TrapHandler::TrapGenerate(const fs::path& directory) {
	BOOLEAN trapsGenerated = FALSE;
	String^ systemStringDirectory = gcnew String(directory.c_str());
	Generic::List<TrapRecord^>^ trapsRecords = gcnew Generic::List<TrapRecord^>;
	std::unordered_map<std::wstring, std::vector<std::wstring>> FilesDir;
	WIN32_FIND_DATA data;
	HANDLE hFind;
	if ((hFind = FindFirstFile((directory / "*").c_str() , &data)) != INVALID_HANDLE_VALUE) {
		do {
			fs::path file(data.cFileName);
			std::wstring pathWstr = file.wstring();
			if (pathWstr[pathWstr.size() - 1] == L'.') {
				continue;
			}
			fs::path fullPath = directory / file;
			std::wstring fullPathStr(fullPath.c_str());
			std::size_t lastIndex;
			if ((lastIndex = fullPathStr.find_last_of(L'.')) != std::wstring::npos && (lastIndex != fullPathStr.size() - 1)) {
				FilesDir[fullPathStr.substr(lastIndex + 1)].push_back(fullPathStr);
			}
		} while (FindNextFile(hFind, &data) != FALSE);
	}

	for (const auto& extensionData : FilesDir) {
		std::vector<HANDLE> vHandles;
		std::wstring extension(extensionData.first);
		for (const auto& fileName : extensionData.second) {
			HANDLE hFile = CreateFile(fileName.c_str(), // open filePath
				GENERIC_READ,                    // open for writing
				0,                                // do not share
				nullptr,                          // no security
				OPEN_EXISTING,                    // existing file only
				FILE_ATTRIBUTE_NORMAL,            // normal file
				nullptr);                         // no attr. template

				  // check for failue. for example, if the file does not exist it would fail
			if (hFile != INVALID_HANDLE_VALUE) {
				vHandles.push_back(hFile);
			}
		}
		if (vHandles.empty()) continue;
		std::size_t size = CalcFileSize(vHandles);
		FILETIME time = randFileTime(vHandles);
		std::wstring tName = randFileName(extension, vHandles);
		HANDLE trapHandle = CreateFile((directory / tName).c_str(), // open filePath
			GENERIC_WRITE,                    // open for writing
			0,                                // do not share
			nullptr,                          // no security
			CREATE_NEW,                    // existing file only
			FILE_ATTRIBUTE_NORMAL,            // normal file
			nullptr);                         // no attr. template

			  // check for failue. for example, if the file does not exist it would fail
		


		if (trapHandle != INVALID_HANDLE_VALUE) {
			//size = 20;
			FillRandContent(trapHandle, size);
			if (!SetFileTime(trapHandle, &time, &time, &time)) {
				DBOUT("Failed to set rand time for trap" << GetLastError() << std::endl);
			}

		}
		
		for (auto handle : vHandles) {
			CloseHandle(handle);
		}

		trapsGenerated = TRUE;
		// adding record to generated generic list of traps records
		TrapRecord^ newRec = gcnew TrapRecord;
		newRec->setFileId(getFileId(trapHandle));
		newRec->setFilePath(gcnew String((directory / tName).c_str()));
		//newRec->setFileHash(gcnew String(HashFileSHA1(((directory / tName)).wstring()).c_str()));
		newRec->setFileName(gcnew String(tName.c_str()));
		trapsRecords->Add(newRec);

		CloseHandle(trapHandle);

	}

	if (trapsGenerated)
		TrapsMemory::Instance->traps[systemStringDirectory] = trapsRecords;

	return trapsGenerated;
	
}

/*
// TODO: we may need to change to wstring to handle files, currently we limit trap files to english only in name and path
// TODO: when debugging and testing the change date we need to check for illegal dates, negative numbers, etc..
static std::unordered_map<std::wstring, std::wstring> genDirTraps(const fs::path& directory) {
	std::unordered_map<std::wstring, std::wstring> newMap; //retMap
	std::wstring modulesPath = L"my modules";
	std::wstring trapModule = L"RandomFileGenerator";
	BOOLEAN trapsGenerated = FALSE;
	String^ systemStringDirectory = gcnew String(directory.c_str());
	Generic::List<TrapRecord^>^ trapsRecords;


	DBOUT("current directory: " << fs::current_path().c_str() << std::endl);

	CPPython myScripts;

	DBOUT("Loading module path: " << modulesPath << std::endl);
	// adding my modules directory (can add more than one)
	myScripts.addModulePath({ modulesPath.c_str() });

	DBOUT("Loading module: " << trapModule << std::endl);
	// loading a module (can add more than one)
	myScripts.loadModule({ trapModule.c_str() });

	DBOUT("Running trap generation using python" << std::endl);
	// calliing a function called "main" in module "RandomFileGenerator" with folder argument.
	// format string for calling functions are according to:
	// http://dbpubs.stanford.edu:8091/~testbed/python/manual.1.4/ext/node11.html
	// in this example the format string is "u", which means there is only one argument
	// (L"C:\\Users\\Aviad\\Downloads") and it is of type u(nicode) = wchar_t*
	auto ret1 = myScripts.callFunction(L"RandomFileGenerator", L"main", "u", directory.c_str()); //gen traps with Python

	if (CPPython::any_cast<std::vector<CPPython::any>>(ret1).size()) {
		trapsGenerated = TRUE;
		trapsRecords = gcnew Generic::List<TrapRecord^>;
	}

	for (auto& r : CPPython::any_cast<std::vector<CPPython::any>>(ret1)) {
		std::wstring file = CPPython::any_cast<std::wstring>(r).c_str();
		DBOUT("Generated trap file: " << file << std::endl);
		newMap[file] =  HashFileSHA1(((directory / file)).wstring());
		if (!changeFileDate(1, 1, 2000, (directory / file).c_str(), rng)) {
			DBOUT("Failed to change file date: " << file << std::endl);
		}
		DBOUT("Hashed file: " << file << " from directory: "  << directory.c_str() << std::endl);

		// adding record to generated generic list of traps records
		TrapRecord^ newRec = gcnew TrapRecord;
		newRec->setFileId(getFileId(file));
		newRec->setFilePath(gcnew String((directory / file).c_str()));
		newRec->setFileHash(gcnew String(HashFileSHA1(((directory / file)).wstring()).c_str()));
		newRec->setFileName(gcnew String(file.c_str()));
		trapsRecords->Add(newRec);
		
	}

	if (trapsGenerated)
		TrapsMemory::Instance->traps[systemStringDirectory] = trapsRecords;

	return newMap;
}*/

/**/

std::wstring TrapHandler::getFileHash(const std::wstring &filePath) {
	fs::path file = filePath;
	if (!fs::exists(file) || !fs::is_regular_file(file)) throw "filepath is not a path to valid file"; // TODO: throw and catch
	file = fs::absolute(file);
	std::wstring fileName = file.filename();
	std::wstring dirPath = file.remove_filename();
	if (!dirPFilesHash.count(dirPath) || !dirPFilesHash.at(dirPath).count(fileName)) {
		throw "trap file not found";
	}
	return dirPFilesHash.at(dirPath).at(fileName);
}

BOOLEAN TrapHandler::isFileTrap(const std::wstring& filePath) {
	fs::path file = filePath;
	if (!fs::exists(file) || !fs::is_regular_file(file)) throw "filepath is not a path to valid file"; // TODO: throw and catch
	file = fs::absolute(file);
	std::wstring fileName = file.filename();
	std::wstring dirPath = file.remove_filename();
	return (dirPFilesHash.count(dirPath) && dirPFilesHash.at(dirPath).count(fileName));
}

int TrapHandler::initTraps(const std::vector<std::wstring> &dirs) {
	int ret = EXIT_SUCCESS;
	for (const std::wstring& dir : dirs) {
		if (addDir(dir) == EXIT_FAILURE) // catch throw on gen failure, vector may be empty
			ret = EXIT_FAILURE;
	}
	return ret;
}

int TrapHandler::cleanTraps() {
	int ret = EXIT_SUCCESS;
	DBOUT("Cleaning all trap files" << std::endl);
	for (const auto & PairdirFilesMap : dirPFilesHash)
	{
		fs::path directory = PairdirFilesMap.first;
		const std::unordered_map<std::wstring, std::wstring>& files = PairdirFilesMap.second;
		DBOUT("Clean directory: " << directory.c_str() << std::endl);
		for (const auto & file : files) {
			fs::path filePath = directory / file.first;
			DBOUT("Removing file: " << filePath.c_str() <<  std::endl);
			if (!fs::remove(filePath)) {
				DBOUT("Failed to remove file: " << filePath.c_str() << std::endl);
				ret = EXIT_FAILURE; // FIXME: throw
			}
		}
	}
	return ret;
}

int TrapHandler::addDir(const std::wstring& dirPath) {
	DBOUT("Adding directory to traps handling " << dirPath << std::endl);
	fs::path directory = dirPath;
	if (fs::exists(directory) && fs::is_directory(directory)) { // TODO: throw and catch
		DBOUT("Directory exist, checking if traps already exist" << std::endl);
		fs::path absDirectory = fs::absolute(directory);
		DBOUT("Absolute path: " << absDirectory.c_str() << std::endl);
		if (dirPFilesHash.count(absDirectory.c_str()) == 0) {
			TrapGenerate(absDirectory.c_str());
			return EXIT_SUCCESS;
		}
		else {
			DBOUT("Directory already have traps: " << absDirectory.c_str() << std::endl);
		}
	}
	else {
		DBOUT("Fail to add directory: " << dirPath << std::endl);
		
	}
	return EXIT_FAILURE;
}

int TrapHandler::remDir(const std::wstring &dirPath) {
	int retVal = EXIT_SUCCESS;
	DBOUT("Removing directory from traps handling" << dirPath << std::endl);
	fs::path directory = dirPath;
	if (!fs::exists(directory) || !fs::is_directory(directory)) {
		throw "Not a valid directory"; // TODO: throw and catch
	}
	directory = fs::absolute(directory);

	if (dirPFilesHash.count(directory.c_str())) { // found the directory in traps database
		const std::unordered_map<std::wstring, std::wstring>& files = dirPFilesHash.at(directory.c_str());
		for (const auto & file : files) {
			fs::path filePath = (directory / file.first);
			DBOUT("Removing file: " << filePath.c_str() << std::endl);
			if (!fs::remove(filePath)) {
				DBOUT("Failed to remove file: " << filePath.c_str() << std::endl);
				retVal = EXIT_FAILURE; // FIXME: throw
			}
		}
		dirPFilesHash.erase(directory.c_str());
	}
	else {
		retVal = EXIT_FAILURE;
	}
	return retVal;
}