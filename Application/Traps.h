#pragma once

using namespace System;
using namespace System::Collections;
using namespace System::Threading;

#include "Common.h"

ref struct TrapRecord {
	FileId fileId; //we represent as string due to size changes, minimum of 128 bits in current systems
	String^ filePath;
	String^ directory;

	void setFilePath(String^ newFilePath) { filePath = newFilePath; };
	void setFileId(FileId newFileId) { fileId = newFileId; };
	void setDirectory(String^ newDir) { directory = newDir; };

	String^ getFilePath() { return filePath; };
	String^ getDirectory() { return directory; };
	FileId getFileId() { return fileId; };
};

ref class TrapsMemory {
	// TODO: add serialize and deserialize of those containers
public: Concurrent::ConcurrentDictionary< String^, Generic::List<TrapRecord^>^ >^ traps;
public: Concurrent::ConcurrentDictionary< FileId, TrapRecord^ >^ fileIdToTrapRecord;
public:
	TrapsMemory() {
		// path to traprecord and fileid to traprecord mappings
		traps = gcnew Concurrent::ConcurrentDictionary< String^, Generic::List<TrapRecord^>^ >;
		fileIdToTrapRecord = gcnew Concurrent::ConcurrentDictionary< FileId, TrapRecord^ >;
	}
private:
	TrapsMemory(const TrapsMemory%) { throw gcnew System::InvalidOperationException("TrapsMemory cannot be copy-constructed"); }
public:
	static property TrapsMemory^ Instance
	{
		TrapsMemory^ get() { return m_Instance; }
	}

private:
	static TrapsMemory^ m_Instance = gcnew TrapsMemory;

};