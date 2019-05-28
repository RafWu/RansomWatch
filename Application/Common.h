#pragma once

#include <sstream>
#include <windows.h>
#include <minwinbase.h>
#include <FltUser.h>
#include "../SharedDefs/SharedDefs.h"

#define NUM_THREADS 2

#define DBOUT( s )            \
{                             \
   std::wstringstream os_;    \
   os_ << s;                   \
   OutputDebugString( os_.str().c_str() );  \
}

/*
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
*/

typedef struct _SCANNER_THREAD_CONTEXT {

	HANDLE Port;
	//HANDLE Completion;

} SCANNER_THREAD_CONTEXT, *PSCANNER_THREAD_CONTEXT;