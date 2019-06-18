#pragma once

#include <fltKernel.h>
#include "KernelCommon.h"
#include "KernelString.h"
#include "HashTable.h"

class DriverData
{
	BOOLEAN FilterRun;
	PFLT_FILTER Filter;
	PDRIVER_OBJECT DriverObject;
	WCHAR systemRootPath[MAX_FILE_NAME_LENGTH];
	ULONG pid;
	ULONG irpOpsSize;
	ULONG directoryRootsSize;
	LIST_ENTRY irpOps;
	LIST_ENTRY rootDirectories;
	KSPIN_LOCK irpOpsLock;
	KSPIN_LOCK directoriesSpinLock;
	
	// new code start
	ULONGLONG GidCounter;
	HashMap GidToPids; // list entry of pids
	HashMap PidToGids;
	ULONGLONG gidsSize;
	LIST_ENTRY GidsList;
	KSPIN_LOCK PIDRecordsLock;

	// new code end

private:
	// call assumes protected code
	BOOLEAN RemoveProcessRecordAux(ULONG ProcessId, ULONGLONG gid);

	// call assumes protected code
	BOOLEAN RemoveGidRecordAux(PGID_ENTRY gidRecord);

public:
	explicit DriverData(PDRIVER_OBJECT DriverObject);
	
	~DriverData();

	PWCHAR GetSystemRootPath() {
		return systemRootPath;
	}

	VOID setSystemRootPath(PWCHAR setsystemRootPath) {
		RtlZeroBytes(systemRootPath, MAX_FILE_NAME_SIZE);
		RtlCopyBytes(systemRootPath, setsystemRootPath, MAX_FILE_NAME_LENGTH);
		RtlCopyBytes(systemRootPath + wcsnlen(systemRootPath, MAX_FILE_NAME_LENGTH / 2), L"\\Windows", wcsnlen(L"\\Windows", MAX_FILE_NAME_LENGTH / 2));
		//CopyWString(systemRootPath, setSystemRootPath, MAX_FILE_NAME_LENGTH);
		//CopyWString(systemRootPath + wcsnlen(systemRootPath, MAX_FILE_NAME_LENGTH / 2), L"\\Windows", MAX_FILE_NAME_LENGTH);
		DbgPrint("Set system root path %ls\n", systemRootPath);
	}

	BOOLEAN RemoveProcess(ULONG ProcessId);

	BOOLEAN RecordNewProcess(PUNICODE_STRING ProcessName, ULONG ProcessId, ULONG ParentPid);

	BOOLEAN RemoveGid(ULONGLONG gid);

	ULONGLONG GetGidSize(ULONGLONG gid, PBOOLEAN found);

	BOOLEAN GetGidPids(ULONGLONG gid, PULONG buffer, ULONGLONG bufferSize, PULONGLONG returnedLength);

	// if found return true on found else return false
	ULONGLONG GetProcessGid(ULONG ProcessId, PBOOLEAN found);

	//clear all data related to Gid system
	VOID ClearGidsPids();
	
	ULONGLONG GidsSize();

	BOOLEAN setFilterStart() {
		return (FilterRun = TRUE);
	}

	BOOLEAN setFilterStop() {
		return (FilterRun = FALSE);
	}

	BOOLEAN isFilterClosed() {
		return !FilterRun;
	}

	PFLT_FILTER* getFilterAdd() { return &Filter; }
	PFLT_FILTER getFilter() { return Filter; }
	ULONG getPID() { return pid; }
	ULONG setPID(ULONG Pid) { pid = Pid; return Pid; }

	VOID ClearIrps();

	ULONG IrpSize();

	BOOLEAN AddIrpMessage(PIRP_ENTRY newEntry);

	BOOLEAN RemIrpMessage(PIRP_ENTRY newEntry);

	PIRP_ENTRY GetFirstIrpMessage();

	VOID DriverGetIrps(PVOID Buffer, ULONG BufferSize, PULONG ReturnOutputBufferLength);

	LIST_ENTRY GetAllEntries();

	BOOLEAN AddDirectoryEntry(PDIRECTORY_ENTRY newEntry);

	PDIRECTORY_ENTRY RemDirectoryEntry(LPCWSTR directory);
	
	/**
		IsContainingDirectory returns true if one of the directory entries in our LIST_ENTRY of PDIRECTORY_ENTRY is in the path passed as param
	*/
	BOOLEAN IsContainingDirectory(CONST PUNICODE_STRING path);

	VOID ClearDirectories();

	VOID Clear() {
		// clear directories
		ClearDirectories();

		// clear irps
		ClearIrps();

		// clear gid system
		ClearGidsPids();

	}
};

extern DriverData* driverData;
