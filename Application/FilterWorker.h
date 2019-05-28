#pragma once


#include <Windows.h>
#include <FltUser.h>
#include <comdef.h>
#include "../SharedDefs/SharedDefs.h"
#include "Common.h"
#include "sharedContainers.h"
#include <msclr\marshal_cppstd.h>
/*
DWORD
ScannerWorker(
	_In_ PSCANNER_THREAD_CONTEXT Context
);
*/

DWORD
FilterWorker(
	_In_ PSCANNER_THREAD_CONTEXT Context
);

HRESULT ProcessIrp(const DRIVER_MESSAGE & msg);
