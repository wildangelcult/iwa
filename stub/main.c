#include <Windows.h>

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

void svc_main(DWORD dwNumServicesArgs, LPSTR *lpServiceArgVectors) {
	svcHandle = RegisterServiceCtrlHandler("iwaclient", svc_handler);
	memset(&svcStatus, 0, sizeof(SERVICE_STATUS));
	svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	svcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	svcStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(svcHandle, &svcStatus);

	svc_stop(0);
}

const char clientPath[] = "\\iwa.exe";

int main(int argc, char *argv[]) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char buf[MAX_PATH  + 1];
	UINT len;
	SERVICE_TABLE_ENTRY dispatch[2] = {
		{"iwaclient", svc_main},
		{NULL, NULL}
	};

	len = GetSystemDirectory(buf, sizeof(buf));
	memcpy(buf + len, clientPath, sizeof(clientPath));

	memset(&si, 0, sizeof(STARTUPINFO));
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);
	CreateProcess(buf, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	StartServiceCtrlDispatcher(dispatch);

	return 0;
}