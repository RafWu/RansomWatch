#pragma once

#include <sstream>
#include <windows.h>
#include <minwinbase.h>
#include <FltUser.h>
#include  <subauth.h>
#include "../SharedDefs/SharedDefs.h"
#include "Globals.h"
#include "FileId.h"
#include "Debug.h"

#define NUM_THREADS 2

LPCSTR IRP_TO_STRING(UCHAR irp);

typedef struct _SCANNER_THREAD_CONTEXT {

	HANDLE Port;
	//HANDLE Completion;

} SCANNER_THREAD_CONTEXT, *PSCANNER_THREAD_CONTEXT;
/*
System::String^ GetFilePathFromObjectId(FILE_ID_INFO fileId) {

}*/