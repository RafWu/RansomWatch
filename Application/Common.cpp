#include "Common.h"

LPCSTR IRP_TO_STRING(UCHAR irp) {
	switch (irp) {
	case IRP_READ: {
		return "IRP_READ";
	}
	case IRP_WRITE: {
		return "IRP_WRITE";
	}
	case IRP_SETINFO: {
		return "IRP_SETINFO";
	}
	case IRP_CREATE: {
		return "IRP_CREATE";
	}
	case IRP_CLEANUP: {
		return "IRP_CLEANUP";
	}
	default:
		return "UNHANDLED";
	}
}