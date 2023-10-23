#ifndef __ASM_H
#define __ASM_H

#pragma pack(push, 1)
typedef struct asm_segDesc_s {
	UINT16 limit;
	ULONG64 base;
} asm_segDesc_t;
#pragma pack(pop)

ULONG64 nrot_asm_vmlaunch();
void root_asm_idtNmiHandler();

UINT16 both_asm_getCs();
#endif //__ASM_H