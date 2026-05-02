.intel_syntax noprefix

.equ MEM, 0x0700
.equ FREEBLK, 64
.equ CURCH_OFF, 0x049c
.equ ATL_OFF, 0x049d
.equ CVTFQ_OFF, 0x04ae
.equ EVQAL_OFF, 0x04b4
.equ NIL_PTR, 0x8000
.equ MEMORY_SIZE, 65536

.text

# This scaffold keeps the original register intent from IMPLEMENTATION_PLAN.md:
#   A  -> AL/AX
#   BC -> BX
#   DE -> DX
#   HL -> SI
# The later port can extend the OCR translation without changing calling
# conventions or the emulated memory layout established here.

.extern lisp_host_inch
.extern lisp_host_outc

.globl lisp_boot
.type lisp_boot, @function
lisp_boot:
  push r12
  push rdi
  xor eax, eax
  lea rdi, [rip + lisp_memory]
  mov ecx, MEMORY_SIZE / 8
  rep stosq

  lea r12, [rip + lisp_memory]
  mov si, MEM
  mov word ptr [r12 + CVTFQ_OFF], si
  mov byte ptr [r12 + MEM], 0xff
  mov byte ptr [r12 + MEM + 1], 0xff
  mov byte ptr [r12 + MEM + 2], 0x00
  mov byte ptr [r12 + MEM + 3], FREEBLK
  mov word ptr [r12 + EVQAL_OFF], NIL_PTR
  mov byte ptr [r12 + CURCH_OFF], 0x00
  mov byte ptr [r12 + ATL_OFF], 0x00
  pop rdi
  pop r12
  ret

.globl OUTC
.type OUTC, @function
OUTC:
  push rdx
  push rsi
  sub rsp, 8
  mov byte ptr [rsp], al
  movzx edi, byte ptr [rsp]
  call lisp_host_outc
  mov al, byte ptr [rsp]
  add rsp, 8
  pop rsi
  pop rdx
  ret

.globl CRLF
.type CRLF, @function
CRLF:
  push rax
  mov al, 10
  call OUTC
  pop rax
  ret

.globl INCH
.type INCH, @function
INCH:
  push rdx
  push rsi
  sub rsp, 8
  call lisp_host_inch
  add rsp, 8
  cmp eax, -1
  jne .Linch_done
  xor eax, eax
.Linch_done:
  and eax, 0xff
  pop rsi
  pop rdx
  ret

.globl lisp_memory_base
.type lisp_memory_base, @function
lisp_memory_base:
  lea rax, [rip + lisp_memory]
  ret

.globl lisp_outc
.type lisp_outc, @function
lisp_outc:
  mov al, dil
  jmp OUTC

.globl lisp_crlf
.type lisp_crlf, @function
lisp_crlf:
  jmp CRLF

.globl lisp_inch
.type lisp_inch, @function
lisp_inch:
  call INCH
  movzx eax, al
  ret

.bss
.align 32
.globl lisp_memory
lisp_memory:
  .zero MEMORY_SIZE

.section .note.GNU-stack, "", @progbits
