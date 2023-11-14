#include "hv.h"

#define SystemModuleInformation 0xb

typedef struct _SYSTEM_MODULE_ENTRY {
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR FullPathName[256];
} SYSTEM_MODULE_ENTRY, * PSYSTEM_MODULE_ENTRY;
typedef struct _SYSTEM_MODULE_INFORMATION {
	ULONG Count;
	SYSTEM_MODULE_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

NTKERNELAPI NTSTATUS ZwQuerySystemInformation(ULONG SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);

void* nrot_util_getModBase(const char *name) {
	ULONG infoSize, i;
	PSYSTEM_MODULE_INFORMATION info;
	PVOID modBase = NULL;

	ZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &infoSize);

	if (!(info = ExAllocatePoolWithTag(NonPagedPool, infoSize, POOL_TAG))) return NULL;

	if (!NT_SUCCESS(ZwQuerySystemInformation(SystemModuleInformation, info, infoSize, &infoSize))) goto finish;

	for (i = 0; i < info->Count; ++i) {
		if (!strcmp(name, info->Module[i].FullPathName + info->Module[i].OffsetToFileName)) {
			modBase = info->Module[i].ImageBase;
			break;
		}
	}

finish:
	ExFreePoolWithTag(info, POOL_TAG);
	return modBase;
}

void both_util_getSect(PVOID modBase, const char *sectName, PVOID *sectBase, PULONG sectSize) {
	USHORT i;
	PIMAGE_NT_HEADERS nt;
	PIMAGE_SECTION_HEADER sect, currSect;
	char currName[9];

	nt = (PIMAGE_NT_HEADERS)(((PUINT8)modBase) + ((PIMAGE_DOS_HEADER)modBase)->e_lfanew);
	sect = IMAGE_FIRST_SECTION(nt);

	*sectBase = NULL;
	*sectSize = 0;

	for (i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
		currSect = &sect[i];

		memcpy(currName, currSect->Name, 8);
		currName[8] = 0;

		if (!strcmp(sectName, currName)) {
			*sectBase = ((PUINT8)modBase) + sect->VirtualAddress;
			*sectSize = ((PUINT8)modBase) + ((ULONG)PAGE_ALIGN((currSect->Misc.VirtualSize > currSect->SizeOfRawData ? currSect->Misc.VirtualSize : currSect->SizeOfRawData) + (PAGE_SIZE - 1)));
			break;
		}
	}
}

void* both_util_sigInRange(PVOID base, ULONG size, const char *patt) {
	PUINT8 start, end, begin;
	BOOLEAN skip;
	const char *currPatt;

#define range(x, a, b) (x >= a && x <= b)
#define getHalf(x) (range(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
#define getByte(x) ((UINT8)(getHalf(x[0]) << 4 | getHalf(x[1])))

	start = base;
	end = start + size;

	currPatt = patt;
	begin = NULL;

	for (; start < end; ++start) {
		skip = *currPatt == '?';

		if (skip || *start == getByte(currPatt)) {
			if (!begin) {
				begin = start;
			}
			currPatt += skip ? 2 : 3;
			if (!currPatt[-1]) return begin;
		} else if (begin) {
			start = begin;
			begin = NULL;
			currPatt = patt;
		}
	}

#undef range
#undef getHalf
#undef getByte

	return NULL;
}