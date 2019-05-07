#include "MyForm.h"
#include <Windows.h>

using namespace System;
using namespace System::Windows::Forms;

DWORD
ScannerWorker(
	_In_ PSCANNER_THREAD_CONTEXT Context
)
/*++

Routine Description

	This is a worker thread that


Arguments

	Context  - This thread context has a pointer to the port handle we use to send/receive messages,
				and a completion port handle that was already associated with the comm. port by the caller

Return Value

	HRESULT indicating the status of thread exit.

--*/
{
	PSCANNER_NOTIFICATION notification;
	SCANNER_REPLY_MESSAGE replyMessage;
	PSCANNER_MESSAGE message;
	LPOVERLAPPED pOvlp;
	BOOL result;
	DWORD outSize;
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

		printf("Received message, size %Id\n", pOvlp->InternalHigh);

		notification = &message->Notification;

		assert(notification->BytesToScan <= SCANNER_READ_BUFFER_SIZE);
		_Analysis_assume_(notification->BytesToScan <= SCANNER_READ_BUFFER_SIZE);

		replyMessage.ReplyHeader.Status = 0;
		replyMessage.ReplyHeader.MessageId = message->MessageHeader.MessageId;

		//
		//  Need to invert the boolean -- result is true if found
		//  foul language, in which case SafeToOpen should be set to false.
		//

		printf("PID of the job requested IRP: %d\n", notification->pid);


		replyMessage.Reply.SafeToOpen = true;

		printf("Replying message, SafeToOpen: %d\n", replyMessage.Reply.SafeToOpen);

		hr = FilterReplyMessage(Context->Port,
			(PFILTER_REPLY_HEADER)&replyMessage,
			sizeof(replyMessage));

		if (SUCCEEDED(hr)) {

			printf("Replied message\n");

		}
		else {

			printf("Scanner: Error replying message. Error = 0x%X\n", hr);
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
}

void AntiRansomWareApp::MyForm::initListenThreads() {

}

void AntiRansomWareApp::MyForm::openKernelCommunication() {
	if ((kernelComOpen = openKernelDriverCom())) {
		DBOUT("Kernel com init successfully, init listening threads" << std::endl);
		initListenThreads();
		DBOUT("Init listening thread successfully" << std::endl);
	}
	else {
		DBOUT("Failed to open com port to the kernel driver, working without kernel attachment" << std::endl);
	}
}

void AntiRansomWareApp::MyForm::closeKernelDriverCom() {
	if (!kernelComOpen) { std::cerr << "No com to close" << std::endl; return; }
	// may need to check first kernel driver state, if the driver didnt dissconnect we need to send it a message first
	CloseHandle(port);
	CloseHandle(completion);
}

BOOLEAN AntiRansomWareApp::MyForm::openKernelDriverCom() {
	if (port == nullptr) {
		pin_ptr<HANDLE> portRef = &port;
		HRESULT resOpen = FilterConnectCommunicationPort(SCAN_PORT, 0, nullptr, 0, nullptr, portRef);
		if (FAILED(resOpen)) { //if open failed
			std::cerr << "Failed to open mini filter port to driver, please check settings!" << std::endl;

			_com_error err(resOpen);
			std::cerr << err.ErrorMessage() << std::endl;
			return false;
		}

		completion = CreateIoCompletionPort(port,
			nullptr,
			0,
			NUM_THREADS);
		if (completion == nullptr) {

			std::cerr << "Failed to create completion port, please check settings!" << std::endl;
			DWORD err = GetLastError();
			std::cerr << "Error number: " << err << std::endl;
			CloseHandle(port);
			return false;
		}

	}
	return true;
}


[STAThread]
void Main(array<String^>^ args)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	AntiRansomWareApp::MyForm form;
	Application::Run(%form);
}