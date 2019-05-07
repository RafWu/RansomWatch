#pragma once

/*++

Module Name:

	FsFilter.h

Abstract:
	
	Header file for the kernel FS driver

Environment:

	Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "ukshared.h"

#define SCAN_PORT L"\\FsFilter"

//
//  The global variable
//

typedef struct _FSFILTER_GLOBAL_DATA {

	//  The object that identifies this driver.

	PDRIVER_OBJECT DriverObject;

	//  The filter handle that results from a call to

	PFLT_FILTER Filter;

	//  Server-side communicate ports.

	PFLT_PORT ServerPort;

	//  port for a connection to user-mode

	PFLT_PORT ClientPort;
	
	//  User process that connected to the port

	PEPROCESS UserProcess;
	
	//  A flag that indicating that the filter is being unloaded.


	BOOLEAN  Unloading;
} GLOBAL_DATA, *PGLOBAL_DATA;


extern GLOBAL_DATA globalData;

typedef struct _SCANNER_STREAM_HANDLE_CONTEXT {

	BOOLEAN RescanRequired;

} SCANNER_STREAM_HANDLE_CONTEXT, *PSCANNER_STREAM_HANDLE_CONTEXT;

#pragma warning(push)
#pragma warning(disable:4200) // disable warnings for structures with zero length arrays.

typedef struct _SCANNER_CREATE_PARAMS {

	WCHAR String[0];

} SCANNER_CREATE_PARAMS, *PSCANNER_CREATE_PARAMS;

#pragma warning(pop)


///////////////////////////////////////////////////////////////////////////
//
//  Prototypes for the startup and unload routines used for 
//  this Filter.
//
//  Implementation in scanner.c
//
///////////////////////////////////////////////////////////////////////////

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
FSUnloadDriver(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
FSPreCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
FSPostCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
FSPreCleanup(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

FLT_PREOP_CALLBACK_STATUS
FSPreWrite(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

#if (WINVER >= 0x0602)

FLT_PREOP_CALLBACK_STATUS
FSPreFileSystemControl(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

#endif

