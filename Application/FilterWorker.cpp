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
			Globals::Instance->postLogMessage(String::Concat("Failed irp request, stopping", numOps, System::Environment::NewLine), PRIORITY_PRINT);
			Globals::Instance->setCommCloseStat(TRUE);
			break;
		}

		if (ReplySize == 0 || ReplySize <= sizeof(AMF_REPLY_IRPS)) {
			Globals::Instance->postLogMessage(String::Concat("No ops to report, waiting", numOps, System::Environment::NewLine), VERBOSE_ONLY);
			Sleep(100);
			continue;
		}
		PAMF_REPLY_IRPS ReplyMsgs = (PAMF_REPLY_IRPS)Buffer;
		PDRIVER_MESSAGE pMsgIrp = ReplyMsgs->data; // get first irp if any
		numOps = ReplyMsgs->numOps();
		if (numOps == 0 || pMsgIrp == nullptr) {
			Globals::Instance->postLogMessage(String::Concat("No ops to report, waiting", numOps, System::Environment::NewLine), VERBOSE_ONLY);
			Sleep(100);
			continue;
		}
		// FIXME: compare memory size, replySize (minus AMP_REPLY_IRPS) with numOps and size of DRIVER_MESSAGE, assert numOps <= 10, log are fail thread accordingly
		// FIXME : assert ReplyMsgs->data points to Buffer + sizeof(AMF_REPLY_IRPS)
		Globals::Instance->postLogMessage(String::Concat("Received num ops: ", numOps, System::Environment::NewLine), VERBOSE_ONLY);
		while (pMsgIrp != nullptr) 
		{
			hr = ProcessIrp(*pMsgIrp);
			if (hr != S_OK) {
				Globals::Instance->postLogMessage(String::Concat("Failed to handle irp msg", System::Environment::NewLine), VERBOSE_ONLY);
			}
			pidsCheck.insert(pMsgIrp->PID);
			if (Globals::Instance->Verbose()) {
				if (pMsgIrp->filePath.Length) {
					std::wstring fileNameStr(pMsgIrp->filePath.Buffer, pMsgIrp->filePath.Length / 2);
					Globals::Instance->postLogMessage(String::Concat("Received irp on file: ", gcnew String(fileNameStr.c_str()), System::Environment::NewLine), VERBOSE_ONLY);
				}
				else {
					Globals::Instance->postLogMessage(String::Concat("Received irp with file len 0", System::Environment::NewLine), VERBOSE_ONLY);
				}
			}
			pMsgIrp = (PDRIVER_MESSAGE)pMsgIrp->next;
		}

		// log 

		TotalIrpCount += numOps;
		Globals::Instance->addIrpHandled(numOps);

		// check Malicious, handle in that case
		for (ULONG pid : pidsCheck) {
			CheckHandleMaliciousApplication(pid, Port);
		}

		Globals::Instance->postLogMessage(String::Concat("... Finished handling irp requests, requesting", System::Environment::NewLine), VERBOSE_ONLY);
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
		DWORD failReason = 0; 
		BOOLEAN isTerminateSuc;
		DWORD isKilled;
		Globals::Instance->postLogMessage(String::Concat("Handling malicious application: ", appName, " with pid: ", pidStr, System::Environment::NewLine), PRIORITY_PRINT);
		if (Globals::Instance->getKillStat()) {
			Globals::Instance->postLogMessage(String::Concat("Attempt to kill process using application", System::Environment::NewLine), PRIORITY_PRINT);
			Monitor::Enter(record);
			if (record->isMalicious() && !record->isKilled())
			{
				HANDLE pHandle = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, false, pid);
				if (pHandle != nullptr)
				{
					isTerminateSuc = TerminateProcess(pHandle, 1); // exit with failure
					if (isTerminateSuc)
					{
						isKilled = WaitForSingleObject(pHandle, 100); // 
						if (isKilled != WAIT_TIMEOUT) {
							record->setKilled();
							CloseHandle(pHandle);
							Monitor::Exit(record);
							Globals::Instance->postLogMessage(String::Concat("Killed malicious process: ", appName, " with pid: ", pidStr, System::Environment::NewLine), PRIORITY_PRINT);
							return;
						}
					}
					// reach here only if kill failed
					CloseHandle(pHandle);
				}
				Monitor::Exit(record);
				String^ failedOn;
				failReason = GetLastError();
				if (pHandle == nullptr) {
					failedOn = gcnew String(" Failed on Open process");
				}
				else if (isTerminateSuc){
					failedOn = isKilled.ToString();
				}
				else {
					failedOn = gcnew String(" Failed Terminate process");
				}
				Globals::Instance->postLogMessage(String::Concat("Failed to kill process using application, using driver to kill. error code: ", failReason.ToString(), failedOn, System::Environment::NewLine), PRIORITY_PRINT);
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
						Globals::Instance->postLogMessage(String::Concat("Failed to kill process using driver, pid: ", pidStr, System::Environment::NewLine), PRIORITY_PRINT);
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
			Globals::Instance->postLogMessage(String::Concat("Auto kill disabled, reporting process: ", appName, "with pid: ", pidStr, System::Environment::NewLine), PRIORITY_PRINT);
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
			Globals::Instance->postLogMessage(String::Concat("Found malicious application", System::Environment::NewLine), PRIORITY_PRINT);
			Generic::SortedSet<String^>^ triggersDetected = record->GetTriggersBreached();
			String^ msg = gcnew String("Breached triggers: ");
			for each (String ^ trigger in triggersDetected) {
				msg = String::Concat(msg, trigger, " ");
			}
			msg = String::Concat(msg, System::Environment::NewLine);
			Globals::Instance->postLogMessage(msg, PRIORITY_PRINT);

			HandleMaliciousApplication(record, comPort);

			Generic::SortedSet<String^>^ createdFiles = record->GetCreatedFiles();
			Generic::SortedSet<String^>^ changedFiles = record->GetChangedFiles();
			String^ reportFile = String::Concat("C:\\Report", record->Pid().ToString(), ".log");
			try {
				StreamWriter^ sw = gcnew StreamWriter(reportFile);
				sw->WriteLine("RansomWatch report file");
				sw->Write("Files report for ransomware running from exe: ");
				sw->WriteLine(record->Name());
				sw->Write("Process started on time: ");
				sw->WriteLine(record->Date());
				sw->Write("Time report: ");
				sw->WriteLine(DateTime::Now);
				Monitor::Enter(record);
				if (record->isKilled()) {
					sw->WriteLine("Process has been killed");
				}
				Monitor::Exit(record);
				sw->Write("Changed files: ");
				int changedSize = changedFiles->Count;
				sw->WriteLine(changedSize.ToString());
				for each (String ^ filePath in changedFiles) {
					sw->WriteLine(filePath);
				}
				sw->Write("Created files: ");
				int createdSize = createdFiles->Count;
				sw->WriteLine(createdSize.ToString());
				for each (String ^ filePath in createdFiles) {
					sw->WriteLine(filePath);
				}
				sw->WriteLine("End file");
				sw->Close();
				Globals::Instance->postLogMessage(String::Concat("Created report file for ransomware: ", reportFile, System::Environment::NewLine), PRIORITY_PRINT);
			}
			catch (...) {
				Globals::Instance->postLogMessage(String::Concat("Failed to create report file", System::Environment::NewLine), PRIORITY_PRINT);
			}

		}
	}
}
