#include "hv.h"

BOOLEAN nrot_hv_init(PUINT8 imageBase) {
	PIMAGE_NT_HEADERS nt;
	PHYSICAL_ADDRESS maxPhys;
	ULONG i, cpuN;
	volatile ULONG currCpu;

	maxPhys.QuadPart = MAXULONG64;

	//alloc
	if (!(ept = ExAllocatePoolWithTag(NonPagedPool, sizeof(ept_ept_t), POOL_TAG))) return FALSE;
	if (!(ept->pageTable = MmAllocateContiguousMemory(sizeof(ept_pageTable_t), maxPhys))) return FALSE;
	if (!nrot_ept_init()) return FALSE;

	nt = (PIMAGE_NT_HEADERS)(imageBase + ((PIMAGE_DOS_HEADER)imageBase)->e_lfanew);
	DbgPrint("[IWA] imageBase= %p BaseOfCode= %u SizeOfCode= %u\n", imageBase, nt->OptionalHeader.BaseOfCode, nt->OptionalHeader.SizeOfCode);

	cpuN = KeQueryActiveProcessorCount(0);
	if (!(vmx = ExAllocatePoolWithTag(NonPagedPool, sizeof(vmx_vmx_t) * cpuN, POOL_TAG))) return FALSE;
	memset(vmx, 0, sizeof(vmx_vmx_t) * cpuN);

	for (i = 0; i < cpuN; ++i) {
#if 0
		if (!(vmx[i].vmxon = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;
		if (!(vmx[i].vmcs = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;
#endif
		if (!(vmx[i].vmxon = MmAllocateContiguousMemory(PAGE_SIZE, maxPhys))) return FALSE;
		if (!(vmx[i].vmcs = MmAllocateContiguousMemory(PAGE_SIZE, maxPhys))) return FALSE;
		if (!(vmx[i].stack = ExAllocatePoolWithTag(NonPagedPool, SIZE_VMX_STACK, POOL_TAG))) return FALSE;
		if (!(vmx[i].msrBitmap = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;
	}

	if (!(idt = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;
	if (!nrot_idt_init()) return FALSE;

	currCpu = 0;
	KeIpiGenericCall(nrot_vmx_init, &currCpu);

	return TRUE;
}

void nrot_hv_exit() {
	ULONG i, cpuN;
	volatile ULONG currCpu;

	currCpu = 0;
	KeIpiGenericCall(nrot_vmx_exit, &currCpu);

	if (ept) {
		if (ept->pageTable) MmFreeContiguousMemory(ept->pageTable);
		ExFreePoolWithTag(ept, POOL_TAG);
	}
	if (vmx) {
		cpuN = KeQueryActiveProcessorCount(0);
		for (i = 0; i < cpuN; ++i) {
#if 0
			if (vmx[i].vmxon) ExFreePoolWithTag(vmx[i].vmxon, POOL_TAG);
			if (vmx[i].vmcs) ExFreePoolWithTag(vmx[i].vmcs, POOL_TAG);
#endif

			if (vmx[i].vmxon) MmFreeContiguousMemory(vmx[i].vmxon);
			if (vmx[i].vmcs) MmFreeContiguousMemory(vmx[i].vmcs);
			if (vmx[i].stack) ExFreePoolWithTag(vmx[i].stack, POOL_TAG);
			if (vmx[i].msrBitmap) ExFreePoolWithTag(vmx[i].msrBitmap, POOL_TAG);
		}
		ExFreePoolWithTag(vmx, POOL_TAG);
	}

	if (idt) ExFreePoolWithTag(idt, POOL_TAG);
}