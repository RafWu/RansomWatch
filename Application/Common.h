#pragma once

#include <sstream>
#include <windows.h>
#include <minwinbase.h>
#include <FltUser.h>
#include "../SharedDefs/SharedDefs.h"
#include "Globals.h"

#define NUM_THREADS 2

#define DBOUT( s )            \
{                             \
   std::wstringstream os_;    \
   os_ << s;                   \
   OutputDebugString( os_.str().c_str() );  \
}

typedef struct _SCANNER_THREAD_CONTEXT {

	HANDLE Port;
	//HANDLE Completion;

} SCANNER_THREAD_CONTEXT, *PSCANNER_THREAD_CONTEXT;
/*
System::String^ GetFilePathFromObjectId(FILE_ID_INFO fileId) {

}*/

value struct FileId {
	ULONGLONG volumeSerialNumber;
	array<BYTE>^ fileId;

	FileId(const FILE_ID_INFO& info) {
		fileId = gcnew array<BYTE>(FILE_OBJECT_ID_SIZE);
		const FILE_ID_128& idStruct = info.FileId;
		for (ULONG i = 0; i < FILE_OBJECT_ID_SIZE; i++) {
			fileId[i] = idStruct.Identifier[i];
		}
		volumeSerialNumber = info.VolumeSerialNumber;

	}

	virtual bool Equals(Object^ obj) override {
		
		if (obj->GetType() != FileId::typeid) return false;
		return Equals(safe_cast<FileId^>(obj));

	}

	virtual bool Equals(FileId^ other) {
		if (other->fileId == nullptr) return false;
		if (other->volumeSerialNumber == volumeSerialNumber) {
			for (ULONG i = 0; i < FILE_OBJECT_ID_SIZE; i++) {
				if (fileId[i] != other->fileId[i]) {
					return false;
				}
			}
			return true;
			
		}

		return false;
	}

	virtual int GetHashCode() override {
		// FIXME: improve hashing of file id
		ULONG val1 = static_cast<ULONG> (volumeSerialNumber);
		ULONG val2 = static_cast<ULONG>((volumeSerialNumber >> (sizeof(ULONG) * 8)));
		ULONG val3 = (((ULONG)(fileId[0])) | (((ULONG)(fileId[1])) << 8) | (((ULONG)(fileId[2])) << 16) | (((ULONG)(fileId[3])) << 24));
		ULONG val4 = (((ULONG)(fileId[4])) | (((ULONG)(fileId[5])) << 8) | (((ULONG)(fileId[6])) << 16) | (((ULONG)(fileId[7])) << 24));
		ULONG val5 = (((ULONG)(fileId[8])) | (((ULONG)(fileId[9])) << 8) | (((ULONG)(fileId[10])) << 16) | (((ULONG)(fileId[11])) << 24));
		ULONG val6 = (((ULONG)(fileId[12])) | (((ULONG)(fileId[13])) << 8) | (((ULONG)(fileId[14])) << 16) | (((ULONG)(fileId[15])) << 24));
		
		ULONG xor_values = val1 ^ val2 ^ val3 ^ val4 ^ val5 ^ val6;
		return ((int)xor_values);

	}

	static bool operator!= (FileId^ obj1, FileId^ obj2) {
		return !FileId::operator==(obj1, obj2);
	}

	static bool operator== (FileId^ obj1, FileId^ obj2) {
		if (obj1->fileId == nullptr || obj2->fileId == nullptr) return false;
		if (obj1->volumeSerialNumber == obj2->volumeSerialNumber) {
			for (ULONG i = 0; i < FILE_OBJECT_ID_SIZE; i++) {
				if (obj1->fileId[i] != obj2->fileId[i]) {
					return false;
				}
			}
			return true;

		}

		return false;
	}

	static bool operator< (FileId^ obj1, FileId^ obj2) {
		if (obj1->fileId == nullptr || obj2->fileId == nullptr) return false;
		if (obj1->volumeSerialNumber < obj2->volumeSerialNumber) {
			return true;
		} 
		else if (obj1->volumeSerialNumber == obj2->volumeSerialNumber) {
			for (ULONG i = 0; i < FILE_OBJECT_ID_SIZE; i++) {
				if (obj1->fileId[i] < obj2->fileId[i]) {
					return true;
				}
			}
			return false;

		}

		return false;
	}

	static bool operator<= (FileId^ obj1, FileId^ obj2) {
		if (obj1->fileId == nullptr || obj2->fileId == nullptr) return false;
		if (obj1->volumeSerialNumber > obj2->volumeSerialNumber) {
			return false;
		}
		else if (obj1->volumeSerialNumber == obj2->volumeSerialNumber) {
			for (ULONG i = 0; i < FILE_OBJECT_ID_SIZE; i++) {
				if (obj1->fileId[i] > obj2->fileId[i]) {
					return false;
				}
			}
			return true;

		}

		return true;
	}

	FILE_ID_INFO ConvertFormatWindows() {
		FILE_ID_INFO newItem;
		newItem.VolumeSerialNumber = volumeSerialNumber;
		for (ULONG i = 0; i < FILE_OBJECT_ID_SIZE; i++) {
			newItem.FileId.Identifier[i] = fileId[i];
		}
		return newItem;
	}

};