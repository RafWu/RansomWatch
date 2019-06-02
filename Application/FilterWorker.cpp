#include "FilterWorker.h"

DWORD
FilterWorker(
	_In_ PSCANNER_THREAD_CONTEXT Context
) 
/*++

Routine Description

	This is a worker thread that get kernel irps, the worker thread issue FilterGetMessage to get last irps


Arguments

	Context  - This thread context has a pointer to the port handle we use to send/receive messages,

Return Value

	HRESULT indicating the status of thread exit.

--*/
{
	HRESULT hr = 0;
	HANDLE Port = Context->Port;
	ULONG IrpCount = 0;
	ULONGLONG TotalIrpCount = 0;
	/*create buffer*/
	CONST DWORD BufferSize = MAX_COMM_BUFFER_SIZE;
	PBYTE Buffer = new BYTE[BufferSize]; // prepare space for message header reply and 10 messages
	COM_MESSAGE GetIrpMsg;
	GetIrpMsg.type = MESSAGE_GET_OPS;
	GetIrpMsg.pid = GetCurrentProcessId();
	GetIrpMsg.path[0] = L'\0';

	while (!Globals::Instance->getCommCloseStat()) { // while communication open
		DWORD ReplySize;
		ULONGLONG numOps = 0;
		hr = FilterSendMessage(Port, &GetIrpMsg, sizeof(COM_MESSAGE), Buffer, BufferSize, &ReplySize);
		if (FAILED(hr)) {
			// log to textbox
			Globals::Instance->getTextBox()->AppendText(".....................................Failed............");
			Globals::Instance->setCommCloseStat(TRUE);
			break;
		}

		if (ReplySize == 0 || ReplySize <= sizeof(AMF_REPLY_IRPS)) {
			//log
			Sleep(100);
			continue;
		}
		PAMF_REPLY_IRPS ReplyMsgs = (PAMF_REPLY_IRPS)Buffer;

		numOps = ReplyMsgs->numOps();
		PDRIVER_MESSAGE MsgsBuffer = ReplyMsgs->data; // we handle like array , all msgs same in size, between 0 to 10 msgs
		// FIXME: compare memory size, replySize (minus AMP_REPLY_IRPS) with numOps and size of DRIVER_MESSAGE, assert numOps <= 10, log are fail thread accordingly
		// FIXME : assert ReplyMsgs->data points to Buffer + sizeof(AMF_REPLY_IRPS)

		for (USHORT i = 0; i < numOps; i++) {
			hr = ProcessIrp(MsgsBuffer[i]);
			if (hr != S_OK) {
				// log
				break;
			}
		}

		// log 

		TotalIrpCount += numOps;
		Globals::Instance->addIrpHandled(numOps);
		Globals::Instance->getTextBox()->AppendText("..........................Recieved an irp request.................");
	}
	delete[] Buffer;
	return hr;
}

HRESULT ProcessIrp(CONST DRIVER_MESSAGE& msg) {
	ULONG IrpPid = msg.PID;
	ProcessRecord^ record;
	if (ProcessesMemory::Instance->Processes->TryGetValue(IrpPid, record)) { // val found
		if (record == nullptr) {
			return S_FALSE;
		}
		record->AddIrpRecord(msg);
		return S_OK;
	}
	record = gcnew ProcessRecord(IrpPid);
	if (!ProcessesMemory::Instance->Processes->TryAdd(IrpPid, record)) { // add failed
		if (ProcessesMemory::Instance->Processes->TryGetValue(IrpPid, record)) { // val found
			record->AddIrpRecord(msg);
			return S_OK;
		}
		else { // cant add but cant get eitherwe fail here
			if (record == nullptr) {
				// log
				return S_FALSE;
			}
		}

	}
	record->AddIrpRecord(msg);

	// log
	return S_OK;
}

BOOLEAN IsMaliciousApplication(ULONG PID) {
	return TRUE;
}



/*
DWORD
ScannerWorker(
	_In_ PSCANNER_THREAD_CONTEXT Context
)*/
/*++

Routine Description

	This is a worker thread that


Arguments

	Context  - This thread context has a pointer to the port handle we use to send/receive messages,
				and a completion port handle that was already associated with the comm. port by the caller

Return Value

	HRESULT indicating the status of thread exit.

--*//*
{
	PSCANNER_NOTIFICATION notification;
	SCANNER_REPLY_MESSAGE replyMessage;
	PSCANNER_MESSAGE message;
	LPOVERLAPPED pOvlp;
	BOOL result;
	DWORD outSize;
	DWORD pid;
	UCHAR FileId[16];
	ULONG BytesFilePath;
	UCHAR Path[MAX_FILE_NAME_LENGTH];
	ULONG Entropy;
	ULONG IRP_OP = IRP_NONE;
	BOOLEAN validOp = true;

	HRESULT hr;
	ULONG_PTR key;

#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant

	while (TRUE) {

#pragma warning(pop)

		//
		//  Poll for messages from the filter component to scan.
		//

		result = GetQueuedCompletionStatus(Context->Completion, &outSize, &key, &pOvlp, INFINITE);

		//
		//  Obtain the message: note that the message we sent down via FltGetMessage() may NOT be
		//  the one dequeued off the completion queue: this is solely because there are multiple
		//  threads per single port handle. Any of the FilterGetMessage() issued messages can be
		//  completed in random order - and we will just dequeue a random one.
		//

		message = CONTAINING_RECORD(pOvlp, SCANNER_MESSAGE, Ovlp);

		if (!result) {

			//
			//  An error occured.
			//

			hr = HRESULT_FROM_WIN32(GetLastError());
			break;
		}

		DBOUT("Received message, size " << pOvlp->InternalHigh << std::endl);

		notification = &message->Notification;

		replyMessage.ReplyHeader.Status = 0;
		replyMessage.ReplyHeader.MessageId = message->MessageHeader.MessageId;

		//
		//  Need to invert the boolean -- result is true if found
		//  foul language, in which case SafeToOpen should be set to false.
		//
		pid = notification->PID;
		IRP_OP = notification->IRP_OP; // get the irp op done
		// TODO
		//validOp = processIrpRequest(notification);

		DBOUT("PID of the job requested IRP: " << pid << std::endl);
		
		ApplicationsMemory^ appsMem = ApplicationsMemory::Instance;
		if (appsMem->applications->ContainsKey(pid)) { //try to locate record using mapping pidtoid

		}
		else { //create record for application
			
		}


		replyMessage.Reply.validOp = validOp;

		DBOUT("Replying message, validOp: " << replyMessage.Reply.validOp << std::endl);

		hr = FilterReplyMessage(Context->Port,
			(PFILTER_REPLY_HEADER)&replyMessage,
			sizeof(replyMessage));

		if (SUCCEEDED(hr)) {

			DBOUT("Replied message" << std::endl);

		}
		else {

			DBOUT("Scanner: Error replying message. Error = " << hr << std::endl);
			break;
		}

		memset(&message->Ovlp, 0, sizeof(OVERLAPPED));

		hr = FilterGetMessage(Context->Port,
			&message->MessageHeader,
			FIELD_OFFSET(SCANNER_MESSAGE, Ovlp),
			&message->Ovlp);

		if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {

			break;
		}
	}

	if (!SUCCEEDED(hr)) {

		if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)) {

			//
			//  Scanner port disconncted.
			//
			OutputDebugString(L"Scanner: Port is disconnected, probably due to scanner filter unloading.\n");
		}
		else {

			OutputDebugString(L"Scanner: Unknown error occured.");
		}
	}

	free(message);

	return hr;
}*/