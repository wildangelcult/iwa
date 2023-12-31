#include "hv.h"

idt_entry_t *idt;

#define GATETYPE_INTERRUPT_GATE	0xe

void root_idt_nmiHandler() {
#if 0
	ULONG32 primaryCtrls;
	__vmx_vmread(VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS, &primaryCtrls);
	primaryCtrls |= VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_NMI_WINDOW_EXITING;
	__vmx_vmwrite(VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_NMI_WINDOW_EXITING, primaryCtrls);
#endif
	vmcs_interruptionInformation_t intInfo;
	intInfo.value = 0;
	intInfo.interruptType = IDT_NMI;
	intInfo.vector = EXCEPTION_NMI;
	intInfo.valid = 1;
	__vmx_vmwrite(VMCS_VMENTRY_INTERRUPTION_INFORMATION, intInfo.value);
}

BOOLEAN nrot_idt_init() {
	asm_descTable_t desc;
	idt_entry_t nmiEntry;
	idt_addr_t addr;

	memset(idt, 0, PAGE_SIZE);

	__sidt(&desc);
	memcpy(idt, desc.base, desc.limit);

	memset(&nmiEntry, 0, sizeof(idt_entry_t));
	addr.addr = root_asm_idtNmiHandler;

	nmiEntry.segmentSelector = both_asm_getCs();
	nmiEntry.gateType = GATETYPE_INTERRUPT_GATE;
	nmiEntry.present = 1;
	nmiEntry.offsetLow = addr.offsetLow;
	nmiEntry.offsetMid = addr.offsetMid;
	nmiEntry.offsetHigh = addr.offsetHigh;

	idt[IDT_NMI] = nmiEntry;
	return TRUE;
}