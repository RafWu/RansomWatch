#pragma once

using namespace System;
using namespace System::Collections;
using namespace System::Threading;



#include <string>
#include "Common.h"
//#include <shared_mutex>

ref struct ProcessRecord {
	ULONG pid;
	String^ appName;
	Generic::List<array<BYTE>^>^ fileIds;
	ULONG totalReadOperations;
	ULONG totalWriteOperations;
	ULONG totalRenameOperations;
	ULONG totalCreateOperations;
	ULONG totalDeleteOperations;
	//ULONG folderListings;
	DOUBLE averegeReadEntropy;
	DOUBLE averegeWriteEntropy;
	ULONG readFilesNumber;
	ULONG writeFilesNumber;
	ULONG renameFilesNumber;
	ULONGLONG totalReadBytes;
	ULONGLONG totalWriteBytes;
	ULONG trapsActions;
	ULONG writeToTraps;
	ULONG renameDeleteOnTraps;

	ProcessRecord() {
		pid = 0;
		appName = nullptr;
		fileIds = gcnew Generic::List<array<BYTE>^>;
		totalReadOperations = 0;
		totalWriteOperations = 0;
		totalRenameOperations = 0;
		totalCreateOperations = 0;
		totalDeleteOperations = 0;
		averegeReadEntropy = 0;
		averegeWriteEntropy = 0;
		readFilesNumber = 0;
		writeFilesNumber = 0;
		renameFilesNumber = 0;
		totalReadBytes = 0;
		totalWriteBytes = 0;
		trapsActions = 0;
		writeToTraps = 0;
		renameDeleteOnTraps = 0;
	}

	ProcessRecord(ULONG PID) {
		pid = PID;
		appName = nullptr;
		fileIds = gcnew Generic::List<array<BYTE>^>;
		totalReadOperations = 0;
		totalWriteOperations = 0;
		totalRenameOperations = 0;
		totalCreateOperations = 0;
		totalDeleteOperations = 0;
		averegeReadEntropy = 0;
		averegeWriteEntropy = 0;
		readFilesNumber = 0;
		writeFilesNumber = 0;
		renameFilesNumber = 0;
		totalReadBytes = 0;
		totalWriteBytes = 0;
		trapsActions = 0;
		writeToTraps = 0;
		renameDeleteOnTraps = 0;
	}

	BOOLEAN AddIrpRecord(const DRIVER_MESSAGE& Irp) {
		Monitor::Enter(this);
		// FIXME: add to fields, check file id and add to new file accordingly
		DBOUT("Recived irp: " << Irp.IRP_OP << " process pid: " << Irp.PID);
		
		Monitor::Exit(this);
		return TRUE;
	}
};

ref struct TrapRecord {
	array<BYTE>^ fileId; //we represent as string due to size changes, minimum of 128 bits in current systems
	String^ filePath;
	String^ fileName;
	String^ fileHash;

	void setFilePath(String^ newFilePath) { filePath = newFilePath; };
	void setFileHash(String^ newFileHash) { fileHash = newFileHash; };
	void setFileId(array<BYTE>^ newFileId) { fileId = newFileId; };
	void setFileName(String^ newFileName) { fileName = newFileName; };
	
	String^ getFilePath() { return filePath; };
	String^ getFileHash() { return fileHash; };
	String^ getFileName() { return fileName; };
	array<BYTE>^ getFileId() { return fileId; };
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

ref class TrapsMemory {
	// TODO: add serialize and deserialize of those containers
public: Concurrent::ConcurrentDictionary< String^, Generic::List<TrapRecord ^>^ > traps;
public: Concurrent::ConcurrentDictionary< array<BYTE>^, TrapRecord ^ > fileIdToTrapRecord;
public:
	static property TrapsMemory^ Instance
	{
		TrapsMemory^ get() { return m_Instance; }
	}

private:
	static TrapsMemory^ m_Instance = gcnew TrapsMemory;

};

ref class FilterDirectories {
	// TODO: add serialize and deserialize of those containers
public: Concurrent::ConcurrentBag< String^> directories;
public:
	static property FilterDirectories^ Instance
	{
		FilterDirectories^ get() { return m_Instance; }
	}

private:
	static FilterDirectories^ m_Instance = gcnew FilterDirectories;

};


ref class Globals {
private:
	BOOLEAN isKillProcess = TRUE;
	BOOLEAN isisMonitorStopped = FALSE;
	BOOLEAN isCommClosed = TRUE;
	ULONGLONG TotalIrpsHandled = 0;
public: System::Windows::Forms::TextBox^ logView;

public:
	static property Globals^ Instance
	{
		Globals^ get() { return m_Instance; }
	}

	void setTextBox(System::Windows::Forms::TextBox^ logViewer) {
		logView = logViewer;
	}

	System::Windows::Forms::TextBox^ getTextBox() {
		return logView;
	}

	ULONGLONG addIrpHandled(ULONGLONG num) {
		ULONGLONG ret;
		Monitor::Enter(this);
		TotalIrpsHandled += num;
		ret = TotalIrpsHandled;
		Monitor::Exit(this);
		return ret;
	}

	ULONGLONG getIrpHandled() {
		ULONGLONG ret;
		Monitor::Enter(this);
		ret = TotalIrpsHandled;
		Monitor::Exit(this);
		return ret;
	}


	BOOLEAN getCommCloseStat() {
		BOOLEAN ret;
		Monitor::Enter(this);
		ret = isCommClosed;
		Monitor::Exit(this);
		return ret;
	}

	BOOLEAN setCommCloseStat(BOOLEAN newStat) {
		Monitor::Enter(this);
		isCommClosed = newStat;
		Monitor::Exit(this);
		return newStat;
	}

	BOOLEAN getKillStat() {
		BOOLEAN ret;
		Monitor::Enter(this);
		ret = isKillProcess;
		Monitor::Exit(this);
		return ret;
	}

	BOOLEAN setKillStat(BOOLEAN newStat) {
		Monitor::Enter(this);
		isKillProcess = newStat;
		Monitor::Exit(this);
		return newStat;
	}

	BOOLEAN getMonitorStat() {
		BOOLEAN ret;
		Monitor::Enter(this);
		ret = isisMonitorStopped;
		Monitor::Exit(this);
		return ret;
	}

	BOOLEAN setMonitorStat(BOOLEAN newStat) {
		Monitor::Enter(this);
		isisMonitorStopped = newStat;
		Monitor::Exit(this);
		return newStat;
	}
private:
	static Globals^ m_Instance = gcnew Globals;

};