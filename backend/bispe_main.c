/***************************************************************************
 * bispe_main.c
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>

#include "bispe_comm.h"
#include "bispe_interpreter.h"
#include "bispe_crypto.h"
#include "bispe_key.h"

/* Kernel module infos */
MODULE_AUTHOR("Maximilian Seitzer");
MODULE_DESCRIPTION("This is the kernel backend for the bytecode interpreter \
for secure program execution");
MODULE_LICENSE("GPL");

/***************************************************************************
 *				KTHREAD MANAGEMENT
 **************************************************************************/

/*
 * lock which prevents more than one program to be executed at a time
 */
static struct mutex interpreter_lock;

/* 
 * lock which signals the end of the execution to the thread creator
 */
static struct semaphore interpreter_finish_lock;

static struct task_struct *interpreter_thread;

static int thread_main(void *data)
{
	/* run interpreter */
	int res = start_interpreter((struct runtime_ctx *) data, 1);

	/* thread stopped naturally */
	if (res != -1) {
		/* wake up thread creator, execution is finished */
		up(&interpreter_finish_lock);
	}

	/* wait until thread creator called kthread_stop*/
	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	set_current_state(TASK_RUNNING);

	/* return interpreter exit code to thread creator */
	return res;
}

static int start_interpreter_thread(struct runtime_ctx *runtime_ctx)
{
	/* create and run interpreter thread */
	interpreter_thread = kthread_run(thread_main, runtime_ctx, "bispe_interpreter");
	if (interpreter_thread == ERR_PTR(-ENOMEM)) {
		printk(KERN_ERR "bispe_invoke: could not create interpreter thread, ENOMEM\n");
		return -2;
	}

	/* wait until interpreter has finished, but catch fatal signals */
	down_killable(&interpreter_finish_lock);

	/* issue interpreter to stop, returns:
	 *	-2: if interpreter error'd during init
	 *	-1: if interpreter was forced to stop
	 *   0: if interpreter executed normally
	 *  >0: if a run time error occurred
	 */
	return kthread_stop(interpreter_thread);
}

/***************************************************************************
 *				SYSFS ENTRIES
 **************************************************************************/

static ssize_t show_dummy(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	return 0;
}

/* Hook for interpreter start */
static ssize_t invoke_interpreter(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	struct invoke_ctx *invoke_ctx;
	struct runtime_ctx *runtime_ctx;
	int interpreter_result;
	size_t print_bytes_written = 0;

	if(count <= 0) {
		return 0;
	} else if (count != sizeof(struct invoke_ctx)) {
		printk(KERN_ERR "bispe_invoke: invalid argument format\n");
		return 0;
	}

	invoke_ctx = (struct invoke_ctx *) buf;

	/* check for argument format errors */
	if (invoke_ctx->out_buf.size % sizeof(uint32_t) != 0) {
		printk(KERN_ERR "bispe_invoke: result buffer must be multiple of dwords\n");
		return 0;
	} else if (invoke_ctx->code_buf.size % (4 * sizeof(uint32_t)) != 0) {
		printk(KERN_ERR "bispe_invoke: invalid code structure\n");
		return 0;
	}

	/* 
	 * try to access crititcal section: only one interpreter thread may live at a time.
	 * return to frontend if interpreter is currently used
	 */
	if (mutex_trylock(&interpreter_lock) != 1) {
		printk(KERN_ERR "bispe_invoke: interpreter already in use\n");
		return 0;
	}

	runtime_ctx = init_interpreter(invoke_ctx);
	if(runtime_ctx == NULL) {
		goto error;
	}

	/* dispatch interpreter thread and wait for it to finish */
	interpreter_result = start_interpreter_thread(runtime_ctx);

	/* interpreter creation has failed or execution was interrupted */
	if (interpreter_result < 0) {
		goto cleanup;
	}

	/* copy print buffer to user space */
	if (invoke_ctx->out_buf.size > 0 && bispe_get_print_count() > 0) {
		print_bytes_written = 4 * bispe_get_print_count();
		copy_to_user(invoke_ctx->out_buf.ptr,
			bispe_get_print_seg_bp(),
			print_bytes_written);
	}

	/* pass interpreter result to user space */
	put_user(interpreter_result, invoke_ctx->result);

cleanup:
	/* perform memory cleanup */
	cleanup_runtime_ctx(runtime_ctx);

error:
	/* give interpreter free for other processes */
	mutex_unlock(&interpreter_lock);

	return print_bytes_written;
}

/*
 * Provides encryption facilities for user space compiler.
 * encrypts passed buffer and copies it back to user space
 */
static ssize_t encrypt_code(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count) {
	struct buf_info *buf_info;
	char *code_buf;

	if(count <= 0) {
		return 0;
	} else if (count != sizeof(struct buf_info)) {
		printk(KERN_ERR "bispe_encrypt: invalid argument format\n");
		return 0;
	}

	buf_info = (struct buf_info *) buf;

	/* check for argument format errors */
	if (buf_info->size % (4 * sizeof(uint32_t)) != 0) {
		printk(KERN_ERR "bispe_encrypt: code must be multiple of 128 bit\n");
		return 0;
	}

	/* allocate a buffer to copy the user code buffer to */
	code_buf = (char *) vmalloc(buf_info->size);
	if (code_buf == NULL) {
		printk(KERN_ERR "bispe_encrypt: failed to allocate code buffer\n");
		return 0;
	}

	/* copy user code buffer to kernel */
	if (copy_from_user(code_buf, buf_info->ptr, buf_info->size) != 0) {
		printk(KERN_ERR "bispe_encrypt: failed to copy code from user\n");
		return 0;
	}

	/* BEGIN CRITICAL SECTION: key material may not be leaked */

	preempt_disable();
	unsigned long irq_flags;
	local_irq_save(irq_flags);

	bispe_gen_rkeys();

	/* encrypt code in cbc mode; first 128 bit are used as init vector */
	for (int i = 16; i < buf_info->size; i = i + 16) {
		u8 tmp[16];
		bispe_encblk_mem_cbc((u8 *) &tmp, &code_buf[i], &code_buf[i-16]);
		memcpy(&code_buf[i], &tmp, 16);
	}

	/* copy encrypted code back to user */
	copy_to_user(buf_info->ptr, code_buf, buf_info->size);

	memset(code_buf, 0, buf_info->size);
	wbinvd();

	bispe_clear_avx_regs();

	local_irq_restore(irq_flags);
	preempt_enable();
	/* END CRITICAL SECTION */

	vfree(code_buf);
	return buf_info->size;
}

static struct kobj_attribute invoke_attribute =
	__ATTR(invoke, 0664, show_dummy, invoke_interpreter);

static struct kobj_attribute crypto_attribute = 
	__ATTR(crypto, 0664, show_dummy, encrypt_code);

static struct kobj_attribute password_attribute =
	__ATTR(password, 0664, hash_show, password_store);

static struct attribute *attrs[] = {
	&invoke_attribute.attr,
	&crypto_attribute.attr,
	&password_attribute.attr,
	NULL
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

/* Sysfs entry to communicate with user space */
static struct kobject *bispe_kobj;

/* Register new sysfs entry /sys/kernel/bispe */
static inline int __init init_sysfs(void)
{
	int ret;

	bispe_kobj = kobject_create_and_add("bispe", kernel_kobj);
	if (!bispe_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(bispe_kobj, &attr_group);
	if (ret)
		kobject_put(bispe_kobj);

	return ret;
}

static inline void __exit exit_sysfs(void)
{
	kobject_put(bispe_kobj);
}

/***************************************************************************
 *				DEBUG CODE
 **************************************************************************/

#ifdef RUN_DEBUG_TEST

/* include header for instruction mnemonics */
#include "bispe_defines.h"
/*
 * Specify some code that gets run on module insert when 
 * compiled with RUN_DEBUG_TEST
 */ 
static const uint32_t debug_code[] = {
	INSTR_PROLOG, 0x1,
	INSTR_PUSH, 0xA,
	INSTR_STORE, 0x0,
	INSTR_CALL,	0xA,
	INSTR_PRINT,
	INSTR_FINISH,

	/* int sum(int n) */
	INSTR_PROLOG, 0x1,

	/* if (n == 1) return 1 */
	INSTR_LOAD, 0x2,
	INSTR_PUSH, 0x1,
	INSTR_JNE, 0x17,

	INSTR_PUSH, 0x1,
	INSTR_EPILOG, 0x1,
	INSTR_RET,

	/* return sum(n-1) + n */
	INSTR_LOAD, 0x2,
	INSTR_PUSH, 0x1,
	INSTR_SUB,
	INSTR_STORE, 0x0,
	INSTR_CALL, 0xA,

	INSTR_LOAD, 0x2,
	INSTR_ADD,

	INSTR_EPILOG, 0x1,
	INSTR_RET,
};
#endif

/***************************************************************************
 *				MODULE MANAGEMENT
 **************************************************************************/

/* Initialize module */
static int __init bispe_init(void)
{
	int ret;
	printk(KERN_INFO "bispe: initializing kernel module.\n");

	if (!bispe_check_features()) {
		printk(KERN_ERR "bispe: CPU does not support AVX and/or AESNI instructions\n");
		printk(KERN_ERR "bispe: module load failed\n");
		return 1;
	}

	/* initialize semaphores */
	mutex_init(&interpreter_lock);
	sema_init(&interpreter_finish_lock, 0);

	ret = init_sysfs();
	if (ret)
		return ret;

/* run debug code on module load */
#ifdef RUN_DEBUG_TEST
	mutex_lock(&interpreter_lock);
	struct invoke_ctx = {
		.code_buf = { (void *) debug_code, 4*ARRAY_SIZE(debug_code) },
		.arg_buf = { NULL, 0 },
	};

	struct runtime_ctx *runtime_ctx = init_interpreter_intern(&invoke_ctx);
	if (!runtime_ctx)
		return 1;

	start_interpreter(runtime_ctx, 0);
	cleanup_runtime_ctx(runtime_ctx);
	mutex_unlock(&interpreter_lock);
#endif

#ifdef TESTS
	mutex_lock(&interpreter_lock);
	run_interpreter_tests();
	mutex_unlock(&interpreter_lock);
#endif

	return 0;
}
module_init(bispe_init);

/* Exit module */
static void __exit bispe_exit(void)
{
	exit_sysfs();
	printk(KERN_INFO "bispe: exiting kernel module.\n");
}
module_exit(bispe_exit);
