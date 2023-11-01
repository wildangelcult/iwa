#ifndef __ASM_H
#define __ASM_H

#pragma pack(push, 1)
typedef struct asm_descTable_s {
	UINT16 limit;
	ULONG64 base;
} asm_descTable_t;
#pragma pack(pop)

ULONG64 nrot_asm_vmlaunch();
void root_asm_idtNmiHandler();
void root_asm_vmexit();

ULONG64 both_asm_vmcall(ULONG64 param1, ULONG64 param2, ULONG64 param3);

void root_asm_invept(ULONG64 type, ept_invept_t *desc);

void both_asm_setGdt(asm_descTable_t *gdt);
void both_asm_getGdt(asm_descTable_t *gdt);
UINT16 both_asm_getEs();
UINT16 both_asm_getCs();
UINT16 both_asm_getSs();
UINT16 both_asm_getDs();
UINT16 both_asm_getFs();
UINT16 both_asm_getGs();
UINT16 both_asm_getLdtr();
UINT16 both_asm_getTr();
ULONG64 both_asm_getRflags();
#endif //__ASM_H