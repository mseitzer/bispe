/***************************************************************************
 * bispe_cycle_asm.c
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

#include "bispe_defines.h"
#include "asm_state_macros.S"

/***************************************************************************
 *				DEBUG SECTION
 **************************************************************************/

#ifdef DEBUG
	#include "asm_debug_macros.S"
#endif

/***************************************************************************
 *				REGISTER ALIASES
 **************************************************************************/

/* register used for rip passing */
#ifdef RIP_PROTECT
.set	rrip,			%r9
#endif

/* holds amount of instructions processed in current cycle */
.set	instr_cnt,		%r10

/* hold current state pointers */
.set	cur_instr_ptr,	%r11
.set	cur_stack_ptr,	%r12
.set	cur_call_ptr,	%r13

/* holds pointer to currently loaded call line */
.set	cur_call_line,	%r14

/* register holding content to en-/decrypt */
.set	rstate,			%xmm0
/* helper register, gets spoiled from en-/decryption */
.set	rhelp,			%xmm1
/* helper register, does not get spoiled from en-/decryption */
.set	rhelp2,			%xmm2

.set	rcall_line,		%xmm4
.set	rstack_line,	%xmm5
.set	rinstr_line,	%xmm6

/***************************************************************************
 *				INTERPRETER DATA
 **************************************************************************/

.data

/* Interpreter state pointers, in memory */
instr_ptr:			.quad 0
stack_ptr:			.quad 0
call_ptr:			.quad 0
print_ptr:			.quad 0

/* Interpreter control flags, in memory */
halt_flag:			.byte 0
error_code:			.byte 0

/***************************************************************************
 *				HELPER MACROS
 **************************************************************************/

/*
 * Pushes all used callee saved registers on the stack 
 * (to be used in conjunction with restore_callee_regs)
 */
.macro	save_callee_regs
	push	%r12
	push	%r13
	push	%r14
.endm

/*
 * Restores all used callee saved register as saved by save_callee_regs
 */
.macro	restore_callee_regs
	pop		%r14
	pop		%r13
	pop		%r12
.endm

/*
 * checks if opcode is valid and jumps indirect through jump table if so
 * throws error otherwise
 * opcode: 64 bit register
 */
.macro	jmp_through_table opcode
	cmp			$maximum_opcode,\opcode
	jg			error_inv_opcode

	jmp			*instr_table(,%rdx,8)
.endm

/***************************************************************************
 *				INTERPRETER INSTRUCTIONS
 **************************************************************************/

#include "asm_instructions.S"

/***************************************************************************
 *				ERROR HANDLING
 **************************************************************************/

#include "asm_error.S"

/***************************************************************************
 *				EXPORTED FUNCTIONS
 **************************************************************************/

.text

.globl	bispe_set_instr_ptr
.globl	bispe_set_stack_ptr
.globl	bispe_set_call_ptr
.globl	bispe_set_print_ptr
.globl	bispe_get_print_ptr

.globl	bispe_chk_halt
.globl	bispe_chk_error
.globl	bispe_get_error
.globl	bispe_reset_flags
.globl	bispe_cycle_entry

bispe_set_instr_ptr:
	mov	%rdi, instr_ptr(%rip)
	retq

bispe_set_stack_ptr:
	mov	%rdi, stack_ptr(%rip)
	retq

bispe_set_call_ptr:
	mov	%rdi, call_ptr(%rip)
	retq

bispe_set_print_ptr:
	mov	%rdi, print_ptr(%rip)
	retq

bispe_get_print_ptr:
	mov	print_ptr(%rip),%rax
	retq

/* returns halt flag */
bispe_chk_halt:
	xor		%rax,%rax
	andb	$1,halt_flag(%rip)
	/* return 1 if result of 'and' was not zero (halt flag is set) */
	setne	%al
	retq

/* returns error flag */
bispe_chk_error:
	xor		%rax,%rax
	andb	$0xFF,error_code(%rip)
	/* return 1 if result of 'and' was not zero (an error code is set) */
	setne	%al
	retq

/* returns error code */
bispe_get_error:
	xor		%rax,%rax
	movb	error_code(%rip),%al
	retq

bispe_reset_flags:
	movb	$0,halt_flag(%rip)
	movb	$0,error_code(%rip)
	retq

/* entry point to instruction cycle */
bispe_cycle_entry:
	save_callee_regs

	/* set instruction count to 0 */
	xor					instr_cnt,instr_cnt

	load_state_ptrs
	
	fetch_instr_line
	fetch_stack_line

	/* initially fetch top call line */
	mov					cur_call_ptr,%rdx
	align_ptr			%rdx
	force_call_line_fetch %rdx

	extr_next_instr		%edx
	jmp_through_table	%rdx

/* transfers control back to interpreter */
bispe_cycle_outro:
	/* save state to memory */
	save_stack_line
	save_call_line
	save_state_ptrs

	restore_callee_regs
	retq
