/***************************************************************************
 * bispe_tests.c
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>

#include "bispe_comm.h"
#include "bispe_defines.h"
#include "bispe_interpreter.h"

struct result {
	int expect_error;
	uint32_t result;
};

struct test {
	const char *name;
	struct result result;
	const int stack_size; /* amount of 16 byte stack rows */
	const int call_size; /* amount of 16 byte call stack rows */
	const size_t size; /* code size in dwords */
	const uint32_t *code;
};

static struct test tests[] = {
	{
		"check stack overflow",
		{ 1, ERR_STACK_OVERFLOW },
		1, 1, 11,
		(const uint32_t [])
		{
			INSTR_PUSH, 0x1,
			INSTR_PUSH, 0x1,
			INSTR_PUSH, 0x1,
			INSTR_PUSH, 0x1,
			INSTR_PUSH, 0x1,
			0x1
		}
	},

	{
		"check stack underflow",
		{ 1, ERR_STACK_UNDERFLOW },
		1, 1, 2,
		(const uint32_t [])
		{
			INSTR_PRINT,
			0x1
		}
	},

	{
		"check stack underflow 2",
		{ 1, ERR_STACK_UNDERFLOW },
		1, 1, 3,
		(const uint32_t [])
		{
			INSTR_STORE, 0x0,
			0x1
		}
	},

	{
		"check call overflow",
		{ 1, ERR_CALL_OVERFLOW },
		1, 1, 3,
		(const uint32_t [])
		{
			INSTR_PROLOG, 0x4,
			0x1
		}
	},

	{
		"check call underflow",
		{ 1, ERR_CALL_UNDERFLOW },
		1, 1, 3,
		(const uint32_t [])
		{
			INSTR_EPILOG, 0x1,
			0x1
		}
	},

	{
		"check call underflow 2",
		{ 1, ERR_CALL_UNDERFLOW },
		1, 1, 3,
		(const uint32_t [])
		{
			INSTR_LOAD, 0x1,
			0x1
		}
	},

	{
		"check call underflow 3",
		{ 1, ERR_CALL_UNDERFLOW },
		1, 1, 5,
		(const uint32_t [])
		{
			INSTR_PUSH, 0x1,
			INSTR_STORE, 0x1,
			0x1
		}
	},

	{
		"check division by zero",
		{ 1, ERR_DIV_ZERO },
		4, 4, 6,
		(const uint32_t [])
		{
			INSTR_PUSH, 0x5,
			INSTR_PUSH, 0x0,
			INSTR_DIV,
			0x1
		}
	},	
};

static int run_test(struct test *test) {
	int res = 0;

	struct invoke_ctx invoke_ctx = {
		.stack_size = 4 * sizeof(uint32_t) * test->stack_size,
		.call_size = 4 * sizeof(uint32_t) * test->call_size, 
		.code_buf = { (void *) test->code, sizeof(uint32_t) * test->size },
		.out_buf = { NULL, 0 },
	};

	struct runtime_ctx *runtime_ctx = init_interpreter_intern(&invoke_ctx);
	if(runtime_ctx == NULL) {
		printk("error initializing test '%s'", test->name);
		goto out;
	}

	int interpreter_result = start_interpreter(runtime_ctx, 0);

	if(test->result.expect_error) {
		if(interpreter_result != test->result.result) {
			printk("test '%s': expected error %i, got %08x\n",
				test->name, test->result.result, interpreter_result);
			goto out;
		}
	} else {
		/* check last element of print buffer */
		uint32_t *print_buf = bispe_get_print_seg_bp();
		int index = bispe_get_print_count()-1;

		if(print_buf[index] != test->result.result) {
			printk("test '%s': results differed. expected %08x, got %08x\n", 
				test->name, test->result.result, print_buf[index]);
		}
		goto out;
	}

	res = 1;

out:
	cleanup_runtime_ctx(runtime_ctx);
	return res;
}

void run_interpreter_tests(void) {
	printk("bispe: running tests...\n");

	for(int i = 0; i < ARRAY_SIZE(tests); i++) {
		if(run_test(&tests[i])) {
			printk("test '%s' successful!\n", tests[i].name);
		} else {
			printk("test '%s' failed!\n", tests[i].name);
		}
	}
}
