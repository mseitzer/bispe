/***************************************************************************
 * asm_state_macros.S
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
 * This file consists only of macros. In reality, this file is content of
 * bispe_instructions_asm.S, but to avoid oversizing that file, 
 * these logically all related macros were split off.
 * Generally, these macros deal with changing the program state.
 * These macros also call the crypto functions in bispe_crypto_asm.S
 *
 * On a technical notice, these macros are freely allowed to use the 
 *  rdi, rsi, r8 
 * registers for their own purpose. This means that these registers 
 * should be regarded "callee-save" by anyone who uses these macros.
 * They may also overwrite the XMM-registers named as
 *  rhelp, rhelp2, rstate.
 * The macros use registers of different size as arguments.
 * This size is specified in the commentary above every macro.
 */

#define ALIGNMENT 0xFFFFFFFFFFFFFFF0

/*
 * Aligns a pointer in a register to 128 bit
 * reg: 64 bit register
 */
.macro	align_ptr reg
	and	$ALIGNMENT,\reg
.endm

/*
 * Calculates offset in bytes between pointer and the 128 bit aligned pointer
 * src, dest: 64 bit register
 */
.macro	ofs_from_ptr src, dest
	.if (\src != \dest)
	mov	\src,\dest
	.endif
	and	$0xC,\dest
.endm

/*
 * Inserts dword from source register into destination register
 * at position specified by offset register
 * src: 32 bit register
 * dest: 128 bit register
 * ofs: 64 bit register
 *
 * NOTE: This is the original version of insert, which uses a 
 * cmp/jmp cascade. For now, it seems to be actually faster than 
 * the version without jumps, so for now, we will keep it.
 */
.macro	ins_by_ofs src, dest, ofs
	test		\ofs,\ofs
	jne			1f

	vpinsrd		$0,\src,\dest,\dest
	jmp			4f

1:	cmp			$4,\ofs
	jne			2f
	vpinsrd		$1,\src,\dest,\dest
	jmp			4f

2:	cmp			$8,\ofs
	jne			3f
	vpinsrd		$2,\src,\dest,\dest
	jmp			4f

3:	vpinsrd		$3,\src,\dest,\dest

4:
.endm

/*
 * Inserts dword from source register into destination register
 * at position specified by offset register
 * src: 32 bit register
 * dest: 128 bit register
 * ofs: 64 bit register
 *
 * NOTE: AVX specifies no instructions that allow a variable length 
 * bitshift, or an dword insert by variable offset.
 * To avoid a slow cmp/jmp cascade with the VPINSRD instruction,
 * this relatively complicated procedure was chosen.
 * The source dword is blended to the destination register.
 * The right position to blend is chosen by the mask generated from the offset
 */
.macro	ins_by_ofs_without_jmp src, dest, ofs
	/* redefine for readability */
	.set	rmask, rhelp2

	/* create shift amount from offset */
	vmovq		\ofs,rhelp
	vpslld		$2,rhelp,rhelp		/* multiply offset by 4 */

	/* build blend mask */
	mov			$0x8000,%r8			/* set 15th bit */
	vmovq		%r8,rmask
	vpsllq		rhelp,rmask,rmask	/* shift by rhelp */

	vpunpcklwd	rmask,rmask,rmask	/* interleave values over 128 bits */

	/* load src dword to every dword in rhelp */
	vmovd		\src,rhelp
	vshufps		$0,rhelp,rhelp,rhelp

	/* change exactly the dword specified by ofs to the src dword */
	vblendvps	rmask,rhelp,\dest,\dest
.endm

/*
 * Extracts dword from source register at position specified 
 * by offset register into the destination register
 * src: 128 bit register
 * ofs: 64 bit register
 * dest: 32 bit register
 */
.macro	extr_by_ofs src, ofs, dest
	vmovq		\ofs,rhelp
	vpsrld		$2,rhelp,rhelp		/* shift offset right by 2 */
	vpermilps	rhelp,\src,rhelp	/* move dword from src to rhelp */
	vmovd		rhelp,\dest
.endm

/*
 * Loads all state pointers from memory to their corresponding state register
 */
.macro	load_state_ptrs
	mov		instr_ptr(%rip),cur_instr_ptr
	mov		stack_ptr(%rip),cur_stack_ptr
	mov		call_ptr(%rip),cur_call_ptr
.endm

/*
 * Saves all state pointers from registers to memory
 */
.macro	save_state_ptrs
	mov		cur_instr_ptr,instr_ptr(%rip)
	mov		cur_stack_ptr,stack_ptr(%rip)
	mov		cur_call_ptr,call_ptr(%rip)
.endm

/***************************************************************************
 *				CRYPTO WRAPPER MACROS
 **************************************************************************/

/*
 * This macro just calls bispe_encblk from the crypto module
 * It may protect the RIP by passing it in a register
 */
.macro	encblk
#ifdef RIP_PROTECT
	lea				5(%rip),rrip
	jmp				bispe_encblk
#else
	call			bispe_encblk
#endif
.endm

/*
 * This macro just calls bispe_encblk from the crypto module
 * It may protect the RIP by passing it in a register
 */
.macro	decblk
#ifdef RIP_PROTECT
	lea				5(%rip),rrip
	jmp				bispe_decblk
#else
	call			bispe_decblk
#endif
.endm

/*
 * Decrypts 128 bit from memory in CBC mode and moves them to register
 * src: 64 bit register containing pointer to 128 bit memory location
 * dest: 128 bit register
 */
.macro	decrypt_memory_cbc src,dest
	vmovdqa			0(\src),rstate
#ifdef ENCRYPTION
	decblk
	vpxor			-16(\src),rstate,rstate
#endif
	vmovdqa			rstate,\dest
.endm

/*
 * Encrypts source register in CBC mode and moves it to memory location
 * src: 128 bit register
 * dest: 64 bit register containing pointer to 128 bit memory location
 */
.macro	encrypt_reg_cbc src,dest
	vmovdqa			\src,rstate
#ifdef ENCRYPTION
	vpxor			-16(\dest),rstate,rstate
	encblk
#endif

	vmovdqa			rstate,0(\dest)
.endm

/*
 * Encrypts source register in CBC mode and moves it to memory location.
 * Every block after the destination location up to the end location 
 * gets reencrypted in CBC mode.
 * This is significantly more expensive than the encrypt_reg_cbc operation
 * src: 128 bit register
 * dest: 64 bit register containing pointer to 128 bit memory location, gets spoiled
 * end: 64 bit register containing pointer to 128 bit memory location
 *
 * NOTE: the jump labels 8, 9 are chosen to not collide with labels chosen
 * by macros including this macro
 */
.macro	encrypt_reg_cbc_chain src,dest,end

#ifdef ENCRYPTION
	vmovdqa			\src,rstate

	/* xor with previous block */
	vpxor			-16(\dest),rstate,rstate

	/* encrypt new block */
	encblk

/* encrypt loop */
8:
	/* check if at last block in chain */
	cmp				\dest,\end
	je				9f

	/* save encrypted new block temporarly */
	vmovdqa			rstate,rhelp2
	
	/* move dest pointer to next block */
	add				$16,\dest

	/* fetch next block to memory */
	vmovdqa			0(\dest),rstate

	/* decrypt next block */
	decblk

	/* xor with previous old block */
	vpxor			-16(\dest),rstate,rstate
	/* xor with previous new block */
	vpxor			rhelp2,rstate,rstate

	/* encrypt new block */
	encblk

	/* copy previous new block to memory */
	vmovdqa			rhelp2,-16(\dest)

	/* jump to loop condition */
	jmp				8b

9:
	/* move last encrypted block to memory */
	vmovdqa			rstate,0(\dest)
#else

	/* without encryption, just copy src to memory */
	vmovdqa			\src,0(\dest)
#endif /* ENCRYPTION */

.endm

/*
 * Decrypts 128 bit from memory in ECB mode and moves them to register
 * src: 64 bit register containing pointer to 128 bit memory location
 * dest: 128 bit register
 */
.macro	decrypt_memory_ecb src,dest
	vmovdqa			0(\src),rstate
#ifdef ENCRYPTION
	decblk
#endif	
	vmovdqa			rstate,\dest
.endm

/*
 * Encrypts source register in ECB mode and moves it memory location
 * src: 128 bit register
 * dest: 64 bit register containing pointer to 128 bit memory location
 */
.macro	encrypt_reg_ecb src,dest
	vmovdqa			\src,rstate
#ifdef ENCRYPTION
	encblk
#endif
	vmovdqa			rstate,0(\dest)
.endm

/***************************************************************************
 *				INSTRUCTION LINE MODIFYING
 **************************************************************************/

/*
 * Extracts current instruction from instruction line to destination register
 * dest: 32 bit register
 */
.macro	extr_next_instr dest
	ofs_from_ptr	cur_instr_ptr,%r8
	extr_by_ofs		rinstr_line,%r8,\dest
.endm

/* 
 * Fetches next instruction line from memory to instruction register
 */
.macro	fetch_instr_line
	/* calculate line pointer */
	mov				cur_instr_ptr,%rdi
	align_ptr		%rdi

	/* decrypt line from memory to instruction line */
	decrypt_memory_cbc	%rdi,rinstr_line
.endm

/*
 * Increases instruction pointer by one.
 * Fetches next instruction line if necessary
 */
.macro	inc_instr_ptr
	ofs_from_ptr	cur_instr_ptr,%rsi

	/* increase instruction pointer by 4 byte */
	add				$4,cur_instr_ptr

	cmp				$12,%rsi
	jne				1f /* if (ofs != 12), jmp */

	/* instruction line is finished, load next one
	 * decrypt new line from memory to instruction line
	 */
	decrypt_memory_cbc	cur_instr_ptr,rinstr_line
1:
.endm

/***************************************************************************
 *				STACK LINE MODIFYING
 **************************************************************************/

/*
 * Inserts source register at current stack pointer position
 * src: 32 bit register
 */
.macro	ins_stack src
	ofs_from_ptr	cur_stack_ptr,%r8
	ins_by_ofs		\src,rstack_line,%r8
.endm

/*
 * Extracts top element from stack line to destination register
 * dest: 32 bit register
 */
.macro	extr_stack dest
	ofs_from_ptr	cur_stack_ptr,%r8
	extr_by_ofs		rstack_line,%r8,\dest
.endm

/* 
 * Fetches stack line from memory to stack register
 */
.macro	fetch_stack_line
	mov				cur_stack_ptr,%rdi
	align_ptr		%rdi

	/* decrypt line from memory to stack line */
	decrypt_memory_cbc	%rdi,rstack_line
.endm

/*
 * Writes stack line from register to memory
 */
.macro	save_stack_line
	mov				cur_stack_ptr,%rdi

	/* encrypt stack line and write it to memory */
	align_ptr		%rdi
	encrypt_reg_cbc	rstack_line,%rdi
.endm

/*
 * Increases stack pointer by one. Saves old stack line if necessary
 */
.macro	inc_stack_ptr
	/* load line pointer and offset */
	mov				cur_stack_ptr,%rdi
	align_ptr		%rdi
	ofs_from_ptr	cur_stack_ptr,%rsi

	/* increase stack pointer by 4 byte */
	add				$4,cur_stack_ptr

	cmp				$12,%rsi
	jne				1f	/* if (ofs != 12), jmp */

	/* stack line is finished
	 * save old stack line to memory 
	 */
	encrypt_reg_cbc	rstack_line,%rdi

	/* check if stack upper bound is violated */
	mov				bispe_stack_seg_bp(%rip),%rsi
	add				bispe_stack_seg_size(%rip),%rsi
	cmp				cur_stack_ptr,%rsi
	/* if (cur_stack_ptr >= stack_bp + stack_size), jmp to error */
	jbe				error_stack_overflow
1:
.endm

/*
 * Decreases stack pointer by one. Fetches previous stack line if necessary
 */
.macro	dec_stack_ptr
	ofs_from_ptr	cur_stack_ptr,%rsi

	/* decrease stack pointer by 4 byte */
	sub				$4,cur_stack_ptr

	test			%rsi,%rsi
	jne				1f	/* if (ofs != 0), jmp */

	/* at the beginning of stack line; load previous one */
	/* check if stack lower bound is violated */
	cmp				cur_stack_ptr,bispe_stack_seg_bp(%rip)
	/* if (stack_bp > stack_ptr), jmp to error */
	ja				error_stack_underflow

	/* decrypt new line from memory to stack line */
	mov				cur_stack_ptr,%rdi
	align_ptr		%rdi
	decrypt_memory_cbc	%rdi,rstack_line
1:
.endm

/***************************************************************************
 *				CALL LINE MODIFYING
 **************************************************************************/

/* 
 * Fetches target call line from memory to register if necessary.
 * Encrypts old call line to memory if a new line has to be fetched.
 * target_line_ptr: 64 bit register
 */
.macro	fetch_call_line target_line_ptr
	/* check if this line is already loaded to rcall */
	cmp				cur_call_line,\target_line_ptr
	je				1f	/* line is already loaded, nothing to do */

	/* line is not yet loaded, encrypt old line, load new line */

	/* check if lower bound is violated */
	cmp				\target_line_ptr,bispe_call_seg_bp(%rip)
	/* if (line_ptr < call_seg_bp), jmp */
	ja				error_call_underflow

	/* fetch end pointer */
	mov				cur_call_ptr,%r8
	align_ptr		%r8

	/* encrypt old call line to memory */
	encrypt_reg_cbc_chain rcall_line,cur_call_line,%r8

	/* decrypt new line from memory to stack line */
	decrypt_memory_cbc	\target_line_ptr,rcall_line

	/* update current line pointer */
	mov				\target_line_ptr,cur_call_line
1:
.endm

/*
 * Fetches an element from call stack to destination register,
 * with displacement relative to call pointer.
 * displacement: 64 bit register
 * dest: 32 bit register
 */
.macro	fetch_call_element displacement,dest
	/* calculate target pointer from displacement */
	mov				cur_call_ptr,%rdi
	sub				\displacement,%rdi

	/* save offset from target pointer */
	ofs_from_ptr	%rdi,%rsi

	/* get target line pointer */
	align_ptr		%rdi

	/* make sure target line is loaded */
	fetch_call_line	%rdi

	/* extract element to destination */
	extr_by_ofs		rcall_line,%rsi,\dest
.endm

/*
 * Writes an element to call line, with displacement relative to call pointer
 * Loads line first if necessary.
 * displacement: 64 bit register
 * src: 32 bit register
 */
.macro	save_call_element displacement,src
	/* calculate target pointer from displacement */
	mov				cur_call_ptr,%rdi
	sub				\displacement,%rdi

	/* save offset from target pointer */
	ofs_from_ptr	%rdi,%rsi

	/* get target line pointer */
	align_ptr		%rdi

	/* make sure target line is loaded */
	fetch_call_line	%rdi

	/* insert element in call line */
	ins_by_ofs		\src,rcall_line,%rsi
.endm

/*
 * Encrypts call line to memory, at position specified by cur_call_line pointer
 */
.macro	save_call_line
	/* calculate pointer to end of call stack */
	mov				cur_call_ptr,%rsi
	align_ptr		%rsi
	/* encrypt call line to memory */
	mov				cur_call_line,%rdi
	encrypt_reg_cbc_chain rcall_line,%rdi,%rsi
.endm

/* 
 * Fetches target call line from memory to register,
 * regardless of currently fetched call line
 * Currently fetched call line will be overwritten!
 * target_line_ptr: 64 bit register
 */
.macro	force_call_line_fetch target_line_ptr
	/* check if lower bound is violated */
	cmp				\target_line_ptr,bispe_call_seg_bp(%rip)
	/* if (line_ptr < call_seg_bp), jmp to error */
	ja				error_call_underflow

	/* decrypt new line from memory to stack line */
	decrypt_memory_cbc	\target_line_ptr,rcall_line

	/* update current line pointer */
	mov				\target_line_ptr,cur_call_line
.endm

/*
 * Increases call pointer by specified amount
 * amount: 64 bit register
 */
.macro	inc_call_ptr amount
	/* calculate new call pointer */
	add				\amount,cur_call_ptr

	/* check if upper bound is violated */
	mov				bispe_call_seg_bp(%rip),%rsi
	add				bispe_call_seg_size(%rip),%rsi

	cmp				cur_call_ptr,%rsi
	/* if (target_ptr >= call_bp+call_size), jmp to error */
	jbe				error_call_overflow
.endm

/*
 * Decreases call pointer by specified amount
 * Because the call line must always lie in a valid memory location 
 * (in respect to call pointer), the call line is flushed to memory
 * and a new call line is loaded instead.
 * amount: 64 bit register
 */
.macro	dec_call_ptr amount
	/* save old call pointer */
	mov				cur_call_ptr,%rdi

	/* calculate new call pointer */
	sub				\amount,cur_call_ptr

	/* check if lower bound is violated */
	cmp				cur_call_ptr,bispe_call_seg_bp(%rip)
	/* if (line_ptr < call_bp), jmp to error */
	ja				error_call_underflow

	/* check if call line is still valid */
	cmp				cur_call_ptr,cur_call_line
	jle				1f

	/* calculate pointer to end of old call stack */
	align_ptr		%rdi

	/* encrypt old call line to memory */
	encrypt_reg_cbc_chain rcall_line,cur_call_line,%rdi

	/* load the new call line */
	mov				cur_call_ptr,cur_call_line
	align_ptr		cur_call_line
	decrypt_memory_cbc	cur_call_line,rcall_line
1:
.endm
