#include "hv.h"

BOOLEAN nrot_ept_init() {
	unsigned int rangesN = 0, i;
	ULONG n;
	struct {ULONG64 start, end; UCHAR memType;} mtrrRanges[10];
	msr_mtrrCap_t mtrrCap;
	msr_mtrrPhysBase_t physBase;
	msr_mtrrPhysMask_t physMask;

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

	return TRUE;
}