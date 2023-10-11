#ifndef __MSR_H
#define __MSR_H

#define MSR_FEATURE_CONTROL	0x3A
#define MSR_VMX_EPT_VPID_CAP	0x48C
#define MSR_MTRR_DEF_TYPE	0x2FF

typedef union msr_featureControl_u {
	ULONG64 value;
	struct {
		ULONG64 lock		: 1;
		ULONG64 VMXinSMX	: 1;
		ULONG64 VMX		: 1;
		ULONG64 _reserved	: 5;
		ULONG64 localSENTER	: 7;
		ULONG64 globalSENTER	: 1;
		ULONG64 _rest		: 48;
	};
} msr_featureControl_t;

typedef union msr_vmxEptVpidCap_u {
	ULONG64 value;
	struct {
		ULONG64 execOnly		: 1;
		ULONG64 _reserved1		: 5;
		ULONG64 pageWalk4		: 1;
		ULONG64 pageWalk5		: 1;
		ULONG64 memoryTypeUC		: 1;
		ULONG64 _reserved2		: 5;
		ULONG64 memoryTypeWB		: 1;
		ULONG64 _reserved3		: 1;
		ULONG64 twoMBPages		: 1;
		ULONG64 oneGBPages		: 1;
		ULONG64 _reserved4		: 2;
		ULONG64 invept			: 1;
		ULONG64 dirty			: 1;
		ULONG64 advEPTVMExit		: 1;
		ULONG64 shadowStackCtrl		: 1;
		ULONG64 _reserved5		: 1;
		ULONG64 inveptSingle		: 1;
		ULONG64 inveptAll		: 1;
		ULONG64 invvpid			: 1;
		ULONG64 _reserved6		: 7;
		ULONG64 invvpidIndividualAddr	: 1;
		ULONG64 invvpidSingleCtx	: 1;
		ULONG64 invvpidAllCtxs		: 1;
		ULONG64 invvpidAllCtxsRetainGlb	: 1;
		ULONG64 _rest			: 20;
	};
} msr_vmxEptVpidCap_t;

typedef union msr_mtrrDefType_u {
	ULONG64 value;
	struct {
		ULONG64 defMemoryType	: 3;
		ULONG64 _reserved1	: 7;
		ULONG64 fixedRangeMTRR	: 1;
		ULONG64 enabled		: 1;
		ULONG64 _rest		: 52;
	};
} msr_mtrrDefType_t;

#endif //__MSR_H