#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include "resource.h"

//https://learn.microsoft.com/en-us/windows/win32/controls/use-a-single-line--edit-control
//https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemdirectorya

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

	return 0;
}