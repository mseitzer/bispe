/***************************************************************************
 * asm_debug_macros.S
 *
 * Copyright (C) 2014-2016	Max Seitzer <maximilian.seitzer@fau.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307 USA.
 *
 ***************************************************************************/

/***************************************************************************
 *				DEBUG SECTION
 **************************************************************************/

dbg_str_nop:		.string "nop\n"
dbg_str_finish:		.string "finishing\n"
dbg_str_push:		.string "pushed to stack: %08x\n"
dbg_str_print:		.string "read from stack: %08x\n"
dbg_str_load:		.string "loaded variable %08x from %08x\n"
dbg_str_store:		.string "stored variable %08x to %08x\n"
dbg_str_add:		.string "added to: %08x\n"
dbg_str_sub:		.string "subtracted to: %08x\n"
dbg_str_mul:		.string "multiplied to: %08x\n"
dbg_str_div:		.string "divided to: %08x\n"
dbg_str_mod:		.string "modulo result: %08x\n"
dbg_str_jmp:		.string "jumped to %p\n"
dbg_str_cmp_ne:		.string "cmp false, no jump\n"
dbg_str_call:		.string "called function at %p, rip=%p\n"
dbg_str_ret:		.string "returning to %p\n"
dbg_str_prolog:		.string "prolog: increased call pointer by %08x\n"
dbg_str_epilog:		.string "epilog: decreased call pointer by %08x\n"
dbg_str_argload:	.string "loaded argument %08x (pos: %d) to call stack\n"

dbg_format_dword:	.string "%08x\n"
dbg_format_qword:	.string "%016lx\n"
dbg_format_dqword:	.string "%016lx %016lx\n"

.lcomm				print_buf,16

.macro	save_regs
	push	%rax
	push	%rcx
	push	%rdx
	push	%rdi
	push	%rsi
	push	%r8
	push	%r9
	push	%r10
	push	%r11
.endm

.macro	restore_regs
	pop		%r11
	pop		%r10
	pop		%r9
	pop		%r8
	pop		%rsi
	pop		%rdi
	pop		%rdx
	pop		%rcx
	pop		%rax
.endm

.macro	print_str string
	save_regs
	lea		\string(%rip),%rdi
	call	printk
	restore_regs
.endm

.macro	print_str1 string, arg
	save_regs
	mov		\arg,%rsi
	lea		\string(%rip),%rdi
	call	printk
	restore_regs
.endm

.macro	print_str2 string, arg1, arg2
	save_regs
	mov		\arg2,%rdx
	mov		\arg1,%rsi
	lea		\string(%rip),%rdi
	call	printk
	restore_regs
.endm

.macro	print_reg32 reg
	save_regs
	movl	\reg,print_buf(%rip)
	movl	print_buf(%rip),%esi
	lea		dbg_format_dword(%rip),%rdi
	call	printk
	restore_regs
.endm

.macro	print_reg64 reg
	save_regs
	mov		\reg,print_buf(%rip)
	mov		print_buf(%rip),%rsi
	lea		dbg_format_qword(%rip),%rdi
	call	printk
	restore_regs
.endm

.macro	print_reg128 reg
	save_regs
	movdqu	\reg,print_buf(%rip)
	mov		$8,%rax
	mov		print_buf(,%rax,1),%rdx
	mov		print_buf(%rip),%rsi
	lea		dbg_format_dqword(%rip),%rdi
	call	printk
	restore_regs
.endm
