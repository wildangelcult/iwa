#include "hv.h"
#include "util.h"

vmx_vmx_t *vmx;

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

	if (!result->attr.p) {
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
	asm_descTable_t gdt;

	currVmx = &vmx[KeGetCurrentProcessorNumberEx(NULL)];

	__writecr4(__readcr4() | (1 << 13));

	memset(currVmx->vmxon, 0, PAGE_SIZE);
	memset(currVmx->vmcs, 0, PAGE_SIZE);
	memset(currVmx->stack, 0, SIZE_VMX_STACK);
	memset(currVmx->msrBitmap, 0, PAGE_SIZE);

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
	both_asm_getGdt(&gdt);
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_ES, both_asm_getEs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_CS, both_asm_getCs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_SS, both_asm_getSs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_DS, both_asm_getDs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_FS, both_asm_getFs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_GS, both_asm_getGs());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_LDTR, both_asm_getLdtr());
	both_vmx_vmcsSetSegReg(gdt.base, SEGREG_TR, both_asm_getTr());

	if (!nrot_asm_vmlaunch()) {
		currVmx->isOn = 0;
		__vmx_vmread(VMCS_INSTRUCTION_ERROR, &err);
		DbgPrint("[IWA] vmlaunch failed %llu\n", err);
		__vmx_off();
		return 0;
	}

	currVmx->isOn = 1;

	DbgPrint("[IWA] " __FUNCTION__ "\n");
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

ULONG64 root_vmx_vmexit(vmx_regCtx_t *ctx) {
	UINT8 fxmem[512];
	_fxsave64(fxmem);


	_fxrstor64(fxmem);
	return 1;
}

ULONG_PTR nrot_vmx_exit(ULONG_PTR ctx) {
	vmx_vmx_t *currVmx;
	currVmx = &vmx[KeGetCurrentProcessorNumberEx(NULL)];

	if (currVmx->isOn) {
		//do vmcall
		__vmx_off();
	}

	__writecr4(__readcr4() & ~(1 << 13));

	return 0;
}