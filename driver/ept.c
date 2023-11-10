#include "hv.h"
#include "util.h"

ept_ept_t *ept = NULL;

BOOLEAN nrot_ept_init() {
	unsigned int rangesN = 0, i;
	ULONG n;
	ULONG64 j, k, memType, addr;
	struct {ULONG64 start, end; UCHAR memType;} mtrrRanges[10];
	msr_mtrrCap_t mtrrCap;
	msr_mtrrPhysBase_t physBase;
	msr_mtrrPhysMask_t physMask;
	ept_pageTable_t *pageTable;
	ept_pml_t defaultPml3;
	ept_pml2e_t defaultPml2, *curr;

	mtrrCap.value = __readmsr(MSR_MTRRCAP);

	for (i = 0; i < mtrrCap.vcnt; ++i) {
		physBase.value = __readmsr(MSR_MTRR_PHYSBASE0 + i * 2);
		physMask.value = __readmsr(MSR_MTRR_PHYSMASK0 + i * 2);
		if (physMask.valid) {
			mtrrRanges[rangesN].start = physBase.pfn * PAGE_SIZE;
			_BitScanForward64(&n, physMask.pfn * PAGE_SIZE);
			mtrrRanges[rangesN].end = mtrrRanges[rangesN].start + ((1ULL << n) - 1ULL);
			if (physBase.memType != MEMTYPE_WB) {
				mtrrRanges[rangesN].memType = (UCHAR)physBase.memType;
				++rangesN;
			}
		}
	}

	pageTable = ept->pageTable;
	memset(pageTable, 0, sizeof(ept_pageTable_t));

	pageTable->pml4[0].pfn = both_util_getPhysical(pageTable->pml3) / PAGE_SIZE;
	pageTable->pml4[0].read = 1;
	pageTable->pml4[0].write = 1;
	pageTable->pml4[0].exec = 1;

	defaultPml3.value = 0;
	defaultPml3.read = 1;
	defaultPml3.write = 1;
	defaultPml3.exec = 1;

	__stosq((PULONG64)pageTable->pml3, defaultPml3.value, EPT_PML_COUNT);

	for (i = 0; i < EPT_PML_COUNT; ++i) {
		pageTable->pml3[i].pfn = both_util_getPhysical(pageTable->pml2[i]) / PAGE_SIZE;
	}

	defaultPml2.value = 0;
	defaultPml2.read = 1;
	defaultPml2.write = 1;
	defaultPml2.exec = 1;
	defaultPml2.largePage = 1;

	__stosq((PULONG64)pageTable->pml2, defaultPml2.value, EPT_PML_COUNT * EPT_PML_COUNT);

	for (j = 0; j < EPT_PML_COUNT; ++j) {
		for (k = 0; k < EPT_PML_COUNT; ++k) {
#define SIZE_2MB ((SIZE_T)(512 * PAGE_SIZE))
			curr = &pageTable->pml2[j][k];
			curr->pfn = j * EPT_PML_COUNT + k;
			if (!curr->pfn) {
				curr->memType = MEMTYPE_UC;
				continue;
			}

			addr = curr->pfn * SIZE_2MB;
			memType = MEMTYPE_WB;

			for (i = 0; i < rangesN; ++i) {
				if (addr <= mtrrRanges[i].end && (addr + SIZE_2MB - 1) >= mtrrRanges[i].start) {
					memType = mtrrRanges[i].memType;
					if (memType == MEMTYPE_UC) break;
				}
			}

			curr->memType = memType;
		}
	}

	ept->eptp.value = 0;
	ept->eptp.memType = MEMTYPE_WB;
	ept->eptp.pageWalk = 3;
	ept->eptp.pfn = both_util_getPhysical(pageTable->pml4) / PAGE_SIZE;

	memset(ept->swap, 0, sizeof(ept->swap));

	return TRUE;
}

BOOLEAN nrot_ept_swapPage(void *origPage, void *swapPage, ULONG swapI) {
	ULONG64 origPhys;
	ULONG i;
	ept_swap_t *currSwap;
	ept_pml2e_t *pml2;
	ept_pml1e_t *pml1, defaultPml1;
	ept_pml_t pml2ptr;

	currSwap = &ept->swap[swapI];

	origPhys = both_util_getPhysical(PAGE_ALIGN(origPage));

	pml2 = &ept->pageTable->pml2[EPT_PML3_INDEX(origPhys)][EPT_PML2_INDEX(origPhys)];

	if (pml2->largePage) {
		if (!(pml1 = currSwap->split = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG))) return FALSE;

		defaultPml1.value = 0;
		defaultPml1.read = 1;
		defaultPml1.write = 1;
		defaultPml1.exec = 1;
		__stosq((PULONG64)pml1, defaultPml1.value, EPT_PML_COUNT);

		for (i = 0; i < EPT_PML_COUNT; ++i) {
			pml1[i].pfn = pml2->pfn * EPT_PML_COUNT + i;
		}

		pml2ptr.value = 0;
		pml2ptr.read = 1;
		pml2ptr.write = 1;
		pml2ptr.exec = 1;
		pml2ptr.pfn = both_util_getPhysical(pml1) / PAGE_SIZE;

		pml2->value = pml2ptr.value;
	}

	pml1 = currSwap->pml1 = &((ept_pml1e_t*)both_util_getVirtual(((ept_pml_t*)pml2)->pfn * PAGE_SIZE))[EPT_PML1_INDEX(origPhys)];

	currSwap->origPhys = origPhys;
	currSwap->origPml1.value = currSwap->swapPml1.value = pml1->value;

	currSwap->swapPml1.read = 0;
	currSwap->swapPml1.write = 0;
	currSwap->swapPml1.pfn = both_util_getPhysical(PAGE_ALIGN(swapPage)) / PAGE_SIZE;

	pml1->value = currSwap->swapPml1.value;

	return TRUE;
}

void nrot_ept_exit() {
	ULONG i;

	for (i = 0; i < HV_HOOK_MAX; ++i) {
		if (ept->swap[i].split) ExFreePoolWithTag(ept->swap[i].split, POOL_TAG);
	}
}