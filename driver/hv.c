#include <ntifs.h>
#include "hv.h"
#include "util.h"
#include "hde/hde64.h"

typedef struct hv_hook_s {
	void *tramp;
	void *swapPage;
} hv_hook_t;

hv_hook_t *hook = NULL;

void *zeroPage;

const UINT8 jmpWithReg[] = {0x48, 0xb8, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0xff, 0xe0};
const UINT8 jmpNoReg[] = {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};


typedef NTSTATUS (*NtCreateFile_t)(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength
);

NTSTATUS nrot_hv_NtCreateFile(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength
) {
	DbgPrint("[IWA] NtCreateFile\n");
	return ((NtCreateFile_t)hook[HV_HOOK_NTCREATEFILE].tramp)(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}


typedef NTSTATUS (*NtOpenFile_t)(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG ShareAccess,
	ULONG OpenOptions
);

NTSTATUS nrot_hv_NtOpenFile(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG ShareAccess,
	ULONG OpenOptions
) {
	return ((NtOpenFile_t)hook[HV_HOOK_NTOPENFILE].tramp)(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

typedef NTSTATUS (*NtOpenProcess_t)(
	PHANDLE ProcessHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PCLIENT_ID ClientId
);

NTSTATUS nrot_hv_NtOpenProcess(
	PHANDLE ProcessHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PCLIENT_ID ClientId
) {
	return ((NtOpenProcess_t)hook[HV_HOOK_NTOPENPROCESS].tramp)(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
}

typedef NTSTATUS (*NtCreateKey_t)(
	PHANDLE KeyHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG TitleIndex,
	PUNICODE_STRING Class,
	ULONG CreateOptions,
	PULONG Disposition
);

NTSTATUS nrot_hv_NtCreateKey(
	PHANDLE KeyHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG TitleIndex,
	PUNICODE_STRING Class,
	ULONG CreateOptions,
	PULONG Disposition
) {
	return ((NtCreateKey_t)hook[HV_HOOK_NTCREATEKEY].tramp)(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
}

typedef NTSTATUS (*CmOpenKey_t)(
	PHANDLE KeyHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG OpenOptions,
	ULONG64 unknown
);

NTSTATUS nrot_hv_CmOpenKey(
	PHANDLE KeyHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG OpenOptions,
	ULONG64 unknown
) {
	return ((CmOpenKey_t)hook[HV_HOOK_CMOPENKEY].tramp)(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, unknown);
}

BOOLEAN nrot_hv_init(PUINT8 imageBase) {
	PIMAGE_NT_HEADERS nt;
	PHYSICAL_ADDRESS maxPhys;
	PVOID sectBase;
	ULONG i, cpuN, sectSize, totalLen, offset;
	LONG j;
	PUINT8 ptr;
	hde64s hs;
	struct {
		void *addr, *hook;
		unsigned int instLen;
	} hookAddr[HV_HOOK_MAX];

	maxPhys.QuadPart = MAXULONG64;

	if (!(ept = ExAllocatePoolWithTag(NonPagedPool, sizeof(ept_ept_t), POOL_TAG))) return FALSE;
	if (!(ept->pageTable = MmAllocateContiguousMemory(sizeof(ept_pageTable_t), maxPhys))) return FALSE;
	if (!nrot_ept_init()) return FALSE;

	if (!(hook = ExAllocatePoolWithTag(NonPagedPool, sizeof(hv_hook_t) * HV_HOOK_MAX, POOL_TAG))) return FALSE;
	memset(hook, 0, sizeof(hv_hook_t) * HV_HOOK_MAX);

	both_util_getSect(nrot_util_getModBase("ntoskrnl.exe"), "PAGE", &sectBase, &sectSize);

	hookAddr[HV_HOOK_NTCREATEFILE].addr = NtCreateFile;
	hookAddr[HV_HOOK_NTCREATEFILE].hook = nrot_hv_NtCreateFile;

	hookAddr[HV_HOOK_NTOPENFILE].addr = NtOpenFile;
	hookAddr[HV_HOOK_NTOPENFILE].hook = nrot_hv_NtOpenFile;

	hookAddr[HV_HOOK_NTOPENPROCESS].addr = NtOpenProcess;
	hookAddr[HV_HOOK_NTOPENPROCESS].hook = nrot_hv_NtOpenProcess;

	hookAddr[HV_HOOK_NTCREATEKEY].addr = both_util_sigInRange(sectBase, sectSize, "48 83 EC 48 48 83 64 24 ? 00 48 8B 84 24");
	hookAddr[HV_HOOK_NTCREATEKEY].hook = nrot_hv_NtCreateKey;

	hookAddr[HV_HOOK_CMOPENKEY].addr = both_util_sigInRange(sectBase, sectSize, "40 53 56 57 41 54 41 55 41 56 41 57 48 81 EC 20 02 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 45 8B F1");
	hookAddr[HV_HOOK_CMOPENKEY].hook = nrot_hv_CmOpenKey;

	for (i = 0, totalLen = 0; i < HV_HOOK_MAX; ++i) {
		hookAddr[i].instLen = 0;
		while (hookAddr[i].instLen < sizeof(jmpWithReg)) {
			hookAddr[i].instLen += hde64_disasm(((PUINT8)hookAddr[i].addr) + hookAddr[i].instLen, &hs);
		}
		DbgPrint("[IWA] hook %u %u\n", i, hookAddr[i].instLen);
		totalLen += hookAddr[i].instLen;
	}
	
	if (!(ptr = ExAllocatePoolWithTag(NonPagedPool, totalLen + sizeof(jmpNoReg) * HV_HOOK_MAX, POOL_TAG))) return FALSE;

	for (i = 0; i < HV_HOOK_MAX; ++i) {
		for (j = i - 1; j >= 0; --j) {
			if (PAGE_ALIGN(hookAddr[i].addr) == PAGE_ALIGN(hookAddr[j].addr)) {
				hook[i].swapPage = hook[j].swapPage;
				hook[j].swapPage = NULL; //avoid double free
				break;
			}
		}
		if (j == -1) {
			if (!(hook[i].swapPage = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;
			memcpy(hook[i].swapPage, PAGE_ALIGN(hookAddr[i].addr), PAGE_SIZE);
		}

		offset = ((ULONG_PTR)hookAddr[i].addr) & (PAGE_SIZE - 1);

		if ((offset + sizeof(jmpWithReg)) > (PAGE_SIZE - 1)) {
			DbgPrint("[IWA] page boundary error\n");
			__debugbreak();
			return FALSE;
		}

		memcpy(((PUINT8)hook[i].swapPage) + offset, jmpWithReg, sizeof(jmpWithReg));
		*((PVOID*)&(((PUINT8)hook[i].swapPage) + offset)[2]) = hookAddr[i].hook;

		hook[i].tramp = ptr;
		memcpy(ptr, hookAddr[i].addr, hookAddr[i].instLen);
		ptr += hookAddr[i].instLen;
		memcpy(ptr, jmpNoReg, sizeof(jmpNoReg));
		*((PVOID*)&ptr[6]) = ((PUINT8)hookAddr[i].addr) + hookAddr[i].instLen;
		ptr += sizeof(jmpNoReg);
	
		if (!nrot_ept_swapPage(hookAddr[i].addr, i)) return FALSE;
		ept->swap[i].execPml1.pfn = both_util_getPhysical(PAGE_ALIGN(hook[i].swapPage)) / PAGE_SIZE;
		ept->swap[i].pml1->value = ept->swap[i].execPml1.value;
	}

	if (!(zeroPage = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;
	memset(zeroPage, 0, PAGE_SIZE);

	nt = (PIMAGE_NT_HEADERS)(imageBase + ((PIMAGE_DOS_HEADER)imageBase)->e_lfanew);
	DbgPrint("[IWA] imageBase= %p BaseOfCode= %u SizeOfCode= %u\n", imageBase, nt->OptionalHeader.BaseOfCode, nt->OptionalHeader.SizeOfCode);

	cpuN = KeQueryActiveProcessorCount(0);
	if (!(vmx = ExAllocatePoolWithTag(NonPagedPool, sizeof(vmx_vmx_t) * cpuN, POOL_TAG))) return FALSE;
	memset(vmx, 0, sizeof(vmx_vmx_t) * cpuN);

	for (i = 0; i < cpuN; ++i) {
		if (!(vmx[i].vmxon = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;
		if (!(vmx[i].vmcs = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;
		if (!(vmx[i].stack = ExAllocatePoolWithTag(NonPagedPool, SIZE_VMX_STACK, POOL_TAG))) return FALSE;
		if (!(vmx[i].msrBitmap = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;
	}

	if (!(idt = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;
	if (!nrot_idt_init()) return FALSE;

	KeIpiGenericCall(nrot_vmx_init, 0);

	return TRUE;
}

void nrot_hv_exit() {
	ULONG i, cpuN;

	KeIpiGenericCall(nrot_vmx_exit, 0);

	if (ept) {
		if (ept->pageTable) MmFreeContiguousMemory(ept->pageTable);
		nrot_ept_exit();
		ExFreePoolWithTag(ept, POOL_TAG);
	}
	if (zeroPage) ExFreePoolWithTag(zeroPage, POOL_TAG);
	if (hook) {
		if (hook[0].tramp) ExFreePoolWithTag(hook[0].tramp, POOL_TAG);
		for (i = 0; i < HV_HOOK_MAX; ++i) {
			if (hook[i].swapPage) ExFreePoolWithTag(hook[i].swapPage, POOL_TAG);
		}
		ExFreePoolWithTag(hook, POOL_TAG);
	}
	if (vmx) {
		cpuN = KeQueryActiveProcessorCount(0);
		for (i = 0; i < cpuN; ++i) {
			if (vmx[i].vmxon) ExFreePoolWithTag(vmx[i].vmxon, POOL_TAG);
			if (vmx[i].vmcs) ExFreePoolWithTag(vmx[i].vmcs, POOL_TAG);
			if (vmx[i].stack) ExFreePoolWithTag(vmx[i].stack, POOL_TAG);
			if (vmx[i].msrBitmap) ExFreePoolWithTag(vmx[i].msrBitmap, POOL_TAG);
		}
		ExFreePoolWithTag(vmx, POOL_TAG);
	}

	if (idt) ExFreePoolWithTag(idt, POOL_TAG);
}