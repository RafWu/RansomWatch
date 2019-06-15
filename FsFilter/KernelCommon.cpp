#include "KernelCommon.h"

void *__cdecl operator new(size_t size) {
	return ExAllocatePoolWithTag(NonPagedPool, size, 'RW');
}

void __cdecl operator delete(void *data, size_t size) {
	UNREFERENCED_PARAMETER(size);
	if (data != NULL)
		ExFreePoolWithTag(data, 'RW');
}


// FIXME: add count param for copy length, MAX_FILE_NAME_LENGTH - 1 is default value
NTSTATUS CopyWString(LPWSTR dest, LPCWSTR source, size_t size)
{
	INT err = wcsncpy_s(dest, size, source, MAX_FILE_NAME_LENGTH - 1);
	if (err == 0) {
		dest[size - 1] = L'\0';
		return STATUS_SUCCESS;
	}
	else {
		return STATUS_INTERNAL_ERROR;
	}
}