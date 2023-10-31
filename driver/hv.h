#ifndef __HV_H
#define __HV_H

#include <wdm.h>
#include <ntddk.h>
#include <intrin.h>

NTKERNELAPI VOID KeGenericCallDpc(PKDEFERRED_ROUTINE Routine, PVOID Context);
NTKERNELAPI VOID KeSignalCallDpcDone(PVOID SystemArgument1);
NTKERNELAPI LOGICAL KeSignalCallDpcSynchronize(PVOID SystemArgument2);

#define POOL_TAG ((ULONG)(' AWI'))

#include "msr.h"
#include "ept.h"
#include "vmx.h"
#include "idt.h"
#include "asm.h"

BOOLEAN nrot_hv_init();
void nrot_hv_exit();

#endif //__HV_H