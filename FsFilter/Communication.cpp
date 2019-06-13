#include "Communication.h"

NTSTATUS InitCommData(

) 
{
	HRESULT status;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uniString;
	PSECURITY_DESCRIPTOR sd;
	//
	//  Create a communication port.
	//
	RtlInitUnicodeString(&uniString, ComPortName);

	//
	//  We secure the port so only ADMINs & SYSTEM can acecss it.
	//

	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);

	if (NT_SUCCESS(status)) {

		InitializeObjectAttributes(&oa,
			&uniString,
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
			NULL,
			sd);

		status = FltCreateCommunicationPort(commHandle->Filter,
			&commHandle->ServerPort,
			&oa,
			NULL,
			AMFConnect,
			AMFDissconnect,
			AMFNewMessage,
			1);
		//
		//  Free the security descriptor in all cases. It is not needed once
		//  the call to FltCreateCommunicationPort() is made.
		//

		FltFreeSecurityDescriptor(sd);

	}

	return status;
}

BOOLEAN IsCommClosed() {
	return commHandle->CommClosed;
}

void CommClose()
{
	//FLT_ASSERT(IsCommClosed());
	
	if (commHandle->ClientPort) {
		FltCloseClientPort(commHandle->Filter, &commHandle->ClientPort);
		commHandle->ClientPort = NULL;
	}

	if (commHandle->ServerPort) {
		FltCloseCommunicationPort(commHandle->ServerPort);
		commHandle->ServerPort = NULL;
	}
	commHandle->UserProcess = NULL;
	commHandle->CommClosed = TRUE;

}

NTSTATUS
AMFConnect(
	_In_ PFLT_PORT ClientPort,
	_In_opt_ PVOID ServerPortCookie,
	_In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Outptr_result_maybenull_ PVOID *ConnectionCookie
)
{

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie = NULL);

	FLT_ASSERT(commHandle->ClientPort == NULL);

	//
	//  Set the user process and port. In a production filter it may
	//  be necessary to synchronize access to such fields with port
	//  lifetime. For instance, while filter manager will synchronize
	//  FltCloseClientPort with FltSendMessage's reading of the port 
	//  handle, synchronizing access to the UserProcess would be up to
	//  the filter.
	//

	commHandle->ClientPort = ClientPort;
	commHandle->CommClosed = FALSE;
	DbgPrint("!!! user connected, port=0x%p\n", ClientPort);

	return STATUS_SUCCESS;
}


VOID
AMFDissconnect(
	_In_opt_ PVOID ConnectionCookie
)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);

	DbgPrint("!!! user disconnected, port=0x%p\n", commHandle->ClientPort);

	//
	//  Close our handle to the connection: note, since we limited max connections to 1,
	//  another connect will not be allowed until we return from the disconnect routine.
	//

	FltCloseClientPort(commHandle->Filter, &commHandle->ClientPort);

	//
	//  Reset the user-process field.
	//
	DbgPrint("Disconnent\n");
	commHandle->CommClosed = TRUE;
}

NTSTATUS AMFNewMessage(
	IN PVOID PortCookie,
	IN PVOID InputBuffer,
	IN ULONG InputBufferLength,
	OUT PVOID OutputBuffer,
	IN ULONG OutputBufferLength,
	OUT PULONG ReturnOutputBufferLength
)
{
	UNREFERENCED_PARAMETER(PortCookie);
	UNREFERENCED_PARAMETER(InputBufferLength);

	*ReturnOutputBufferLength = 0;

	COM_MESSAGE* message = static_cast<COM_MESSAGE*> (InputBuffer);
	if (message == NULL) return STATUS_INTERNAL_ERROR; //failed message type

	if (message->type == MESSAGE_ADD_SCAN_DIRECTORY) {
		DbgPrint("Recived add directory message\n");
		PDIRECTORY_ENTRY newEntry = new DIRECTORY_ENTRY();
		if (newEntry == NULL) {
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		NTSTATUS hr = CopyWString(newEntry->path, message->path, MAX_FILE_NAME_LENGTH);
		if (!NT_SUCCESS(hr)) {
			delete newEntry;
			return STATUS_INTERNAL_ERROR;
		}
		*ReturnOutputBufferLength = 1;
		if (driverData->AddDirectoryEntry(newEntry)) {
			/*if (OutputBuffer != NULL && OutputBufferLength > 0) {
				swprintf_s((WCHAR*)OutputBuffer, (size_t)OutputBufferLength, L"Added scan directory \"%ls\"", message->path);
				*ReturnOutputBufferLength = (ULONG)wcsnlen_s((WCHAR*)OutputBuffer, static_cast<size_t>(OutputBufferLength)) + 1;
			}*/
			*((PBOOLEAN)OutputBuffer) = TRUE;
			DbgPrint("Added scan directory successfully\n");
			return STATUS_SUCCESS;
		}
		else {
			delete newEntry;
			/*if (OutputBuffer != NULL && OutputBufferLength > 0) {
				swprintf_s((WCHAR*)OutputBuffer, (size_t)OutputBufferLength, L"Failed to add directory \"%ls\"", message->path);
				*ReturnOutputBufferLength = (ULONG)wcsnlen_s((WCHAR*)OutputBuffer, static_cast<size_t>(OutputBufferLength)) + 1;
			}*/
			*((PBOOLEAN)OutputBuffer) = FALSE;
			DbgPrint("Failed to addscan directory\n");
			return STATUS_SUCCESS;
		}
		
	}
	else if (message->type == MESSAGE_REM_SCAN_DIRECTORY) {
		PDIRECTORY_ENTRY ptr = driverData->RemDirectoryEntry(message->path);
		*ReturnOutputBufferLength = 1;
		if (ptr == NULL) {
			/*if (OutputBuffer != NULL && OutputBufferLength > 0) {
				swprintf_s((WCHAR*)OutputBuffer, (size_t)OutputBufferLength, L"Failed to remove directory \"%ls\"", message->path);
				*ReturnOutputBufferLength = (ULONG)wcsnlen_s((WCHAR*)OutputBuffer, static_cast<size_t>(OutputBufferLength)) + 1;
			}*/
			*((PBOOLEAN)OutputBuffer) = FALSE;
			DbgPrint("Failed to remove directory\n");
			return STATUS_SUCCESS;
		}
		else {
			delete ptr;
			/*(if (OutputBuffer != NULL && OutputBufferLength > 0) {
				swprintf_s((WCHAR*)OutputBuffer, (size_t)OutputBufferLength, L"Removed directory \"%ls\"", message->path);
				*ReturnOutputBufferLength = (ULONG)wcsnlen_s((WCHAR*)OutputBuffer, static_cast<size_t>(OutputBufferLength)) + 1;
			}*/
		}
		*((PBOOLEAN)OutputBuffer) = TRUE;
		DbgPrint("Removed scan directory successfully\n");
		return STATUS_SUCCESS;
	}
	else if (message->type == MESSAGE_GET_OPS) {
		if (OutputBuffer == NULL || OutputBufferLength != MAX_COMM_BUFFER_SIZE) {
			return STATUS_INVALID_PARAMETER;
		}
		return driverData->DriverGetIrps(OutputBuffer, OutputBufferLength, ReturnOutputBufferLength);

	}
	else if (message->type == MESSAGE_SET_PID) {
		if (message->pid != 0) {
			driverData->setPID(message->pid);
			return STATUS_SUCCESS;
		}
		return STATUS_INVALID_PARAMETER;
		
	}
	else if (message->type == MESSAGE_KILL_PID) {
		NTSTATUS status = STATUS_SUCCESS;
		HANDLE processHandle;
		ULONG PID = message->pid;

		/* following code may be wrong*/
		CLIENT_ID clientId;
		clientId.UniqueProcess = &PID;
		clientId.UniqueThread = &PID;

		OBJECT_ATTRIBUTES objAttribs;
		NTSTATUS exitStatus = STATUS_FAIL_CHECK;
		
		if (OutputBuffer == NULL || OutputBufferLength != sizeof(LONG)) {
			return STATUS_INVALID_PARAMETER;
		}

		*ReturnOutputBufferLength = sizeof(LONG);

		InitializeObjectAttributes(&objAttribs,
			NULL,
			OBJ_KERNEL_HANDLE,
			NULL,
			NULL);

		status = ZwOpenProcess(&processHandle,
				PROCESS_ALL_ACCESS,
				&objAttribs,
				&clientId);
		if (!NT_SUCCESS(status)) {
			*((PLONG)OutputBuffer) = status; // fail
			return STATUS_SUCCESS; // we dont fail code we notify back a failure
		}
		
		status = ZwTerminateProcess(processHandle, exitStatus);
		if (NT_SUCCESS(status)) {
			*((PLONG)OutputBuffer) = status; // success we can close handle
			status = NtClose(processHandle);
			return status; // we dont fail code we notify back a failure
		}
		DbgPrint("Failed to kill a process\n");
		*((PLONG)OutputBuffer) = status; // fail to kill process
		return STATUS_SUCCESS;
	}

	return STATUS_INTERNAL_ERROR;
}

CommHandler* commHandle;