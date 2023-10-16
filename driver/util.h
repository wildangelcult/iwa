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

#endif //__UTIL_H