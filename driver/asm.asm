EXTERN root_idt_nmiHandler:PROC
EXTERN root_vmx_vmexit:PROC
EXTERN eror_vmx_vmresumeError:PROC

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

root_asm_vmexit PROC
	push r15
	push r14
	push r13
	push r12
	push r11
	push r10
	push r9
	push r8
	push rdi
	push rsi
	push rbp
	push rbp
	push rbx
	push rdx
	push rcx
	push rax

	mov rcx, rsp

	sub rsp, 20h
	call root_vmx_vmexit
	add rsp, 20h

	test rax, rax
	jz vmoff

	pop rax
	pop rcx
	pop rdx
	pop rbx
	pop rbp
	pop rbp
	pop rsi
	pop rdi
	pop r8
	pop r9
	pop r10
	pop r11
	pop r12
	pop r13
	pop r14
	pop r15

	vmresume
	jmp eror_vmx_vmresumeError

vmoff:
	mov rax, 06820h		; VMCS_GUEST_RFLAGS
	vmread rax, rax
	push rax

	mov rax, 0681Ch		; VMCS_GUEST_RSP
	vmread rax, rax
	push rax

	mov rax, 0681Eh		; VMCS_GUEST_RIP
	vmread rax, rax

	vmxoff

	mov rbx, [rsp + 88h]
	mov rcx, [rsp]
	mov [rsp + 88h], rcx
	mov [rsp], rbx

	mov rcx, rsp
	mov rbx, [rsp + 88h]
	mov rsp, rbx
	push rax
	mov rsp, rcx
	sub rbx, 8
	mov [rsp + 88h], rbx

	pop r15
	popfq
	pop rax
	pop rcx
	pop rdx
	pop rbx
	pop rbp
	pop rbp
	pop rsi
	pop rdi
	pop r8
	pop r9
	pop r10
	pop r11
	pop r12
	pop r13
	pop r14

	pop rsp
	ret
root_asm_vmexit ENDP

both_asm_vmcall PROC
	vmcall
	ret
both_asm_vmcall ENDP

root_asm_invept PROC
	invept rcx, oword ptr [rdx]
	ret
root_asm_invept ENDP

both_asm_setGdt PROC
	lgdt fword ptr [rcx]
	ret
both_asm_setGdt ENDP

both_asm_getGdt PROC
	sgdt fword ptr [rcx]
	ret
both_asm_getGdt ENDP

both_asm_getEs PROC
	mov rax, es
	ret
both_asm_getEs ENDP

both_asm_getCs PROC
	mov rax, cs
	ret
both_asm_getCs ENDP

both_asm_getSs PROC
	mov rax, ss
	ret
both_asm_getSs ENDP

both_asm_getDs PROC
	mov rax, ds
	ret
both_asm_getDs ENDP

both_asm_getFs PROC
	mov rax, fs
	ret
both_asm_getFs ENDP

both_asm_getGs PROC
	mov rax, gs
	ret
both_asm_getGs ENDP

both_asm_getLdtr PROC
	sldt rax
	ret
both_asm_getLdtr ENDP

both_asm_getTr PROC
	str rax
	ret
both_asm_getTr ENDP

both_asm_getRflags PROC
	pushfq
	pop rax
	ret
both_asm_getRflags ENDP

end