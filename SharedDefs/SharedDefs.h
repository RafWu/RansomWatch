#pragma once

/*++

Abstract :

Header file which contains the structures, type definitions,
constants, global variables and function prototypes that are
shared between kernel and user mode.

Environment :

	Kernel & user mode

--*/

#include "Common.h"

//
//  Name of port used to communicate
//

const PWSTR ComPortName = L"\\AMFilter";

#define MAX_FILE_NAME_LENGTH 260
#define MAX_FILE_NAME_SIZE (MAX_FILE_NAME_LENGTH * sizeof(WCHAR))
#define FILE_OBJECT_ID_SIZE 16
#define FILE_OBJEC_MAX_EXTENSION_SIZE 11
#define MAX_IRP_OPS_PER_REQUEST 100

enum COM_MESSAGE_TYPE {
	MESSAGE_ADD_SCAN_DIRECTORY,
	MESSAGE_REM_SCAN_DIRECTORY,
	MESSAGE_GET_OPS,
	MESSAGE_SET_PID,
	MESSAGE_KILL_PID // FIXME: add support
	

};

typedef struct _COM_MESSAGE {
	ULONG type;
	ULONG pid;
	WCHAR path[MAX_FILE_NAME_LENGTH];
	
} COM_MESSAGE, *PCOM_MESSAGE;

enum FILE_CHANGE {
	FILE_CHANGE_NOT_SET, 
	FILE_CHANGE_NEW_FILE, 
	FILE_CHANGE_RENAME_FILE, 
	FILE_CHANGE_DELETE_FILE,
	FILE_CHANGE_DELETE_NEW_FILE
};

enum IRP_MAJOR_OP { 
	IRP_NONE, IRP_READ, 
	IRP_WRITE, IRP_SETINFO, 
	IRP_CREATE, 
	IRP_CLEANUP 
};

// -64- bytes structure, fixed to 88 bytes, hold pointer to fileName and its size, used for create and cleanup/close to handle fileName(needed for LZJD)
// FIXME: handle Extension in kernel and application
typedef struct _DRIVER_MESSAGE {
	WCHAR Extension[FILE_OBJEC_MAX_EXTENSION_SIZE + 1]; // null terminated 24 bytes
	UCHAR FileID[FILE_OBJECT_ID_SIZE]; // 16 bytes
	ULONGLONG MemSizeUsed; // for read and write, we follow buffer sizes 8 bytes
	DOUBLE Entropy; // 8 bytes
	ULONG PID;  // 4 bytes
	UCHAR IRP_OP;  // 1 byte
	BOOLEAN isEntropyCalc; // 1 byte
	UCHAR FileChange; // 1 byte
	UCHAR DUMMY; // 1 byte align
	ULONGLONG fileNameSize; // 8 bytes - size of the fileName used
	PWCHAR fileName; // 8 bytes, will be reserved on the buffer if needed
	PVOID next; // 8 bytes - next DRIVER_MESSAGE, we use it to allow adding the fileName to the same buffer, this pointer should point to the next DRIVER_MESSAGE in buffer (kernel handled)
	
} DRIVER_MESSAGE, *PDRIVER_MESSAGE;

/*
typedef struct _AMF_IRP_OP {

} AMF_IRP_OP, PAMF_IRP_OP;
*/

typedef struct _AMF_REPLY_IRPS {
	size_t dataSize; // 8 bytes
	PDRIVER_MESSAGE data; // 8 bytes points to the first IRP driver message, the next DRIVER_MESSAGE is a pointer inside DRIVER_MESSAGE
	ULONGLONG num_ops; // 8 bytes
	
	size_t size() {
		return dataSize + sizeof(AMF_REPLY_IRPS);
	}
	size_t addSize(size_t size) {
		dataSize += size;
		return dataSize;
	}
	ULONGLONG addOp() {
		num_ops++;
		return num_ops;
	}
	ULONGLONG numOps() {
		return num_ops;
	}
	_AMF_REPLY_IRPS() : dataSize(sizeof(_AMF_REPLY_IRPS)), data(nullptr), num_ops(0){

	}
} AMF_REPLY_IRPS, *PAMF_REPLY_IRPS;

constexpr ULONG MAX_COMM_BUFFER_SIZE = sizeof(AMF_REPLY_IRPS) + MAX_IRP_OPS_PER_REQUEST * sizeof(DRIVER_MESSAGE);