/***************************************************************************
 * bispe_interpreter.c
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
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/random.h>
#include <linux/uaccess.h>

#include "bispe_comm.h"
#include "bispe_defines.h"
#include "bispe_interpreter.h"
#include "bispe_crypto.h"
#include "bispe_state.h"

/* Struct bundling information about the execution together */
struct runtime_ctx {
	/* Pointers to segment start addresses */
	uint32_t *code_seg_bp;
	uint32_t *stack_seg_bp;
	uint32_t *call_seg_bp;
	uint32_t *print_seg_bp;

	/* Segment sizes */
	size_t code_seg_size; 
	size_t stack_seg_size;
	size_t call_seg_size;
	size_t print_seg_size;

	/* Program arguments */
	size_t argc;
	uint32_t *argv;

	/* Amount of instructions to process in one cycle */
	uint64_t ipc;
};

/* Amount of instructions to process in one cycle */
uint64_t bispe_instr_per_cycle = 0;

/* Pointers to segment start addresses */
uint32_t *bispe_code_seg_bp = NULL;
uint32_t *bispe_stack_seg_bp = NULL;
uint32_t *bispe_call_seg_bp = NULL;
uint32_t *bispe_print_seg_bp = NULL;

/* Segment sizes in bytes */
size_t bispe_code_seg_size = 0;
size_t bispe_stack_seg_size = 0;
size_t bispe_call_seg_size = 0;
size_t bispe_print_seg_size = 0;

/* 
 * Holds program arguments:
 * if argc is 0, argv is NULL
 */
size_t bispe_argc;
uint32_t *bispe_argv;

/*
 * Returns pointer to "size" bytes memory, aligned to ALIGMENT.
 * Allocates memory, then adjusts the pointer to fit the alignment.
 * The pointer to the original address is saved just before the aligned address,
 * so that it can be later freed with "afree()"
 */ 
static void *amalloc(size_t size)
{
	#define ALIGNMENT 128
	void *mem, *ptr;
	uintptr_t mask;

	mem = vmalloc(size + ALIGNMENT-1 + sizeof(void *));
	if (mem == NULL) {
		return NULL;
	}

	mask = ~(uintptr_t) (ALIGNMENT - 1);
	ptr = (void *) (((uintptr_t) mem + (ALIGNMENT-1) + sizeof(void *)) & mask);
	((void**) ptr)[-1] = mem;
	return ptr;
}

/*
 * Frees aligned memory allocated by amalloc
 */
static void afree(void *ptr)
{
	if (ptr != NULL) {
		vfree(((void **) ptr)[-1]);
	}
}

/*
 * Debug code to print registers
 */
static void __attribute__((__unused__)) print_registers(void)
{
	char regs[512];
	char db_regs[32];
	bispe_dump_regs(regs);
	bispe_get_key(db_regs);

	printk("DB:    ");
	for(int i = 0; i < 32; i++) {
		printk("%02x", (unsigned char) db_regs[i]);
		if(i != 0 && (i+1) % 8 == 0) {
			printk(" ");
		}
	}
	printk("\n");

	for(int i = 0; i < 16; i++) {
		printk("YMM%02d: ", i);
		for(int j = 31; j >= 0; j--) {
			printk("%02x", (unsigned char) regs[i*32+j]);
			if(j < 31 && j % 8 == 0) {
				printk(" ");
			}
		}
		printk("\n");
	}
	printk("\n");
}

/*
 * Debug code to print a segment
 */
static void __attribute__((__unused__)) print_seg(uint32_t *seg, size_t size) {
	for(int i = 0; i < size; i++) {
		printk("%08x ", *(seg+i));
		if((i+1) % 4 == 0)
			printk("\n");
	}
	printk("\n");
}

uint32_t *bispe_get_print_seg_bp(void)
{
	return bispe_print_seg_bp;
}

size_t bispe_get_print_count(void)
{
	return (size_t) (bispe_get_print_ptr() - bispe_print_seg_bp);
}

static void free_segments(struct runtime_ctx *runtime_ctx)
{
	afree(runtime_ctx->code_seg_bp);
	afree(runtime_ctx->stack_seg_bp);
	afree(runtime_ctx->call_seg_bp);
	afree(runtime_ctx->print_seg_bp);
	afree(runtime_ctx->argv);
}

/*
 * Frees all memory allocated for an interpreter execution
 */
void cleanup_runtime_ctx(struct runtime_ctx *runtime_ctx)
{
	if(runtime_ctx == NULL) {
		return;
	}

#ifdef ENCRYPTION
	/* restore original pointer: unshadow init vector */
	runtime_ctx->code_seg_bp -= 4;
	runtime_ctx->stack_seg_bp -= 4;
	runtime_ctx->call_seg_bp -= 4;
#endif

	free_segments(runtime_ctx);
	vfree(runtime_ctx);
}

static struct runtime_ctx *create_runtime_ctx(struct invoke_ctx *invoke_ctx,
												struct buf_info *code_buf,
												struct buf_info *arg_buf)
{
	/* allocate runtime context */
	struct runtime_ctx *runtime_ctx = vzalloc(sizeof(runtime_ctx));
	if (runtime_ctx == NULL) {
		goto error;
	}

	/* set code and argument buffers */
	runtime_ctx->code_seg_size = code_buf->size;
	runtime_ctx->code_seg_bp = code_buf->ptr;

	runtime_ctx->argc = arg_buf->size / 4; /* count of 32 bit arguments */
	runtime_ctx->argv = (runtime_ctx->argc > 0) ? arg_buf->ptr : NULL;

	/* set segment sizes, use default size if given size is zero */
	runtime_ctx->stack_seg_size = (invoke_ctx->stack_size > 0) 
						? invoke_ctx->stack_size : DEFAULT_STACK_SIZE;

	runtime_ctx->call_seg_size = (invoke_ctx->call_size > 0) 
						? invoke_ctx->call_size : DEFAULT_CALL_SIZE;

	runtime_ctx->print_seg_size = (invoke_ctx->out_buf.size > 0) 
						? invoke_ctx->out_buf.size : DEFAULT_PRINT_SIZE;
	
	/* 
	 * call and stack segment sizes are given in 16 bytes, 
	 * so they must be changed to bytes before allocation
	 */
	runtime_ctx->stack_seg_size *= 4*sizeof(uint32_t);
	runtime_ctx->call_seg_size *= 4*sizeof(uint32_t);

	runtime_ctx->ipc = (invoke_ctx->ipc > 0) ?
		invoke_ctx->ipc : DEFAULT_INSTR_PER_CYCLE;

	#ifdef ENCRYPTION
	/* add 16 bytes to make space for the init vector */
	runtime_ctx->stack_seg_size += 16;
	runtime_ctx->call_seg_size += 16;
	#endif

	/* allocate segment memory */
	runtime_ctx->stack_seg_bp = amalloc(bispe_stack_seg_size);
	if (runtime_ctx->stack_seg_bp == NULL) {
		goto error;
	}

	runtime_ctx->call_seg_bp = amalloc(bispe_call_seg_size);
	if (runtime_ctx->call_seg_bp == NULL) {
		goto error;
	}

	runtime_ctx->print_seg_bp = amalloc(bispe_print_seg_size);
	if (runtime_ctx->print_seg_bp == NULL) {
		goto error;
	}

	#ifdef ENCRYPTION
	/* generate init vectors */
	get_random_bytes(runtime_ctx->stack_seg_bp, 16);
	get_random_bytes(runtime_ctx->call_seg_bp, 16);

	/* shadow init vectors */
	runtime_ctx->code_seg_bp += 4;
	runtime_ctx->stack_seg_bp += 4;
	runtime_ctx->call_seg_bp += 4;

	runtime_ctx->code_seg_size -= 16;
	runtime_ctx->stack_seg_size -= 16;
	runtime_ctx->call_seg_size -= 16;
	#endif

	return runtime_ctx;

error:
	/* 
	 * This will not work after the init vectors were shadowed,
	 * because then the original pointers are gone
	 */
	free_segments(runtime_ctx);
	vfree(runtime_ctx);
	return NULL;
}

/*
 * Allocates a kernel space buffer and copies a buffer from user space to it.
 */ 
static char *get_buf_from_user(const void __user *src, size_t size)
{
	char *ptr = (char *) amalloc(size);
	if (ptr == NULL) {
		return NULL;
	}

	if (copy_from_user(ptr, src, size) != 0) {
		return NULL;
	}

	return ptr;
}

/*
 * Create an initialized runtime context. 
 * Code and arguments are copied from userspace.
 */
struct runtime_ctx *init_interpreter(struct invoke_ctx *invoke_ctx)
{
	struct runtime_ctx *runtime_ctx;
	struct buf_info code_buf = { .size = invoke_ctx->code_buf.size };
	struct buf_info arg_buf = { .size = invoke_ctx->arg_buf.size };

	/* get buffers from user space */
	code_buf.ptr = get_buf_from_user(invoke_ctx->code_buf.ptr, invoke_ctx->code_buf.size);
	if(code_buf.ptr == NULL) {
		printk(KERN_ERR "bispe_invoke: failed to initialize code buffer.\n");
		goto error;
	}

	if(invoke_ctx->arg_buf.size > 0) {
		arg_buf.ptr = get_buf_from_user(invoke_ctx->arg_buf.ptr, invoke_ctx->arg_buf.size);
		if(arg_buf.ptr == NULL) {
			printk(KERN_ERR "bispe_invoke: failed to initialize argument buffer.\n");
			goto error;
		}
	}

	runtime_ctx = create_runtime_ctx(invoke_ctx, &code_buf, &arg_buf);
	if(runtime_ctx == NULL) {
		printk(KERN_ERR "bispe_invoke: failed to initialize runtime environment.\n");
	}

	return runtime_ctx;

error:
	afree(code_buf.ptr);
	afree(arg_buf.ptr);
	return NULL;
}

/*
 * Create an initialized runtime context. Code and arguments are taken 
 * from the invoke context. This initialization is used for invoking
 * the interpreter from kernel context, e.g. for debugging. 
 */
struct runtime_ctx *init_interpreter_intern(struct invoke_ctx *invoke_ctx)
{
	struct runtime_ctx *runtime_ctx;
	struct buf_info code_buf = { .size = invoke_ctx->code_buf.size };
	struct buf_info arg_buf = { .size = invoke_ctx->arg_buf.size };

	code_buf.ptr = amalloc(invoke_ctx->code_buf.size);
	if (code_buf.ptr == NULL) {
		goto error;
	}
	memcpy(code_buf.ptr, invoke_ctx->code_buf.ptr, code_buf.size);

	if (arg_buf.size > 0) {
		arg_buf.ptr = amalloc(invoke_ctx->arg_buf.size);
		if (code_buf.ptr == NULL) {
			goto error;
		}
		memcpy(arg_buf.ptr, invoke_ctx->arg_buf.ptr, arg_buf.size);
	}

	runtime_ctx = create_runtime_ctx(invoke_ctx, &code_buf, &arg_buf);
	if (runtime_ctx == NULL) {
		printk(KERN_ERR "bispe_invoke: failed to initialize runtime environment.\n");
	}

	return runtime_ctx;
error:
	afree(code_buf.ptr);
	afree(arg_buf.ptr);
	return NULL;
}

static inline void cycle_prolog(unsigned long *irq_flags)
{	
	/* disable scheduler */
	preempt_disable();
	/* save irq_flags and disable interrupts */
	local_irq_save(*irq_flags);

	/* 
	 * Silencing the watchdogs:
	 * If the interpreter runs longer than the watchdog threshold, the kernel
	 * suspects that a task got stuck on the CPU and creates a softlockup warning. 
	 * The warning itself is harmless, but it dumps the registers to the log.
	 * To prevent that, we call touch_softlockup_watchdog before each cycle, 
	 * which sets the watchdog timestamp of the current CPU to 0. 
	 * Because the timestamp will stay 0 throughout the cycle (the watchdog
	 * thread can not run), no warning is generated. 
	 */
	touch_softlockup_watchdog();
}

static inline void cycle_epilog(unsigned long *irq_flags)
{
	local_irq_restore(*irq_flags);
	preempt_enable();
}

int start_interpreter(struct runtime_ctx *runtime_ctx, int threaded)
{
	volatile bool halt = 0;
	volatile uint8_t error_code = 0;
	unsigned long irq_flags;

	/*
	 * We are currently using static variables for holding the runtime
	 * environment, so the runtime context is copied to them first. 
	 * This will be changed if multiple executions are allowed.
	 */
	bispe_code_seg_bp = runtime_ctx->code_seg_bp;
	bispe_stack_seg_bp = runtime_ctx->stack_seg_bp;
	bispe_call_seg_bp = runtime_ctx->call_seg_bp;
	bispe_print_seg_bp = runtime_ctx->print_seg_bp;
	bispe_argv = runtime_ctx->argv;

	bispe_code_seg_size = runtime_ctx->code_seg_size;
	bispe_stack_seg_size = runtime_ctx->stack_seg_size;
	bispe_call_seg_size = runtime_ctx->call_seg_size;
	bispe_print_seg_size = runtime_ctx->print_seg_size;
	bispe_argc = runtime_ctx->argc;

	bispe_instr_per_cycle = runtime_ctx->ipc;

	/* set interpreter state pointers */
	bispe_set_instr_ptr(bispe_code_seg_bp);
	bispe_set_stack_ptr(bispe_stack_seg_bp);
	bispe_set_call_ptr(bispe_call_seg_bp);
	bispe_set_print_ptr(bispe_print_seg_bp);

	#ifdef DEBUG
	printk("bispe_interpreter: starting execution\n");
	#endif

	bispe_reset_flags();

	/*
	 * Each loop performs one instruction cycle,
	 * with at most "instr_per_cycle" instructions 
	 */
	while (!threaded || !kthread_should_stop()) {
		/* 
		 * Begin atomic section:
		 * Disable interrupts and scheduling
		 */
		cycle_prolog(&irq_flags);

		#ifdef DEBUG
		printk("---begin new cycle---\n");
		#endif

		#ifdef ENCRYPTION
		/* generate round keys */
		bispe_gen_rkeys();
		#endif

		/* transfers control to instruction cycle */
		bispe_cycle_entry();
	
		/* if halt flag is set, program execution is finished */
		if (bispe_chk_halt()) {
			halt = 1;
			if (bispe_chk_error()) {
				error_code = bispe_get_error();
			}
		}

		/* clear all registers before anyone else has access again */
		bispe_clear_regs();

		#ifdef DEBUG
		printk("---end cycle---\n");
		#endif

		/* 
		 * End atomic section: 
		 * Enable interrupts and scheduling again
		 */
		cycle_epilog(&irq_flags);

		if(halt) {
			break;
		}
	}

	/* thread was told by signal to stop */
	if (threaded && kthread_should_stop()) {
		printk(KERN_ERR "bispe_interpreter: execution interrupted by fatal signal\n");
		return -1;
	}

	#ifdef DEBUG
	if(!threaded) {
		print_seg(bispe_print_seg_bp, bispe_get_print_count());
	}
	#endif

	if (error_code) {
		#ifdef DEBUG
		printk("bispe_interpreter: error %hhu\n", error_code);
		#endif
		return error_code;
	}

	#ifdef DEBUG
	printk("bispe_interpreter: finishing execution\n");
	#endif

	return 0;
}
