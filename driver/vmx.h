#ifndef __VMX_H
#define __VMX_H

#define SIZE_VMX_STACK 0x8000

#define VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_NMI_WINDOW_EXITING		(1 << 22)

#define VMCS_GUEST_ES_SELECTOR										0x800
#define VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS							0x4002
#define VMCS_INSTRUCTION_ERROR										0x4400
#define VMCS_GUEST_ES_LIMIT											0x4800
#define VMCS_GUEST_ES_AR											0x4814
#define VMCS_GUEST_ES_BASE											0x6806

typedef struct vmx_vmx_s {
	int isOn;
	void *vmxon;
	void *vmcs;
	void *stack;
	void *msrBitmap;
} vmx_vmx_t;

extern vmx_vmx_t *vmx;

ULONG_PTR nrot_vmx_init(ULONG_PTR ctx);
ULONG_PTR nrot_vmx_exit(ULONG_PTR ctx);

#endif //__VMX_H