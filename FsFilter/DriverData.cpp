#include "DriverData.h"

DriverData::DriverData(PDRIVER_OBJECT DriverObject) : FilterRun(FALSE), DriverObject(DriverObject), pid(0), irpOpsSize(0), directoryRootsSize(0)
{
	InitializeListHead(&irpOps);
	InitializeListHead(&rootDirectories);
	KeInitializeSpinLock(&irpOpsLock); //init spin lock
	KeInitializeSpinLock(&directoriesSpinLock); //init spin lock
}

DriverData::~DriverData()
{
	
	Clear();
	
}


DriverData* driverData;