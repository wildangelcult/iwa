#include <Windows.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <stdint.h>
#include "resource.h"

//https://learn.microsoft.com/en-us/windows/win32/controls/use-a-single-line--edit-control

DWORD addr = 0;
const char clientPath[] = "\\iwa.exe";
const char stubPath[] = "\\iwastub.exe";
const char driverPath[] = "\\drivers\\iwa.sys";

INT_PTR ipInput(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_COMMAND && wParam == IDOK) {
		if (SendDlgItemMessage(hDlg, IDC_IPADDRESS, IPM_GETADDRESS, 0, &addr) == 4) {
			if (MessageBox(NULL, "Sure?", "IWA installer", MB_YESNO) == IDYES) {
				EndDialog(hDlg, TRUE);
				return TRUE;
			}
		}
	} else if (msg == WM_CLOSE) {
		EndDialog(hDlg, TRUE);
		return TRUE;
	}
	return FALSE;
}

void fail() {
	char buf[256];
	snprintf(buf, sizeof(buf), "Installation failed.\n%u", GetLastError());
	MessageBox(NULL, buf, "IWA installer", MB_OK);
}


//int main(int argc, char* argv[]) {
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	HRSRC clientRes, stubRes, driverRes;
	uint8_t *client, *stub, *driver;
	DWORD clientSize, stubSize, driverSize;
	TOKEN_ELEVATION el;
	HANDLE token = NULL, file;
	SC_HANDLE scMan, service;
	HKEY key;
	DWORD sizeEl = sizeof(TOKEN_ELEVATION);
	UINT systemSize;
	char buf[MAX_PATH + 1];

	FreeConsole();

	if (!(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) && GetTokenInformation(token, TokenElevation, &el, sizeof(TOKEN_ELEVATION), &sizeEl) && el.TokenIsElevated)) {
		MessageBox(NULL, "You need to have administrative privileges to install this software.", "IWA installer", MB_OK);
		return 1;
	}

	DialogBox(NULL, MAKEINTRESOURCE(IDD_INPUT), NULL, ipInput);

	if (!addr) {
		printf("Dialog closed or address is 0.0.0.0\n");
		return 1;
	}

	printf("%x\n", addr);


	clientRes = FindResource(NULL, MAKEINTRESOURCE(IDR_CLIENT), RT_RCDATA);
	stubRes = FindResource(NULL, MAKEINTRESOURCE(IDR_STUB), RT_RCDATA);
	driverRes = FindResource(NULL, MAKEINTRESOURCE(IDR_DRIVER), RT_RCDATA);

	client = LockResource(LoadResource(NULL, clientRes));
	stub = LockResource(LoadResource(NULL, stubRes));
	driver = LockResource(LoadResource(NULL, driverRes));

	clientSize = SizeofResource(NULL, clientRes);
	stubSize = SizeofResource(NULL, stubRes);
	driverSize = SizeofResource(NULL, driverRes);

	printf("%u %u %u\n", clientSize, stubSize, driverSize);

	if (!(systemSize = GetSystemDirectory(buf, sizeof(buf)))) {
		fail();
		return 1;
	}

	memcpy(buf + systemSize, clientPath, sizeof(clientPath));
	printf("%s\n", buf);
	if ((file = CreateFile(buf, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		fail();
		return 1;
	}
	if (!WriteFile(file, client, clientSize, NULL, NULL)) {
		CloseHandle(file);
		fail();
		return 1;
	}
	CloseHandle(file);

	memcpy(buf + systemSize, stubPath, sizeof(stubPath));
	printf("%s\n", buf);
	if ((file = CreateFile(buf, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		fail();
		return 1;
	}
	if (!WriteFile(file, stub, stubSize, NULL, NULL)) {
		CloseHandle(file);
		fail();
		return 1;
	}
	CloseHandle(file);

	memcpy(buf + systemSize, driverPath, sizeof(driverPath));
	printf("%s\n", buf);
	if ((file = CreateFile(buf, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		fail();
		return 1;
	}
	if (!WriteFile(file, driver, driverSize, NULL, NULL)) {
		CloseHandle(file);
		fail();
		return 1;
	}
	CloseHandle(file);

	if (!(scMan = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
		fail();
		return 1;
	}

	if (!(service = CreateService(scMan,
				      "iwa",
				      "Indestructible Wild Angel - driver",
				      SERVICE_ALL_ACCESS,
				      SERVICE_KERNEL_DRIVER,
				      SERVICE_DEMAND_START,
				      SERVICE_ERROR_NORMAL,
				      buf,
				      NULL,
				      NULL,
				      NULL,
				      NULL,
				      NULL))) {
		fail();
		return 1;
	}
	CloseServiceHandle(service);
	
	memcpy(buf + systemSize, stubPath, sizeof(stubPath));
	if (!(service = CreateService(scMan,
				      "iwaclient",
				      "Indestructible Wild Angel - client",
				      SERVICE_ALL_ACCESS,
				      SERVICE_WIN32_OWN_PROCESS,
				      SERVICE_AUTO_START,
				      SERVICE_ERROR_NORMAL,
				      buf,
				      NULL,
				      NULL,
				      NULL,
				      NULL,
				      NULL))) {
		fail();
		return 1;
	}
	
	printf("%p %p\n", scMan, service);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\iwaclient", 0, KEY_ALL_ACCESS, &key)) {
		//docs doesnt talk about GetLastError, but at this point there is no way RegOpenKeyEx fails
		fail();
		return 1;
	}

	printf("%p\n", key);

	if (RegSetValueEx(key, "ipaddr", 0, REG_DWORD, &addr, sizeof(addr))) {
		//same as above
		fail();
		return 1;
	}
	
	RegCloseKey(key);

	MessageBox(NULL, "Installation successful.", "IWA installer", MB_OK);

	if (MessageBox(NULL, "Start the service?", "IWA installer", MB_YESNO) == IDYES) {
		if (!StartService(service, 0, NULL)) {
			fail();
			return 1;
		}
	}

	CloseServiceHandle(service);
	CloseServiceHandle(scMan);

	return 0;
}