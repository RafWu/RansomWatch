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
		std::set<ULONG> pidsCheck;
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
		Globals::Instance->postLogMessage(String::Concat("Received num ops: ", numOps, System::Environment::NewLine));
		for (USHORT i = 0; i < numOps; i++) {
			Globals::Instance->postLogMessage(String::Concat("Received irp: ", MsgsBuffer[i].IRP_OP, " from pid: ", MsgsBuffer[i].PID, System::Environment::NewLine));
			hr = ProcessIrp(MsgsBuffer[i]);
			if (hr != S_OK) {
				// log
				break;
			}
			pidsCheck.insert(MsgsBuffer[i].PID);
		}

		// log 

		TotalIrpCount += numOps;
		Globals::Instance->addIrpHandled(numOps);

		// check Malicious, handle in that case
		for (ULONG pid : pidsCheck) {
			CheckHandleMaliciousApplication(pid, Port);
		}

		Globals::Instance->postLogMessage(String::Concat("... Finished handling irp requests, requesting", System::Environment::NewLine));
	}
	delete[] Buffer;
	return hr;
}

HRESULT ProcessIrp(CONST DRIVER_MESSAGE& msg) {
	ULONG IrpPid = msg.PID;
	ProcessRecord^ record;
	if (ProcessesMemory::Instance->Processes->TryGetValue(IrpPid, record)) { // val found
		if (record == nullptr) { // found pid but no process record/ shouldnt happen
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
		else { // cant add but cant get either we fail here
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



VOID HandleMaliciousApplication(ProcessRecord^ record, HANDLE comPort) {
	if (record != nullptr) {
		ULONG pid = record->Pid();
		String^ pidStr = pid.ToString();
		String^ appName = record->Name();
		
		Globals::Instance->postLogMessage(String::Concat("Handling malicious application: ", appName, " with pid: ", pidStr, System::Environment::NewLine));
		if (Globals::Instance->getKillStat()) {
			Globals::Instance->postLogMessage(String::Concat("Attempt to kill process using application", System::Environment::NewLine));
			Monitor::Enter(record);
			if (record->isMalicious() && !record->isKilled())
			{
				HANDLE pHandle = OpenProcess(PROCESS_TERMINATE, false, pid);
				if (pHandle != nullptr)
				{
					BOOLEAN isTerminateSuc = TerminateProcess(pHandle, 1); // exit with failure
					if (!isTerminateSuc)
					{
						DWORD isKilled = WaitForSingleObject(pHandle, 100); // 
						if (isKilled != WAIT_TIMEOUT && isKilled != WAIT_FAILED) {
							record->setKilled();
							CloseHandle(pHandle);
							Monitor::Exit(record);
							Globals::Instance->postLogMessage(String::Concat("Killed malicious process: ", appName, " with pid: ", pidStr, System::Environment::NewLine));
							return;
						}
					}
					CloseHandle(pHandle);
				}
				Monitor::Exit(record);
				Globals::Instance->postLogMessage(String::Concat("Failed to kill process using application, using driver to kill",System::Environment::NewLine));
				COM_MESSAGE killPidMsg;
				NTSTATUS retOp = S_OK;
				DWORD retSize;
				killPidMsg.type = MESSAGE_KILL_PID;
				killPidMsg.pid = pid;
				Monitor::Enter(record);
				if (!record->isKilled()) {
					HRESULT hr = FilterSendMessage(comPort, &killPidMsg, sizeof(COM_MESSAGE), &retOp, sizeof(NTSTATUS), &retSize);
					if (SUCCEEDED(hr) && retOp == S_OK) {
						record->setKilled();
					}
					Monitor::Exit(record);
					if (FAILED(hr)) {
						DBOUT("Failed to send kill pid message" << std::endl);
						Globals::Instance->setCommCloseStat(TRUE);
					}

					if (FAILED(hr) || retOp != S_OK) {
						Globals::Instance->postLogMessage(String::Concat("Failed to kill process using driver, pid: ", pidStr, System::Environment::NewLine));
					}
				}
				else {
					Monitor::Exit(record);
					DBOUT("Process already killed" << std::endl);
				}
			}
			else {
				Monitor::Exit(record);
				DBOUT("Process not malicious or already handled" << std::endl);
			}
		}
		else {
			Globals::Instance->postLogMessage(String::Concat("Auto kill disabled, reporting process: ", appName, "with pid: ", pidStr, System::Environment::NewLine));
			pin_ptr<const wchar_t> content = PtrToStringChars(appName);
			std::wstring result(content, appName->Length);
			std::wstring pidStrStd = std::to_wstring(pid);
			std::wstring msg = result + L" malicious with pid: " + pidStrStd;
			MessageBox(NULL, L"Malicious Application", msg.c_str(), MB_OK);
		}
		
	}
}

VOID CheckHandleMaliciousApplication(ULONG pid, HANDLE comPort) {
	ProcessRecord^ record = nullptr;
	if (ProcessesMemory::Instance->Processes->TryGetValue(pid, record) && record != nullptr) {
		if (record->isSafe()) {
			DBOUT("Safe process skipping malicious check" << std::endl);
			return;
		}
		if (record->isProcessMalicious()) {
			Globals::Instance->postLogMessage(String::Concat("Found malicious application", System::Environment::NewLine));
			Generic::SortedSet<String^>^ triggersDetected = record->GetTriggersBreached();
			String^ msg = gcnew String("Breached triggers: ");
			for each (String ^ trigger in triggersDetected) {
				msg = String::Concat(msg, trigger, " ");
			}
			msg = String::Concat(msg, System::Environment::NewLine);
			Globals::Instance->postLogMessage(msg);
			HandleMaliciousApplication(record, comPort);
		}
	}
}
