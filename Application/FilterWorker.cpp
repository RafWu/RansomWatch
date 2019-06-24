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
	GetIrpMsg.gid = 0;
	
	// FIXME
	while (!Globals::Instance->getCommCloseStat()) { // while communication open
		std::set<ULONGLONG> gidsCheck;
		DWORD ReplySize;
		ULONGLONG numOps = 0;
		hr = FilterSendMessage(Port, &GetIrpMsg, sizeof(COM_MESSAGE), Buffer, BufferSize, &ReplySize);
		if (FAILED(hr)) {
			Globals::Instance->postLogMessage(String::Concat("<V> Failed irp request, stopping", System::Environment::NewLine), PRIORITY_PRINT);
			Globals::Instance->setCommCloseStat(TRUE);
			break;
		}

		if (ReplySize == 0 || ReplySize <= sizeof(RWD_REPLY_IRPS)) {
			Globals::Instance->postLogMessage(String::Concat("<V> No ops to report, waiting", System::Environment::NewLine), VERBOSE_ONLY);
			Sleep(100);
			continue;
		}
		PRWD_REPLY_IRPS ReplyMsgs = (PRWD_REPLY_IRPS)Buffer;
		PDRIVER_MESSAGE pMsgIrp = ReplyMsgs->data; // get first irp if any
		numOps = ReplyMsgs->numOps();
		if (numOps == 0 || pMsgIrp == nullptr) {
			Globals::Instance->postLogMessage(String::Concat("<V> No ops to report, waiting", System::Environment::NewLine), VERBOSE_ONLY);
			Sleep(100);
			continue;
		}
		// FIXME: compare memory size, replySize (minus AMP_REPLY_IRPS) with numOps and size of DRIVER_MESSAGE, assert numOps <= 10, log are fail thread accordingly
		// FIXME : assert ReplyMsgs->data points to Buffer + sizeof(AMF_REPLY_IRPS)
		Globals::Instance->postLogMessage(String::Concat("<V> Received num ops: ", numOps, System::Environment::NewLine), VERBOSE_ONLY);
		while (pMsgIrp != nullptr) 
		{
			hr = ProcessIrp(*pMsgIrp);
			if (hr != S_OK) {
				Globals::Instance->postLogMessage(String::Concat("<V> Failed to handle irp msg", System::Environment::NewLine), VERBOSE_ONLY);
			}
			gidsCheck.insert(pMsgIrp->Gid);
			if (Globals::Instance->Verbose()) {
				if (pMsgIrp->filePath.Length) {
					std::wstring fileNameStr(pMsgIrp->filePath.Buffer, pMsgIrp->filePath.Length / 2);
					Globals::Instance->postLogMessage(String::Concat("<V> Received irp on file: ", gcnew String(fileNameStr.c_str()), System::Environment::NewLine), VERBOSE_ONLY);
				}
				else {
					Globals::Instance->postLogMessage(String::Concat("<V> Received irp with file len 0", System::Environment::NewLine), VERBOSE_ONLY);
				}
			}
			pMsgIrp = (PDRIVER_MESSAGE)pMsgIrp->next;
		}

		// log 

		TotalIrpCount += numOps;
		Globals::Instance->addIrpHandled(numOps);

		// check Malicious, handle in that case
		for (ULONGLONG gid : gidsCheck) {
			CheckHandleMaliciousApplication(gid, Port);
		}

		Globals::Instance->postLogMessage(String::Concat("<V> ... Finished handling irp requests, requesting", System::Environment::NewLine), VERBOSE_ONLY);
	}
	delete[] Buffer;
	return hr;
}

HRESULT ProcessIrp(CONST DRIVER_MESSAGE& msg) {
	ULONGLONG gid = msg.Gid;
	ULONG pid = msg.PID;
	GProcessRecord^ record;
	if (ProcessesMemory::Instance->Processes->TryGetValue(gid, record)) { // val found
		if (record == nullptr) { // found pid but no process record/ shouldnt happen
			return S_FALSE;
		}
		record->AddIrpRecord(msg);
		return S_OK;
	}
	record = gcnew GProcessRecord(gid, pid);
	if (!ProcessesMemory::Instance->Processes->TryAdd(gid, record)) { // add failed
		if (ProcessesMemory::Instance->Processes->TryGetValue(gid, record)) { // val found
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



VOID HandleMaliciousApplication(GProcessRecord^ record, HANDLE comPort) {
	if (record != nullptr) {
		ULONGLONG gid = record->Gid();
		String^ gidStr = gid.ToString();
		String^ appName = record->Name();
		//DWORD failReason = 0; 
		//BOOLEAN isTerminateSuc;
		//DWORD isKilled;
		Globals::Instance->postLogMessage(String::Concat("<I> Handling malicious application: ", appName, " with RansomWatch gid: ", gidStr, System::Environment::NewLine), PRIORITY_PRINT);
		if (Globals::Instance->getKillStat()) {
			Globals::Instance->postLogMessage(String::Concat("<I> Attempt to kill processes using driver", System::Environment::NewLine), PRIORITY_PRINT);
			Monitor::Enter(record);
			if (record->isMalicious() && !record->isKilled())
			{
				/*HANDLE pHandle = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, false, pid);
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
				*/
				COM_MESSAGE killPidMsg;
				NTSTATUS retOp = S_OK;
				DWORD retSize;
				killPidMsg.type = MESSAGE_KILL_GID;
				killPidMsg.gid = gid;
				//Monitor::Enter(record);
				//if (!record->isKilled()) {
				HRESULT hr = FilterSendMessage(comPort, &killPidMsg, sizeof(COM_MESSAGE), &retOp, sizeof(NTSTATUS), &retSize);
				if (SUCCEEDED(hr) && retOp != E_ACCESSDENIED) {
					record->setKilled();
				}
				Monitor::Exit(record);
				if (FAILED(hr)) {
					DBOUT("Failed to send kill gid message" << std::endl);
					Globals::Instance->setCommCloseStat(TRUE);
				}

				if (FAILED(hr) || retOp != S_OK) {
					Globals::Instance->postLogMessage(String::Concat("<E> Failed to kill process using driver, gid: ", gidStr, System::Environment::NewLine), PRIORITY_PRINT);
				}
				//}
				//else {
				//	Monitor::Exit(record);
				//	DBOUT("Process already killed" << std::endl);
				//}
			}
			else {
				Monitor::Exit(record);
				Globals::Instance->postLogMessage(String::Concat("<I> Process already killed: ", appName, " Gid: ", gidStr, System::Environment::NewLine), PRIORITY_PRINT);
			}
		}
		else {
			String^ strMessage = String::Concat("<W> Auto kill disabled, reporting process: ", appName, "with gid: ", gidStr, System::Environment::NewLine);
			String^ caption = "Found malicious application";
			Globals::Instance->postLogMessage(strMessage, PRIORITY_PRINT);
			System::Windows::Forms::MessageBoxButtons buttons = System::Windows::Forms::MessageBoxButtons::OK;
			System::Windows::Forms::MessageBox::Show(strMessage, caption, buttons);
		}
		
	}
}

VOID CheckHandleMaliciousApplication(ULONGLONG gid, HANDLE comPort) {
	GProcessRecord^ record = nullptr;
	if (ProcessesMemory::Instance->Processes->TryGetValue(gid, record) && record != nullptr) {
		if (record->isProcessMalicious()) {
			if (!record->setReported()) return;
			//Globals::Instance->postLogMessage(String::Concat("<W> Found malicious application", System::Environment::NewLine), PRIORITY_PRINT);
			HandleMaliciousApplication(record, comPort);
			Generic::SortedSet<String^>^ triggersDetected = record->GetTriggersBreached();
			String^ msg = gcnew String("<I> Breached triggers: ");
			msg = String::Concat(msg, String::Join(", ", triggersDetected));
			msg = String::Concat(msg, System::Environment::NewLine);
			Globals::Instance->postLogMessage(msg, PRIORITY_PRINT);
			Generic::SortedSet<String^>^ createdFiles = record->GetCreatedFiles();
			Generic::SortedSet<String^>^ changedFiles = record->GetChangedFiles();
			String^ reportFile = String::Concat("C:\\Report", record->Gid().ToString(), ".log");
			try {
				StreamWriter^ sw = gcnew StreamWriter(reportFile);
				sw->WriteLine("RansomWatch report file");
				sw->Write("Files report for ransomware running from exe: "); // FIXME: on kill message return image files paths and report
				sw->WriteLine(record->Name());
				sw->Write("Process started on time: ");
				sw->WriteLine(record->DateStart().ToLocalTime());
				sw->Write("Time report: ");
				sw->WriteLine(record->DateKilled().ToLocalTime());
				Monitor::Enter(record);
				if (record->isKilled()) {
					sw->WriteLine("Process has been killed");
				}
				sw->WriteLine("Pids found:");
				Generic::SortedSet<ULONG>^ pids = record->pids();
				for each (ULONG pid in pids) {
					sw->Write(pid.ToString() + " ");
				}
				sw->WriteLine();
				
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
				sw->WriteLine();

				Globals::Instance->postLogMessage(String::Concat("<I> Created report file for ransomware: ", reportFile, System::Environment::NewLine), PRIORITY_PRINT);			
				sw->Flush();
				sw->WriteLine("Restore result:");
				BackupService^ backService = Globals::Instance->backupService();
				if (backService == nullptr) {
					Globals::Instance->postLogMessage(String::Concat("<E> Failed to restore files, backupservice not init", System::Environment::NewLine), PRIORITY_PRINT);
					sw->WriteLine("Failed to use backup service");
				}
				else {
					try {
						Generic::List<String^>^ returnedOutput = backService->RestoreFilesFromSnapShot(changedFiles, record->DateStart());
						for each (String ^ restoreOut in returnedOutput) {
							sw->WriteLine(restoreOut);
						}
						Globals::Instance->postLogMessage(String::Concat("<I> Files restore written to report file: ", reportFile, System::Environment::NewLine), PRIORITY_PRINT);

					}
					catch (Exception^ e) {
						Globals::Instance->postLogMessage(String::Concat("<E> Failed to restore files, backupservice exception: ", e->Message, System::Environment::NewLine), PRIORITY_PRINT);
						sw->WriteLine("Failed to use backup service");
					}

				}

				sw->WriteLine();
				sw->WriteLine("End file");
				sw->Close();
			}
			catch (Exception^ e) {
				Globals::Instance->postLogMessage(String::Concat("<E> Failed to create report file: ", e->Message, System::Environment::NewLine), PRIORITY_PRINT);
			}

		}
	}
}
