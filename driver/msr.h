#ifndef __MSR_H
#define __MSR_H

#define MSR_FEATURE_CONTROL			0x3A
#define MSR_MTRRCAP					0xFE
#define MSR_SYSENTER_CS				0x174
#define MSR_SYSENTER_ESP			0x175
#define MSR_SYSENTER_EIP			0x176
#define MSR_DEBUGCTL				0x1D9
#define MSR_MTRR_PHYSBASE0			0x200
#define MSR_MTRR_PHYSMASK0			0x201
#define MSR_MTRR_DEF_TYPE			0x2FF

#define MSR_VMX_BASIC				0x480
#define MSR_VMX_PINBASED_CTLS		0x481
#define MSR_VMX_PROCBASED_CTLS		0x482
#define MSR_VMX_EXIT_CTLS			0x483
#define MSR_VMX_ENTRY_CTLS			0x484
#define MSR_VMX_PROCBASED_CTLS2		0x48B
#define MSR_VMX_EPT_VPID_CAP		0x48C
#define MSR_VMX_TRUE_PINBASED_CTLS	0x48D
#define MSR_VMX_TRUE_PROCBASED_CTLS	0x48E
#define MSR_VMX_TRUE_EXIT_CTLS		0x48F
#define MSR_VMX_TRUE_ENTRY_CTLS		0x490
#define MSR_FS_BASE					0xC0000100
#define MSR_GS_BASE					0xC0000101

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
		ULONG64 execOnly				: 1;
		ULONG64 _reserved1				: 5;
		ULONG64 pageWalk4				: 1;
		ULONG64 pageWalk5				: 1;
		ULONG64 memTypeUC				: 1;
		ULONG64 _reserved2				: 5;
		ULONG64 memTypeWB				: 1;
		ULONG64 _reserved3				: 1;
		ULONG64 twoMBPages				: 1;
		ULONG64 oneGBPages				: 1;
		ULONG64 _reserved4				: 2;
		ULONG64 invept					: 1;
		ULONG64 dirty					: 1;
		ULONG64 advEPTVMExit			: 1;
		ULONG64 shadowStackCtrl			: 1;
		ULONG64 _reserved5				: 1;
		ULONG64 inveptSingle			: 1;
		ULONG64 inveptAll				: 1;
		ULONG64 invvpid					: 1;
		ULONG64 _reserved6				: 7;
		ULONG64 invvpidIndividualAddr	: 1;
		ULONG64 invvpidSingleCtx		: 1;
		ULONG64 invvpidAllCtxs			: 1;
		ULONG64 invvpidAllCtxsRetainGlb	: 1;
		ULONG64 _rest					: 20;
	};
} msr_vmxEptVpidCap_t;

typedef union msr_mtrrDefType_u {
	ULONG64 value;
	struct {
		ULONG64 defMemType	: 3;
		ULONG64 _reserved1	: 7;
		ULONG64 fixedRangeMTRR	: 1;
		ULONG64 enabled		: 1;
		ULONG64 _rest		: 52;
	};
} msr_mtrrDefType_t;

typedef union msr_mtrrCap_u {
	ULONG64 value;
	struct {
		ULONG64 vcnt			: 8;
		ULONG64 fixedRangeSup	: 1;
		ULONG64 _reserved		: 1;
		ULONG64 WCSup			: 1;
		ULONG64 SMRRSup			: 1;
		ULONG64 PRMRRSup		: 1;
		ULONG64 _rest			: 51;
	};
} msr_mtrrCap_t;

typedef union msr_mtrrPhysBase_u {
	ULONG64 value;
	struct {
		ULONG64 memType		: 8;
		ULONG64 _reserved	: 4;
		ULONG64 pfn			: 36;
		ULONG64 _rest		: 16;
	};
} msr_mtrrPhysBase_t;

typedef union msr_mtrrPhysMask_u {
	ULONG64 value;
	struct {
		ULONG64 memType		: 8;
		ULONG64 _reserved	: 3;
		ULONG64 valid		: 1;
		ULONG64 pfn			: 36;
		ULONG64 _rest		: 16;
	};
} msr_mtrrPhysMask_t;

typedef union msr_vmxBasic_u {
	ULONG64 value;
	struct {
		ULONG64 revId			: 31;
		ULONG64 _reserved1		: 1;
		ULONG64	regionSize		: 13;
		ULONG64 _reserved2		: 3;
		ULONG64 physWidth		: 1;
		ULONG64 dualMonitor		: 1;
		ULONG64 memType			: 4;
		ULONG64 vmexitInsOuts	: 1;
		ULONG64 vmxCap			: 1;
		ULONG64 hwExErrorCode	: 1;
		ULONG64 _rest			: 7;
	};
} msr_vmxBasic_t;

#endif //__MSR_H