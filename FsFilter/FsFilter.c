/*++

Module Name:

    FsFilter.c

Abstract:

    This is the main module of the FsFilter miniFilter driver.

Environment:

    Kernel mode

--*/

#include "FsFilter.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")
#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

//PFLT_FILTER FilterHandle = NULL;

NTSTATUS FsFilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags);
FLT_POSTOP_CALLBACK_STATUS MiniPostCreateFile(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags);
FLT_PREOP_CALLBACK_STATUS MiniPreCreateFile(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);
FLT_PREOP_CALLBACK_STATUS MiniPreWriteFile(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
	{IRP_MJ_CREATE, 0, MiniPreCreateFile, MiniPostCreateFile}, //create file requests
	{IRP_MJ_WRITE, 0, MiniPreWriteFile, NULL}, // monitor write operations 
	{ IRP_MJ_OPERATION_END }
};

/*++

FilterRegistration Defines what we want to filter with the driver

--*/
CONST FLT_REGISTRATION FilterRegistration = {                          
	sizeof(FLT_REGISTRATION),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags
	ContextRegistration,                //  Context Registration.
	Callbacks,                          //  Operation callbacks
	FsFilterUnload,                      //  FilterUnload
	FsInstanceSetup,               //  InstanceSetup
	FsQueryTeardown,               //  InstanceQueryTeardown
	NULL,                               //  InstanceTeardownStart
	NULL,                               //  InstanceTeardownComplete
	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent
};

FLT_POSTOP_CALLBACK_STATUS MiniPostCreateFile(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);


	//KdPrint(("post create is running \r\n"));
	return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS MiniPreCreateFile(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	PFLT_FILE_NAME_INFORMATION FileNameInfo;
	NTSTATUS status;
	const int maxLenData = 1024;
	WCHAR Name[1024] = { 0 }; // buffer for file name
	WCHAR dirPath[1024] = { 0 };

	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo); //get file name
	
	if (NT_SUCCESS(status)) {
		
		status = FltParseFileNameInformation(FileNameInfo); // parse name info, data will be in FileNameInfo
		
		if (NT_SUCCESS(status)) {

			if (FileNameInfo->Name.MaximumLength > 120) {
				int copyLen = FileNameInfo->Name.MaximumLength;
				if (copyLen >= maxLenData) copyLen = maxLenData - 1;
				
				RtlCopyMemory(Name, FileNameInfo->Name.Buffer, copyLen);
				RtlCopyMemory(dirPath, FileNameInfo->ParentDir.Buffer, FileNameInfo->ParentDir.MaximumLength);
				
				KdPrint(("Create file: %ws, in path %ws \r\n", Name, dirPath));

			}
		}

		FltReleaseFileNameInformation(FileNameInfo);
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK; //indicate we have post callback
}

FLT_PREOP_CALLBACK_STATUS MiniPreWriteFile(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	PFLT_FILE_NAME_INFORMATION FileNameInfo;
	NTSTATUS status;
	const int maxLenData = 1024;
	WCHAR Name[1024] = { 0 }; // buffer for file name

	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo); //get file name

	if (NT_SUCCESS(status)) {

		status = FltParseFileNameInformation(FileNameInfo); // parse name info, data will be in FileNameInfo

		if (NT_SUCCESS(status)) {

			if (FileNameInfo->Name.MaximumLength > 10) {
				int copyLen = FileNameInfo->Name.MaximumLength;
				if (copyLen >= maxLenData) copyLen = maxLenData - 1;
				
				RtlCopyMemory(Name, FileNameInfo->Name.Buffer, copyLen);
				_wcsupr(Name); //upper case string

				if (wcsstr(Name, L"TEXT.TXT") != NULL) {
					KdPrint(("Write to file: %ws blocked \r\n", Name));
					
					//set error on IO operation
					Data->IoStatus.Status = STATUS_INVALID_PARAMETER;
					FltReleaseFileNameInformation(FileNameInfo);
					Data->IoStatus.Information = 0;
					return FLT_PREOP_COMPLETE; //prevent passing to next filter

				}


				KdPrint(("writing to file: %ws allowed\r\n", Name));

			}
		}

		FltReleaseFileNameInformation(FileNameInfo);
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK; //indicate we dont have post callback
}

/*++

Routine Description:

	This is the unload routine for this miniFilter driver. This is called
	when the minifilter is about to be unloaded. We can fail this unload
	request if this is not a mandatory unload indicated by the Flags
	parameter.

Arguments:

	Flags - Indicating if this is a mandatory unload.

Return Value:

	Returns STATUS_SUCCESS.

--*/
NTSTATUS FsFilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Flags);
	KdPrint(("Acamol bye bye \r\n"));
	FltCloseCommunicationPort(globals.comServerPort);
	globals.comServerPort = NULL;
	FltUnregisterFilter(globals.Filter);
	KdPrint(("Acamol bye bye \r\n"));
	return STATUS_SUCCESS;
}

/*++

Routine Description:

	This is the initialization routine for this miniFilter driver.  This
	registers with FltMgr and initializes all global data structures.

Arguments:

	DriverObject - Pointer to driver object created by the system to
		represent this driver.

	RegistryPath - Unicode string identifying where the parameters for this
		driver are located in the registry.

Return Value:

	Routine can return non success error codes.

--*/
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) 
{
	{
		NTSTATUS status; // Driver status
		PSECURITY_DESCRIPTOR sd = NULL;

		status = FltRegisterFilter(DriverObject, &FilterRegistration, &globals.Filter); //  Register with FltMgr to tell it our callback routines

		UNREFERENCED_PARAMETER(RegistryPath);

		KdPrint(("Acamol filter DriverEntry entered\r\n"));

		if (NT_SUCCESS(status)) {

			// add security port for user mode apps
			status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
			if (!NT_SUCCESS(status)) {

				FltUnregisterFilter(globals.Filter); // fail to start - unregister
				globals.Filter = NULL;
			}
			
			//open com port
			status = FsPreparePort(sd);
			if (!NT_SUCCESS(status)) {
				FltUnregisterFilter(globals.Filter); // fail to start - unregister
				globals.Filter = NULL;
			}
			
			if (sd != NULL) {
				FltFreeSecurityDescriptor(sd);
			}

			//  Start filtering i/o
			status = FltStartFiltering(globals.Filter); // start Driver filtering
			if (NT_SUCCESS(status)) {
				return status;
			}
			FltCloseCommunicationPort(globals.comServerPort);
			FltUnregisterFilter(globals.Filter); // fail to start - unregister
			globals.Filter = NULL;
			globals.comServerPort = NULL;
		}

		return status;
	}
}