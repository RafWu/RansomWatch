#pragma once

#include <windows.h>
#include <subauth.h>
#include "Traps.h"
#include "Debug.h"
#include <vcclr.h>
#include <string>
#include <sstream>



using namespace Microsoft::WindowsAzure::Storage;
using namespace Microsoft::WindowsAzure::Storage::Blob;
using namespace Microsoft::WindowsAzure::Storage::Auth;
using namespace Microsoft::WindowsAzure::Storage::RetryPolicies;
using namespace System;
using namespace System::Collections::Generic;
using namespace System::IO;
using namespace System::Linq;
using namespace System::Security::Cryptography;
using namespace System::ServiceModel::Channels;
using namespace System::Text;
using namespace System::Threading::Tasks;


ref class BackupService
{
// FIXME: storageConnectionString should be encrypted, change this string to a valid Azure storage string
private: CloudStorageAccount^ storageAccount;
private: CloudBlobClient^ blobClient;
private: CloudBlobContainer^ container;
private: String^ storageConnectionString = "DefaultEndpointsProtocol=https;AccountName=ransomwatch;AccountKey=NuMOweAnOleVFw6O2o9FCVyIxiSkpaPcTQVpHhH1CPgAO3bBkV3CxEOCm9kHUQDKpG0bpAtadocqJOW+aAfGAw==;EndpointSuffix=core.windows.net";

// init connection and get container reference, all ops will be done to ransomwatchbackup container
public: BackupService() {
	storageAccount = CloudStorageAccount::Parse(storageConnectionString);
	blobClient = storageAccount->CreateCloudBlobClient(); // blob service
	container = blobClient->GetContainerReference("ransomwatchbackup");
	container->CreateIfNotExistsAsync()->Wait();
}

// Creates a snapshot of a dir to azure storage , into the container set in c'tor
// we upload each file in dir recursivly and create a snapshot for the file
public: BOOLEAN SnapshotDirectory(String ^ dirPath) {
	if (dirPath == nullptr) return FALSE;
	BOOLEAN isDirTraps = FALSE;
	DirectoryInfo^ diSource = gcnew DirectoryInfo(dirPath);
	if (diSource->Exists == false) return FALSE; // no source directory

	Generic::List<TrapRecord^>^ listTrapsRecords = nullptr;

	if (TrapsMemory::Instance->traps->TryGetValue(dirPath, listTrapsRecords)) {
		isDirTraps = TRUE;
	}
		
	for each(FileInfo^ fi in diSource->GetFiles())
	{
		BOOLEAN isFileTrap = FALSE;
		if (isDirTraps) {
			// if the file checked is trap we skil it later, we dont upload traps
			for each (TrapRecord ^ tRecord in listTrapsRecords) {
				if (tRecord->getFilePath() == fi->FullName) {
					isFileTrap = TRUE;
				}
			}
		}
		if (!isFileTrap) {
			String^ upFileName = fi->FullName->Replace('\\', '/');
			CloudBlockBlob^ blob = container->GetBlockBlobReference(upFileName);
			//bool exist = blob->Exists(nullptr, nullptr);
			blob->UploadFromFile(fi->FullName, nullptr, nullptr, nullptr); //upload file and wait for it
			blob->CreateSnapshotAsync(nullptr, nullptr, nullptr, nullptr); // create snapshot and continue, dont wait
				
				
		}
	}

	// recurse to sub dirs
	for each (DirectoryInfo ^ di in diSource->GetDirectories()) {
		if (!SnapshotDirectory(di->FullName)) {
			return FALSE;
		}
	}

	return TRUE;
}

// removes a directory from azure storage with all the files and snapshots
public: BOOLEAN RemoveSnapshotDirectory(String^ dirPath) {
	if (dirPath == nullptr) return FALSE;
	String^ blobDirName = dirPath->Replace('\\', '/'); // now in blob format
	// Get the value of the continuation token returned by the listing call.

	Generic::IEnumerable<IListBlobItem^>^  blobsItemsDir = container->ListBlobs(blobDirName, true, BlobListingDetails::None , nullptr, nullptr);
	for each (IListBlobItem ^ blob in blobsItemsDir) {
		if (blob->GetType() == CloudBlob::typeid || blob->GetType()->BaseType == CloudBlob::typeid)
			((CloudBlob^)blob)->DeleteIfExists(DeleteSnapshotsOption::IncludeSnapshots, nullptr, nullptr, nullptr);
	}
		
	return TRUE;
}

// Recovery method, we get files changed and the start time of ransomware
// the function try to restore each file edited by the ransomware in protected areas
// for each file return a report string, which tells the opeartion output, printed to the report log of ransomware
public: Generic::List<String^>^ RestoreFilesFromSnapShot(Generic::SortedSet<String ^>^ filesPaths, DateTime RansomStartTimeUtc) {
	if (filesPaths == nullptr) return nullptr;
	Generic::List<String^>^ restoreReturn = gcnew Generic::List<String^>; // same order as filesPaths
	for each (String ^ filePath in filesPaths) 
	{
		FileInfo^ fileInfo;
		String^ directory;
		try {
			fileInfo = gcnew FileInfo(filePath);
			directory = Path::GetDirectoryName(filePath);
		}
		catch (...) {
			restoreReturn->Add(filePath + " - invalid file path");
			continue;
		}
		if (TrapsMemory::Instance->traps->ContainsKey(directory)) {  // traps dir
			Generic::List<TrapRecord^>^ trapsRecords = nullptr;
			if (TrapsMemory::Instance->traps->TryGetValue(directory, trapsRecords)) {
				BOOLEAN isTrap = FALSE;
				for each (TrapRecord ^ record in trapsRecords) {
					if (record->filePath->Contains(filePath)) {
						isTrap = TRUE;
						break;
					}
				}
				if (isTrap) {
					restoreReturn->Add(filePath + " - trap file, not restored");
					continue; // we may recreate the trap here or delete it
				}
			}
		}
		try {
			// try getting snapshot
			String^ blobFilePath = filePath->Replace('\\', '/'); // now in blob format
			Generic::IEnumerable<IListBlobItem^>^ blobsItemsFile = container->ListBlobs(blobFilePath, true, BlobListingDetails::Snapshots, nullptr, nullptr);
			DateTime bestTime = RansomStartTimeUtc;
			CloudBlockBlob^ bestTimeBlob = nullptr;
			BOOLEAN foundNewTime = FALSE;
			for each (IListBlobItem ^ blobItem in blobsItemsFile) {
				CloudBlockBlob^ blob = (CloudBlockBlob^)blobItem;
				Nullable<DateTimeOffset>^ timeOffest = blob->SnapshotTime;
				if (timeOffest != nullptr && timeOffest->HasValue) {
					String^ debug1 = String::Concat("ransom start time utc: ", RansomStartTimeUtc, " blob create time: ", timeOffest->Value.UtcDateTime);
					pin_ptr<const wchar_t> wch = PtrToStringChars(debug1);
					std::wstring stringStr(wch);
					DBOUT(stringStr << std::endl);
					if (DateTime::Compare(timeOffest->Value.UtcDateTime, RansomStartTimeUtc) < 0) { //earlier
						if (!foundNewTime) { // new item is better
							String^ debug2 = String::Concat("set file time: ", timeOffest->Value.UtcDateTime);
							pin_ptr<const wchar_t> wch = PtrToStringChars(debug1);
							std::wstring stringStr(wch);
							DBOUT(stringStr << std::endl);
							bestTime = timeOffest->Value.UtcDateTime;
							bestTimeBlob = blob;
							foundNewTime = TRUE;

						}
						else if (DateTime::Compare(timeOffest->Value.UtcDateTime, bestTime) > 0){
							String^ debug3 = String::Concat("replace time: ", bestTime, " blob create time: ", timeOffest->Value.UtcDateTime, System::Environment::NewLine);
							pin_ptr<const wchar_t> wch = PtrToStringChars(debug1);
							std::wstring stringStr(wch);
							DBOUT(stringStr << std::endl);
							bestTime = timeOffest->Value.UtcDateTime;
							bestTimeBlob = blob;
						}
					}
					else { // considered changed
						blob->DeleteIfExistsAsync();
					}
				}
			}

			if (foundNewTime && bestTimeBlob != nullptr) { // found blob to take
				bestTimeBlob->DownloadToFileAsync(filePath, FileMode::Create);
				restoreReturn->Add(filePath + " - restored file");
			}
			else {
				restoreReturn->Add(filePath + " - fail to locate backup valid checked");
			}
					
		}
		catch (...) {
			restoreReturn->Add(filePath + " - failed to restore file");
			continue;
		}
	}
	return restoreReturn;
}
		
};

