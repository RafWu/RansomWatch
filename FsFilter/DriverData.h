#pragma once

#include <fltKernel.h>
#include "KernelCommon.h"
#include "KernelString.h"

class DriverData
{
	BOOLEAN FilterRun;
	PFLT_FILTER Filter;
	PDRIVER_OBJECT DriverObject;
	ULONG pid;
	ULONG irpOpsSize;
	ULONG directoryRootsSize;
	LIST_ENTRY irpOps;
	LIST_ENTRY rootDirectories;
	KSPIN_LOCK irpOpsLock;
	KSPIN_LOCK directoriesSpinLock;

public:
	DriverData(PDRIVER_OBJECT DriverObject);
	~DriverData();


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

	ULONG IrpSize() 
	{
		ULONG ret = 0;
		KIRQL irql = KeGetCurrentIrql();
		KeAcquireSpinLock(&irpOpsLock, &irql);
		ret = irpOpsSize;
		KeReleaseSpinLock(&irpOpsLock, irql);
		return ret;
	}

	BOOLEAN AddIrpMessage(PIRP_ENTRY newEntry)
	{

		KIRQL irql = KeGetCurrentIrql();
		KeAcquireSpinLock(&irpOpsLock, &irql);
		if (irpOpsSize < MAX_OPS_SAVE) {
			irpOpsSize++;
			InsertTailList(&irpOps, &newEntry->entry);
		}
		else {
			KeReleaseSpinLock(&irpOpsLock, irql);
			return FALSE;
		}
		KeReleaseSpinLock(&irpOpsLock, irql);
		return TRUE;
	}

	BOOLEAN RemIrpMessage(PIRP_ENTRY newEntry)
	{

		KIRQL irql = KeGetCurrentIrql();
		KeAcquireSpinLock(&irpOpsLock, &irql);
		RemoveEntryList(&newEntry->entry);
		irpOpsSize--;

		KeReleaseSpinLock(&irpOpsLock, irql);
		return TRUE;
	}

	PIRP_ENTRY GetFirstIrpMessage()
	{
		PLIST_ENTRY ret;
		KIRQL irql = KeGetCurrentIrql();
		KeAcquireSpinLock(&irpOpsLock, &irql);
		ret = RemoveHeadList(&irpOps);
		irpOpsSize--;
		KeReleaseSpinLock(&irpOpsLock, irql);
		if (ret == &irpOps) {
			return NULL;
		}
		return (PIRP_ENTRY)CONTAINING_RECORD(ret, IRP_ENTRY, entry);
	}

	// FIXME
	VOID DriverGetIrps(PVOID Buffer, ULONG BufferSize, PULONG ReturnOutputBufferLength) {
		*ReturnOutputBufferLength = sizeof(AMF_REPLY_IRPS);
		
		PCHAR OutputBuffer = (PCHAR)Buffer;
		OutputBuffer += sizeof(AMF_REPLY_IRPS);

		ULONG BufferSizeRemain = BufferSize - sizeof(AMF_REPLY_IRPS);
		
		AMF_REPLY_IRPS outHeader;
		PLIST_ENTRY irpEntryList;

		PIRP_ENTRY PrevEntry = nullptr;
		PDRIVER_MESSAGE Prev = nullptr;
		USHORT prevBufferSize = 0;
		//UNICODE_STRING  PrevFilePath;
		//PrevFilePath.Length = 0;
		//PrevFilePath.MaximumLength = 0;
		//PrevFilePath.Buffer = nullptr;
		
		KIRQL irql = KeGetCurrentIrql();
		KeAcquireSpinLock(&irpOpsLock, &irql);

		while (irpOpsSize) {
			irpEntryList = RemoveHeadList(&irpOps);
			irpOpsSize--;
			PIRP_ENTRY irp = (PIRP_ENTRY)CONTAINING_RECORD(irpEntryList, IRP_ENTRY, entry);
			UNICODE_STRING FilePath = irp->filePath;
			PDRIVER_MESSAGE irpMsg = &(irp->data);
			USHORT nameBufferSize = FilePath.Length;
			irpMsg->next = nullptr;
			if (FilePath.Length) {
				irpMsg->filePath.Length = nameBufferSize;
				irpMsg->filePath.MaximumLength = nameBufferSize;
				irpMsg->filePath.Buffer = nullptr;
			}

			if (sizeof(DRIVER_MESSAGE) + nameBufferSize >= BufferSizeRemain) { // return to irps list, not enough space
				InsertHeadList(&irpOps, irpEntryList);
				irpOpsSize++;
				break;
			} else {
				if (Prev != nullptr) {
					Prev->next = PDRIVER_MESSAGE(OutputBuffer + sizeof(DRIVER_MESSAGE) + prevBufferSize); // PrevFilePath might be 0 size
					Prev->filePath.Buffer = PWCH(OutputBuffer + sizeof(DRIVER_MESSAGE)); // filePath buffer is after irp
					RtlCopyMemory(OutputBuffer, Prev, sizeof(DRIVER_MESSAGE)); // copy previous irp
					if (prevBufferSize)
						RtlCopyMemory(OutputBuffer + sizeof(DRIVER_MESSAGE), PrevEntry->Buffer, prevBufferSize); // copy previous filePath
					OutputBuffer += prevBufferSize + sizeof(DRIVER_MESSAGE);
					outHeader.addSize(sizeof(DRIVER_MESSAGE) + prevBufferSize);
					*ReturnOutputBufferLength += sizeof(DRIVER_MESSAGE) + prevBufferSize;
					delete PrevEntry;
				}
			}

			PrevEntry = irp;
			Prev = irpMsg;
			prevBufferSize = nameBufferSize;
			if (prevBufferSize > MAX_FILE_NAME_SIZE) prevBufferSize = MAX_FILE_NAME_SIZE;
			BufferSizeRemain -= (sizeof(DRIVER_MESSAGE) + prevBufferSize);

			outHeader.addOp();
			
		}
		KeReleaseSpinLock(&irpOpsLock, irql);
		if (prevBufferSize > MAX_FILE_NAME_SIZE) prevBufferSize = MAX_FILE_NAME_SIZE;
		if (Prev != nullptr && PrevEntry != nullptr) {
			Prev->filePath.Buffer = PWCH(OutputBuffer + sizeof(DRIVER_MESSAGE)); // filePath buffer is after irp
			RtlCopyMemory(OutputBuffer, Prev, sizeof(DRIVER_MESSAGE)); // copy previous irp
			if (prevBufferSize)
				RtlCopyMemory(OutputBuffer + sizeof(DRIVER_MESSAGE), PrevEntry->Buffer, prevBufferSize); // copy previous filePath
			OutputBuffer += prevBufferSize + sizeof(DRIVER_MESSAGE);
			outHeader.addSize(sizeof(DRIVER_MESSAGE) + prevBufferSize);
			*ReturnOutputBufferLength += sizeof(DRIVER_MESSAGE) + prevBufferSize;
			delete PrevEntry;
		}

		if (outHeader.numOps()) {
			outHeader.data = PDRIVER_MESSAGE((PCHAR)Buffer + sizeof(AMF_REPLY_IRPS));
		}
		
		RtlCopyMemory((PCHAR)Buffer, &(outHeader), sizeof(AMF_REPLY_IRPS));
	}

	LIST_ENTRY GetAllEntries()
	{
		LIST_ENTRY newList;
		KIRQL irql = KeGetCurrentIrql();
		KeAcquireSpinLock(&irpOpsLock, &irql);
		irpOpsSize = 0;
		newList = irpOps;
		InitializeListHead(&irpOps);

		KeReleaseSpinLock(&irpOpsLock, irql);
		return newList;
	}

	BOOLEAN AddDirectoryEntry(PDIRECTORY_ENTRY newEntry)
	{
		BOOLEAN ret = FALSE;
		BOOLEAN foundMatch = FALSE;
		KIRQL irql = KeGetCurrentIrql();
		KeAcquireSpinLock(&directoriesSpinLock, &irql);

		PLIST_ENTRY pEntry = rootDirectories.Flink;
		while (pEntry != &rootDirectories)
		{
			PDIRECTORY_ENTRY pStrct;
			//
			// Do some processing.
			//
			pStrct = (PDIRECTORY_ENTRY)CONTAINING_RECORD(pEntry, DIRECTORY_ENTRY, entry);

			if (!wcsncmp(newEntry->path, pStrct->path, wcsnlen_s(newEntry->path, MAX_FILE_NAME_LENGTH))) // TODO: improve compare
			{
				foundMatch = TRUE;
				break;
			}
			//
			//Move to next Entry in list.
			//
			pEntry = pEntry->Flink;
		}
		if (foundMatch == FALSE) {
			InsertHeadList(&rootDirectories, &newEntry->entry);
			directoryRootsSize++;
			ret = TRUE;
		}
		KeReleaseSpinLock(&directoriesSpinLock, irql);
		return ret;
	}

	PDIRECTORY_ENTRY RemDirectoryEntry(LPCWSTR directory)
	{
		PDIRECTORY_ENTRY ret = NULL;
		KIRQL irql = KeGetCurrentIrql();
		KeAcquireSpinLock(&directoriesSpinLock, &irql);
		
		PLIST_ENTRY pEntry = rootDirectories.Flink;

		while(pEntry != &rootDirectories)
		{
			PDIRECTORY_ENTRY pStrct;
			//
			// Do some processing.
			//
			pStrct = (PDIRECTORY_ENTRY)CONTAINING_RECORD(pEntry, DIRECTORY_ENTRY, entry);
			
			if (!wcsncmp(directory, pStrct->path , wcsnlen_s(directory, MAX_FILE_NAME_LENGTH))) // TODO: improve compare
			{
				if (RemoveEntryList(pEntry)) {
					ret = pStrct;
					directoryRootsSize--;
					break;
				}
			}
			//
			//Move to next Entry in list.
			//
			pEntry = pEntry->Flink;
		}
		KeReleaseSpinLock(&directoriesSpinLock, irql);
		return ret;
	}
	
	/**
		IsContainingDirectory returns true if one of the directory entries in our LIST_ENTRY of PDIRECTORY_ENTRY is in the path passed as param
	*/
	BOOLEAN IsContainingDirectory(CONST PUNICODE_STRING path) {
		if (path == NULL || path->Buffer == NULL) return FALSE;
		BOOLEAN ret = FALSE;
		KIRQL irql = KeGetCurrentIrql();
		//DbgPrint("Looking for path: %ls in lookup dirs", path);
		KeAcquireSpinLock(&directoriesSpinLock, &irql);
		if (directoryRootsSize != 0) {
			PLIST_ENTRY pEntry = rootDirectories.Flink;
			while (pEntry != &rootDirectories) {
				PDIRECTORY_ENTRY pStrct = (PDIRECTORY_ENTRY)CONTAINING_RECORD(pEntry, DIRECTORY_ENTRY, entry);
				for (ULONG i = 0; i < path->Length; i++) {
					if (pStrct->path[i] == L'\0') {
						ret = TRUE; 
						break;
					}

					else if (pStrct->path[i] == path->Buffer[i]) {
						continue;
					}
					else {
						break; // for loop
					}
				}
				
				
				//ret = (wcsstr(path, pStrct->path) != NULL);
				if (ret) break;
				//Move to next Entry in list.
				pEntry = pEntry->Flink;
			}
		}
		KeReleaseSpinLock(&directoriesSpinLock, irql);
		return ret;
	}

	void Clear() {
		KIRQL irql = KeGetCurrentIrql();
		KeAcquireSpinLock(&directoriesSpinLock, &irql);
		PLIST_ENTRY pEntryDirs = rootDirectories.Flink;
		while (pEntryDirs != &rootDirectories) {
			LIST_ENTRY temp = *pEntryDirs;
			PDIRECTORY_ENTRY pStrct = (PDIRECTORY_ENTRY)CONTAINING_RECORD(pEntryDirs, DIRECTORY_ENTRY, entry);
			delete pStrct;
			//next
			pEntryDirs = temp.Flink;
		}
		directoryRootsSize = 0;
		InitializeListHead(&rootDirectories);
		KeReleaseSpinLock(&directoriesSpinLock, irql);
		
		KeAcquireSpinLock(&irpOpsLock, &irql);
		PLIST_ENTRY pEntryIrps = irpOps.Flink;
		while (pEntryIrps != &irpOps) {
			LIST_ENTRY temp = *pEntryIrps;
			PIRP_ENTRY pStrct = (PIRP_ENTRY)CONTAINING_RECORD(pEntryIrps, IRP_ENTRY, entry);
			delete pStrct;
			//next
			pEntryIrps = temp.Flink;
		}
		irpOpsSize = 0;
		InitializeListHead(&irpOps);
		KeReleaseSpinLock(&irpOpsLock, irql);

	}
};

extern DriverData* driverData;
