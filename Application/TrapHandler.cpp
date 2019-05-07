#include "TrapHandler.h"

std::size_t CPPython::refcount = 0;

/*
static QString dirName(const std::wstring& filePath) {
	return filePath.substr(0, filePath.find_last_of('/'));
}

static QString fileName(const QString& filePath) {
	return filePath.substr(filePath.find_last_of('/') + 1);
}
*/

static std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>> dirPFilesHash;

static std::random_device rd;
static std::mt19937 rng(rd());

// TODO: maybe change return value to enum/define
// I use rng in the function to speed things up. it would be unneccesary
// and wasteful to create an rng for every honeypot we want to change.
static bool changeFileDate(const WORD day, const WORD month, const WORD year, const LPCWSTR& filePath, std::mt19937& rng) {
	// create uniform distribution number generator
	std::uniform_int_distribution<WORD> uniMinSec(0, 59);
	std::uniform_int_distribution<WORD> uniMilli(0, 999);

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
}

static std::wstring calcFileHash(const fs::path& filePath) {
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	DWORD dwStatus = 0;
	HANDLE hFile = nullptr;
	BOOLEAN bResult = FALSE;
	DWORD cbRead = 0; //read length
	DWORD cbHash = 0;
	return std::wstring();

}

// TODO: we may need to change to wstring to handle files, currently we limit trap files to english only in name and path
// TODO: when debugging and testing the change date we need to check for illegal dates, negative numbers, etc..
static std::unordered_map<std::wstring, std::wstring> genDirTraps(const fs::path& directory) {
	std::unordered_map<std::wstring, std::wstring> newMap; //retMap
	std::wstring modulesPath = L"C:\\test";
	std::wstring trapModule = L"RandomFileGenerator";
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

	for (auto& r : CPPython::any_cast<std::vector<CPPython::any>>(ret1)) {
		std::wstring file = CPPython::any_cast<std::wstring>(r).c_str();
		DBOUT("Generated trap file: " << file << std::endl);
		newMap[file] = calcFileHash((directory / file));
		if (!changeFileDate(1, 1, 2000, (directory / file).c_str(), rng)) {
			DBOUT("Failed to change file date: " << file << std::endl);
		}
		DBOUT("Hashed file: " << file << " from directory: "  << directory.c_str() << std::endl);
	}
	return newMap;
}

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
			dirPFilesHash[dirPath] = genDirTraps(absDirectory.c_str());
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
