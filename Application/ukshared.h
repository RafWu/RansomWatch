#pragma once

#include "Windows.h"
#include <minwinbase.h>
#include <FltUser.h>


#define SCANNER_READ_BUFFER_SIZE   1024


typedef struct _SCANNER_REPLY {

	BOOLEAN SafeToOpen;

} SCANNER_REPLY, *PSCANNER_REPLY;

typedef struct _SCANNER_THREAD_CONTEXT {

	HANDLE Port;
	HANDLE Completion;

} SCANNER_THREAD_CONTEXT, *PSCANNER_THREAD_CONTEXT;


typedef struct _SCANNER_NOTIFICATION {
	ULONG BytesToScan;
	ULONG Reserved;             // for quad-word alignement of the Contents structure
	UCHAR Contents[SCANNER_READ_BUFFER_SIZE];
	ULONG pid;

} SCANNER_NOTIFICATION, *PSCANNER_NOTIFICATION;

#pragma pack(push, 1) // exact fit - no padding

typedef struct _SCANNER_MESSAGE {

	//
	//  Required structure header.
	//

	FILTER_MESSAGE_HEADER MessageHeader;


	//
	//  Private scanner-specific fields begin here.
	//

	SCANNER_NOTIFICATION Notification;

	//
	//  Overlapped structure: this is not really part of the message
	//  However we embed it instead of using a separately allocated overlap structure
	//

	OVERLAPPED Ovlp;

} SCANNER_MESSAGE, *PSCANNER_MESSAGE;

typedef struct _SCANNER_REPLY_MESSAGE {

	//
	//  Required structure header.
	//

	FILTER_REPLY_HEADER ReplyHeader;

	//
	//  Private scanner-specific fields begin here.
	//

	SCANNER_REPLY Reply;

} SCANNER_REPLY_MESSAGE, *PSCANNER_REPLY_MESSAGE;

#pragma pack(pop)