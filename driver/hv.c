#include <ntifs.h>
#include "hv.h"
#include "util.h"

hv_hook_t *hook = NULL;

BOOLEAN nrot_hv_init(PUINT8 imageBase) {
	PIMAGE_NT_HEADERS nt;
	PHYSICAL_ADDRESS maxPhys;
	ULONG i, cpuN;
	void *hookAddr[HV_HOOK_MAX];

	maxPhys.QuadPart = MAXULONG64;

	if (!(ept = ExAllocatePoolWithTag(NonPagedPool, sizeof(ept_ept_t), POOL_TAG))) return FALSE;
	if (!(ept->pageTable = MmAllocateContiguousMemory(sizeof(ept_pageTable_t), maxPhys))) return FALSE;
	if (!nrot_ept_init()) return FALSE;

	if (!(hook = ExAllocatePoolWithTag(NonPagedPool, sizeof(hv_hook_t) * HV_HOOK_MAX, POOL_TAG))) return FALSE;
	memset(hook, 0, sizeof(hv_hook_t) * HV_HOOK_MAX);

	hookAddr[HV_HOOK_NTCREATEFILE] = NtCreateFile;
	hookAddr[HV_HOOK_NTOPENFILE] = NtOpenFile;
	hookAddr[HV_HOOK_NTOPENPROCESS] = NtOpenProcess;
	hookAddr[HV_HOOK_NTCREATEKEY] = NULL; //48 83 EC 48 48 83 64 24 ? 00 48 8B 84 24 PAGE
	hookAddr[HV_HOOK_CMOPENKEY] = NULL; //40 53 56 57 41 54 41 55 41 56 41 57 48 81 EC 20 02 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 45 8B F1 PAGE

	if (!(hook[HV_HOOK_NTCREATEFILE].swapPage = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;
	memcpy(hook[HV_HOOK_NTCREATEFILE].swapPage, PAGE_ALIGN(hookAddr[HV_HOOK_NTCREATEFILE]), PAGE_SIZE);

	if (!nrot_ept_swapPage(hookAddr[HV_HOOK_NTCREATEFILE], HV_HOOK_NTCREATEFILE)) return FALSE;
	ept->swap[HV_HOOK_NTCREATEFILE].execPml1.pfn = both_util_getPhysical(PAGE_ALIGN(hook[HV_HOOK_NTCREATEFILE].swapPage)) / PAGE_SIZE;
	ept->swap[HV_HOOK_NTCREATEFILE].pml1->value = ept->swap[HV_HOOK_NTCREATEFILE].execPml1.value;

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