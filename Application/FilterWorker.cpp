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
			//FIXME: Globals::Instance->getTextBox()->AppendText("...........Failed............");

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

		//Globals::Instance->getTextBox()->AppendText("........Recieved an irp request.........\r\n");
		//Globals::Instance->getTextBox()->Invoke(gcnew Action<String^> (Globals::Instance->getTextBox()), gcnew array<Object^> { "........Recieved an irp request.........\r\n" }) ;
		Globals::Instance->postLogMessage("........Recieved an irp request.........\r\n");
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
