/***************************************************************************
 * crypto_test.c
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
#include <linux/smp.h>
#include <crypto/algapi.h>
#include <crypto/aes.h>

#include "bispe_crypto.h"

/* Kernel module infos */
MODULE_AUTHOR("Maximilian Seitzer");
MODULE_DESCRIPTION("Test module for crypto of bytecode interpreter \
for secure program execution");
MODULE_LICENSE("GPL");

static void setkey_current_cpu(void *data)
{
	bispe_set_key((const u8 *)data);
}

static int bispe_setkey(struct crypto_tfm *tfm, const u8 *in_key,
							unsigned int key_len)
{
	struct crypto_aes_ctx *ctx = crypto_tfm_ctx(tfm);
	ctx->key_length = key_len;

	on_each_cpu(setkey_current_cpu, (void *) in_key, 1);
	return 0;
}

static inline void cycle_prolog(unsigned long *irq_flags)
{	
	/* disable scheduler */
	preempt_disable();
	/* save irq_flags and disable interrupts */
	local_irq_save(*irq_flags);
}

static inline void cycle_epilog(unsigned long *irq_flags)
{
	local_irq_restore(*irq_flags);
	preempt_enable();
}

/*
 * Encrypt one block
 */
void bispe_encrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	struct crypto_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	/* as we do not support AES-128/AES-192, we have to use the 
	 * original TRESOR routines for encryption in that cases */
	if (ctx->key_length != AES_KEYSIZE_256) {
		struct crypto_cipher *tresor_tfm;
		tresor_tfm = crypto_alloc_cipher("tresor-driver", 0, 0);
		if (IS_ERR(tresor_tfm)) {
			printk(KERN_ERR "alg: cipher: Failed to load transform for "
			       "%s: %ld\n", "tresor-driver", PTR_ERR(tresor_tfm));
			return;
		}

		struct crypto_aes_ctx *tresor_ctx = crypto_tfm_ctx((struct crypto_tfm *)tresor_tfm);
		tresor_ctx->key_length = ctx->key_length;

		crypto_cipher_encrypt_one(tresor_tfm, dst, src);

		crypto_free_cipher(tresor_tfm);
		return;
	}

	unsigned long irq_flags;

	cycle_prolog(&irq_flags);

	bispe_gen_rkeys();
	bispe_encblk_mem(dst, src);
	bispe_clear_avx_regs();

	cycle_epilog(&irq_flags);
}


/*
 * Decrypt one block
 */
void bispe_decrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	struct crypto_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	/* as we do not support AES-128/AES-192, we have to use the 
	 * original TRESOR routines for decryption in that cases */
	if (ctx->key_length != AES_KEYSIZE_256) {
		struct crypto_cipher *tresor_tfm;
		tresor_tfm = crypto_alloc_cipher("tresor-driver", 0, 0);
		if (IS_ERR(tresor_tfm)) {
			printk(KERN_ERR "alg: cipher: Failed to load transform for "
			       "%s: %ld\n", "tresor-driver", PTR_ERR(tresor_tfm));
			return;
		}

		struct crypto_aes_ctx *tresor_ctx = crypto_tfm_ctx((struct crypto_tfm *)tresor_tfm);
		tresor_ctx->key_length = ctx->key_length;

		crypto_cipher_decrypt_one(tresor_tfm, dst, src);

		crypto_free_cipher(tresor_tfm);
		return;
	}

	unsigned long irq_flags;

	cycle_prolog(&irq_flags);

	bispe_gen_rkeys();
	bispe_decblk_mem(dst, src);
	bispe_clear_avx_regs();

	cycle_epilog(&irq_flags);
}

/*
 * Crypto API algorithm
 */
static struct crypto_alg bispe_alg = {
	.cra_name		= "bispe",
	.cra_driver_name	= "bispe-driver",
	.cra_priority		= 100,
	.cra_flags		= CRYPTO_ALG_TYPE_CIPHER,
	.cra_blocksize		= AES_BLOCK_SIZE,
	.cra_ctxsize		= sizeof(struct crypto_aes_ctx),
	.cra_alignmask		= 3,
	.cra_module		= THIS_MODULE,
	.cra_list		= LIST_HEAD_INIT(bispe_alg.cra_list),
	.cra_u	= {
		.cipher	= {
			.cia_min_keysize	= AES_MIN_KEY_SIZE,
			.cia_max_keysize	= AES_MAX_KEY_SIZE,
			.cia_setkey			= bispe_setkey,
			.cia_encrypt		= bispe_encrypt,
			.cia_decrypt		= bispe_decrypt
		}
	}
};

/* Initialize module */
static int __init bispe_init(void)
{	
	printk(KERN_INFO "bispe_crypto_test: loading test module.\n");

	if(crypto_register_alg(&bispe_alg)) {
		printk(KERN_ERR "bispe_crypto_test: could not register algorithm.\n");
		return 1;
	}

	int res = alg_test("bispe-driver", "aes", CRYPTO_ALG_TYPE_CIPHER, 0);
	printk(KERN_INFO "bispe_crypto_test: result of AES test: %d, %s\n", res, 
		!res ? "passed" : "failed");

	return 0;
}
module_init(bispe_init);

/* Exit module */
static void __exit bispe_exit(void)
{
	crypto_unregister_alg(&bispe_alg);
	printk(KERN_INFO "bispe_crypto_test: exiting test module.\n");
}
module_exit(bispe_exit);
