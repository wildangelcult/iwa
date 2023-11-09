#ifndef __HV_H
#define __HV_H

#include <wdm.h>
#include <ntddk.h>
#include <intrin.h>
#include <ntimage.h>

#if 0
NTKERNELAPI VOID KeGenericCallDpc(PKDEFERRED_ROUTINE Routine, PVOID Context);
NTKERNELAPI VOID KeSignalCallDpcDone(PVOID SystemArgument1);
NTKERNELAPI LOGICAL KeSignalCallDpcSynchronize(PVOID SystemArgument2);
#endif

#define POOL_TAG ((ULONG)(' AWI'))

#include "msr.h"
#include "ept.h"
#include "vmx.h"
#include "idt.h"
#include "asm.h"

//#define __vmx_vmwrite(x,y) DbgPrint("[IWA] vmwrite %016X %p\n", (x), (y)); __vmx_vmwrite((x), (y));

BOOLEAN nrot_hv_init(PUINT8 imageBase);
void nrot_hv_exit();

#endif //__HV_H