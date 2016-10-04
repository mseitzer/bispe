/***************************************************************************
 * bispe_defines.h
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

#ifndef _BISPE_DEFINES_H
#define _BISPE_DEFINES_H

/***************************************************************************
 *				INTERPRETER SETTINGS
 **************************************************************************/

/* Default size of stack, call and print segment in 16 byte portions */
#define DEFAULT_STACK_SIZE 20
#define DEFAULT_CALL_SIZE 20
#define DEFAULT_PRINT_SIZE 20

#define DEFAULT_INSTR_PER_CYCLE 2000

/***************************************************************************
 *				ERROR CODES
 **************************************************************************/

#define ERR_INV_OPCODE 0x1
#define ERR_JMP_BOUNDS 0x2
#define ERR_STACK_OVERFLOW 0x3
#define ERR_STACK_UNDERFLOW 0x4
#define ERR_CALL_OVERFLOW 0x5
#define ERR_CALL_UNDERFLOW 0x6
#define ERR_DIV_ZERO 0x7
#define ERR_ARG_RANGE 0x8

/***************************************************************************
 *				INSTRUCTION MNEMONICS
 **************************************************************************/

#define INSTR_NOP 0x00
#define INSTR_FINISH 0x01

#define INSTR_PUSH 0x02
#define INSTR_PRINT 0x03
#define INSTR_LOAD 0x04
#define INSTR_STORE 0x05

#define INSTR_ADD 0x06
#define INSTR_SUB 0x07
#define INSTR_MUL 0x08
#define INSTR_DIV 0x09
#define INSTR_MOD 0x0A

#define INSTR_JMP 0x0B
#define INSTR_JEQ 0x0C
#define INSTR_JNE 0x0D
#define INSTR_JL 0x0E
#define INSTR_JLE 0x0F
#define INSTR_JG 0x10
#define INSTR_JGE 0x11

#define INSTR_CALL 0x12
#define INSTR_RET 0x13
#define INSTR_PROLOG 0x14
#define INSTR_EPILOG 0x15

#endif /* _BISPE_DEFINES_H */
