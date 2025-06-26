section .data

section .text
global add_three_integers
global main
global _start

_start:

add_three_integers:
	push rbp
	mov rbp, rsp
	sub rsp, 32
	mov eax, edi
	mov ebx, esi
	mov r8, edx
	mov r9, rcx
	mov r10, r8
	mov r11, r9
	mov r12, (null)
	mov r13, dword [rbp - 0]
	mov r14, dword [rbp - 0]
	add r13, r14
	mov r13, dword [rbp - 0]
	add r13, r13
	mov r13, dword [rbp - 0]
	add r13, r13
	mov r13, dword [rbp - 0]
	add r13, r13
	mov r13, dword [rbp - 0]
	add r13, r13
	mov r13, dword [rbp - 0]
	add r13, r13
	mov eax, r13
	leave
	ret

main:
	push rbp
	mov rbp, rsp
	sub rsp, 16
	mov dword [rbp - 4], 10
	mov dword [rbp - 8], 20
	mov dword [rbp - 12], 5
	mov r13, dword [rbp - 4]
	mov edi, r13
	mov r14, dword [rbp - 8]
	mov esi, r14
	mov r15, dword [rbp - 12]
