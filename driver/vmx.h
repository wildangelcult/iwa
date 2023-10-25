#ifndef __VMX_H
#define __VMX_H

#define SIZE_VMX_STACK 0x8000

#define VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_NMI_WINDOW_EXITING		(1 << 22)
#define VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_USE_MSR_BITMAPS			(1 << 28)
#define VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_ACTIVATE_SECONDARY_CTRLS	(1 << 31)

#define VMCS_SECONDARY_PROC_BASED_EXEC_CTRLS_ENABLE_EPT				(1 << 1)
#define VMCS_SECONDARY_PROC_BASED_EXEC_CTRLS_ENABLE_RDTSCP			(1 << 3)
#define VMCS_SECONDARY_PROC_BASED_EXEC_CTRLS_ENABLE_INVPCID			(1 << 12)
#define VMCS_SECONDARY_PROC_BASED_EXEC_CTRLS_ENABLE_XSAVES_XRSTORS	(1 << 20)

#define VMCS_PRIMARY_VMEXIT_CTRLS_HOST_ADDRESS_SPACE_SIZE			(1 << 9)

#define VMCS_VMENTRY_CTRLS_IA32E_MODE_GUEST							(1 << 9)

#define VMCS_GUEST_ES_SELECTOR										0x800
#define VMCS_HOST_ES_SELECTOR										0xC00
#define VMCS_HOST_CS_SELECTOR										0xC02
#define VMCS_HOST_SS_SELECTOR										0xC04
#define VMCS_HOST_DS_SELECTOR										0xC06
#define VMCS_HOST_FS_SELECTOR										0xC08
#define VMCS_HOST_GS_SELECTOR										0xC0A
#define VMCS_HOST_TR_SELECTOR										0xC0C
#define VMCS_ADDRESS_OF_MSR_BITMAPS									0x2004
#define VMCS_TSC_OFFSET												0x2010
#define VMCS_EPT_POINTER											0x201A
#define VMCS_VMCS_LINK_POINTER										0x2800
#define VMCS_GUEST_DEBUGCTL											0x2802
#define VMCS_PIN_BASED_EXEC_CTRLS									0x4000
#define VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS							0x4002
#define VMCS_PAGE_FAULT_ERROR_CODE_MASK								0x4006
#define VMCS_PAGE_FAULT_ERROR_CODE_MATCH							0x4008
#define VMCS_PRIMARY_VMEXIT_CTRLS									0x400C
#define VMCS_VMEXIT_MSR_STORE_COUNT									0x400E
#define VMCS_VMEXIT_MSR_LOAD_COUNT									0x4010
#define VMCS_VMENTRY_CTRLS											0x4012
#define VMCS_VMENTRY_MSR_LOAD_COUNT									0x4014
#define VMCS_VMENTRY_INTERRUPTION_INFORMATION						0x4016
#define VMCS_SECONDARY_PROC_BASED_EXEC_CTRLS						0x401E
#define VMCS_INSTRUCTION_ERROR										0x4400
#define VMCS_GUEST_ES_LIMIT											0x4800
#define VMCS_GUEST_GDTR_LIMIT										0x4810
#define VMCS_GUEST_IDTR_LIMIT										0x4812
#define VMCS_GUEST_ES_AR											0x4814
#define VMCS_GUEST_SYSENTER_CS										0x482A
#define VMCS_HOST_SYSENTER_CS										0x4C00
#define VMCS_CR0_GUEST_HOST_MASK									0x6000
#define VMCS_CR4_GUEST_HOST_MASK									0x6002
#define VMCS_CR0_READ_SHADOW										0x6004
#define VMCS_CR4_READ_SHADOW										0x6006
#define VMCS_GUEST_CR0												0x6800
#define VMCS_GUEST_CR3												0x6802
#define VMCS_GUEST_CR4												0x6804
#define VMCS_GUEST_ES_BASE											0x6806
#define VMCS_GUEST_FS_BASE											0x680E
#define VMCS_GUEST_GS_BASE											0x6810
#define VMCS_GUEST_GDTR_BASE										0x6816
#define VMCS_GUEST_IDTR_BASE										0x6818
#define VMCS_GUEST_DR7												0x681A
#define VMCS_GUEST_RFLAGS											0x6820
#define VMCS_GUEST_SYSENTER_ESP										0x6824
#define VMCS_GUEST_SYSENTER_EIP										0x6826
#define VMCS_HOST_CR0												0x6C00
#define VMCS_HOST_CR3												0x6C02
#define VMCS_HOST_CR4												0x6C04
#define VMCS_HOST_FS_BASE											0x6C06
#define VMCS_HOST_GS_BASE											0x6C08
#define VMCS_HOST_TR_BASE											0x6C0A
#define VMCS_HOST_GDTR_BASE											0x6C0C
#define VMCS_HOST_IDTR_BASE											0x6C0E
#define VMCS_HOST_SYSENTER_ESP										0x6C10
#define VMCS_HOST_SYSENTER_EIP										0x6C12
#define VMCS_HOST_RSP												0x6C14
#define VMCS_HOST_RIP												0x6C16

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