#pragma once

#include "Common.h"
#include "Traps.h"

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
// FIXME: cred should be from outside
private: String^ storageConnectionString = "DefaultEndpointsProtocol=https;AccountName=ransomwatch;AccountKey=NuMOweAnOleVFw6O2o9FCVyIxiSkpaPcTQVpHhH1CPgAO3bBkV3CxEOCm9kHUQDKpG0bpAtadocqJOW+aAfGAw==;EndpointSuffix=core.windows.net";
private: CloudStorageAccount^ storageAccount;
private: CloudBlobClient^ blobClient;
private: CloudBlobContainer^ container;

public: BackupService() {
		storageAccount = CloudStorageAccount::Parse(storageConnectionString);
		blobClient = storageAccount->CreateCloudBlobClient(); // blob service
		container = blobClient->GetContainerReference("ransomwatchbackup");
		container->CreateIfNotExistsAsync()->Wait();
		// TODO: catch exception from here
	}

	public: BOOLEAN SnapshotDirectory(String ^ dirPath) {
		if (dirPath == nullptr) return FALSE;
		BOOLEAN isDirTraps = FALSE;
		DirectoryInfo^ diSource = gcnew DirectoryInfo(dirPath); // TODO: may throw catch
		if (diSource->Exists == false) return FALSE; // no source directory
		//String^ BlobPrefix = diSource->Name;

		DateTime thisDate = DateTime::Now;
		String^ date = thisDate.ToString("dd.mm.yy") + "/";

		Generic::List<TrapRecord^>^ listTrapsRecords = nullptr;

		if (TrapsMemory::Instance->traps->TryGetValue(dirPath, listTrapsRecords)) {
			isDirTraps = TRUE;
		}

		for each(FileInfo^ fi in diSource->GetFiles()) // TODO: may throw directory not found
		{
			BOOLEAN isFileTrap = FALSE;
			if (isDirTraps) {
				for each (TrapRecord ^ tRecord in listTrapsRecords) {
					if (tRecord->getFilePath() == fi->FullName) {
						isFileTrap = TRUE;
					}
				}
			}
			if (!isFileTrap) {
				String^ upFileName = fi->FullName->Substring(3)->Replace('\\', '/')->Insert(0, date);
				CloudBlockBlob^ blob = container->GetBlockBlobReference(upFileName);
				blob->UploadFromFileAsync(fi->FullName);
			}
		}
		for each (DirectoryInfo ^ di in diSource->GetDirectories()) {
			if (!SnapshotDirectory(di->FullName)) {
				return FALSE;
			}
		}

		return TRUE;
	}


};

