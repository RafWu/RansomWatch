#include "TrapHandler.h"

#pragma comment(lib, "bcrypt.lib") 

static std::random_device rd;
static std::mt19937 rng(rd());
std::uniform_int_distribution<WORD> uniMinSec(0, 59);
std::uniform_int_distribution<WORD> uniMilli(0, 999);

void TrapHandler::FillRandContent(HANDLE file, std::size_t size) {
	BCryptGenRandom(nullptr, Buffer, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	Buffer[size - 1] = 0;
	WriteFile(file, Buffer, size, nullptr, nullptr);	
}

FileId TrapHandler::getFileId(HANDLE file) {
	FILE_ID_INFO idInfo;
	
	// open file and return a file handler
	// get file id info from file handle, store into idInfo which has 2 fields
	// field 1 is volume serial number
	// field 2 is unique file id stored in struct FILE_ID_128 which is a char array -  BYTE Identifier[16]
	if (!GetFileInformationByHandleEx(file, FileIdInfo, &idInfo, sizeof(idInfo))) {
		DBOUT("GetFileId failed" << std::endl);
	}
	FileId retFileId(idInfo);

	return retFileId;
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
		// it shouldn't throw normally, but if it does we don't want our application to crash because of it
	}
	return retval;

}

BOOLEAN TrapHandler::TrapGenerate(const fs::directory_entry& DirPath) {
	BOOLEAN trapsGenerated = FALSE;
	fs::path directory = DirPath.path();
	String^ systemStringDirectory = gcnew String(directory.c_str());
	Generic::List<TrapRecord^>^ trapsRecords = gcnew Generic::List<TrapRecord^>;
	std::unordered_map<std::wstring, std::vector<std::wstring>> FilesDir;
	WIN32_FIND_DATA data;
	HANDLE hFind;
	if ((hFind = FindFirstFile((directory / "*").c_str(), &data)) != INVALID_HANDLE_VALUE) {
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
		FileId newId = getFileId(trapHandle);
		newRec->setFileId(newId);
		newRec->setFilePath(gcnew String((directory / tName).c_str()));
		newRec->setFileName(gcnew String(tName.c_str()));
		trapsRecords->Add(newRec);

		TrapsMemory::Instance->fileIdToTrapRecord[newId] = newRec;

		CloseHandle(trapHandle);

	}

	if (trapsGenerated) {
		TrapsMemory::Instance->traps[systemStringDirectory] = trapsRecords;
	}

	return trapsGenerated;
	
}

int TrapHandler::initDirTraps(System::String ^ Path) {
	pin_ptr<const wchar_t> wch = PtrToStringChars(Path);
	std::wstring directoryStr(wch);
	fs::path Directory(directoryStr);
	ULONGLONG dirFiles = 0;
	ULONGLONG dirSubDirs = 0;
	int ret = EXIT_SUCCESS;
	for (auto& pDir : fs::recursive_directory_iterator(Directory)) { //iterate recursively all sub dirs
		if (pDir.is_directory() && addDir(pDir) == EXIT_FAILURE) {
			ret = EXIT_FAILURE;
		}
		if (pDir.is_directory()) {
			dirSubDirs++;
		}
		else {
			dirFiles++;
		}
		
		//DBOUT("Found file/dir: " << pDir << std::endl);
	}
	if (fs::is_directory(Directory) && addDir(fs::directory_entry(Directory)) == EXIT_FAILURE) {
		ret = EXIT_FAILURE;
	}
	Globals::Instance->addNumOfDirsProtected(dirSubDirs);
	Globals::Instance->addNumOfFilesProtected(dirFiles);
	return ret;
}

int TrapHandler::remDirTraps(System::String^ Path) {
	int ret = EXIT_SUCCESS;
	pin_ptr<const wchar_t> wch = PtrToStringChars(Path);
	std::wstring directoryStr(wch);
	fs::path Directory(directoryStr);
	if (TrapsMemory::Instance->traps->ContainsKey(Path)) {
		for (auto& pDir : fs::recursive_directory_iterator(Directory)) { //iterate recursively
			if (pDir.is_directory() && remDir(pDir) == EXIT_FAILURE) {
				ret = EXIT_FAILURE;
			}
		}
		if (fs::is_directory(Directory) && remDir(fs::directory_entry(Directory)) == EXIT_FAILURE) {
			ret = EXIT_FAILURE;
		}
	}
	else {
		return EXIT_FAILURE;
	}
	return ret;
}

int TrapHandler::cleanTraps() {
	int ret = EXIT_SUCCESS;
	DBOUT("Cleaning all trap files" << std::endl);
	for each (String^ RootPath in TrapsMemory::Instance->traps->Keys)
	{
		pin_ptr<const wchar_t> wch = PtrToStringChars(RootPath);
		std::wstring rootDirectoryStr(wch);
		DBOUT("Clean directory: " << rootDirectoryStr << std::endl);
		if (!remDirTraps(RootPath))  ret = EXIT_FAILURE;
	}
	return ret;
}

int TrapHandler::addDir(const fs::directory_entry& dirPath) {
	DBOUT("Adding directory to traps handling " << dirPath << std::endl);
	System::String^ PathSystem = gcnew System::String(dirPath.path().c_str());
	if (dirPath.exists()) {
		DBOUT("Directory exist, checking if traps already exist" << std::endl);
		if (!TrapsMemory::Instance->traps->ContainsKey(PathSystem)) {
			TrapGenerate(dirPath);
			return EXIT_SUCCESS;
		}
		else {
			DBOUT("Directory already have traps: " << dirPath << std::endl);
		}
	}
	else {
		DBOUT("Fail to add directory: " << dirPath << std::endl);
		
	}
	return EXIT_FAILURE;
}

int TrapHandler::remDir(const fs::directory_entry& dirPath) {
	int retVal = EXIT_SUCCESS;
	System::String^ Path = gcnew String(dirPath.path().c_str());
	DBOUT("Removing directory from traps handling" << dirPath << std::endl);
	fs::path directory = dirPath;
	if (!dirPath.exists()) {
		throw "Not a valid directory";
	}
	Generic::List<TrapRecord^>^ trapRecords = nullptr;
	if (TrapsMemory::Instance->traps->TryGetValue(Path, trapRecords) && trapRecords != nullptr) {
		if (trapRecords != nullptr) {
			
			for each (TrapRecord ^ record in trapRecords) {

				System::String^ filePath = record->getFilePath();
				pin_ptr<const wchar_t> wch = PtrToStringChars(filePath);
				std::wstring filePathStr(wch);
				DBOUT("Removing file: " << filePathStr << std::endl);

				FileId id = record->getFileId();
				TrapRecord^ tmp;
				TrapsMemory::Instance->fileIdToTrapRecord->TryRemove(id, tmp); //unlink
				try {
				fs::path fileFsPath(filePathStr);
				if (!fs::remove(fileFsPath)) {
					DBOUT("Failed to remove file: " << filePathStr << std::endl);
					retVal = EXIT_FAILURE; // FIXME: throw
				}
				}
				catch (const std::exception& exp) {
					DBOUT("Failed to remove file: " << exp.what() << std::endl);
					retVal = EXIT_FAILURE; // FIXME: throw
				}
				
			}
		}
		TrapsMemory::Instance->traps->TryRemove(Path, trapRecords);

	}
	else {
		DBOUT("No directory to remove: " << dirPath << std::endl);
		retVal = EXIT_FAILURE;
	}
	return retVal;
}