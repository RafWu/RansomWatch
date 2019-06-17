#include "DriverData.h"

DriverData::DriverData(PDRIVER_OBJECT DriverObject) : 
	FilterRun(FALSE), 
	Filter(nullptr), 
	DriverObject(DriverObject), 
	pid(0), 
	irpOpsSize(0), 
	directoryRootsSize(0),
	GidToPids(),
	PidToGids()
{
	InitializeListHead(&irpOps);
	InitializeListHead(&rootDirectories);
	KeInitializeSpinLock(&irpOpsLock); //init spin lock
	KeInitializeSpinLock(&directoriesSpinLock); //init spin lock
	
	GidCounter = 0;
	KeInitializeSpinLock(&PIDRecordsLock); //init spin lock
}

DriverData::~DriverData()
{
	Clear();
}


DriverData* driverData;