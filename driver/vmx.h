#ifndef __VMX_H
#define __VMX_H

#define SIZE_VMX_STACK 0x8000

typedef struct vmx_vmx_s {
	void *vmxon;
	void *vmcs;
	void *stack;
	void *msrBitmap;
} vmx_vmx_t;

extern vmx_vmx_t *vmx;

void nrot_vmx_init(KDPC *Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
void nrot_vmx_exit(KDPC *Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

#endif //__VMX_H