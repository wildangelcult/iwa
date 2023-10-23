#include "hv.h"
#include "util.h"

vmx_vmx_t *vmx;

ULONG_PTR nrot_vmx_init(ULONG_PTR ctx) {
	ULONG64 physVmxon, physVmcs, err;
	vmx_vmx_t *currVmx;
	msr_vmxBasic_t vmxBasic;
	unsigned char status;

	currVmx = &vmx[KeGetCurrentProcessorNumberEx(NULL)];

	__writecr4(__readcr4() | (1 << 13));

	memset(currVmx->vmxon, 0, PAGE_SIZE);

	vmxBasic.value = __readmsr(MSR_VMX_BASIC);
	*((PULONG64)currVmx->vmxon) = vmxBasic.revId;
	*((PULONG64)currVmx->vmcs) = vmxBasic.revId;

	physVmxon = both_util_getPhysical(vmx->vmxon);
	status = __vmx_on(&physVmxon);
	if (status) {
		DbgPrint("[IWA] vmxon failed %u\n", status);
		return 0;
	}


	physVmcs = both_util_getPhysical(currVmx->vmcs);

	status = __vmx_vmclear(&physVmcs);
	if (status) {
		DbgPrint("[IWA] vmclear failed %u\n", status);
		currVmx->isOn = 0;
		__vmx_off();
		return 0;
	}

	status = __vmx_vmptrld(&physVmcs);
	if (status) {
		DbgPrint("[IWA] vmptrld failed %u\n", status);
		currVmx->isOn = 0;
		__vmx_off();
		return 0;
	}

	//setup vmcs


	if (!nrot_asm_vmlaunch()) {
		currVmx->isOn = 0;
		__vmx_vmread(VMCS_INSTRUCTION_ERROR, &err);
		DbgPrint("[IWA] vmlaunch failed %llu\n", err);
		__vmx_off();
		return 0;
	}

	currVmx->isOn = 1;

	DbgPrint("[IWA] " __FUNCTION__ "\n");
}

void nrot_vmx_exit(ULONG_PTR ctx) {
	vmx_vmx_t *currVmx;
	currVmx = &vmx[KeGetCurrentProcessorNumberEx(NULL)];

	if (currVmx->isOn) {
		//do vmcall
		__vmx_off();
	}

	__writecr4(__readcr4() & ~(1 << 13));
}