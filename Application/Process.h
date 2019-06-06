#pragma once

#include "Common.h"
using namespace System::Collections;
using namespace System::Threading;
using namespace System;

#include <Psapi.h>
#include "Traps.h"

ref struct ProcessRecord {
	ULONG pid;
	BOOLEAN isMalicious;
	String^ appName;
	ULONGLONG totalReadOperations;
	ULONGLONG totalWriteOperations;
	ULONGLONG totalRenameOperations;
	ULONGLONG totalCreateOperations;
	
	ULONGLONG trapsRead;
	ULONGLONG trapsWrite;
	ULONGLONG trapsOpened;
	ULONGLONG trapsRenameDelete;
	
	ULONGLONG dirListingOps;

	ULONGLONG filesOpenedCount;
	ULONGLONG filesCreatedCount;
	ULONGLONG filesWrittenCount;
	ULONGLONG filesReadCount;
	ULONGLONG filesRenamedCount;
	ULONGLONG filesMovedInCount;
	ULONGLONG filesMovedOutCount;
	ULONGLONG filesDeletedCount;
	ULONGLONG filesExtensionChanged;

	Generic::HashSet<FileId>^ fileIdsOpened;
	Generic::HashSet<FileId>^ fileIdsCreate;
	Generic::HashSet<FileId>^ fileIdsWrite;
	Generic::HashSet<FileId>^ fileIdsRead;
	Generic::HashSet<FileId>^ fileIdsRenamed;
	Generic::HashSet<FileId>^ fileIdsMoveIn;
	Generic::HashSet<FileId>^ fileIdsMoveOut;
	Generic::List<String^>^		 filesDeleted;   // paths

	ULONG fileExtensionTypesWrite;
	ULONG fileExtensionTypesRead;
	Generic::SortedSet<String^>^ extensionsRead;
	Generic::SortedSet<String^>^ extensionsWrite;

	DOUBLE averegeReadEntropy;
	DOUBLE averegeWriteEntropy;
	//ULONGLONG highEntropyReplaces;
	
	ULONGLONG totalReadBytes;
	ULONGLONG totalWriteBytes;

	//ULONGLONG LZJDDistanceExceededFiles;
	//ULONGLONG LZJDDistanceCalculatedFiles;

	ProcessRecord() {
		pid = 0;
		appName = nullptr;
		isMalicious = FALSE;

		totalReadOperations = 0;
		totalWriteOperations = 0;
		totalRenameOperations = 0;
		totalCreateOperations = 0;

		trapsRead = 0;
		trapsWrite = 0;
		trapsOpened = 0;
		trapsRenameDelete = 0;

		dirListingOps = 0;

		filesOpenedCount = 0;
		filesCreatedCount = 0;
		filesWrittenCount = 0;
		filesReadCount = 0;
		filesRenamedCount = 0;
		filesMovedInCount = 0;
		filesMovedOutCount = 0;
		filesDeletedCount = 0;
		filesExtensionChanged = 0;

		fileIdsOpened = gcnew Generic::HashSet<FileId>;
		fileIdsCreate = gcnew Generic::HashSet<FileId>;
		fileIdsWrite = gcnew Generic::HashSet<FileId>;
		fileIdsRead = gcnew Generic::HashSet<FileId>;
		fileIdsRenamed = gcnew Generic::HashSet<FileId>;
		fileIdsMoveIn = gcnew Generic::HashSet<FileId>;
		fileIdsMoveOut = gcnew Generic::HashSet<FileId>;
		filesDeleted = gcnew Generic::List<String^>;

		fileExtensionTypesWrite = 0;
		fileExtensionTypesRead = 0;
		extensionsRead = gcnew Generic::SortedSet<String^>;
		extensionsWrite = gcnew Generic::SortedSet<String^>;

		averegeReadEntropy = 0;
		averegeWriteEntropy = 0;
		//ULONGLONG highEntropyReplaces;

		totalReadBytes = 0;
		totalWriteBytes = 0;

	}

	ProcessRecord(ULONG PID) {
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

		isMalicious = FALSE;
		
		totalReadOperations = 0;
		totalWriteOperations = 0;
		totalRenameOperations = 0;
		totalCreateOperations = 0;
		
		trapsRead = 0;
		trapsWrite = 0;
		trapsOpened = 0;
		trapsRenameDelete = 0;

		dirListingOps = 0;

		filesOpenedCount = 0;
		filesCreatedCount = 0;
		filesWrittenCount = 0;
		filesReadCount = 0;
		filesRenamedCount = 0;
		filesMovedInCount = 0;
		filesMovedOutCount = 0;
		filesDeletedCount = 0;
		filesExtensionChanged = 0;

		fileIdsOpened  = gcnew Generic::HashSet<FileId>;
		fileIdsCreate = gcnew Generic::HashSet<FileId>;
		fileIdsWrite = gcnew Generic::HashSet<FileId>;
		fileIdsRead = gcnew Generic::HashSet<FileId>;
		fileIdsRenamed = gcnew Generic::HashSet<FileId>;
		fileIdsMoveIn = gcnew Generic::HashSet<FileId>;
		fileIdsMoveOut = gcnew Generic::HashSet<FileId>;
		filesDeleted = gcnew Generic::List<String^>;

		fileExtensionTypesWrite = 0;
		fileExtensionTypesRead = 0;
		extensionsRead = gcnew Generic::SortedSet<String^>;
		extensionsWrite = gcnew Generic::SortedSet<String^>;

		averegeReadEntropy = 0;
		averegeWriteEntropy = 0;
		//ULONGLONG highEntropyReplaces;

		totalReadBytes = 0;
		totalWriteBytes = 0;

	}

	BOOLEAN AddIrpRecord(const DRIVER_MESSAGE& Irp) {
		Monitor::Enter(this);
		// FIXME: add to fields, check file id and add to new file accordingly

		//DBOUT("Recived irp: " << Irp.IRP_OP << " process pid: " << Irp.PID);
		System::String^ newMsg = "Recieved an irp request: ";
		newMsg = System::String::Concat(newMsg, Irp.IRP_OP.ToString(), " From process id: ", Irp.PID.ToString(), System::Environment::NewLine);
		Globals::Instance->postLogMessage(newMsg);
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
			}

		}

		Monitor::Exit(this);
		return TRUE;
	}

	BOOLEAN IsFileIdTrapFIle(FileId id) {
		if (TrapsMemory::Instance->fileIdToTrapRecord->ContainsKey(id)) return TRUE;
		return FALSE;
	}

	void UpdateSetInfo(UCHAR fileChangeEnum, const FILE_ID_INFO& idInfo) {
		DBOUT("Update set info for irp message\n");
		FileId^ newId = gcnew FileId(idInfo);
		/*if (IsFileIdTrapFIle(idInfo)) {
			trapsRenameDelete++;
		}*/

		if (fileChangeEnum == FILE_CHANGE_DELETE_FILE) {
			filesDeletedCount++;
		}
		else if (fileChangeEnum == FILE_CHANGE_RENAME_FILE) {
			fileIdsRenamed->Add(*newId);
			filesRenamedCount++;
			totalRenameOperations++;
		}

	}

	void UpdateCreateInfo(UCHAR fileChangeEnum, const FILE_ID_INFO& idInfo) 
	{
		DBOUT("Update create info for irp message " << fileChangeEnum << "\n");
		FileId newId(idInfo);
		DBOUT("File id: ");
		for (ULONG i = 0; i < FILE_OBJECT_ID_SIZE ; i++)
		{
			DBOUT(newId.fileId[i] << " ");
		}
		DBOUT("\n");
		/*if (IsFileIdTrapFIle(newId)) {
			trapsOpened++;
		}*/
		totalCreateOperations++;
		switch (fileChangeEnum) {
		case FILE_CHANGE_NEW_FILE:
		{
			DBOUT("fileIdsCreate in\n");
			filesCreatedCount++;
			fileIdsCreate->Add(newId);
			DBOUT("fileIdsCreate out\n");
			break;
		}
		case FILE_CHANGE_OVERWRITE_FILE: // file is overwritten
		{
			filesCreatedCount++;
			//fileIdsCreate->Add(*newId);
		}
		case FILE_CHANGE_DELETE_FILE: //file opened but will be deleted when closed
		{
			filesDeletedCount++;
			/*if (IsFileIdTrapFIle(idInfo)) trapsRenameDelete++;*/
			break;
		}

		case FILE_CHANGE_DELETE_NEW_FILE: //new file but will be deleted when closed
		{
			break;
		}
		case FILE_OPEN_DIRECTORY: // directory listing
		{
			dirListingOps++;
			break;
		}
		default:
			DBOUT("fileIdsOpened in \n");
			if (fileIdsOpened->Add(newId)) filesOpenedCount++;
			DBOUT("fileIdsOpened out, size fileIdsOpened: " << filesOpenedCount << "\n");
			
			break;
		}
		DBOUT("Exit Create info for irp\n");

	}

	void UpdateWriteInfo(DOUBLE entropy, ULONGLONG writeSize, const LPCWSTR Extension, const FILE_ID_INFO& idInfo)
	{
		DBOUT("Update write info for irp message\n");
		FileId^ newId = gcnew FileId(idInfo);
		//BOOLEAN isTrap = IsFileIdTrapFIle(newId);
		totalWriteBytes += writeSize;
		
		//if (!fileIdsWrite->Contains(*newId)) {
			//fileIdsWrite->Add(*newId);
			filesWrittenCount++;
			//if (isTrap) trapsWrite++;
		//}
		// extensions
		/*
		if (extensionsWrite->Add(gcnew String(Extension))) {
			fileExtensionTypesWrite++;
		}*/

		//handle entropy
		// FIXME: need to account for sizes in bytes
		averegeWriteEntropy = ((averegeWriteEntropy * totalWriteOperations + entropy) / (totalWriteOperations + 1));

		totalWriteOperations++;
	}

	void UpdateReadInfo(DOUBLE entropy, ULONGLONG readSize, const LPCWSTR Extension, const FILE_ID_INFO& idInfo)
	{
		DBOUT("Update read info for irp message\n");
		FileId^ newId = gcnew FileId(idInfo);
		//BOOLEAN isTrap = IsFileIdTrapFIle(newId);
		totalReadBytes += readSize;
		
		//if (!fileIdsRead->Contains(*newId)) {
			//fileIdsRead->Add(*newId);
			filesReadCount++;
			//if (isTrap) trapsRead++;
		//}
		// extensions
		/*
		if (extensionsRead->Add(gcnew String(Extension))) {
			fileExtensionTypesRead++;
		}*/

		//handle entropy
		averegeReadEntropy = ((averegeReadEntropy * totalReadOperations + entropy) / (totalReadOperations + 1));

		totalReadOperations++;
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