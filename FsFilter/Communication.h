#pragma once

#include <fltKernel.h>
#include "../SharedDefs/SharedDefs.h"
#include "DriverData.h"
#include <stdio.h>

struct CommHandler {

	//  Server-side communicate ports.
	PFLT_PORT ServerPort;

	//  port for a connection to user-mode
	PFLT_PORT ClientPort;

	//  The filter handle that results from a call to
	PFLT_FILTER Filter;
	
	//  A flag that indicating that the filter is connected
	BOOLEAN  CommClosed;

	//  User process that connected to the port

	ULONG UserProcess;

	CommHandler(PFLT_FILTER Filter) : ServerPort(NULL), ClientPort(NULL), Filter(Filter), CommClosed(TRUE), UserProcess(0){}

};

extern CommHandler* commHandle;

NTSTATUS InitCommData(
);

void CommClose();

BOOLEAN IsCommClosed();

NTSTATUS
AMFConnect(
	_In_ PFLT_PORT ClientPort,
	_In_opt_ PVOID ServerPortCookie,
	_In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Outptr_result_maybenull_ PVOID* ConnectionCookie
);

NTSTATUS AMFNewMessage(
	IN PVOID PortCookie,
	IN PVOID InputBuffer,
	IN ULONG InputBufferLength,
	OUT PVOID OutputBuffer,
	IN ULONG OutputBufferLength,
	OUT PULONG ReturnOutputBufferLength
);

VOID
AMFDissconnect(
	_In_opt_ PVOID ConnectionCookie
);



