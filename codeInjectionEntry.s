BITS 64


SECTION .text 
global main
	;extern _printf

main:
	; save context 
	push rax
	push rcx
	push rdx
	push rsi
	push rdi
	push r11

	call callback

main1:
	;write to stdout
	pop rsi			;MSG
	;mov rsi, message
	mov rax, 1		;write
	mov rdi, rax	;destination index to rax (stdout)
	mov rdx, 23		;sizeof(MSG)
	syscall


	;load context
	pop r11
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax

	;return 
	;ret

	mov rax, 0x4022e0
	jmp rax

callback:
	pop rsi
	call main1
	db "Je suis trop un hacker", 0xa;db peut faire un segfault si exec par le programme donc on le met après le call pour qu'il soit évalué par le compilo mais pas exec
