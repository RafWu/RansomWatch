#pragma once

using namespace System;
using namespace System::Collections;
using namespace System::Threading;

#include <string>
#include <windows.h>
#include <minwinbase.h>
#include "BackupService.h"

// For verbose levels
constexpr BOOLEAN VERBOSE_ONLY = FALSE;
constexpr BOOLEAN PRIORITY_PRINT = TRUE;

// singleton for managing program across modules
ref class Globals {
private:
	BOOLEAN isKillProcess = TRUE;
	BOOLEAN isMonitorStopped = FALSE;
	BOOLEAN isCommClosed = TRUE;
	BOOLEAN verbose = FALSE;
	ULONGLONG TotalIrpsHandled = 0;
	ULONGLONG numOfFilesProtected = 0;
	ULONGLONG numOfDirsProtected = 0;
public: System::Windows::Forms::TextBox^ logView;
public: BackupService^ serviceBackup;

public: BackupService^ backupService() {
	return serviceBackup;
}

public: VOID setBackupService(BackupService^ newService) {
	serviceBackup = newService;
}

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

	BOOLEAN setVerbose(BOOLEAN newVerbose) {
		Monitor::Enter(logView);
		verbose = newVerbose;
		Monitor::Exit(logView);
		return newVerbose;
	}

	BOOLEAN Verbose() {
		BOOLEAN ret;
		Monitor::Enter(logView);
		ret = verbose;
		Monitor::Exit(logView);
		return ret;
	}
	
	/*FIXME: Might not work well*/
	void postLogMessage(String^ message, BOOLEAN isPriority)
	{
		Monitor::Enter(logView);
		if ((isPriority == VERBOSE_ONLY && verbose == TRUE) || (isPriority == PRIORITY_PRINT)) {
			logView->AppendText(message);
		}
		Monitor::Exit(logView);
	}

	ULONGLONG addNumOfFilesProtected(ULONGLONG Val) {
		ULONGLONG ret;
		Monitor::Enter(this);
		numOfFilesProtected += Val;
		ret = numOfFilesProtected;
		Monitor::Exit(this);
		return ret;
	}

	ULONGLONG redNumOfFilesProtected(ULONGLONG Val) {
		ULONGLONG ret;
		Monitor::Enter(this);
		if (numOfFilesProtected >= Val) {
			numOfFilesProtected -= Val;
		}
		else {
			numOfFilesProtected = 0;
		}
		ret = numOfFilesProtected;
		Monitor::Exit(this);
		return ret;
	}

	ULONGLONG addNumOfDirsProtected(ULONGLONG Val) {
		ULONGLONG ret;
		Monitor::Enter(this);
		numOfDirsProtected += Val;
		ret = numOfDirsProtected;
		Monitor::Exit(this);
		return ret;
	}

	ULONGLONG redNumOfDirsProtected(ULONGLONG Val) {
		ULONGLONG ret;
		Monitor::Enter(this);
		if (numOfDirsProtected >= Val) {
			numOfDirsProtected -= Val;
		}
		else {
			numOfDirsProtected = 0;
		}
		ret = numOfDirsProtected;
		Monitor::Exit(this);
		return ret;
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
		ret = isMonitorStopped;
		Monitor::Exit(this);
		return ret;
	}

	BOOLEAN setMonitorStat(BOOLEAN newStat) {
		Monitor::Enter(this);
		isMonitorStopped = newStat;
		Monitor::Exit(this);
		return newStat;
	}
private:
	static Globals^ m_Instance = gcnew Globals;

};

ref class FilterDirectories {
	// TODO: add serialize and deserialize of those containers
public: Concurrent::ConcurrentDictionary< String^, UCHAR>^ directories;
public:
	FilterDirectories() {
		directories = gcnew Concurrent::ConcurrentDictionary< String^, UCHAR>;
	}
private:
	FilterDirectories(const FilterDirectories%) { throw gcnew System::InvalidOperationException("FilterDirectories cannot be copy-constructed"); }
public:
	static property FilterDirectories^ Instance
	{
		FilterDirectories^ get() { return m_Instance; }
	}

private:
	static FilterDirectories^ m_Instance = gcnew FilterDirectories;

};

public delegate void msgDelegate(String^ msg);