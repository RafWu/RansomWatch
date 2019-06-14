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

} IRP_ENTRY, *PIRP_ENTRY;

void* __cdecl operator new(size_t size);
void* __cdecl operator new[](size_t size);

void __cdecl operator delete(void *object, unsigned __int64);
void __cdecl operator delete[](void *p);

NTSTATUS CopyWString(LPWSTR dest, LPCWSTR source, size_t size);