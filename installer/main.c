#include <Windows.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <stdint.h>
#include "resource.h"

//https://learn.microsoft.com/en-us/windows/win32/controls/use-a-single-line--edit-control
//https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemdirectorya

DWORD addr = 0;

INT_PTR ipInput(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_COMMAND && wParam == IDOK) {
		if (SendDlgItemMessage(hDlg, IDC_IPADDRESS, IPM_GETADDRESS, 0, &addr) == 4) {
			if (MessageBox(NULL, "Sure?", "IWA installer", MB_YESNO) == IDYES) {
				EndDialog(hDlg, TRUE);
				return TRUE;
			}
		}
	}
	return FALSE;
}

int main(int argc, char* argv[]) {
	HRSRC clientRes, driverRes;
	uint8_t* client, driver;
	DWORD clientSize, driverSize;

	clientRes = FindResource(NULL, MAKEINTRESOURCE(IDR_CLIENT), RT_RCDATA);
	driverRes = FindResource(NULL, MAKEINTRESOURCE(IDR_DRIVER), RT_RCDATA);

	client = LockResource(LoadResource(NULL, clientRes));
	driver = LockResource(LoadResource(NULL, driverRes));

	clientSize = SizeofResource(NULL, clientRes);
	driverSize = SizeofResource(NULL, driverRes);

	printf("%u %u\n", clientSize, driverSize);

	DialogBox(NULL, MAKEINTRESOURCE(IDD_INPUT), NULL, ipInput);
	
	printf("%x\n", addr);

	return 0;
}