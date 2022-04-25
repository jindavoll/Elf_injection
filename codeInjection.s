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

	call callback

main1:
	;write to stdout
	pop rsi			;MSG
	mov rax, 1		;write
	mov rdi, rax	;destination index to rax (stdout)
	mov rdx, 23		;sizeof(MSG)
	syscall

	;exit
	;xor rdi, rdi
	;mov rax, 0x3c  ;60 --> exit
	;syscall

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
	;jmp rax
	;ret
	;call 0x4022e0
	;ret

	;du coup mov rax ret fonctionne --> du coup non mdr mais attention dans le cas du true il faut écrire l'opcode à la mano car dans le cas du false sinon on va avoir une boucle infini
	;dans le cas du false bien check la table des symbols et modifier le GETENV qui sert à rien y'a 3-4 section à modifier donc normal qu'il n'y ai pas accès à la got faut tout modifier des suites d'un appel
	;

	callback:
	call main1
	db "Je suis trop un hacker" ;db peut faire un segfault si exec par le programme donc on le met après le call pour qu'il soit évalué par le compilo mais pas exec
