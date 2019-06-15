#pragma once

#include <fltKernel.h>
#include "../SharedDefs/SharedDefs.h"

typedef struct _DIRECTORY_ENTRY {
	LIST_ENTRY entry;
	WCHAR path[MAX_FILE_NAME_LENGTH];
	
} DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;

typedef struct _IRP_ENTRY {
	LIST_ENTRY entry;
	DRIVER_MESSAGE data;
	UNICODE_STRING filePath; // keep path to unicode string related to the object, we copy it later to user
	WCHAR Buffer[MAX_FILE_NAME_LENGTH];

	_IRP_ENTRY() {
		filePath.Length = 0;
		filePath.MaximumLength = MAX_FILE_NAME_SIZE;
		filePath.Buffer = Buffer;
		RtlZeroBytes(Buffer, MAX_FILE_NAME_LENGTH);
		data.next = nullptr;
		data.IRP_OP = IRP_NONE;
		data.MemSizeUsed = 0;
		data.isEntropyCalc = FALSE;
		data.FileChange = FILE_CHANGE_NOT_SET;
		data.FileLocationInfo = FILE_NOT_PROTECTED;
	}

	void* _IRP_ENTRY::operator new(size_t size)
	{
		void* ptr = ExAllocatePoolWithTag(NonPagedPool, size, 'RW');
		memset(ptr, 0, size);
		return ptr;
	}

	void _IRP_ENTRY::operator delete(void* ptr)
	{
		ExFreePoolWithTag(ptr, 'RW');
	}
	
} IRP_ENTRY, *PIRP_ENTRY;

void* __cdecl operator new(size_t size);

void __cdecl operator delete(void* data, size_t size);

NTSTATUS CopyWString(LPWSTR dest, LPCWSTR source, size_t size);