#include "hv.h"
#include "util.h"

vmx_vmx_t *vmx;

void nrot_vmx_init(KDPC *Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2) {
	ULONG64 physVmxon;
	vmx_vmx_t *currVmx;
	msr_vmxBasic_t vmxBasic;
	unsigned char status;

	currVmx = &vmx[KeGetCurrentProcessorNumberEx(NULL)];

	__writecr4(__readcr4() | (1 << 13));

	memset(vmx->vmxon, 0, PAGE_SIZE);

	vmxBasic.value = __readmsr(MSR_VMX_BASIC);
	*((PULONG64)vmx->vmxon) = vmxBasic.revId;

	physVmxon = both_util_getPhysical(vmx->vmxon);
	status = __vmx_on(&physVmxon);
	if (status) {
		DbgPrint("[IWA] vmxon failed %u\n", status);
		goto end;
	}

	DbgPrint("[IWA] " __FUNCTION__ "\n");
end:
	KeSignalCallDpcSynchronize(SystemArgument2);
	KeSignalCallDpcDone(SystemArgument1);
}

void nrot_vmx_exit(KDPC *Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2) {
	__vmx_off();

	__writecr4(__readcr4() & ~(1 << 13));

	KeSignalCallDpcSynchronize(SystemArgument2);
	KeSignalCallDpcDone(SystemArgument1);
}