/***************************************************************************
 * asm_error.S
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

.text

/*
 * Encodes one byte error code, sets halt flag
 * code: 1 byte immediate
 */
.macro	set_error code
	/* encode error code */
	movb	$0,error_code(%rip)
	orb		$\code,error_code(%rip)

	/* set halt flag */
	orb		$1,halt_flag(%rip)
.endm

error_inv_opcode:
	set_error	ERR_INV_OPCODE
	restore_callee_regs
	retq

error_jmp_bounds:
	set_error	ERR_JMP_BOUNDS
	restore_callee_regs
	retq

error_stack_overflow:
	set_error	ERR_STACK_OVERFLOW
	restore_callee_regs
	retq

error_stack_underflow:
	set_error	ERR_STACK_UNDERFLOW
	restore_callee_regs
	retq

error_call_overflow:
	set_error	ERR_CALL_OVERFLOW
	restore_callee_regs
	retq

error_call_underflow:
	set_error	ERR_CALL_UNDERFLOW
	restore_callee_regs
	retq

error_div_zero:
	set_error	ERR_DIV_ZERO
	restore_callee_regs
	retq

error_arg_range:
	set_error	ERR_ARG_RANGE
	restore_callee_regs
	retq
