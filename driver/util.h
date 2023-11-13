#ifndef __UTIL_H
#define __UTIL_H

NTKERNELAPI PVOID NTAPI MmGetVirtualForPhysical(PHYSICAL_ADDRESS PhysicalAddress);
NTKERNELAPI PHYSICAL_ADDRESS NTAPI MmGetPhysicalAddress(IN PVOID BaseAddress);


inline PVOID both_util_getVirtual(ULONG64 addr) {
	PHYSICAL_ADDRESS phys;
	phys.QuadPart = addr;
	return MmGetVirtualForPhysical(phys);
}

inline ULONG64 both_util_getPhysical(PVOID addr) {
	return MmGetPhysicalAddress(addr).QuadPart;
}

void* nrot_util_getModBase(const char *name);
void both_util_getSect(PVOID modBase, const char *sectName, PVOID *sectBase, PULONG sectSize);
void* both_util_sigInRange(PVOID modBase, ULONG size, const char *patt);

#endif //__UTIL_H