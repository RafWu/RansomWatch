#pragma once

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#define SCAN_PORT L"\\FsFilter"

//
//  The global variable
//

typedef struct _FSFILTER_GLOBAL_DATA {

	//
	//  The global FLT_FILTER pointer. Many API needs this, such as 
	//  FltAllocateContext(...)
	//

	PFLT_FILTER Filter;

	//
	//  Server-side communicate ports.
	//

	PFLT_PORT comServerPort;


	PFLT_PORT comClientPort;

#if DBG
	ULONG DebugLevel; // Field to control nature of debug output
#endif

	
	BOOLEAN  Unloading; // A flag that indicating that the filter is being unloaded.

} FSFILTER_GLOBAL_DATA, *PFSFILTER_GLOBAL_DATA;

NTSTATUS FsPreparePort(_In_  PSECURITY_DESCRIPTOR SecurityDescriptor);

FSFILTER_GLOBAL_DATA globals;
