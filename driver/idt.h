#ifndef __IDT_H
#define __IDT_H

#define IDT_NMI	0x2

typedef union idt_entry_u {
	__m128 value;
	struct {
		ULONG64 offsetLow		: 16;
		ULONG64 segmentSelector	: 16;
		ULONG64 ist				: 3;
		ULONG64 _reserved		: 5;
		ULONG64 gateType		: 5;
		ULONG64 dpl				: 2;
		ULONG64 present			: 1;
		ULONG64 offsetMid		: 16;
		ULONG64 offsetHigh		: 32;
		ULONG64 _rest			: 32;
	};
} idt_entry_t;

typedef union idt_addr_u {
	void *addr;
	struct {
		ULONG64 offsetLow : 16;
		ULONG64 offsetMid : 16;
		ULONG64 offsetHigh : 32;
	};
} idt_addr_t;

idt_entry_t *idt;

BOOLEAN nrot_idt_init();

#endif //__IDT_H