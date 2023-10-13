#ifndef __HV_H
#define __HV_H

#include <wdm.h>
#include <ntddk.h>
#include <intrin.h>

NTKERNELAPI VOID KeGenericCallDpc(PKDEFERRED_ROUTINE Routine, PVOID Context);
NTKERNELAPI VOID KeSignalCallDpcDone(PVOID SystemArgument1);
NTKERNELAPI LOGICAL KeSignalCallDpcSynchronize(PVOID SystemArgument2);

#include "msr.h"
#include "ept.h"
#include "vmx.h"

BOOLEAN nrot_hv_init();

#endif //__HV_H