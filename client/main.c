#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>

#include "../shared.h"

SERVICE_STATUS_HANDLE svcHandle;
SERVICE_STATUS svcStatus;

void svc_stop(DWORD exitCode) {
	svcStatus.dwWin32ExitCode = GetLastError();
	svcStatus.dwServiceSpecificExitCode = exitCode;
	svcStatus.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(svcHandle, &svcStatus);
}

void svc_handler(DWORD dwControl) {
	if (dwControl == SERVICE_CONTROL_STOP) {
		svc_stop(0);
	}
}

void svc_init() {
	svcHandle = RegisterServiceCtrlHandler("iwaclient", svc_handler);
	memset(&svcStatus, 0, sizeof(SERVICE_STATUS));
	svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	svcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	svcStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(svcHandle, &svcStatus);
}

void svc_main(DWORD dwNumServicesArgs, LPSTR *lpServiceArgVectors) {
	FILE *fw;
	SC_HANDLE driverSvc;
	HANDLE driver, cmdPid;
	DWORD32 serverIp = 0;
	DWORD regSize = sizeof(DWORD32);

	svc_init();

	fw = fopen("iwa.log", "w");

	if (RegGetValue(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\iwaclient", "ipaddr", RRF_RT_DWORD, NULL, &serverIp, &regSize)) {
		fprintf(fw, "RegGetValue");
		fclose(fw);
		svc_stop(1);
		return;
	}

	fprintf(fw, "Server IP %x\n", serverIp);
	driverSvc = OpenService(OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS), "iwa", SERVICE_START);
	if (!StartService(driverSvc, 0, NULL)) {
		fprintf(fw, "StartService %u\n", GetLastError());
		/*
		fclose(fw);
		svc_stop(1);
		return;
		*/
	}
	CloseServiceHandle(driverSvc);


	if ((driver = CreateFile("\\\\.\\iwa", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		fprintf(fw, "CreateFile %u\n", GetLastError());
		fclose(fw);
		svc_stop(1);
		return;
	}
	cmdPid = INVALID_HANDLE_VALUE;
	if (!DeviceIoControl(driver, IOCTL_PROTECT_PID, &cmdPid, sizeof(HANDLE), NULL, 0, NULL, NULL)) {
		fprintf(fw, "DeviceIoControl %u\n", GetLastError());
		fclose(fw);
		svc_stop(1);
		return;
	}

	for (;;) {
	}

	CloseHandle(driver);

	fclose(fw);

	svc_stop(0);
}

int main(int argc, char *argv[]) {
	SERVICE_TABLE_ENTRY dispatch[2] = {
		{"iwaclient", svc_main},
		{NULL, NULL}
	};
	StartServiceCtrlDispatcher(dispatch);
	return 0;
}