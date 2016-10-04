/***************************************************************************
 *
 * Cold boot resistant AES for 64-bit machines with AES-NI support
 * (currently all Core-i5/7 processors and some Core-i3)
 * Modified TRESOR code for use in BISPE
 *
 * Copyright (C) 2010		Tilo Mueller <tilo.mueller@informatik.uni-erlangen.de>
 * Copyright (C) 2012		Hans Spath <tresor@hans-spath.de>
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


/* 64-bit debug registers */
.set	db0,	%db0	/* round key 0a */
.set	db1,	%db1	/* round key 0b */
.set	db2,	%db2	/* round key 1a */
.set	db3,	%db3	/* round key 1b */

/* register used for rip passing */
#ifdef RIP_PROTECT
.set	rrip,	%r9
#endif

/* AVX registers */
.set	rstate,	%xmm0
.set	rhelp,	%xmm1
.set	rsrc,	%xmm2	/* only used during key schedule */
.set	rdest,	%xmm3	/* only used during key schedule */

.set	rk0,	%ymm8
.set	rk1,	%ymm9
.set	rk2,	%ymm10
.set	rk3,	%ymm11
.set	rk4,	%ymm12
.set	rk5,	%ymm13
.set	rk6,	%ymm14
.set	rk7,	%ymm15
.set	rk8,	%xmm8
.set	rk9,	%xmm9
.set	rk10,	%xmm10
.set	rk11,	%xmm11
.set	rk12,	%xmm12
.set	rk13,	%xmm13
.set	rk14,	%xmm14

/***************************************************************************
 *				MACROs
 ***************************************************************************/

/* load from rkey register to destination xmm register */
.macro	load_rkey src dest
	.if (\src <= 7)
	vextractf128	$1,rk\src,\dest
	.else
	vmovdqa			rk\src,\dest
	.endif
.endm

/* save from xmm register to rkey register */
.macro	save_rkey src dest
	.if (\dest <= 7)
	vinsertf128	$1,\src,rk\dest,rk\dest
	.else
	vinsertf128	$0,\src,%ymm\dest,%ymm\dest
	.endif
.endm

/* generate next round key */
.macro  key_schedule r0 r1 r2 rcon
	/* swap rsrc and rdest from previous runs if not first run */
	.if (\r0 > 0)
	vpxor				rsrc,rdest,rdest
	vpxor				rdest,rsrc,rsrc
	vpxor				rsrc,rdest,rdest
	.endif

	vpxor				rhelp,rhelp,rhelp
	vshufps				$0x1f,rdest,rhelp,rhelp
	vpxor				rhelp,rdest,rdest
	vshufps				$0x8c,rdest,rhelp,rhelp
	vpxor				rhelp,rdest,rdest
	vaeskeygenassist 	$\rcon,rsrc,rhelp
	.if (\rcon == 0)
	vshufps				$0xaa,rhelp,rhelp,rhelp
	.else
	vshufps				$0xff,rhelp,rhelp,rhelp
	.endif
	vpxor				rhelp,rdest,rdest

	save_rkey			rdest \r2
.endm

/* copy secret key from dbg regs into xmm regs */
.macro	read_key r0 r1
	movq	db0,%rax
	vmovq	%rax,\r0
	movq	db1,%rax
	vmovq	%rax,rhelp
	vshufps	$0x44,rhelp,\r0,\r0
	movq	db2,%rax
	vmovq	%rax,\r1
	movq	db3,%rax
	vmovq	%rax,rhelp
	vshufps	$0x44,rhelp,\r1,\r1
	xorq	%rax,%rax	/* clear rax, as it contained key material */
.endm

/* generate round keys rk1 to rk14 */
.macro	generate_rks
	read_key			rdest rsrc
	save_rkey			rdest 0
	save_rkey			rsrc 1

	key_schedule		0  1  2  0x1
	key_schedule		1  2  3  0x0
	key_schedule		2  3  4  0x2
	key_schedule		3  4  5  0x0
	key_schedule		4  5  6  0x4
	key_schedule		5  6  7  0x0
	key_schedule		6  7  8  0x8
	key_schedule		7  8  9  0x0
	key_schedule		8  9  10 0x10
	key_schedule		9  10 11 0x0
	key_schedule		10 11 12 0x20
	key_schedule		11 12 13 0x0
	key_schedule		12 13 14 0x40
.endm

.macro	do_enc_round rk
	load_rkey		\rk,rhelp
	vaesenc			rhelp,rstate,rstate
.endm

/* encrypt */
.macro	encrypt_block
	load_rkey			0,rhelp
	vpxor				rhelp,rstate,rstate
	do_enc_round		1
	do_enc_round		2
	do_enc_round		3
	do_enc_round		4
	do_enc_round		5
	do_enc_round		6
	do_enc_round		7
	do_enc_round		8
	do_enc_round		9
	do_enc_round		10
	do_enc_round		11
	do_enc_round		12
	do_enc_round		13
	load_rkey			14,rhelp
	vaesenclast			rhelp,rstate,rstate
.endm

/* inversed normal round */
.macro	do_dec_round rk
	load_rkey	\rk,rhelp
	vaesimc		rhelp,rhelp
	vaesdec		rhelp,rstate,rstate
.endm

/* decrypt */
.macro	decrypt_block
	load_rkey			14,rhelp
	vpxor				rhelp,rstate,rstate
	do_dec_round		13
	do_dec_round		12
	do_dec_round		11
	do_dec_round		10
	do_dec_round		9
	do_dec_round		8
	do_dec_round		7
	do_dec_round		6
	do_dec_round		5
	do_dec_round		4
	do_dec_round		3
	do_dec_round		2
	do_dec_round		1
	load_rkey			0,rhelp
	vaesdeclast			rhelp,rstate,rstate
.endm

/***************************************************************************
 *				CODE SEGMENT
 **************************************************************************/

.text
	.globl	bispe_gen_rkeys
	.globl	bispe_clear_avx_regs
	.globl	bispe_clear_regs
	.globl	bispe_encblk
	.globl	bispe_decblk
	.globl	bispe_encblk_mem
	.globl	bispe_decblk_mem
	.globl	bispe_encblk_mem_cbc
	.globl	bispe_decblk_mem_cbc
	.globl	bispe_set_key
	.globl	bispe_get_key
	.globl	bispe_check_features
	.globl	bispe_dump_regs

bispe_gen_rkeys:
	generate_rks
	movl			$0,%eax
	retq

bispe_clear_avx_regs:
	/* clear only avx registers */
	vzeroall
	retq

bispe_clear_regs:
	/* clear all avx registers */
	vzeroall
	/* also clear all caller save registers, just in case */
	xor		%rax,%rax
	xor		%rcx,%rcx
	xor		%rdx,%rdx
	xor		%rsi,%rsi
	xor		%rdi,%rdi
	xor		%r8,%r8
	xor		%r9,%r9
	xor		%r10,%r10
	xor		%r11,%r11
	retq

/*
 * encrypts content in rstate register; 
 * should not be called from the outside, as it may use %r8 for rip passing
 */
bispe_encblk:
	encrypt_block
#ifdef RIP_PROTECT
	jmp	*rrip
#else
	retq
#endif

/*
 * decrypts content in rstate register; 
 * should not be called from the outside, as it may use a register for rip passing
 */
bispe_decblk:
	decrypt_block
#ifdef RIP_PROTECT
	jmp	*rrip
#else
	retq
#endif

bispe_encblk_mem:
	vmovdqu			0(%rsi),rstate
	encrypt_block
	vmovdqu			rstate,0(%rdi)
	movl			$0,%eax
	retq

bispe_decblk_mem:
	vmovdqu			0(%rsi),rstate
	decrypt_block
	vmovdqu			rstate,0(%rdi)
	movl			$0,%eax
	retq

bispe_encblk_mem_cbc:
	vmovdqu			0(%rsi),rstate
	vpxor			0(%rdx),rstate,rstate
	encrypt_block
	vmovdqu			rstate,0(%rdi)
	movl			$0,%eax
	retq

bispe_decblk_mem_cbc:
	vmovdqu			0(%rsi),rstate
	decrypt_block
	vpxor			0(%rdx),rstate,rstate
	vmovdqu			rstate,0(%rdi)
	movl			$0,%eax
	retq	

bispe_set_key:
	movq	0(%rdi),%rax
	movq	%rax,db0
	movq	8(%rdi),%rax
	movq	%rax,db1
	movq	16(%rdi),%rax
	movq	%rax,db2
	movq	24(%rdi),%rax
	movq	%rax,db3
	xorq	%rax,%rax	/* clear rax, as it contained key material */
	retq

bispe_get_key:
	movq    db0,%rax
	movq    %rax,0(%rdi)
	movq    db1,%rax
	movq    %rax,8(%rdi)
	movq    db2,%rax
	movq    %rax,16(%rdi)
	movq    db3,%rax
	movq    %rax,24(%rdi)
	movq    $0,%rax
	retq

/* checks cpu features for AVX and AESNI support */
bispe_check_features:
	mov		$0x1,%eax
	cpuid
	and     $0x12000000,%ecx
	jz		unsupported
	mov		$1,%eax
	retq
unsupported:
	mov		$0,%eax
	retq

bispe_dump_regs:
	vmovdqu	%ymm0,0(%rdi)
	vmovdqu	%ymm1,32(%rdi)
	vmovdqu	%ymm2,64(%rdi)
	vmovdqu	%ymm3,96(%rdi)
	vmovdqu	%ymm4,128(%rdi)
	vmovdqu	%ymm5,160(%rdi)
	vmovdqu	%ymm6,192(%rdi)
	vmovdqu	%ymm7,224(%rdi)
	vmovdqu	%ymm8,256(%rdi)
	vmovdqu	%ymm9,288(%rdi)
	vmovdqu	%ymm10,320(%rdi)
	vmovdqu	%ymm11,352(%rdi)
	vmovdqu	%ymm12,384(%rdi)
	vmovdqu	%ymm13,416(%rdi)
	vmovdqu	%ymm14,448(%rdi)
	vmovdqu	%ymm15,480(%rdi)
	retq
