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
		String^ date = thisDate.ToString("dd.MM.yy") + "/";

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
				String^ upFileName = fi->FullName->Replace('\\', '/');
				CloudBlockBlob^ blob = container->GetBlockBlobReference(upFileName);
				bool exist = blob->ExistsAsync()->Result;
				if (exist)
					blob->CreateSnapshotAsync()->Wait();
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

	public: BOOLEAN RemoveSnapshotDirectory(String^ dirPath) {
		if (dirPath == nullptr) return FALSE;
		String^ blobDirName = dirPath->Substring(3)->Replace('\\', '/'); // now in blob format
		CloudBlockBlob^ blob;
		BlobContinuationToken^ blobContinuationToken = nullptr;
		do
		{
			Microsoft::WindowsAzure::Storage::Blob::BlobResultSegment^ result = (container->ListBlobsSegmented(nullptr, blobContinuationToken));
			// Get the value of the continuation token returned by the listing call.

			blobContinuationToken = result->ContinuationToken;
			for each(IListBlobItem^ item in result->Results)
			{
				Globals::Instance->postLogMessage(blobDirName + System::Environment::NewLine, PRIORITY_PRINT);
				Globals::Instance->postLogMessage(item->Uri->LocalPath + System::Environment::NewLine, PRIORITY_PRINT);
				if (item->ToString()->Contains(blobDirName)) {
					CloudBlockBlob^ blob = container->GetBlockBlobReference(item->Uri->LocalPath);
					
					/*BlobContinuationToken^ blobContinuationToken = nullptr;
					do
					{
						Microsoft::WindowsAzure::Storage::Blob::BlobResultSegment^ result = (container->ListBlobsSegmented(nullptr, blobContinuationToken));
					} while (blobContinuationToken != nullptr); // Loop while the continuation token is not null.
					*/
				}
			}
		} while (blobContinuationToken != nullptr); // Loop while the continuation token is not null.
		return TRUE;
	}
	

};

