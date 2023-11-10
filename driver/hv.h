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

typedef enum hv_hookType_e {
	HV_HOOK_NTCREATEFILE = 0,
	HV_HOOK_NTOPENFILE,
	HV_HOOK_NTOPENPROCESS,
	HV_HOOK_NTCREATEKEY,
	HV_HOOK_CMOPENKEY,
	HV_HOOK_MAX
} hv_hookType_t;

typedef struct hv_hook_s {
	void *tramp;
	void *swapPage;
} hv_hook_t;

extern hv_hook_t *hook;

#include "msr.h"
#include "ept.h"
#include "vmx.h"
#include "idt.h"
#include "asm.h"

//#define __vmx_vmwrite(x,y) DbgPrint("[IWA] vmwrite %016X %p\n", (x), (y)); __vmx_vmwrite((x), (y));

BOOLEAN nrot_hv_init(PUINT8 imageBase);
void nrot_hv_exit();

#endif //__HV_H