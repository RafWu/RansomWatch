#pragma once

using namespace System;
using namespace System::Collections;
using namespace System::Threading;

#include <string>
#include <windows.h>
#include <minwinbase.h>

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
	
	/*FIXME: Might not work well*/
	void postLogMessage(String^ message)
	{
		Monitor::Enter(logView);
		logView->AppendText(message);
		Monitor::Exit(logView);
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

ref class FilterDirectories { // FIXME make use of
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

public delegate void msgDelegate(String^ msg);