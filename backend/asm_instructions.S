/***************************************************************************
 * asm_instructions.S
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

/*
 * This is the last opcode which may be used. Do only change this value if 
 * new instructions are added.
 */
.set	maximum_opcode, 0x16

/***************************************************************************
 *				JUMP TABLE
 **************************************************************************/

.data

/* Table containing instructions addresses at offset specified by opcode */
.p2align 3,,7
instr_table:
	.quad	instr_nop	/* 0 */
	.quad	instr_finish

	/* data instructions */
	.quad	instr_push	/* 2 */
	.quad	instr_print	/* 3 */
	.quad	instr_load	/* 4 */
	.quad	instr_store	/* 5 */

	/* arithmetic instructions */
	.quad	instr_add	/* 6 */
	.quad	instr_sub
	.quad	instr_mul
	.quad	instr_div
	.quad	instr_mod

	/* control flow instructions */
	.quad	instr_jmp	/* 0xB */
	.quad	instr_jeq
	.quad	instr_jne
	.quad	instr_jl
	.quad	instr_jle
	.quad	instr_jg	/* 0x10 */
	.quad	instr_jge

	.quad	instr_call	/* 0x12 */
	.quad	instr_ret	/* 0x13 */
	.quad	instr_prolog	/* 0x14 */
	.quad	instr_epilog	/* 0x15 */

	.quad	instr_argload

/***************************************************************************
 *				INSTRUCTION MACROS
 **************************************************************************/

/*
 * increases instruction count by one instruction, finishes the cycle if 
 * instr_per_cycle instructions were processed this cycle
 */
.macro	inc_and_check_cycle_cnt
	add					$1,instr_cnt
	cmp					bispe_instr_per_cycle(%rip),instr_cnt
	je					bispe_cycle_outro
.endm

/*
 * checks if jump target is in bounds, does error handling if not
 * adds relative address to code segment base address
 * to get absolute address for jump otherwise
 * target: 64 bit register
 */
.macro	calc_jmp_target target
	shl		$2,\target /* multiply by 4 to get byte addressing */
	cmp		\target,bispe_code_seg_size(%rip)
	jbe		error_jmp_bounds

	add		bispe_code_seg_bp(%rip),\target
.endm

/*
 * increases instruction pointer, increases instruction count, 
 * checks for cycle end, goes to loop outro if so,
 * jumps to next instruction otherwise
 */
.macro	goto_next_instr
	inc_instr_ptr

	inc_and_check_cycle_cnt

	extr_next_instr		%edx
	jmp_through_table	%rdx
.endm

/***************************************************************************
 *				INTERPRETER INSTRUCTIONS
 **************************************************************************/

.text

/* nop instruction */
instr_nop:
#ifdef DEBUG
	print_str			dbg_str_nop
#endif
	goto_next_instr

/* sets finish flag and jumps to outro */
instr_finish:
#ifdef DEBUG
	print_str	dbg_str_finish
#endif

	/* set halt flag */
	or		$1,halt_flag(%rip)

	jmp		bispe_cycle_outro

/***************************************************************************
 *				DATA INSTRUCTIONS
 **************************************************************************/

/* pushes constant on stack */
instr_push:
	/* fetch constant from code */
	inc_instr_ptr
	extr_next_instr		%edx

	inc_stack_ptr
	ins_stack			%edx

#ifdef DEBUG
	print_str1			dbg_str_push,%rdx
#endif

	goto_next_instr

/* pops element from stack and writes it to print buffer (unencrypted) */
instr_print:
	extr_stack			%edx
	dec_stack_ptr

	/* write element to print buffer */
	mov					print_ptr(%rip),%rsi
	movl				%edx,0(%rsi)

	/* increase print pointer */
	add					$4,print_ptr(%rip)
	
	/* check if upper bound is violated */
	mov					bispe_print_seg_bp(%rip),%rsi
	add					bispe_print_seg_size(%rip),%rsi
	cmp					print_ptr(%rip),%rsi
	jg					1f

	/* end of buffer reached, set pointer to beginning */
	mov					bispe_print_seg_bp(%rip),%rsi
	mov					%rsi,print_ptr(%rip)

1:
#ifdef DEBUG
	print_str1			dbg_str_print,%rdx
#endif

	goto_next_instr

/* pushes variable with call pointer displacement specified by immediate on stack */
instr_load:
	/* fetch displacement from code */
	inc_instr_ptr
	extr_next_instr		%edx

	/* multiply by 4 to get byte addressing */
	shl					$2,%edx

	/* load element from call line by displacement */
	fetch_call_element	%rdx,%ecx

	/* move variable to operand stack */
	inc_stack_ptr
	ins_stack			%ecx

#ifdef DEBUG
	print_str2			dbg_str_load,%rcx,%rdx
#endif

	goto_next_instr

/* pops element from stack and saves it to call stack 
 * with call pointer displacement specified by immediate */
instr_store:
	/* fetch displacement from code */
	inc_instr_ptr
	extr_next_instr		%edx

	/* multiply by 4 to get byte addressing */
	shl					$2,%edx

	/* pop variable from stack */
	extr_stack			%ecx
	dec_stack_ptr

	/* store variable to call stack by displacement */
	save_call_element	%rdx,%ecx

#ifdef DEBUG
	print_str2			dbg_str_store,%rcx,%rdx
#endif

	goto_next_instr


/***************************************************************************
 *				ARITHMETIC INSTRUCTIONS
 **************************************************************************/

/* pops top two elements from stack, adds them and pushes result on stack */
instr_add:
	extr_stack			%edx
	dec_stack_ptr

	extr_stack			%ecx

	addl				%edx,%ecx

	ins_stack			%ecx

#ifdef DEBUG
	print_str1			dbg_str_add,%rcx
#endif

	goto_next_instr

/* pops top two elements from stack, subtracts them and pushes result on stack */
instr_sub:
	extr_stack			%edx
	dec_stack_ptr

	extr_stack			%ecx

	subl				%edx,%ecx

	ins_stack			%ecx

#ifdef DEBUG
	print_str1			dbg_str_sub,%rcx
#endif

	goto_next_instr

/* pops top two elements from stack, multiplies them and pushes result on stack */
instr_mul:
	extr_stack			%eax
	dec_stack_ptr

	extr_stack			%ecx

	mul					%ecx

	ins_stack			%eax

#ifdef DEBUG
	print_str1			dbg_str_mul,%rax
#endif

	goto_next_instr

/* pops top two elements from stack, divides them and pushes quotient on stack */
instr_div:
	extr_stack			%ecx
	dec_stack_ptr

	/* check for division through 0 */
	test				%ecx,%ecx
	je					error_div_zero

	extr_stack			%eax

	xor					%rdx,%rdx
	div					%ecx

	ins_stack			%eax

#ifdef DEBUG
	print_str1			dbg_str_div,%rax
#endif

	goto_next_instr

/* pops top two elements from stack, divides them and pushes remainder on stack */
instr_mod:
	extr_stack			%ecx
	dec_stack_ptr

	/* check for division through 0 */
	test				%ecx,%ecx
	je					error_div_zero

	extr_stack			%eax

	xor					%rdx,%rdx
	div					%ecx

	ins_stack			%edx

#ifdef DEBUG
	print_str1			dbg_str_mod,%rdx
#endif

	goto_next_instr	

/***************************************************************************
 *				CONTROL FLOW INSTRUCTIONS
 **************************************************************************/

/* sets instruction pointer to jump target */
instr_jmp:
	/* fetch jump target from code */
	inc_instr_ptr
	extr_next_instr		%edx

	calc_jmp_target		%rdx
	mov					%rdx,cur_instr_ptr

#ifdef DEBUG
	print_str1			dbg_str_jmp,%rdx
#endif

	fetch_instr_line

	inc_and_check_cycle_cnt
	extr_next_instr		%edx
	jmp_through_table	%rdx

/* pops top two elements from stack and jumps to target if equal */
instr_jeq:
	extr_stack			%edx
	dec_stack_ptr

	extr_stack			%ecx
	dec_stack_ptr

	cmp					%edx,%ecx
	je					instr_jmp

#ifdef DEBUG
	print_str			dbg_str_cmp_ne
#endif

	inc_instr_ptr
	goto_next_instr

/* pop top two elements from stack and jumps to target if not equal */
instr_jne:
	extr_stack			%edx
	dec_stack_ptr

	extr_stack			%ecx
	dec_stack_ptr

	cmp					%edx,%ecx
	jne					instr_jmp

#ifdef DEBUG
	print_str			dbg_str_cmp_ne
#endif

	inc_instr_ptr
	goto_next_instr

/* pop top two elements from stack and jumps to target if 
 * stack[stack_ptr-1] < stack[stack_ptr]
 */
instr_jl:
	extr_stack			%edx
	dec_stack_ptr

	extr_stack			%ecx
	dec_stack_ptr

	cmp					%edx,%ecx
	jl					instr_jmp

#ifdef DEBUG
	print_str			dbg_str_cmp_ne
#endif

	inc_instr_ptr
	goto_next_instr

/* pop top two elements from stack and jumps to target if 
 * stack[stack_ptr-1] <= stack[stack_ptr]
 */
instr_jle:
	extr_stack			%edx
	dec_stack_ptr

	extr_stack			%ecx
	dec_stack_ptr

	cmp					%edx,%ecx
	jle					instr_jmp

#ifdef DEBUG
	print_str			dbg_str_cmp_ne
#endif

	inc_instr_ptr
	goto_next_instr

/* pop top two elements from stack and jumps to target if 
 * stack[stack_ptr-1] > stack[stack_ptr]
 */
instr_jg:
	extr_stack			%edx
	dec_stack_ptr

	extr_stack			%ecx
	dec_stack_ptr

	cmp					%edx,%ecx
	jg					instr_jmp

#ifdef DEBUG
	print_str			dbg_str_cmp_ne
#endif

	inc_instr_ptr
	goto_next_instr

/* pop top two elements from stack and jumps to target if 
 * stack[stack_ptr-1] >= stack[stack_ptr]
 */
instr_jge:
	extr_stack			%edx
	dec_stack_ptr

	extr_stack			%ecx
	dec_stack_ptr

	cmp					%edx,%ecx
	jge					instr_jmp

#ifdef DEBUG
	print_str			dbg_str_cmp_ne
#endif

	inc_instr_ptr
	goto_next_instr

instr_call:
	/* fetch jump target from code */
	inc_instr_ptr
	extr_next_instr		%edx

	calc_jmp_target		%rdx

	/* calculate return address 
	 * rip = (cur_instr_ptr - code_bp) >> 2 + 1
	 */
	mov					cur_instr_ptr,%rax
	sub					bispe_code_seg_bp(%rip),%rax
	shr					$2,%rax
	inc					%rax	/* instruction after current instruction */

	/* push return address on call stack */

	/* increase call pointer by one */
	mov					$4,%rcx
	inc_call_ptr		%rcx
 
	/* store return address to top call line element 
     * (displacement == 0)
	 */
	xor					%rcx,%rcx
	save_call_element	%rcx,%eax

	/* change instruction pointer to jump target */
	mov					%rdx,cur_instr_ptr
	fetch_instr_line

#ifdef DEBUG
	print_str2			dbg_str_call,cur_instr_ptr,%rax
#endif

	/* execute next instruction */
	inc_and_check_cycle_cnt
	extr_next_instr		%edx
	jmp_through_table	%rdx

instr_ret:
	/* load return address from call stack 
	 * (displacement == 0)
	 */
	xor					%rcx,%rcx
	fetch_call_element	%rcx,%edx

	/* calculate jump target */
	calc_jmp_target		%rdx

#ifdef DEBUG
	print_str1			dbg_str_ret,%rdx
#endif

	/* decrease call pointer by one */
	mov					$4,%rcx
	dec_call_ptr		%rcx

	/* change instruction pointer to jump target */
	mov					%rdx,cur_instr_ptr
	fetch_instr_line

	/* execute next instruction */
	inc_and_check_cycle_cnt
	extr_next_instr		%edx
	jmp_through_table	%rdx


/* increases call pointer by amount from immediate data */
instr_prolog:
	/* fetch amount from code */
	inc_instr_ptr
	extr_next_instr		%edx

	/* multiply by 4 to get byte addressing */
	shl					$2,%edx

	inc_call_ptr		%rdx

#ifdef DEBUG
	print_str1			dbg_str_prolog,%rdx
#endif

	goto_next_instr

/* decreases call pointer by amount from immediate data */
instr_epilog:
	/* fetch amount from code */
	inc_instr_ptr
	extr_next_instr		%edx

	/* multiply by 4 to get byte addressing */
	shl					$2,%edx

	dec_call_ptr		%rdx

#ifdef DEBUG
	print_str1			dbg_str_epilog,%rdx
#endif

	goto_next_instr

/* 
 * Loads program argument to call stack. The position of the argument in argv
 * is specified by immediate data. Simultaneously, this is also the position
 * on the call stack the argument gets loaded to. This instruction is only called 
 * from pre-main context, and assumes that enough space is allocated on the 
 * call stack.
 */
instr_argload:
	/* fetch argument position from code */
	inc_instr_ptr
	extr_next_instr		%edx	

	/* check if argument is in range */
	cmp					bispe_argc(%rip),%rdx
	/* if(argc <= pos), jmp to error */
	jae					error_arg_range

	/* copy element from argv at offset %rdx to %ecx*/
	mov					bispe_argv(%rip),%rax
	mov					(%rax, %rdx, 4),%ecx

	/* multiply by 4 to get byte addressing */
	shl					$2,%edx

	/* store variable to call stack by displacement */
	save_call_element	%rdx,%ecx

#ifdef DEBUG
	print_str2			dbg_str_argload,%rcx,%rdx
#endif

	goto_next_instr
