#include "MyForm.h"
#include <Windows.h>

using namespace System;
using namespace System::Windows::Forms;

//globalSettings globals;

//SCANNER_THREAD_CONTEXT context;

HRESULT AntiRansomWareApp::MyForm::initWorkThread() {
	DWORD numOfThreads = NUM_THREADS;
	//P msg;
	HRESULT hr = S_OK;
	DWORD threadId;
	HANDLE threads[NUM_THREADS];

	for (DWORD i = 0; i < numOfThreads; i++) {
		threads[i] = CreateThread(NULL,
			0,
			(LPTHREAD_START_ROUTINE)FilterWorker,
			&context,
			0,
			&threadId);

		if (threads[i] == NULL) {

			//
			//  Couldn't create thread.
			//

			hr = GetLastError();
			printf("ERROR: Couldn't create thread: %d\n", hr);
			return hr;
			// raise error
			//goto threadsCleanup;
		}
	}
	return hr;
}

void AntiRansomWareApp::MyForm::openKernelCommunication() {
	BOOLEAN openComDriver = openKernelDriverCom(); // if true com is open else false
	//Globals::Instance->setCommCloseStat(!openComDriver);
	if (openComDriver) {
		HRESULT res = S_OK;
		//Globals::Instance->setCommCloseStat(FALSE);

		DBOUT("Kernel com init successfully, setting app pid to driver" << std::endl);
		COM_MESSAGE setPidMsg;
		setPidMsg.type = MESSAGE_SET_PID;
		setPidMsg.pid = GetCurrentProcessId();
		setPidMsg.path[0] = L'\0';
		HRESULT hr = FilterSendMessage(context.Port, &setPidMsg, sizeof(COM_MESSAGE), NULL, 0, NULL);
		if (FAILED(hr)) {
			DBOUT("Failed to send set pid message, cancel kernel comm" << std::endl);
			closeKernelDriverCom();
			return;
		}

		DBOUT("Kernel com init successfully, init listening threads" << std::endl);
		if ((res = initWorkThread()) == S_OK) { // success
			DBOUT("Init listening thread successfully" << std::endl);
		}
		else {
			DBOUT("Failed to create worker thread" << std::endl);
			// FIXME: add exception, notify, close program
		}
		
	}
	else {
		DBOUT("Failed to open com port to the kernel driver, working without kernel attachment" << std::endl);
		// FIXME: add exception, notify, close program
	}
}

void AntiRansomWareApp::MyForm::closeKernelDriverCom() {
	if (Globals::Instance->getCommCloseStat()) { std::cerr << "No com to close" << std::endl; return; }
	// may need to check first kernel driver state, if the driver didnt dissconnect we need to send it a message first
	Globals::Instance->setCommCloseStat(TRUE);
	CloseHandle(context.Port);

	//CloseHandle(context.Completion);
}

HRESULT AntiRansomWareApp::MyForm::RemoveFilterDirectory(String ^ directory)
{
	if (Globals::Instance->getCommCloseStat()) { // no comm
		return S_FALSE;
	}
	else if (directory == nullptr) {
		return S_FALSE;
	}

	COM_MESSAGE setDirMsg;
	BOOLEAN retOp = FALSE;
	DWORD retSize;
	setDirMsg.type = MESSAGE_REM_SCAN_DIRECTORY;
	setDirMsg.pid = GetCurrentProcessId();

	pin_ptr<const wchar_t> wch = PtrToStringChars(directory);
	std::wstring Path(wch);
	wcsncpy_s(setDirMsg.path, MAX_FILE_NAME_LENGTH, Path.c_str(), MAX_FILE_NAME_LENGTH);
	HRESULT hr = FilterSendMessage(context.Port, &setDirMsg, sizeof(COM_MESSAGE), &retOp, 1, &retSize);
	if (FAILED(hr)) {
		DBOUT("Failed to send rem dir message" << std::endl);
		Globals::Instance->setCommCloseStat(TRUE);
		return S_FALSE;
	}
	else if (retSize == 1 && retOp == TRUE) { //reply op
		return S_OK;
	}

	return S_FALSE;
}


HRESULT AntiRansomWareApp::MyForm::AddFilterDirectory(String ^ directory)
{
	if (Globals::Instance->getCommCloseStat() || context.Port == nullptr) { // no comm
		return S_FALSE;
	}
	else if (directory == nullptr) {
		return S_FALSE;
	}
	//DBOUT("Kernel com init successfully, setting app pid to driver" << std::endl);
	COM_MESSAGE setDirMsg;
	BOOLEAN retOp = FALSE;
	DWORD retSize;
	setDirMsg.type = MESSAGE_ADD_SCAN_DIRECTORY;
	setDirMsg.pid = GetCurrentProcessId();

	pin_ptr<const wchar_t> wch = PtrToStringChars(directory);
	std::wstring Path(wch);
	wcsncpy_s(setDirMsg.path, MAX_FILE_NAME_LENGTH, Path.c_str(), MAX_FILE_NAME_LENGTH);

	HRESULT hr = FilterSendMessage(context.Port, &setDirMsg, sizeof(COM_MESSAGE), &retOp, 1, &retSize);
	if (FAILED(hr)) {
		DBOUT("Failed to send add pid message" << std::endl);
		Globals::Instance->setCommCloseStat(TRUE);
		return S_FALSE;
	}
	else if (retSize == 1 && retOp == TRUE) { //reply op
		return S_OK;
	}

	return S_FALSE;
}



BOOLEAN AntiRansomWareApp::MyForm::openKernelDriverCom() {
	if (context.Port == nullptr) {
		HRESULT resOpen = FilterConnectCommunicationPort(ComPortName, 0, nullptr, 0, nullptr, &context.Port);
		if (FAILED(resOpen)) { //if open failed
			std::cerr << "Failed to open mini filter port to driver, please check settings!" << std::endl;

			_com_error err(resOpen);
			std::cerr << err.ErrorMessage() << std::endl;
			return FALSE;
		}
		Globals::Instance->setCommCloseStat(FALSE);

		/*
		context.Completion = CreateIoCompletionPort(context.Port,
			nullptr,
			0,
			NUM_THREADS);
		if (context.Completion == nullptr) {

			std::cerr << "Failed to create completion port, please check settings!" << std::endl;
			DWORD err = GetLastError();
			std::cerr << "Error number: " << err << std::endl;
			CloseHandle(context.Port);
			return false;
		}*/

	}
	return TRUE;
}

[STAThread]
void Main(array<String^>^ args)
{

	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	AntiRansomWareApp::MyForm^ form = gcnew AntiRansomWareApp::MyForm();
	Application::Run(form);
}