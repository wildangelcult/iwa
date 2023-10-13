#ifndef __EPT_H
#define __EPT_H

#define MEMTYPE_UC	0x0
#define MEMTYPE_WC	0x1
#define MEMTYPE_WT	0x4
#define MEMTYPE_WP	0x5
#define MEMTYPE_WB	0x6
#define MEMTYPE_INV	0xFF

#define EPT_PML_COUNT	512

typedef union ept_pml_u {
	ULONG64 value;
	struct {
		ULONG64 read		: 1;
		ULONG64 write		: 1;
		ULONG64 exec		: 1;
		ULONG64 _reserved1	: 5;
		ULONG64 accessed	: 1;
		ULONG64 _reserved2	: 1;
		ULONG64 userModeExec	: 1;
		ULONG64 _reserved3	: 1;
		ULONG64 pfn		: 36;
		ULONG64 _rest		: 16;
	};
} ept_pml_t;

typedef union ept_pml2e_u {
	ULONG64 value;
	struct {
		ULONG64 read		: 1;
		ULONG64 write		: 1;
		ULONG64 exec		: 1;
		ULONG64 memType		: 3;
		ULONG64 ignorePAT	: 1;
		ULONG64 largePage	: 1;
		ULONG64 accessed	: 1;
		ULONG64 dirty		: 1;
		ULONG64 userModeExec	: 1;
		ULONG64 _reserved1	: 10;
		ULONG64 pfn		: 27;
		ULONG64 _reserved2	: 15;
		ULONG64 suppressVe	: 1;
	};
} ept_pml2e_t;

typedef union ept_pml1e_u {
	ULONG64 value;
	struct {
		ULONG64 read		: 1;
		ULONG64 write		: 1;
		ULONG64 exec		: 1;
		ULONG64 memType		: 3;
		ULONG64 ignorePAT	: 1;
		ULONG64 _reserved1	: 1;
		ULONG64 accessed	: 1;
		ULONG64 dirty		: 1;
		ULONG64 userModeExec	: 1;
		ULONG64 _reserved2	: 1;
		ULONG64 pfn		: 36;
		ULONG64 _reserved3	: 15;
		ULONG64 suppressVe	: 1;
	};
} ept_pml1e_t;

typedef struct ept_pageTable_s {
	DECLSPEC_ALIGN(PAGE_SIZE) ept_pml_t pml4[EPT_PML_COUNT];
	DECLSPEC_ALIGN(PAGE_SIZE) ept_pml_t pml3[EPT_PML_COUNT];
	DECLSPEC_ALIGN(PAGE_SIZE) ept_pml2e_t pml2[EPT_PML_COUNT][EPT_PML_COUNT];
} ept_pageTable_t;

typedef union ept_eptp_u {
	ULONG64 value;
	struct {
		ULONG64 memType		: 3;
		ULONG64 pageWalk	: 3;
		ULONG64 dirty		: 1;
		ULONG64 _reserved	: 5;
		ULONG64 pfn		: 36;
		ULONG64 _rest		: 16;
	};
} ept_eptp_t;

typedef struct ept_ept_s {
	ept_pageTable_t *pageTable;
	ept_eptp_t eptp;
} ept_ept_t;

ept_ept_t *ept;

BOOLEAN nrot_ept_init();

#endif //__EPT_H