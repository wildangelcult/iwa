#ifndef __HV_H
#define __HV_H

NTKERNELAPI VOID KeGenericCallDpc(PKDEFERRED_ROUTINE Routine, PVOID Context);
NTKERNELAPI VOID KeSignalCallDpcDone(PVOID SystemArgument1);
NTKERNELAPI LOGICAL KeSignalCallDpcSynchronize(PVOID SystemArgument2);

BOOLEAN nrot_hv_init();

#endif //__HV_H