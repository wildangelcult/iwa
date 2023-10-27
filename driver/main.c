#include <wdm.h>
#include <ntddk.h>
#include <intrin.h>

#include "msr.h"
#include "../shared.h"
#include "hv.h"

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(-1))
#endif

BOOLEAN shouldProtect = TRUE;

static DWORD32 ipAddr = 0;
HANDLE clientPid = INVALID_HANDLE_VALUE;

NTKERNELAPI HANDLE PsGetCurrentProcessId();

NTSTATUS nrot_mjr_create(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("[IWA] " __FUNCTION__ "\n");
	DbgPrint("[IWA] %p\n", PsGetCurrentProcessId());

	if (clientPid == INVALID_HANDLE_VALUE) {
		clientPid = PsGetCurrentProcessId();
	} else if (clientPid != PsGetCurrentProcessId()) {
		status = STATUS_ACCESS_DENIED;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS nrot_mjr_ioctl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	PIO_STACK_LOCATION stack;
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("[IWA] " __FUNCTION__ "\n");

	stack = IoGetCurrentIrpStackLocation(Irp);

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_PROTECT_PID:
			break;
		case IOCTL_SET_PROTECT_STATE:
			break;
		default:
			status = STATUS_INVALID_PARAMETER;
			break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS nrot_mjr_default(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

BOOLEAN nrot_cpu_checkFeatures() {
	unsigned int data[4];
	unsigned int intel[3] = {0x756e6547, 0x6c65746e, 0x49656e69};
	msr_featureControl_t featCtrl;
	msr_vmxEptVpidCap_t eptCap;
	msr_mtrrDefType_t mttr;

	__cpuid(data, 0);
	if (memcmp(&data[1], intel, sizeof(intel))) return FALSE;

	__cpuid(data, 1);
	if (!(data[2] & (1 << 5))) return FALSE;

	featCtrl.value = __readmsr(MSR_FEATURE_CONTROL);

	if (featCtrl.lock) {
		if (!featCtrl.VMX) return FALSE;
	} else {
		//where is bios lol
		featCtrl.lock = 1;
		featCtrl.VMX = 1;
		__writemsr(MSR_FEATURE_CONTROL, featCtrl.value);
	}

	eptCap.value = __readmsr(MSR_VMX_EPT_VPID_CAP);
	if (!(eptCap.execOnly && eptCap.pageWalk4 && eptCap.memTypeWB && eptCap.twoMBPages)) return FALSE;

	mttr.value = __readmsr(MSR_MTRR_DEF_TYPE);
	if (!mttr.enabled) return FALSE;

	return TRUE;
}

void nrot_DriverUnload(PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING dosDeviceName;
	DbgPrint("[IWA] " __FUNCTION__ "\n");
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\iwa");
	IoDeleteSymbolicLink(&dosDeviceName);
	IoDeleteDevice(DriverObject->DeviceObject);

	nrot_hv_exit();
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	PDEVICE_OBJECT deviceObj;
	UNICODE_STRING deviceName, dosDeviceName;
	SIZE_T i;
	DbgPrint("[IWA] " __FUNCTION__ "\n");

	if (!nrot_cpu_checkFeatures()) {
		return STATUS_INVALID_PARAMETER;
	}

	RtlInitUnicodeString(&deviceName, L"\\Device\\iwa");
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\iwa");
	IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &deviceObj);
	IoCreateSymbolicLink(&dosDeviceName, &deviceName);

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i) DriverObject->MajorFunction[i] = nrot_mjr_default;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = nrot_mjr_create;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = nrot_mjr_ioctl;
	DriverObject->DriverUnload = nrot_DriverUnload;


	//imageBase = DriverObject->DriverStart;

	if (!nrot_hv_init()) {
		nrot_DriverUnload(DriverObject);
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}