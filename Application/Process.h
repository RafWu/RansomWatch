#pragma once

#include "Common.h"
using namespace System::Collections;
using namespace System::Threading;
using namespace System;

#include <Psapi.h>
#include "Traps.h"
#include "Thresholds.h"

ref class ProcessRecord {
	ULONG pid;
	BOOLEAN malicious;
	BOOLEAN killed;
	String^ appName;
	ULONGLONG totalReadOperations;
	ULONGLONG totalWriteOperations;
	ULONGLONG totalRenameOperations;
	ULONGLONG totalCreateOperations;
	
	ULONGLONG trapsRead;
	ULONGLONG trapsWrite;
	ULONGLONG trapsOpened;
	ULONGLONG trapsRenameDelete;
	
	ULONGLONG dirListingCount;
	ULONGLONG filesOpenedCount;
	ULONGLONG filesCreatedCount;
	ULONGLONG filesWrittenCount;
	ULONGLONG filesReadCount;
	ULONGLONG filesRenamedCount;
	ULONGLONG filesMovedInCount;
	ULONGLONG filesMovedOutCount;
	ULONGLONG filesDeletedCount;
	ULONGLONG filesExtensionChanged;

	Generic::HashSet<FileId>^ dirsListed;
	Generic::HashSet<FileId>^ fileIdsOpened;
	Generic::HashSet<FileId>^ fileIdsCreate;
	Generic::HashSet<FileId>^ fileIdsWrite;
	Generic::HashSet<FileId>^ fileIdsRead;
	Generic::HashSet<FileId>^ fileIdsRenamed;
	Generic::HashSet<FileId>^ fileIdsMoveIn;
	Generic::HashSet<FileId>^ fileIdsMoveOut;
	Generic::List<String^>^	  filesDeleted;   // paths
	Generic::SortedSet<String^>^ triggersBreached;   // triggers that were breached, we use this to report later

	ULONG fileExtensionTypesWrite;
	ULONG fileExtensionTypesRead;
	Generic::SortedSet<String^>^ extensionsRead;
	Generic::SortedSet<String^>^ extensionsWrite;

	DOUBLE averegeReadEntropy;
	DOUBLE averegeWriteEntropy;
	
	ULONGLONG totalReadBytes;
	ULONGLONG totalWriteBytes;

	ULONGLONG highEntropyWrites;

	public: ProcessRecord() {
		pid = 0;
		appName = nullptr;
		malicious = FALSE;
		killed = FALSE;

		totalReadOperations = 0;
		totalWriteOperations = 0;
		totalRenameOperations = 0;
		totalCreateOperations = 0;

		trapsRead = 0;
		trapsWrite = 0;
		trapsOpened = 0;
		trapsRenameDelete = 0;

		dirListingCount = 0;
		filesOpenedCount = 0;
		filesCreatedCount = 0;
		filesWrittenCount = 0;
		filesReadCount = 0;
		filesRenamedCount = 0;
		filesMovedInCount = 0;
		filesMovedOutCount = 0;
		filesDeletedCount = 0;
		filesExtensionChanged = 0;
		
		dirsListed = gcnew Generic::HashSet<FileId>;
		fileIdsOpened = gcnew Generic::HashSet<FileId>;
		fileIdsCreate = gcnew Generic::HashSet<FileId>;
		fileIdsWrite = gcnew Generic::HashSet<FileId>;
		fileIdsRead = gcnew Generic::HashSet<FileId>;
		fileIdsRenamed = gcnew Generic::HashSet<FileId>;
		fileIdsMoveIn = gcnew Generic::HashSet<FileId>;
		fileIdsMoveOut = gcnew Generic::HashSet<FileId>;
		filesDeleted = gcnew Generic::List<String^>;
		triggersBreached = gcnew Generic::SortedSet<String^>;

		fileExtensionTypesWrite = 0;
		fileExtensionTypesRead = 0;
		extensionsRead = gcnew Generic::SortedSet<String^>;
		extensionsWrite = gcnew Generic::SortedSet<String^>;

		averegeReadEntropy = 0;
		averegeWriteEntropy = 0;
		//ULONGLONG highEntropyReplaces;

		totalReadBytes = 0;
		totalWriteBytes = 0;

		highEntropyWrites = 0;

	}
	public: ProcessRecord(ULONG PID) {
		pid = PID;
		appName = nullptr;
		HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			FALSE,
			PID);
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

		totalReadOperations = 0;
		totalWriteOperations = 0;
		totalRenameOperations = 0;
		totalCreateOperations = 0;
		
		trapsRead = 0;
		trapsWrite = 0;
		trapsOpened = 0;
		trapsRenameDelete = 0;

		dirListingCount = 0;
		filesOpenedCount = 0;
		filesCreatedCount = 0;
		filesWrittenCount = 0;
		filesReadCount = 0;
		filesRenamedCount = 0;
		filesMovedInCount = 0;
		filesMovedOutCount = 0;
		filesDeletedCount = 0;
		filesExtensionChanged = 0;

		dirsListed = gcnew Generic::HashSet<FileId>;
		fileIdsOpened  = gcnew Generic::HashSet<FileId>;
		fileIdsCreate = gcnew Generic::HashSet<FileId>;
		fileIdsWrite = gcnew Generic::HashSet<FileId>;
		fileIdsRead = gcnew Generic::HashSet<FileId>;
		fileIdsRenamed = gcnew Generic::HashSet<FileId>;
		fileIdsMoveIn = gcnew Generic::HashSet<FileId>;
		fileIdsMoveOut = gcnew Generic::HashSet<FileId>;
		filesDeleted = gcnew Generic::List<String^>;
		triggersBreached = gcnew Generic::SortedSet<String^>;

		fileExtensionTypesWrite = 0;
		fileExtensionTypesRead = 0;
		extensionsRead = gcnew Generic::SortedSet<String^>;
		extensionsWrite = gcnew Generic::SortedSet<String^>;

		averegeReadEntropy = 0;
		averegeWriteEntropy = 0;
		//ULONGLONG highEntropyReplaces;

		totalReadBytes = 0;
		totalWriteBytes = 0;

		highEntropyWrites = 0;

	}
	public: BOOLEAN AddIrpRecord(const DRIVER_MESSAGE& Irp) {
		
		System::String^ newMsg = "Recieved an irp request: ";
		newMsg = System::String::Concat(newMsg, Irp.IRP_OP.ToString(), " From process id: ", Irp.PID.ToString(), " AppName: ", appName, System::Environment::NewLine);
		Globals::Instance->postLogMessage(newMsg);
		
		Monitor::Enter(this);
		// FIXME: add to fields, check file id and add to new file accordingly

		//DBOUT("Recived irp: " << Irp.IRP_OP << " process pid: " << Irp.PID);

		//Globals::Instance->getTextBox()->AppendText(newMsg);
		switch (Irp.IRP_OP) 
		{
			case IRP_SETINFO: 
			{
				UpdateSetInfo(Irp.FileChange, Irp.FileID);
				break;
			}
			case IRP_READ: 
			{
				UpdateReadInfo(Irp.Entropy, Irp.MemSizeUsed, Irp.Extension, Irp.FileID);
				break;
			}
			case IRP_WRITE: 
			{
				UpdateWriteInfo(Irp.Entropy, Irp.MemSizeUsed, Irp.Extension, Irp.FileID);
				break;
			}
			case IRP_CREATE: 
			{
				UpdateCreateInfo(Irp.FileChange, Irp.FileID);
				break;
			}
			case IRP_CLEANUP: 
			{
				DBOUT("Recived IRP_CLEANUP message: " << Irp.IRP_OP << " process pid: " << Irp.PID);
				break;
			}
			default:
			{
				DBOUT("Recived unhandled irp message: " << Irp.IRP_OP << " process pid: " << Irp.PID);
				Monitor::Exit(this);
				return FALSE;
			}

		}

		Monitor::Exit(this);
		return TRUE;
	}

	public: void UpdateSetInfo(UCHAR fileChangeEnum, const FILE_ID_INFO& idInfo) {
		DBOUT("Update set info for irp message\n");
		FileId newId(idInfo);
		if (IsFileIdTrapFIle(newId)) {
			trapsRenameDelete++;
		}

		if (fileChangeEnum == FILE_CHANGE_DELETE_FILE) {
			filesDeletedCount++;
		}
		else if (fileChangeEnum == FILE_CHANGE_RENAME_FILE) {
			fileIdsRenamed->Add(newId);
			filesRenamedCount++;
			totalRenameOperations++;
		}

	}

	private: void UpdateCreateInfo(UCHAR fileChangeEnum, const FILE_ID_INFO& idInfo) 
	{
		DBOUT("Update create info for irp message " << fileChangeEnum << "\n");
		FileId newId(idInfo);
		/*DBOUT("File id: ");
		for (ULONG i = 0; i < FILE_OBJECT_ID_SIZE ; i++)
		{
			DBOUT(newId.fileId[i] << " ");
		}
		DBOUT("\n");*/
		if (IsFileIdTrapFIle(newId)) {
			trapsOpened++;
		}
		totalCreateOperations++;
		switch (fileChangeEnum) {
		case FILE_CHANGE_NEW_FILE:
		{
			//DBOUT("fileIdsCreate in\n");
			filesCreatedCount++;
			fileIdsCreate->Add(newId);
			//DBOUT("fileIdsCreate out\n");
			break;
		}
		case FILE_CHANGE_OVERWRITE_FILE: // file is overwritten
		{
			filesCreatedCount++;
			fileIdsCreate->Add(newId);
		}
		case FILE_CHANGE_DELETE_FILE: //file opened but will be deleted when closed
		{
			filesDeletedCount++;
			if (IsFileIdTrapFIle(newId)) trapsRenameDelete++;
			break;
		}

		case FILE_CHANGE_DELETE_NEW_FILE: //new file but will be deleted when closed
		{
			break;
		}
		case FILE_OPEN_DIRECTORY: // directory listing
		{
			if (dirsListed->Add(newId)) dirListingCount++;
			break;
		}
		default:
			//DBOUT("fileIdsOpened in \n");
			if (fileIdsOpened->Add(newId)) filesOpenedCount++;
			//DBOUT("fileIdsOpened out, size fileIdsOpened: " << filesOpenedCount << "\n");
			
			break;
		}
		DBOUT("Exit Create info for irp\n");

	}

	private: void UpdateWriteInfo(DOUBLE entropy, ULONGLONG writeSize, const LPCWSTR Extension, const FILE_ID_INFO& idInfo)
	{
		DBOUT("Update write info for irp message\n");
		FileId newId(idInfo);
		BOOLEAN isTrap = IsFileIdTrapFIle(newId);
		totalWriteBytes += writeSize;
		
		if (!fileIdsWrite->Contains(newId)) {
			fileIdsWrite->Add(newId);
			filesWrittenCount++;
			if (isTrap) trapsWrite++;
		}
		// extensions
		
		if (extensionsWrite->Add(gcnew String(Extension))) {
			DBOUT("Added extension " << Extension << std::endl);
			//System::String^ newMsg = gcnew String(Extension);
			//newMsg = System::String::Concat(newMsg, "IRP_MJ_WRITE added extension: ", System::Environment::NewLine);
			//Globals::Instance->postLogMessage(newMsg);
			fileExtensionTypesWrite++;
		}
		else {
			DBOUT("No extension to add " << Extension << std::endl);
		}
		if (entropy > ENTROPY_THRESHOLD) {
			highEntropyWrites++;
		}

		//handle entropy
		// FIXME: need to account for sizes in bytes
		averegeWriteEntropy = ((averegeWriteEntropy * totalWriteOperations + entropy) / (totalWriteOperations + 1));

		totalWriteOperations++;
	}

	private: void UpdateReadInfo(DOUBLE entropy, ULONGLONG readSize, const LPCWSTR Extension, const FILE_ID_INFO& idInfo)
	{
		DBOUT("Update read info for irp message\n");
		FileId newId(idInfo);
		BOOLEAN isTrap = IsFileIdTrapFIle(newId);
		totalReadBytes += readSize;
		
		if (!fileIdsRead->Contains(newId)) {
			fileIdsRead->Add(newId);
			filesReadCount++;
			if (isTrap) trapsRead++;
		}
		// extensions
		
		if (extensionsRead->Add(gcnew String(Extension))) {
			DBOUT("Added extension " << Extension << std::endl);
			//System::String^ newMsg = gcnew String(Extension);
			//newMsg = System::String::Concat(newMsg, "IRP_MJ_READ added extension: ", System::Environment::NewLine);
			//Globals::Instance->postLogMessage(newMsg);
			fileExtensionTypesRead++;
		}
		else {
			DBOUT("No extension to add " << Extension << std::endl);
		}

		//handle entropy
		// FIXME: account for bytes sizes
		averegeReadEntropy = ((averegeReadEntropy * totalReadOperations + entropy) / (totalReadOperations + 1));

		totalReadOperations++;
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
	}

	// assumes that caller protect this call
	public: ULONG Pid() {
		return pid;
	}
	// assumes that caller protect this call
	public: String^ Name() {
		return appName;
	}

	public: BOOLEAN isProcessMalicious() {
		Monitor::Enter(this);

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
		BOOLEAN writeFilesTrigger = WriteToFilesTrigger(); // compared to read files number
		if (writeFilesTrigger) {
			triggersBreached->Add("High writes threshold");
		}
		BOOLEAN trapsTrigger = TrapsTrigger(); // checked in other trigger but we do raise another trigger when work on traps reached certain point
		if (trapsTrigger) {
			triggersBreached->Add("Traps operations");
		}

		BYTE triggersReached = deleteTrigger + createTrigger + renameTrigger + listingTrigger +
			highEntropyTrigger + extensionsTrigger + writeFilesTrigger + trapsTrigger;
		if (triggersReached >= TRIGGERS_TRESHOLD && highEntropyWrites >= NUM_WRITES_FOR_TRIGGER) {
			malicious = TRUE;
			Monitor::Exit(this);
			return TRUE;
		}

		Monitor::Exit(this);
		return FALSE;
	}

	private: BOOLEAN IsFileIdTrapFIle(FileId id) {
		if (TrapsMemory::Instance->fileIdToTrapRecord->ContainsKey(id)) return TRUE;
		return FALSE;
	}

	private: BOOLEAN DeletionTrigger() {
		return FALSE;
	}
	private: BOOLEAN CreationTrigger() {
		return FALSE;
	}
	private: BOOLEAN RenamingTrigger() {
		return FALSE;
	}
	private: BOOLEAN ListingTrigger() {
		return FALSE;
	}
	private: BOOLEAN HighEntropyTrigger() {
		return FALSE;
	}
	private: BOOLEAN FileExtensionsTrigger() {
		return FALSE;
	}
	private: BOOLEAN WriteToFilesTrigger() {
		return FALSE;
	}
	private: BOOLEAN TrapsTrigger() {
		return FALSE;
	}
};

ref class ProcessesMemory {
	//std::shared_mutex appLock;
public:
	ProcessesMemory() {
		// pid to record
		Processes = gcnew Concurrent::ConcurrentDictionary<ULONG, ProcessRecord^>;
	}
private:
	ProcessesMemory(const ProcessesMemory%) { throw gcnew System::InvalidOperationException("ApplicationsMemory cannot be copy-constructed"); }
	// TODO: add serialize and deserialize of those containers
public: Concurrent::ConcurrentDictionary<DWORD, ProcessRecord^>^ Processes;
public:
	static property ProcessesMemory^ Instance
	{
		ProcessesMemory^ get() { return m_Instance; }
	}

private:
	static ProcessesMemory^ m_Instance = gcnew ProcessesMemory;
};