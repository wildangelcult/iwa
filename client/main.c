#include <Windows.h>
#include <stdio.h>

#include "../shared.h"

int main(int argc, char *argv[]) {
	HANDLE driver;
	DWORD32 serverIp = 0xdeadbeef;

	printf("%x\n", IOCTL_GET_IPADDR);
	if ((driver = CreateFile("\\\\.\\iwa", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		printf("CreateFile %u\n", GetLastError());
		return 1;
	}
	if (!DeviceIoControl(driver, IOCTL_GET_IPADDR, NULL, 0, &serverIp, sizeof(DWORD32), NULL, NULL)) {
		printf("DeviceIoControl %u\n", GetLastError());
		return 1;
	}
	printf("Server IP %x\n", serverIp);

	CloseHandle(driver);

	return 0;
}