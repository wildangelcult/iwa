#include "hv.h"

BOOLEAN nrot_hv_init() {
	PHYSICAL_ADDRESS maxPhys;

	maxPhys.QuadPart = MAXULONG64;

	//alloc
	if (!(ept = ExAllocatePoolWithTag(NonPagedPool, sizeof(ept_ept_t), POOL_TAG))) return FALSE;
	if (!(ept->pageTable = MmAllocateContiguousMemory(sizeof(ept_pageTable_t), maxPhys))) return FALSE;


	if (!nrot_ept_init()) return FALSE;
	if (!nrot_vmx_init()) return FALSE;
	return TRUE;
}

void nrot_hv_exit() {
	if (ept) {
		if (ept->pageTable) MmFreeContiguousMemory(ept->pageTable);
		ExFreePoolWithTag(ept, POOL_TAG);
	}
}