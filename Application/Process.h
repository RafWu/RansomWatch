#pragma once

#include "Common.h"
using namespace System::Collections;
using namespace System::Threading;
using namespace System;

#include <Psapi.h>
#include "Traps.h"
#include "Thresholds.h"

ref class GProcessRecord {
	ULONGLONG gid;
	Generic::SortedSet<ULONG>^ Pids;
	BOOLEAN malicious;
	BOOLEAN killed;
	BOOLEAN reported;
	String^ appName;

	DateTime startTime;
	DateTime killTime;

	ULONGLONG totalReadOperations; // not used, for debug only
	ULONGLONG totalWriteOperations; // not used, for debug only
	ULONGLONG totalRenameOperations; // not used, for debug only
	ULONGLONG totalCreateOperations; // not used, for debug only
	
	ULONGLONG trapsRead;
	ULONGLONG trapsWrite;
	ULONGLONG trapsOpened; // not used, for debug only
	ULONGLONG trapsRenamed;
	ULONGLONG trapsDeleted;
	
	ULONGLONG filesMovedInCount;
	ULONGLONG filesMovedOutCount;
	ULONGLONG filesExtensionChanged;

	Generic::HashSet<FileId>^ dirsListed;
	Generic::HashSet<FileId>^ fileIdsChecked; // accessed
	Generic::HashSet<FileId>^ fileIdsDeleted;
	Generic::HashSet<FileId>^ fileIdsCreate;
	Generic::HashSet<FileId>^ fileIdsWrite;
	Generic::HashSet<FileId>^ fileIdsRead;
	Generic::HashSet<FileId>^ fileIdsRenamed;
	Generic::HashSet<FileId>^ fileIdsTraps;

	Generic::SortedSet<String^>^ filesCreated;  // paths created or moved in but not moved out before
	Generic::SortedSet<String^>^ filesChanged;  // paths removed, moved out, written to or renamed


	Generic::SortedSet<String^>^ dirsTrapsActions;	 // dir paths of which an action took place on a trap, write, read, rename or delete
	Generic::SortedSet<String^>^ triggersBreached;   // triggers that were breached, we use this to report later

	array<BOOLEAN>^ extensionsCategories;
	Generic::SortedSet<String^>^ extensionsRead;
	Generic::SortedSet<String^>^ extensionsWrite;

	DOUBLE sumWeightWriteEntropy;
	DOUBLE sumWeightReadEntropy;
	
	ULONGLONG totalReadBytes;
	ULONGLONG totalWriteBytes;

	public: Generic::SortedSet<String^>^ GetTriggersBreached() {
		Generic::SortedSet<String^>^ ret = gcnew Generic::SortedSet<String^>;
		Monitor::Enter(this);
		ret->UnionWith(triggersBreached);
		Monitor::Exit(this);
		return ret;
	}

	public: Generic::SortedSet<String^>^ GetChangedFiles() {
		Generic::SortedSet<String^>^ ret = gcnew Generic::SortedSet<String^>;
		Monitor::Enter(this);
		ret->UnionWith(filesChanged);
		Monitor::Exit(this);
		return ret;
	}
	public: Generic::SortedSet<String^>^ GetCreatedFiles() {
		Generic::SortedSet<String^>^ ret = gcnew Generic::SortedSet<String^>;
		Monitor::Enter(this);
		ret->UnionWith(filesCreated);
		Monitor::Exit(this);
		return ret;
	}

	private: DOUBLE AverageReadEntropy() {
		if (totalReadBytes)
			return (sumWeightReadEntropy / (DOUBLE)totalReadBytes);
		return 0;
	}

	private: DOUBLE AverageWriteEntropy() {
	if (totalWriteBytes)
		return (sumWeightWriteEntropy / (DOUBLE)totalWriteBytes);
	return 0;
	}

	public: GProcessRecord() {
		gid = 0;
		Pids = gcnew Generic::SortedSet<ULONG>;
		appName = nullptr;
		malicious = FALSE;
		killed = FALSE;
		reported = FALSE;

		startTime = DateTime::UtcNow; // TODO: change to utc to support azure

		totalReadOperations = 0;
		totalWriteOperations = 0;
		totalRenameOperations = 0;
		totalCreateOperations = 0;
		
		trapsRead = 0;
		trapsWrite = 0;
		trapsOpened = 0;
		trapsRenamed = 0;
		trapsDeleted = 0;
		
		filesMovedInCount = 0;
		filesMovedOutCount = 0;
		filesExtensionChanged = 0;
		
		dirsListed = gcnew Generic::HashSet<FileId>;
		fileIdsChecked = gcnew Generic::HashSet<FileId>;
		fileIdsCreate = gcnew Generic::HashSet<FileId>;
		fileIdsWrite = gcnew Generic::HashSet<FileId>;
		fileIdsRead = gcnew Generic::HashSet<FileId>;
		fileIdsRenamed = gcnew Generic::HashSet<FileId>;
		fileIdsDeleted = gcnew Generic::HashSet<FileId>;
		fileIdsTraps = gcnew Generic::HashSet<FileId>;

		filesCreated = gcnew Generic::SortedSet<String^>;
		filesChanged = gcnew Generic::SortedSet<String^>;

		dirsTrapsActions = gcnew Generic::SortedSet<String^>;
		triggersBreached = gcnew Generic::SortedSet<String^>;

		extensionsCategories = gcnew array<BOOLEAN>(NUM_CATEGORIES_WITH_OTHERS);
		for (ULONG i = 0; i < NUM_CATEGORIES_WITH_OTHERS; i++) {
			extensionsCategories[i] = FALSE;
		}
		extensionsRead   = gcnew Generic::SortedSet<String^>;
		extensionsWrite  = gcnew Generic::SortedSet<String^>;

		sumWeightWriteEntropy = 0;
		sumWeightReadEntropy = 0;

		totalReadBytes = 0;
		totalWriteBytes = 0;

	}
	public: GProcessRecord(ULONGLONG GID, ULONG Pid) {
		gid = GID;
		Pids = gcnew Generic::SortedSet<ULONG>;
		Pids->Add(Pid);
		appName = nullptr;
		HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			FALSE,
			Pid);
		if (nullptr == h)
		{
			DBOUT("OpenProcess() failed: " << GetLastError() << "\n");
		}
		else
		{
			WCHAR exe_path[1024] ;

			if (GetModuleFileNameEx(h, 0, exe_path, sizeof(exe_path) - sizeof(WCHAR)))
			{
				appName = gcnew System::String(exe_path);
			}
			else
			{
				DBOUT("GetModuleFileNameEx() failed: " << GetLastError() << "\n");
			}
			CloseHandle(h);
		}
		malicious = FALSE;
		killed = FALSE;
		reported = FALSE;

		startTime = DateTime::UtcNow;

		totalReadOperations = 0;
		totalWriteOperations = 0;
		totalRenameOperations = 0;
		totalCreateOperations = 0;
		
		trapsRead = 0;
		trapsWrite = 0;
		trapsOpened = 0;
		trapsRenamed = 0;
		trapsDeleted = 0;

		filesMovedInCount = 0;
		filesMovedOutCount = 0;
		filesExtensionChanged = 0;

		dirsListed = gcnew Generic::HashSet<FileId>;
		fileIdsChecked  = gcnew Generic::HashSet<FileId>;
		fileIdsCreate = gcnew Generic::HashSet<FileId>;
		fileIdsWrite = gcnew Generic::HashSet<FileId>;
		fileIdsRead = gcnew Generic::HashSet<FileId>;
		fileIdsRenamed = gcnew Generic::HashSet<FileId>;
		fileIdsDeleted = gcnew Generic::HashSet<FileId>;
		fileIdsTraps = gcnew Generic::HashSet<FileId>;

		filesCreated = gcnew Generic::SortedSet<String^>;
		filesChanged = gcnew Generic::SortedSet<String^>;

		dirsTrapsActions = gcnew Generic::SortedSet<String^>;
		triggersBreached = gcnew Generic::SortedSet<String^>;

		extensionsCategories = gcnew array<BOOLEAN>(NUM_CATEGORIES_WITH_OTHERS);
		for (ULONG i = 0; i < NUM_CATEGORIES_WITH_OTHERS; i++) {
			extensionsCategories[i] = FALSE;
		}
		extensionsRead = gcnew Generic::SortedSet<String^>;
		extensionsWrite = gcnew Generic::SortedSet<String^>;

		sumWeightWriteEntropy = 0;
		sumWeightReadEntropy = 0;

		totalReadBytes = 0;
		totalWriteBytes = 0;

	}
	public: BOOLEAN AddIrpRecord(const DRIVER_MESSAGE& Irp) {
		
		System::String^ newMsg = "Recieved an irp: ";
		newMsg = System::String::Concat(newMsg, gcnew String(IRP_TO_STRING(Irp.IRP_OP)), " From process id: ", Irp.PID.ToString(), " AppName: ", appName, System::Environment::NewLine);
		Globals::Instance->postLogMessage(newMsg, VERBOSE_ONLY);
		
		Pids->Add(Irp.PID);

		Monitor::Enter(this);
		switch (Irp.IRP_OP) 
		{
			case IRP_SETINFO: 
			{
				UpdateSetInfo(Irp.FileChange, Irp.FileID, Irp.Extension, Irp.FileLocationInfo, Irp.filePath);
				break;
			}
			case IRP_READ: 
			{
				UpdateReadInfo(Irp.Entropy, Irp.MemSizeUsed, Irp.Extension, Irp.FileID, Irp.FileLocationInfo);
				break;
			}
			case IRP_WRITE: 
			{
				UpdateWriteInfo(Irp.Entropy, Irp.MemSizeUsed, Irp.Extension, Irp.FileID, Irp.FileLocationInfo, Irp.filePath);
				break;
			}
			case IRP_CREATE: 
			{
				UpdateCreateInfo(Irp.FileChange, Irp.FileID, Irp.filePath, Irp.Extension);
				break;
			}
			case IRP_CLEANUP: 
			{
				DBOUT("Recived IRP_CLEANUP message: " << Irp.IRP_OP << " process pid: " << Irp.PID << std::endl);
				break;
			}
			default:
			{
				DBOUT("Recived unhandled irp message: " << Irp.IRP_OP << " process pid: " << Irp.PID << std::endl);
				Monitor::Exit(this);
				return FALSE;
			}

		}

		Monitor::Exit(this);
		return TRUE;
	}

	public: void UpdateSetInfo(UCHAR fileChangeEnum, const FILE_ID_INFO& idInfo, const LPCWSTR Extension, UCHAR fileLocationEnum, UNICODE_STRING filePath) {
		DBOUT("Update set info for irp message\n");
		FileId newId(idInfo);
		if (fileLocationEnum == FILE_MOVED_IN) {
			std::wstring fileNameStr(filePath.Buffer, filePath.Length / 2);
			String^ filePath = gcnew String(fileNameStr.c_str());
			if (!filesChanged->Contains(filePath)) { // file might be deleted and brought back
				DBOUT("Add file to created files: " << fileNameStr << "\n");
				filesCreated->Add(filePath);
			}
			filesMovedInCount++;
		}
		if (fileLocationEnum == FILE_MOVED_OUT || fileChangeEnum == FILE_CHANGE_DELETE_FILE) {
			filesMovedOutCount++;
			std::wstring fileNameStr(filePath.Buffer, filePath.Length / 2);
			String^ filePath = gcnew String(fileNameStr.c_str());
			if (!filesCreated->Contains(filePath)) { // file might be created inside before moving out
				DBOUT("Add file to changed files: " << fileNameStr << "\n");
				filesChanged->Add(filePath);
			}
		}

		if (fileChangeEnum == FILE_CHANGE_DELETE_FILE) {
			fileIdsDeleted->Add(newId);
			fileIdsChecked->Add(newId);
			if (IsFileIdTrapFIle(newId)) {
				trapsDeleted++;
				dirsTrapsActions->Add(TrapsMemory::Instance->fileIdToTrapRecord[newId]->getDirectory());
				fileIdsTraps->Add(newId);
			}
		}
		else if (fileChangeEnum == FILE_CHANGE_RENAME_FILE || fileChangeEnum == FILE_CHANGE_EXTENSION_CHANGED) {
			std::wstring fileNameStr(filePath.Buffer, filePath.Length / 2);
			String^ filePath = gcnew String(fileNameStr.c_str());
			if (!filesCreated->Contains(filePath)) { // if the was already deleted or created by the app, or moved in we report as only rename
				DBOUT("Add file to changed files: " << fileNameStr << "\n");
				filesChanged->Add(filePath);
			}
			fileIdsRenamed->Add(newId);
			fileIdsChecked->Add(newId);
			totalRenameOperations++;
			if (IsFileIdTrapFIle(newId)) {
				trapsRenamed++;
				fileIdsTraps->Add(newId);
			}
			if (fileChangeEnum == FILE_CHANGE_EXTENSION_CHANGED) {
				filesExtensionChanged++;
				DBOUT("Extension changed: " << Extension << std::endl);
				if (extensionsWrite->Add(gcnew String(Extension))) {
					DBOUT("Added extension " << Extension << std::endl);
				}
				else {
					DBOUT("No extension to add " << Extension << std::endl);
				}
			}

		}
	}

	private: void UpdateCreateInfo(UCHAR fileChangeEnum, const FILE_ID_INFO& idInfo, UNICODE_STRING filePath, const LPCWSTR Extension)
	{
		DBOUT("Update create info for irp message " << fileChangeEnum << "\n");
		FileId newId(idInfo);
		if (IsFileIdTrapFIle(newId)) {
			trapsOpened++;
			fileIdsTraps->Add(newId);
		}
		totalCreateOperations++;
		
		USHORT category = ExtensionCategory(Extension);
		if (category < NUM_CATEGORIES_WITH_OTHERS) {
			extensionsCategories[category] = TRUE;
		}
		

		switch (fileChangeEnum) {
		case FILE_CHANGE_NEW_FILE:
		{
			//DBOUT("fileIdsCreate in\n");
			fileIdsCreate->Add(newId);
			//DBOUT("fileIdsCreate out\n");
			std::wstring fileNameStr(filePath.Buffer, filePath.Length / 2);
			String^ filePath = gcnew String(fileNameStr.c_str());
			if (!filesChanged->Contains(filePath)) {
				DBOUT("Add file to changed files: " << fileNameStr << "\n");
				filesCreated->Add(filePath);
			}
			break;
		}
		case FILE_CHANGE_OVERWRITE_FILE: // file is overwritten
		{

			fileIdsCreate->Add(newId);
		}
		case FILE_CHANGE_DELETE_FILE: //file opened but will be deleted when closed
		{
			fileIdsDeleted->Add(newId);
			if (IsFileIdTrapFIle(newId)) {
				trapsDeleted++;
				dirsTrapsActions->Add(TrapsMemory::Instance->fileIdToTrapRecord[newId]->getDirectory());
			}
			std::wstring fileNameStr(filePath.Buffer, filePath.Length / 2);
			String^ filePath = gcnew String(fileNameStr.c_str());
			if (!filesCreated->Contains(filePath)) {
				DBOUT("Add file to changed files: " << fileNameStr << "\n");
				filesChanged->Add(filePath);
			}
			break;
		}

		case FILE_CHANGE_DELETE_NEW_FILE: //new file but will be deleted when closed
		{
			break;
		}
		case FILE_OPEN_DIRECTORY: // directory listing
		{
			dirsListed->Add(newId);
			break;
		}
		default:
			break;
		}
		DBOUT("Exit Create info for irp\n");

	}

	private: void UpdateWriteInfo(DOUBLE entropy, ULONGLONG writeSize, const LPCWSTR Extension, const FILE_ID_INFO& idInfo, UCHAR fileLocationEnum, UNICODE_STRING filePath)
	{
		DBOUT("Update write info for irp message\n");
		FileId newId(idInfo);
		BOOLEAN isTrap = IsFileIdTrapFIle(newId);
		totalWriteBytes += writeSize;
		
		if (fileLocationEnum == FILE_PROTECTED && !fileIdsWrite->Contains(newId)) {
			fileIdsWrite->Add(newId);
			fileIdsChecked->Add(newId);
			if (isTrap) {
				trapsWrite++; 
				fileIdsTraps->Add(newId);
				dirsTrapsActions->Add(TrapsMemory::Instance->fileIdToTrapRecord[newId]->getDirectory());
			}
			std::wstring fileNameStr(filePath.Buffer, filePath.Length / 2);
			String^ filePath = gcnew String(fileNameStr.c_str());
			if (!filesCreated->Contains(filePath)) {
				DBOUT("Add file to changed files: " << fileNameStr << "\n");
				filesChanged->Add(filePath);
			}
		}
		// extensions
		
		if (extensionsWrite->Add(gcnew String(Extension))) {
			DBOUT("Added extension " << Extension << std::endl);
		}
		else {
			DBOUT("No extension to add " << Extension << std::endl);
		}

		//handle entropy
		sumWeightWriteEntropy = (entropy * (DOUBLE)writeSize) + sumWeightWriteEntropy;
		//averegeWriteEntropy = ((averegeWriteEntropy * totalWriteOperations + entropy) / (totalWriteOperations + 1));

		totalWriteOperations++;
	}

	private: void UpdateReadInfo(DOUBLE entropy, ULONGLONG readSize, const LPCWSTR Extension, const FILE_ID_INFO& idInfo, UCHAR fileLocationEnum)
	{
		DBOUT("Update read info for irp message\n");
		FileId newId(idInfo);
		BOOLEAN isTrap = IsFileIdTrapFIle(newId);
		totalReadBytes += readSize;
		
		if (fileLocationEnum == FILE_PROTECTED && !fileIdsRead->Contains(newId)) {
			fileIdsRead->Add(newId);
			fileIdsChecked->Add(newId);
			if (isTrap) {
				trapsRead++;
				//fileIdsTraps->Add(newId); // TODO: remove overkill
				dirsTrapsActions->Add(TrapsMemory::Instance->fileIdToTrapRecord[newId]->getDirectory());
			}
		}
		// extensions
		
		if (extensionsRead->Add(gcnew String(Extension))) {
			DBOUT("Added extension " << Extension << std::endl);
		}
		else {
			DBOUT("No extension to add " << Extension << std::endl);
		}

		//handle entropy
		sumWeightReadEntropy = (entropy * (DOUBLE)readSize) + sumWeightReadEntropy;
		//averegeReadEntropy = ((averegeReadEntropy * totalReadOperations + entropy) / (totalReadOperations + 1));

		totalReadOperations++;
	}

	public: BOOLEAN setReported() {
		Monitor::Enter(this);
		if (reported) {
			Monitor::Exit(this);
			return FALSE;
		}
		reported = TRUE;
		Monitor::Exit(this);
		return TRUE;
	}

	// assumes that caller protect this call
	public: BOOLEAN isMalicious() {
		return malicious;
	}

	// assumes that caller protect this call
	public: BOOLEAN isKilled() {
		return killed;
	}
	
	// assumes that caller protect this call
	public: VOID setKilled() {
		killed = TRUE;
		killTime = DateTime::UtcNow;
	}
	
	// assumes that caller protect this call
	public: ULONGLONG Gid() {
		return gid;
	}
			// assumes that caller protect this call
	public: Generic::SortedSet<ULONG>^ pids() {
		Generic::SortedSet<ULONG>^ ret = gcnew Generic::SortedSet<ULONG>;
		ret->UnionWith(Pids);
		return ret;
	}
	// assumes that caller protect this call
	public: String^ Name() {
		return appName;
	}

	// assumes that caller protect this call
	public: DateTime DateStart() {
		return startTime;
	}
	
			// assumes that caller protect this call
	public: DateTime DateKilled() {
		return killTime;
	}

	public: BOOLEAN isProcessMalicious() {
		Monitor::Enter(this);

		ULONG numFilesChanged = filesChanged->Count;
		triggersBreached->Clear();

		BOOLEAN deleteTrigger = DeletionTrigger();
		if (deleteTrigger) {
			triggersBreached->Add("Delete");
		}
		BOOLEAN createTrigger = CreationTrigger();
		if (createTrigger) {
			triggersBreached->Add("Create");
		}
		BOOLEAN renameTrigger = RenamingTrigger();
		if (renameTrigger) {
			triggersBreached->Add("Renaming");
		}
		BOOLEAN listingTrigger = ListingTrigger();
		if (listingTrigger) {
			triggersBreached->Add("Listing");
		}
		BOOLEAN highEntropyTrigger = HighEntropyTrigger();
		if (highEntropyTrigger) {
			triggersBreached->Add("High entropy writes");
		}
		BOOLEAN extensionsTrigger = FileExtensionsTrigger();
		if (extensionsTrigger) {
			triggersBreached->Add("Extensions");
		}
		BOOLEAN extensionsChangeTrigger = ExtensionsChangedTrigger();
		if (extensionsChangeTrigger) {
			triggersBreached->Add("Extensions changed");
		}
		BOOLEAN trapsTrigger = TrapsTrigger(); // checked in other trigger but we do raise another trigger when work on traps reached certain point
		if (trapsTrigger) {
			triggersBreached->Add("Traps operations");
		}
		BOOLEAN readTrigger = ReadingTrigger(); // checked in other trigger but we do raise another trigger when work on traps reached certain point
		if (readTrigger) {
			triggersBreached->Add("Reading");
		}
		BOOLEAN accessTrigger = HighAccessTrigger(); // checked in other trigger but we do raise another trigger when work on traps reached certain point
		if (accessTrigger) {
			triggersBreached->Add("High accessing");
		}
		BOOLEAN moveTrigger = FilesMovedTrigger(); // checked in other trigger but we do raise another trigger when work on traps reached certain point
		if (moveTrigger) {
			triggersBreached->Add("Moving files");
		}

		BYTE triggersReached = deleteTrigger + createTrigger + 2 * renameTrigger + listingTrigger +
			2 * highEntropyTrigger + 2 * extensionsTrigger  + 2 * trapsTrigger +
			readTrigger + 2 * accessTrigger + 2 * extensionsChangeTrigger + 2 * moveTrigger;
		if (triggersReached >= TRIGGERS_TRESHOLD  && numFilesChanged > 2) { // might want to remove num writes check
			malicious = TRUE;
			Monitor::Exit(this);
			return TRUE;
		}

		Monitor::Exit(this);
		return FALSE;
	}

	// check if file id is a trap file
	private: BOOLEAN IsFileIdTrapFIle(FileId id) {
		if (TrapsMemory::Instance->fileIdToTrapRecord->ContainsKey(id)) return TRUE;
		return FALSE;
	}

	// deletion of files (with account to traps weight) vs number of protected files and vs number of files accessed in total
	private: BOOLEAN DeletionTrigger() {
		DOUBLE normDeletedFiles = 0;
		DOUBLE normDeletedFilesVsAccessed = 0;
		int deletedFiles = fileIdsDeleted->Count + TRAP_WEIGHT * trapsDeleted;
		int accessedFiles = fileIdsChecked->Count;
		ULONGLONG numOfFilesProtected = Globals::Instance->addNumOfDirsProtected(0);
		if (numOfFilesProtected > MINIMUM_FILES_THRESHOLD && accessedFiles > 0) { // enough files to decide
			normDeletedFiles = (DOUBLE)(deletedFiles) / (DOUBLE)(numOfFilesProtected);
			normDeletedFilesVsAccessed = (DOUBLE)(deletedFiles) / (DOUBLE)(accessedFiles);
		}
		if (normDeletedFiles > FILES_DELETED_THRESHOLD || normDeletedFilesVsAccessed > DELETED_ACCESSED_THRESHOLD) return TRUE;
		return FALSE;
	}

	// TODO: might collide with access trigger, maybe remove
	private: BOOLEAN CreationTrigger() {
		int createdFiles = fileIdsCreate->Count;
		int readWriteFilesCount = fileIdsChecked->Count;
		DOUBLE normFiles = 0;
		if (readWriteFilesCount)
			normFiles = (DOUBLE)(createdFiles) / (DOUBLE)(readWriteFilesCount);
		if (normFiles > FILES_CREATED_THRESHOLD && readWriteFilesCount > MINIMUM_FILES_CREATE_THRESHOLD) return TRUE;
		return FALSE;
	}

	// renaming of files (with account to traps weight) vs number of protected files and vs number of files accessed in total
	private: BOOLEAN RenamingTrigger() {
		DOUBLE normRenamedFiles = 0;
		DOUBLE normRenamedFilesVsAccessed = 0;
		int renamedFiles = fileIdsRenamed->Count + TRAP_WEIGHT * trapsRenamed;
		int accessedFiles = fileIdsChecked->Count;
		ULONGLONG numOfFilesProtected = Globals::Instance->addNumOfDirsProtected(0);
		if (numOfFilesProtected > MINIMUM_FILES_THRESHOLD && accessedFiles > 0) { // enough files to decide
			normRenamedFiles = (DOUBLE)(renamedFiles) / (DOUBLE)(numOfFilesProtected);
			normRenamedFilesVsAccessed = (DOUBLE)(renamedFiles) / (DOUBLE)(accessedFiles);
		}
		if (normRenamedFiles > FILES_RENAMED_THRESHOLD || normRenamedFilesVsAccessed > RENAMED_ACCESSED_THRESHOLD) return TRUE;
		return FALSE;
	}

	// listing od directories in protected area vs number of protected directories
	private: BOOLEAN ListingTrigger() {
		DOUBLE normListedDirs = 0;
		int dirListed = dirsListed->Count;
		ULONGLONG numOfSubdirsProtected = Globals::Instance->addNumOfDirsProtected(0);
		if (numOfSubdirsProtected > MINIMUM_DIRS_THRESHOLD) // enough dirs to decide
			normListedDirs = (DOUBLE)(dirListed) / (DOUBLE)(numOfSubdirsProtected);
		if (normListedDirs > LISTING_THRESHOLD) return TRUE;
		return FALSE;
	}


	private: BOOLEAN HighEntropyTrigger() {
		DOUBLE averegeReadEntropy  = AverageReadEntropy();
		DOUBLE averegeWriteEntropy = AverageWriteEntropy();

		if (averegeReadEntropy > 0 && averegeWriteEntropy > 0) { // read and write been done
			DOUBLE averageDiffEntropy = averegeWriteEntropy - averegeReadEntropy;
			return (averageDiffEntropy > ((MAX_ENTROPY - averegeReadEntropy) / 2.0));
		}
		if (averegeWriteEntropy > HIGH_ENTROPY_THRESHOLD) { // only writes been done, process can get data from other processes
			return TRUE;
		}
		return FALSE;
	}

	// extensions changed due to rename
	private: BOOLEAN ExtensionsChangedTrigger() {
		int extensionsChanged = filesExtensionChanged;
		int accessedFiles = fileIdsChecked->Count;
		DOUBLE normChangedExtensionsFiles = 0;
		ULONGLONG numOfFilesProtected = Globals::Instance->addNumOfDirsProtected(0);
		if (numOfFilesProtected > MINIMUM_FILES_THRESHOLD && accessedFiles > 0) { // enough files to decide
			normChangedExtensionsFiles = (DOUBLE)(extensionsChanged) / (DOUBLE)(accessedFiles);
		}
		if (normChangedExtensionsFiles > CHANGE_EXTENSION_THRESHOLD) return TRUE;
		return FALSE;
	}

	// check extension based on write and rename compared to read
	private: BOOLEAN FileExtensionsTrigger() {
		UCHAR numCategories = 0;
		for (ULONG i = 0; i < NUM_CATEGORIES_WITH_OTHERS; i++) {
			if (extensionsCategories[i]) numCategories++;
		}
		DOUBLE normNumCategories = ((DOUBLE)numCategories) / ((DOUBLE)NUM_CATEGORIES_WITH_OTHERS);
		if (normNumCategories > EXTENSION_OPENED_SENSITIVE) return TRUE;

		Generic::SortedSet<String^>^ extensionsUnion = gcnew Generic::SortedSet<String^>;
		extensionsUnion->UnionWith(extensionsWrite);
		extensionsUnion->UnionWith(extensionsRead);
		if (extensionsUnion->Count > 0) {
			return (((DOUBLE)extensionsWrite->Count / (DOUBLE)extensionsUnion->Count) < FILES_EXTENSION_THRESHOLD);

		}
		return FALSE;
	}

	// compare files moved inside protected area to files deleted and files moved out
	private: BOOLEAN FilesMovedTrigger() {
		if (filesMovedInCount > MINIMUM_FILES_CREATE_THRESHOLD && filesMovedOutCount > MINIMUM_FILES_CREATE_THRESHOLD) {
			return (((DOUBLE(filesMovedInCount)) / (DOUBLE(filesMovedOutCount))) > MOVE_THRESHOLD);
		}
		return FALSE;
	}

	private: BOOLEAN WriteToFilesTrigger() {
		return FALSE;
	}

	
	private: BOOLEAN HighAccessTrigger() {
		int writeCount = fileIdsWrite->Count + TRAP_WEIGHT * trapsWrite;
		int readWriteFilesCount = fileIdsChecked->Count;
		DOUBLE normFiles = 0;
		if (readWriteFilesCount)
			normFiles = (DOUBLE)(writeCount) / (DOUBLE)(readWriteFilesCount);
		if (normFiles > ACCESS_FILES_TRESHOLDS && readWriteFilesCount > MINIMUM_FILES_ACCESS_THRESHOLD) return TRUE;

		// if reached here there are not enough files written or number of files written is small
		// we can try to check bytes write data
		return FALSE;
	}

	// trigger when traps at least TRESHOLD num of directories been touched
	// better action against ransomware which targets one or two type of files
	private: BOOLEAN TrapsTrigger() {
		if (dirsTrapsActions->Count > TRAPS_DIRS_TRIGGER_THRESHOLD) return TRUE;
		return FALSE;
	}

	// Triggers when reading (with acount to traps weight went beyond threshold in account to number of protected files, minimum files required
	private: BOOLEAN ReadingTrigger() {
		DOUBLE normReadProtected = 0;
		int readCount = fileIdsRead->Count + TRAP_WEIGHT * trapsRead;
		ULONGLONG numOfFilesProtected = Globals::Instance->addNumOfDirsProtected(0);
		if (numOfFilesProtected > MINIMUM_FILES_THRESHOLD) // enough files to decide
			normReadProtected = (DOUBLE)(readCount) / (DOUBLE)(numOfFilesProtected);
		if (normReadProtected > FILES_READ_THRESHOLD) return TRUE;
		return FALSE;
	}
};

ref class ProcessesMemory {
public:
	ProcessesMemory() {
		// gid to record
		Processes = gcnew Concurrent::ConcurrentDictionary<ULONGLONG, GProcessRecord^>;
	}
private:
	ProcessesMemory(const ProcessesMemory%) { throw gcnew System::InvalidOperationException("ApplicationsMemory cannot be copy-constructed"); }
public: Concurrent::ConcurrentDictionary<ULONGLONG, GProcessRecord^>^ Processes;
public:
	static property ProcessesMemory^ Instance
	{
		ProcessesMemory^ get() { return m_Instance; }
	}

private:
	static ProcessesMemory^ m_Instance = gcnew ProcessesMemory;
};