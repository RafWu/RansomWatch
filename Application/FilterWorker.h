#pragma once

#include <comdef.h>
#include "Common.h"
#include "Process.h"
#include "Traps.h"
#include <set>
#include <msclr\marshal_cppstd.h>

DWORD
FilterWorker(
	_In_ PSCANNER_THREAD_CONTEXT Context
);

HRESULT ProcessIrp(const DRIVER_MESSAGE & msg);

VOID HandleMaliciousApplication(ProcessRecord^ record, HANDLE comPort);

VOID CheckHandleMaliciousApplication(ULONG pid, HANDLE comPort);