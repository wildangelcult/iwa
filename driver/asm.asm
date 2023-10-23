EXTERN root_idt_nmiHandler:PROC

.code

nrot_asm_vmlaunch PROC
	pushfq

	mov rcx, 0681Ch		; VMCS_GUEST_RSP
	vmwrite rcx, rsp

	mov rcx, 0681Eh		; VMCS_GUEST_RIP
	lea rdx, done
	vmwrite rcx, rdx
	vmlaunch

	mov rax, 0
	jmp finish

done:
	mov rax, 1
finish:
	popfq
	ret
nrot_asm_vmlaunch ENDP

root_asm_idtNmiHandler PROC
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	sub rsp, 20h

	call root_idt_nmiHandler

	add rsp, 20h
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp 
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax

	iretq
root_asm_idtNmiHandler ENDP

both_asm_getCs PROC
	mov rax, cs
	ret
both_asm_getCs ENDP

end