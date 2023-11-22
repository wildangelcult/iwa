#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <stdio.h>

#include "../shared.h"

int main(int argc, char *argv[]) {
	FILE *fw;
	SC_HANDLE driverSvc;
	HANDLE driver, cmdPid;
	DWORD32 serverIp = 0, protectState;
	DWORD regSize = sizeof(DWORD32);

	fw = fopen("iwa.log", "w");

	if (RegGetValue(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\iwaclient", "ipaddr", RRF_RT_DWORD, NULL, &serverIp, &regSize)) {
		fprintf(fw, "RegGetValue");
		fclose(fw);
		return 1;
	}

	fprintf(fw, "Server IP %x\n", serverIp);
	driverSvc = OpenService(OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS), "iwa", SERVICE_START);
	if (!StartService(driverSvc, 0, NULL)) {
		fprintf(fw, "StartService %u\n", GetLastError());
		/*
		fclose(fw);
		return 1;
		*/
	}
	CloseServiceHandle(driverSvc);


	if ((driver = CreateFile("\\\\.\\iwa", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		fprintf(fw, "CreateFile %u\n", GetLastError());
		fclose(fw);
		return 1;
	}
	cmdPid = INVALID_HANDLE_VALUE;
	if (!DeviceIoControl(driver, IOCTL_PROTECT_PID, &cmdPid, sizeof(HANDLE), NULL, 0, NULL, NULL)) {
		fprintf(fw, "DeviceIoControl %u\n", GetLastError());
		fclose(fw);
		return 1;
	}

	protectState = 0;
	if (!DeviceIoControl(driver, IOCTL_SET_PROTECT_STATE, &protectState, sizeof(DWORD32), NULL, 0, NULL, NULL)) {
		fprintf(fw, "DeviceIoControl %u\n", GetLastError());
		fclose(fw);
		return 1;
	}

	for (;;) {
	}

	CloseHandle(driver);

	fclose(fw);

	return 0;
}