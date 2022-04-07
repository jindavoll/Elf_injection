BITS 64

SECTION .text 
global main

main:
	; save context 
	push rax
	push rcx
	push rdx
	push rsi
	push rdi
	push r11

	;fill here later

	;load context
	pop r11
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax

	;return 
	ret