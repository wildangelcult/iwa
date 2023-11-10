#include "hv.h"
#include "util.h"

//#define DbgPrint

vmx_vmx_t *vmx;

typedef struct vmx_kprocess_s {
	DISPATCHER_HEADER header;
	LIST_ENTRY profileListHead;
	ULONG_PTR directoryTableBase;
	UINT8 data[1];
} vmx_kprocess_t;

extern NTKERNELAPI vmx_kprocess_t* PsInitialSystemProcess;

typedef union vmx_segDesc_u {
	ULONG64 value;
	struct {
		ULONG64 limit0		: 16;
		ULONG64 base0		: 24;
		ULONG64 attr0		: 8;
		ULONG64 limit1		: 4;
		ULONG64 attr1		: 4;
		ULONG64 base1		: 8;
	};
} vmx_segDesc_t;

typedef struct vmx_segSel_s {
	UINT16 align;
	union {
		UINT16 value;
		struct {
			USHORT type	: 4;
			USHORT s	: 1;
			USHORT dpl	: 2;
			USHORT p	: 1;
			USHORT avl	: 1;
			USHORT l	: 1;
			USHORT db	: 1;
			USHORT g	: 1;
			USHORT gap	: 4;
		};
	} attr;
	ULONG32 limit;
	ULONG64 base;
} vmx_segSel_t;

typedef enum vmx_segReg_e {
	SEGREG_ES = 0,
	SEGREG_CS,
	SEGREG_SS,
	SEGREG_DS,
	SEGREG_FS,
	SEGREG_GS,
	SEGREG_LDTR,
	SEGREG_TR,
} vmx_segReg_t;

//fuj
static void both_vmx_getSegDesc(ULONG64 gdtBase, UINT16 sel, vmx_segSel_t *result) {
	vmx_segDesc_t *desc;
	ULONG64 tmp;

	if (sel & 0x4) return;

	desc = (vmx_segDesc_t*)(((PUCHAR)gdtBase) + (sel & ~0x7));

	result->base = desc->base0 | desc->base1 << 24;
	result->limit = desc->limit0 | desc->limit1 << 16;
	result->attr.value = desc->attr0 | desc->attr1 << 8;

	if (!result->attr.s) {
		tmp = (*(PULONG64)(((PUCHAR)desc) + 8));
		result->base = (result->base & 0xffffffff) | tmp << 32;
	}
	
	if (result->attr.g) {
		result->limit = (result->limit << 12) + 0xfff;
	}
}

static void both_vmx_vmcsSetSegReg(ULONG64 gdtBase, ULONG segReg, UINT16 sel) {
	vmx_segSel_t segSel;
	ULONG ar;

	both_vmx_getSegDesc(gdtBase, sel, &segSel);

	ar = ((PUCHAR)&segSel.attr)[0] + (((PUCHAR)&segSel.attr)[1] << 12);
	
	if (!sel) ar |= 0x10000;

	__vmx_vmwrite(VMCS_GUEST_ES_SELECTOR + segReg * 2, sel);
	__vmx_vmwrite(VMCS_GUEST_ES_LIMIT + segReg * 2, segSel.limit);
	__vmx_vmwrite(VMCS_GUEST_ES_AR + segReg * 2, ar);
	__vmx_vmwrite(VMCS_GUEST_ES_BASE + segReg * 2, segSel.base);
}

static ULONG64 both_vmx_adjustCtrls(ULONG ctrls, ULONG msr) {
	ULARGE_INTEGER value;
	value.QuadPart = __readmsr(msr);
	ctrls &= value.HighPart;
	ctrls |= value.LowPart;
	return ctrls;
}

ULONG_PTR nrot_vmx_init(ULONG_PTR ctx) {
	ULONG64 physVmxon, physVmcs, err;
	vmx_vmx_t *currVmx;
	msr_vmxBasic_t vmxBasic;
	unsigned char status;
	asm_descTable_t gdt, idtDesc;
	vmx_segSel_t trSel;

	currVmx = &vmx[KeGetCurrentProcessorNumberEx(NULL)];

	__writecr4(__readcr4() | (1 << 13));

	memset(currVmx->vmxon, 0, PAGE_SIZE);
	memset(currVmx->vmcs, 0, PAGE_SIZE);
	memset(currVmx->stack, 0, SIZE_VMX_STACK);
	memset(currVmx->msrBitmap, 0, PAGE_SIZE);

	vmxBasic.value = __readmsr(MSR_VMX_BASIC);
	*((PULONG64)currVmx->vmxon) = vmxBasic.revId;
	*((PULONG64)currVmx->vmcs) = vmxBasic.revId;

	physVmxon = both_util_getPhysical(currVmx->vmxon);
	status = __vmx_on(&physVmxon);
	if (status) {
		DbgPrint("[IWA] vmxon failed %u\n", status);
		return 0;
	}


	physVmcs = both_util_getPhysical(currVmx->vmcs);

	//DbgPrint("[IWA] vmxon virt= %p phys= %p\n[IWA] vmcs  virt= %p phys= %p\n", currVmx->vmxon, physVmxon, currVmx->vmcs, physVmcs);

	status = __vmx_vmclear(&physVmcs);
	if (status) {
		DbgPrint("[IWA] vmclear failed %u\n", status);
		__vmx_off();
		return 0;
	}

	status = __vmx_vmptrld(&physVmcs);
	if (status) {
		DbgPrint("[IWA] vmptrld failed %u\n", status);
		__vmx_off();
		return 0;
	}

	//setup vmcs
	__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, both_asm_getEs() & 0xf8);
	__vmx_vmwrite(VMCS_HOST_CS_SELECTOR, both_asm_getCs() & 0xf8);
	__vmx_vmwrite(VMCS_HOST_SS_SELECTOR, both_asm_getSs() & 0xf8);
	__vmx_vmwrite(VMCS_HOST_DS_SELECTOR, both_asm_getDs() & 0xf8);
	__vmx_vmwrite(VMCS_HOST_FS_SELECTOR, both_asm_getFs() & 0xf8);
	__vmx_vmwrite(VMCS_HOST_GS_SELECTOR, both_asm_getGs() & 0xf8);
	__vmx_vmwrite(VMCS_HOST_TR_SELECTOR, both_asm_getTr() & 0xf8);

	__vmx_vmwrite(VMCS_VMCS_LINK_POINTER, ~0ULL);

	__vmx_vmwrite(VMCS_GUEST_DEBUGCTL, __readmsr(MSR_DEBUGCTL));

	__vmx_vmwrite(VMCS_TSC_OFFSET, 0);

	__vmx_vmwrite(VMCS_PAGE_FAULT_ERROR_CODE_MASK, 0);
	__vmx_vmwrite(VMCS_PAGE_FAULT_ERROR_CODE_MATCH, 0);
	__vmx_vmwrite(VMCS_VMEXIT_MSR_STORE_COUNT, 0);
	__vmx_vmwrite(VMCS_VMEXIT_MSR_LOAD_COUNT, 0);
	__vmx_vmwrite(VMCS_VMENTRY_MSR_LOAD_COUNT, 0);
	__vmx_vmwrite(VMCS_VMENTRY_INTERRUPTION_INFORMATION, 0);

	both_asm_getGdt(&gdt);
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_ES, both_asm_getEs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_CS, both_asm_getCs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_SS, both_asm_getSs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_DS, both_asm_getDs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_FS, both_asm_getFs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_GS, both_asm_getGs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_LDTR, both_asm_getLdtr());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_TR, both_asm_getTr());
	__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(MSR_FS_BASE));
	__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(MSR_GS_BASE));

	__vmx_vmwrite(VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS, both_vmx_adjustCtrls(
		VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_USE_MSR_BITMAPS | VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_ACTIVATE_SECONDARY_CTRLS,
		vmxBasic.vmxCap ? MSR_VMX_TRUE_PROCBASED_CTLS : MSR_VMX_PROCBASED_CTLS
	));

	__vmx_vmwrite(VMCS_SECONDARY_PROC_BASED_EXEC_CTRLS, both_vmx_adjustCtrls(
		VMCS_SECONDARY_PROC_BASED_EXEC_CTRLS_ENABLE_EPT | VMCS_SECONDARY_PROC_BASED_EXEC_CTRLS_ENABLE_RDTSCP | VMCS_SECONDARY_PROC_BASED_EXEC_CTRLS_ENABLE_INVPCID | VMCS_SECONDARY_PROC_BASED_EXEC_CTRLS_ENABLE_XSAVES_XRSTORS,
		MSR_VMX_PROCBASED_CTLS2
	));

	__vmx_vmwrite(VMCS_PIN_BASED_EXEC_CTRLS, both_vmx_adjustCtrls(
		/*VMCS_PIN_BASED_NMI_EXITING | VMCS_PIN_BASED_VIRTUAL_NMIS,*/ 0,
		vmxBasic.vmxCap ? MSR_VMX_TRUE_PINBASED_CTLS : MSR_VMX_PINBASED_CTLS
	));

	__vmx_vmwrite(VMCS_PRIMARY_VMEXIT_CTRLS, both_vmx_adjustCtrls(VMCS_PRIMARY_VMEXIT_CTRLS_HOST_ADDRESS_SPACE_SIZE, vmxBasic.vmxCap ? MSR_VMX_TRUE_EXIT_CTLS : MSR_VMX_EXIT_CTLS));
	__vmx_vmwrite(VMCS_VMENTRY_CTRLS, both_vmx_adjustCtrls(VMCS_VMENTRY_CTRLS_IA32E_MODE_GUEST, vmxBasic.vmxCap ? MSR_VMX_TRUE_ENTRY_CTLS : MSR_VMX_ENTRY_CTLS));

	__vmx_vmwrite(VMCS_CR0_GUEST_HOST_MASK, 0);
	__vmx_vmwrite(VMCS_CR4_GUEST_HOST_MASK, 0);

	__vmx_vmwrite(VMCS_CR0_READ_SHADOW, 0);
	__vmx_vmwrite(VMCS_CR4_READ_SHADOW, 0);

	__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());
	__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4());

	__vmx_vmwrite(VMCS_GUEST_DR7, 0x400);

	__vmx_vmwrite(VMCS_HOST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_HOST_CR3, PsInitialSystemProcess->directoryTableBase);
	__vmx_vmwrite(VMCS_HOST_CR4, __readcr4());

	__vmx_vmwrite(VMCS_GUEST_GDTR_BASE, gdt.base);
	__vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, gdt.limit);
	__sidt(&idtDesc);
	__vmx_vmwrite(VMCS_GUEST_IDTR_BASE, idtDesc.base);
	__vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, idtDesc.limit);

	__vmx_vmwrite(VMCS_GUEST_RFLAGS, both_asm_getRflags());

	__vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, __readmsr(MSR_SYSENTER_CS));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, __readmsr(MSR_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, __readmsr(MSR_SYSENTER_EIP));

	both_vmx_getSegDesc(gdt.base, both_asm_getTr(), &trSel);
	__vmx_vmwrite(VMCS_HOST_TR_BASE, trSel.base);

	__vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(MSR_FS_BASE));
	__vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(MSR_GS_BASE));

	__vmx_vmwrite(VMCS_HOST_GDTR_BASE, gdt.base);
	//__vmx_vmwrite(VMCS_HOST_IDTR_BASE, idtDesc.base);
	__vmx_vmwrite(VMCS_HOST_IDTR_BASE, idt);

	__vmx_vmwrite(VMCS_HOST_SYSENTER_CS, __readmsr(MSR_SYSENTER_CS));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, __readmsr(MSR_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, __readmsr(MSR_SYSENTER_EIP));

	__vmx_vmwrite(VMCS_ADDRESS_OF_MSR_BITMAPS, both_util_getPhysical(currVmx->msrBitmap));

	//__vmx_vmwrite(VMCS_EXCEPTION_BITMAP, MAXULONG32);
	//__vmx_vmwrite(VMCS_PAGE_FAULT_ERROR_CODE_MATCH, MAXULONG32);

	__vmx_vmwrite(VMCS_EPT_POINTER, ept->eptp.value);

	__vmx_vmwrite(VMCS_HOST_RSP, ((ULONG64)currVmx->stack) + SIZE_VMX_STACK);
	//DbgPrint("[IWA] %u host rsp= %p - %p\n", KeGetCurrentProcessorNumberEx(NULL), currVmx->stack, ((ULONG64)currVmx->stack) + SIZE_VMX_STACK);

	__vmx_vmwrite(VMCS_HOST_RIP, root_asm_vmexit);

	if (!nrot_asm_vmlaunch()) {
		__vmx_vmread(VMCS_INSTRUCTION_ERROR, &err);
		DbgPrint("[IWA] vmlaunch failed %llu\n", err);
		__vmx_off();
		return 0;
	}
	//DbgPrint("[IWA] %u\n", KeGetCurrentProcessorNumberEx(NULL));

	currVmx->isOn = 1;


	//DbgPrint("[IWA] " __FUNCTION__ "\n");
	return 0;
}

typedef struct vmx_regCtx_s {
	ULONG64 rax;
	ULONG64 rcx;
	ULONG64 rdx;
	ULONG64 rbx;
	ULONG64 _rsp; //not rsp, alignment
	ULONG64 rbp;
	ULONG64 rsi;
	ULONG64 rdi;
	ULONG64 r8;
	ULONG64 r9;
	ULONG64 r10;
	ULONG64 r11;
	ULONG64 r12;
	ULONG64 r13;
	ULONG64 r14;
	ULONG64 r15;
} vmx_regCtx_t;

void root_vmx_invept() {
	ept_invept_t desc;
	desc.eptp = ept->eptp.value;
	desc.reserved = 0;
	root_asm_invept(EPT_INVEPT_SINGLE_CONTEXT, &desc);
}

ULONG64 root_vmx_vmexit(vmx_regCtx_t *ctx) {
	UINT8 fxmem[512];
	ULONG64 rip, instLen, rflags, debugCtl, pendingDbg, *gpr, val64, result;
	ULARGE_INTEGER msrValue;
	ULONG32 exitReason, interrupt, primaryCtrls;
	ULONG i;
	INT32 cpuidData[4];
	vmcs_exitQualCrAccess_t crAccessQual;
	asm_descTable_t descTable;

	_fxsave64(fxmem);

	result = 1;

	__vmx_vmread(VMCS_EXIT_REASON, &exitReason);

	switch (exitReason) {
		case VMCS_EXIT_REASON_EXCEPTION_OR_NMI:
#if 0
			__vmx_vmread(VMCS_GUEST_RSP, (PULONG64)&gpr);
			__vmx_vmread(VMCS_GUEST_RIP, &rip);

			__vmx_vmread(VMCS_VMEXIT_INTERRUPTION_INFORMATION, &intInfo.value);
			__vmx_vmread(VMCS_VMEXIT_INTERRUPTION_ERROR_CODE, &interrupt);

			DbgPrint("[IWA] %u exception ctx= %p rsp= %p rip= %p\n", KeGetCurrentProcessorNumberEx(NULL), ctx, gpr, rip);
			DbgPrint("[IWA] vector= %u intType= %u deliverErr= %u valid= %u error= %u\n", intInfo.vector, intInfo.interruptType, intInfo.deliverError, intInfo.valid, interrupt);

			__vmx_vmwrite(VMCS_VMENTRY_INTERRUPTION_INFORMATION, intInfo.value);
			__vmx_vmwrite(VMCS_VMENTRY_EXCEPTION_ERROR_CODE, interrupt);

			//__debugbreak();
			goto dontSkipInst;

			__vmx_vmread(VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS, &primaryCtrls);
			primaryCtrls |= VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_NMI_WINDOW_EXITING;
			__vmx_vmwrite(VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_NMI_WINDOW_EXITING, primaryCtrls);
#endif
			goto dontSkipInst; //shouldnt ever happen
		case VMCS_EXIT_REASON_TRIPLE_FAULT:
			__vmx_vmread(VMCS_GUEST_RSP, (PULONG64)&gpr);
			__vmx_vmread(VMCS_GUEST_RIP, &rip);
			DbgPrint("[IWA] %u Ah shit, here we go again. ctx= %p rsp= %p rip= %p\n", KeGetCurrentProcessorNumberEx(NULL), ctx, gpr, rip);
			/*
			intInfo.value = 0;
			intInfo.interruptType = 6;
			intInfo.vector = 3;
			intInfo.valid = 1;
			__vmx_vmwrite(VMCS_VMENTRY_INTERRUPTION_INFORMATION, intInfo.value);
			*/
			/*
			__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &instLen);
			ULONG64 hostCr3, guestCr3;
			__vmx_vmread(VMCS_GUEST_CR3, &guestCr3);
			hostCr3 = __readcr3();

			__writecr3(guestCr3);
			__writecr0(__readcr0() & ~(1ULL << 16));
			for (; instLen; --instLen) {
				((PUINT8)rip)[instLen - 1] = 0xcc;
			}
			__writecr0(__readcr0() | (1ULL << 16));
			__writecr3(hostCr3);
			*/
			/*
			ULONG64 hostCr3, guestCr3;
			__vmx_vmread(VMCS_GUEST_CR3, &guestCr3);
			hostCr3 = __readcr3();
			DbgPrint("[IWA] cr3 host= %p guest= %p\n", hostCr3, guestCr3);
			*/

			__debugbreak();
			goto dontSkipInst;
		case VMCS_EXIT_REASON_NMI_WINDOW:
#if 0
			vmcs_interruptionInformation_t intInfo;
			intInfo.value = 0;
			intInfo.interruptType = IDT_NMI;
			intInfo.vector = EXCEPTION_NMI;
			intInfo.valid = 1;
			__vmx_vmwrite(VMCS_VMENTRY_INTERRUPTION_INFORMATION, intInfo.value);

			__vmx_vmread(VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS, &primaryCtrls);
			primaryCtrls &= ~VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_NMI_WINDOW_EXITING;
			__vmx_vmwrite(VMCS_PRIMARY_PROC_BASED_EXEC_CTRLS_NMI_WINDOW_EXITING, primaryCtrls);
#endif
			goto dontSkipInst; //shouldnt ever happen
		case VMCS_EXIT_REASON_CPUID:
			__cpuidex(cpuidData, ctx->rax, ctx->rcx);
			switch (ctx->rax) {
				case 1:
					cpuidData[2] |= VMX_HYPERV_HV_PRESENT;
					break;
				case VMX_HYPERV_VENDOR:
					cpuidData[0] = VMX_HYPERV_INTERFACE;
					cpuidData[1] = 'rUmI';
					cpuidData[2] = 'revO';
					cpuidData[3] = 'rees';
					break;
				case VMX_HYPERV_INTERFACE:
					cpuidData[0] = '0#vH';
					cpuidData[1] = cpuidData[2] = cpuidData[3] = 0;
					break;
				default:
					break;
			}
			ctx->rax = cpuidData[0];
			ctx->rbx = cpuidData[1];
			ctx->rcx = cpuidData[2];
			ctx->rdx = cpuidData[3];
			break;
		case VMCS_EXIT_REASON_INVD:
			__wbinvd();
			break;
		case VMCS_EXIT_REASON_VMCALL:
			__vmx_vmread(VMCS_GUEST_RIP, &rip);
			if (rip == both_asm_vmcall) {
				//DbgPrint("[IWA] vmxoff\n");

				__vmx_vmread(VMCS_GUEST_CR3, &val64);
				__writecr3(val64);

				__vmx_vmread(VMCS_GUEST_FS_BASE, &val64);
				__writemsr(MSR_FS_BASE, val64);

				__vmx_vmread(VMCS_GUEST_GS_BASE, &val64);
				__writemsr(MSR_GS_BASE, val64);

				__vmx_vmread(VMCS_GUEST_GDTR_BASE, &descTable.base);
				__vmx_vmread(VMCS_GUEST_GDTR_LIMIT, &val64);
				descTable.limit = (UINT16)val64;
				both_asm_setGdt(&descTable);

				__vmx_vmread(VMCS_GUEST_IDTR_BASE, &descTable.base);
				__vmx_vmread(VMCS_GUEST_IDTR_LIMIT, &val64);
				descTable.limit = (UINT16)val64;
				__lidt(&descTable);

				result = 0;
			} else {
				ctx->rax = both_asm_vmcall(ctx->rcx, ctx->rdx, ctx->r8);
			}
			break;
		case VMCS_EXIT_REASON_VMCLEAR:
		case VMCS_EXIT_REASON_VMLAUNCH:
		case VMCS_EXIT_REASON_VMPTRLD:
		case VMCS_EXIT_REASON_VMPTRST:
		case VMCS_EXIT_REASON_VMREAD:
		case VMCS_EXIT_REASON_VMRESUME:
		case VMCS_EXIT_REASON_VMWRITE:
		case VMCS_EXIT_REASON_VMXOFF:
		case VMCS_EXIT_REASON_VMXON:
		case VMCS_EXIT_REASON_VMFUNC:
		case VMCS_EXIT_REASON_INVEPT:
		case VMCS_EXIT_REASON_INVVPID:
			__vmx_vmread(VMCS_GUEST_RFLAGS, &rflags);
			rflags |= VMX_RFLAGS_CARRY_FLAG;
			__vmx_vmwrite(VMCS_GUEST_RFLAGS, rflags);
			break;
		case VMCS_EXIT_REASON_CR_ACCESS:
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, &crAccessQual.value);

			gpr = &(&ctx->rax)[crAccessQual.gpr];

			if (crAccessQual.gpr == 4) __vmx_vmread(VMCS_GUEST_RSP, gpr);

			if (crAccessQual.accessType == VMCS_EXIT_QUALIFICATION_CR_ACCESS_ACCESS_TYPE_MOV_TO_CR) {
				switch (crAccessQual.cr) {
					case 0:
						__vmx_vmwrite(VMCS_GUEST_CR0, *gpr);
						__vmx_vmwrite(VMCS_CR0_READ_SHADOW, *gpr);
						break;
					case 3:
						__vmx_vmwrite(VMCS_GUEST_CR3, *gpr & ~(1ULL << 63));
						root_vmx_invept();
						break;
					case 4:
						__vmx_vmwrite(VMCS_GUEST_CR4, *gpr);
						__vmx_vmwrite(VMCS_CR4_READ_SHADOW, *gpr);
						break;
				}
			} else if (crAccessQual.accessType == VMCS_EXIT_QUALIFICATION_CR_ACCESS_ACCESS_TYPE_MOV_FROM_CR) {
				switch (crAccessQual.cr) {
					case 0:
						__vmx_vmread(VMCS_GUEST_CR0, gpr);
						break;
					case 3:
						__vmx_vmread(VMCS_GUEST_CR0, gpr);
						break;
					case 4:
						__vmx_vmread(VMCS_GUEST_CR0, gpr);
						break;
				}
			}
			break;
		case VMCS_EXIT_REASON_RDMSR:
			msrValue.QuadPart = 0;
			if ((ctx->rcx <= 0x00001FFF) || ((ctx->rcx >= 0xC0000000) && (ctx->rcx <= 0xC0001FFF)) || ((ctx->rcx >= 0x40000000) && ctx->rcx <= 0x400000F0)) {
				msrValue.QuadPart = __readmsr(ctx->rcx);
			}
			ctx->rax = msrValue.LowPart;
			ctx->rdx = msrValue.HighPart;
			//DbgPrint("[IWA] rdmsr rcx= %p rax= %p rdx= %p\n", ctx->rcx, ctx->rax, ctx->rdx);
			break;
		case VMCS_EXIT_REASON_WRMSR:
			if ((ctx->rcx <= 0x00001FFF) || ((ctx->rcx >= 0xC0000000) && (ctx->rcx <= 0xC0001FFF)) || ((ctx->rcx >= 0x40000000) && ctx->rcx <= 0x400000F0)) {
				msrValue.LowPart = ctx->rax;
				msrValue.HighPart = ctx->rdx;
				__writemsr(ctx->rcx, msrValue.QuadPart);
			}
			break;
		case VMCS_EXIT_REASON_MONITOR_TRAP_FLAG:
			goto dontSkipInst;
		case VMCS_EXIT_REASON_EPT_VIOLATION:
			__vmx_vmread(
			goto dontSkipInst;
		case VMCS_EXIT_REASON_EPT_MISCONFIGURATION:
			DbgPrint("[IWA] EPT is fucked\n");
			break;
		default:
			break;
	}

	__vmx_vmread(VMCS_GUEST_RIP, &rip);
	__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &instLen);
	__vmx_vmwrite(VMCS_GUEST_RIP, rip + instLen);

	__vmx_vmread(VMCS_GUEST_RFLAGS, &rflags);
	__vmx_vmread(VMCS_GUEST_DEBUGCTL, &debugCtl);

	if ((rflags & VMX_RFLAGS_TRAP_FLAG) && !(debugCtl & VMX_DEBUGCTL_BTF)) {
		__vmx_vmread(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, &pendingDbg);
		pendingDbg |= VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS_BS;
		__vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, pendingDbg);
	}

	__vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY_STATE, &interrupt);
	interrupt &= ~(VMCS_GUEST_INTERRUPTIBILITY_STATE_BLOCKING_BY_STI | VMCS_GUEST_INTERRUPTIBILITY_STATE_BLOCKING_BY_MOV_SS);
	__vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, interrupt);

dontSkipInst:

	//DbgPrint("[IWA] %u " __FUNCTION__ " %u\n", KeGetCurrentProcessorNumberEx(NULL), exitReason);

	_fxrstor64(fxmem);
	return result;
}

void eror_vmx_vmresumeError() {
	ULONG64 err;
	__vmx_vmread(VMCS_INSTRUCTION_ERROR, &err);
	DbgPrint("[IWA] vmlaunch failed %llu\n", err);
	__debugbreak();
}

ULONG_PTR nrot_vmx_exit(ULONG_PTR ctx) {
	vmx_vmx_t *currVmx;

	//DbgPrint("[IWA] vmxoff %u\n", currCpu);

	currVmx = &vmx[KeGetCurrentProcessorNumberEx(NULL)];

	if (currVmx->isOn) {
		both_asm_vmcall(0, 0, 0);
	}

	__writecr4(__readcr4() & ~(1 << 13));

	return 0;
}