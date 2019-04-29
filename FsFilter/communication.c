/*++
communication.c

Abstract:
	Communication module.
	Contains the routines that involves the communication between kernel mode and user mode applications.

Environment:
	Kernel mode
--*/

#include "FsFilter.h"

NTSTATUS FsPreparePort(_In_  PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSTATUS FsConnectNotifyCallback(PFLT_PORT clientPort, PVOID serverPortCookie, PVOID Context, ULONG size, PVOID ConnectionCookie);

VOID FsDisconnectNotifyCallback(PVOID ConnectionCookie);

NTSTATUS FsMessageNotifyCallback(PVOID PortCookie, PVOID InputBuffer, ULONG InputBufferLen, PVOID OutputBuffer, ULONG OutputBufferLen, PULONG RetLength);

/*++
Routine Description:
	A wrapper function that prepare the communicate port.

Arguments:
	SecurityDescriptor - Specifies a security descriptor to InitializeObjectAttributes(...).

Return Value:
	Returns the status of the prepartion.

--*/
NTSTATUS FsPreparePort(_In_  PSECURITY_DESCRIPTOR SecurityDescriptor)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES oa = { 0 };
	UNICODE_STRING uniString;
	LONG maxConnections = 1;
	PCWSTR portName = SCAN_PORT;
	PFLT_PORT *pServerPort = &globals.comServerPort;

	PAGED_CODE();
	/*
	AV_DBG_PRINT(AVDBG_TRACE_DEBUG,
		("[AV]: AvPrepareServerPort entered. \n"));

	switch (ConnectionType) {
	case AvConnectForScan:
		portName = AV_SCAN_PORT_NAME;
		pServerPort = &Globals.ScanServerPort;
		break;
	case AvConnectForAbort:
		portName = AV_ABORT_PORT_NAME;
		pServerPort = &Globals.AbortServerPort;
		break;
	case AvConnectForQuery:
		portName = AV_QUERY_PORT_NAME;
		pServerPort = &Globals.QueryServerPort;
		break;
	default:
		FLT_ASSERTMSG("No such connection type.\n", FALSE);
		return STATUS_INVALID_PARAMETER;
	}*/

	RtlInitUnicodeString(&uniString, portName);

	InitializeObjectAttributes(&oa,
		&uniString,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		SecurityDescriptor);



	status = FltCreateCommunicationPort(globals.Filter,
		pServerPort,  // this is the output to server port.
		&oa,
		NULL,
		FsConnectNotifyCallback,
		FsDisconnectNotifyCallback,
		FsMessageNotifyCallback,
		maxConnections);

	return status;
}

NTSTATUS FsConnectNotifyCallback(PFLT_PORT clientPort, PVOID serverPortCookie, PVOID Context, ULONG size, PVOID connectionCookie)
{
	globals.comClientPort = clientPort;
	KdPrint(("Connect client\r\n"));
	return STATUS_SUCCESS;
}

VOID FsDisconnectNotifyCallback(PVOID ConnectionCookie) 
{
	KdPrint(("DISConnecting \r\n"));
	FltCloseClientPort(globals.Filter, &globals.comClientPort);
	globals.comClientPort = NULL;
}

NTSTATUS FsMessageNotifyCallback(PVOID PortCookie, PVOID InputBuffer, ULONG InputBufferLen, PVOID OutputBuffer, ULONG OutputBufferLen, PULONG RetLength)
{
	PCHAR msg = "kernel msg";
	KdPrint(("user msg is : %s", (PCHAR)InputBuffer));

	strcpy((PCHAR)OutputBuffer, msg);
	return STATUS_SUCCESS;

}