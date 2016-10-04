/***************************************************************************
 * instructions.h
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

#ifndef INSTRUCTIONS_H 
#define INSTRUCTIONS_H

struct instr_info {
	const char *name;
	int has_arg;
	uint32_t opcode;
};

typedef enum {
	INSTR_NOP,
	INSTR_FINISH,

	INSTR_PUSH,
	INSTR_PRINT,
	INSTR_LOAD,
	INSTR_STORE,

	INSTR_ADD,
	INSTR_SUB,
	INSTR_MUL,
	INSTR_DIV,
	INSTR_MOD,

	INSTR_JMP,
	INSTR_JEQ,
	INSTR_JNE,
	INSTR_JL,
	INSTR_JLE,
	INSTR_JG,
	INSTR_JGE,
	
	INSTR_CALL,
	INSTR_RET,
	INSTR_PROLOG,
	INSTR_EPILOG,

	INSTR_ARGLOAD
} instr_type;

const struct instr_info instr_info[] = {
	{"nop", 0, 0x0},
	{"finish", 0, 0x01},

	{"push", 1, 0x02},
	{"print", 0, 0x03},
	{"load", 1, 0x04},
	{"store", 1, 0x05},

	{"add", 0, 0x06},
	{"sub", 0, 0x07},
	{"mul", 0, 0x08},
	{"div", 0, 0x09},
	{"mod", 0, 0x0A},

	{"jmp", 1, 0x0B},
	{"jeq", 1, 0x0C},
	{"jne", 1, 0x0D},
	{"jl", 1, 0x0E},
	{"jle", 1, 0x0F},
	{"jge", 1, 0x10},
	{"jg", 1, 0x11},

	{"call", 1, 0x12},
	{"ret", 0, 0x13},
	{"prolog", 1, 0x14},
	{"epilog", 1, 0x15},

	{"argload", 1, 0x16},
};

#endif /* INSTRUCTIONS_H */
