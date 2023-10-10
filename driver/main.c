#include <wdm.h>
#include <ntddk.h>
#include "../shared.h"

BOOLEAN shouldProtect = TRUE;

static DWORD32 ipAddr = 0;

HANDLE PsGetCurrentProcessId();

NTSTATUS nrot_mjr_create(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	DbgPrint("[IWA] " __FUNCTION__ "\n");
	DbgPrint("[IWA] %p\n", PsGetCurrentProcessId());
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS nrot_mjr_ioctl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	PIO_STACK_LOCATION stack;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG_PTR info = 0;

	DbgPrint("[IWA] " __FUNCTION__ "\n");

	stack = IoGetCurrentIrpStackLocation(Irp);

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_GET_IPADDR:
			if (stack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(DWORD32)) {
				status = STATUS_INVALID_PARAMETER;
			} else {
				memcpy(Irp->AssociatedIrp.SystemBuffer, &ipAddr, sizeof(DWORD32));
				info = sizeof(DWORD32);
			}
			break;
		case IOCTL_PROTECT_PID:
			break;
		case IOCTL_SET_PROTECT_STATE:
			break;
		default:
			status = STATUS_INVALID_PARAMETER;
			break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
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
	return TRUE;
}

void nrot_DriverUnload(PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING dosDeviceName;
	DbgPrint("[IWA] " __FUNCTION__ "\n");
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\iwa");
	IoDeleteSymbolicLink(&dosDeviceName);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	PDEVICE_OBJECT deviceObj;
	UNICODE_STRING deviceName, dosDeviceName;
	NTSTATUS status = STATUS_SUCCESS;
	SIZE_T i;
	DbgPrint("[IWA] " __FUNCTION__ "\n");

	if (!nrot_cpu_checkFeatures()) {
		return STATUS_UNSUCCESSFUL;
	}

	RtlInitUnicodeString(&deviceName, L"\\Device\\iwa");
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\iwa");
	status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &deviceObj);
	IoCreateSymbolicLink(&dosDeviceName, &deviceName);

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i) DriverObject->MajorFunction[i] = nrot_mjr_default;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = nrot_mjr_create;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = nrot_mjr_ioctl;
	DriverObject->DriverUnload = nrot_DriverUnload;



	//imageBase = DriverObject->DriverStart;

	return status;
}