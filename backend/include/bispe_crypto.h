/***************************************************************************
 * bispe_crypto.h
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

#ifndef _BISPE_CRYPTO_H
#define _BISPE_CRYPTO_H

#include <linux/kernel.h>
#include <linux/types.h>

#define BISPE_KDF_ITER 2000

void bispe_sha256(const char *message, int msglen, unsigned char *digest);

/* Assembly functions defined in bispe_crypto_asm.S */
asmlinkage void bispe_gen_rkeys(void);
asmlinkage void bispe_clear_avx_regs(void);
asmlinkage void bispe_clear_regs(void);

asmlinkage void bispe_encblk(void);
asmlinkage void bispe_decblk(void);

asmlinkage void bispe_encblk_mem(u8 *out, const u8 *in);
asmlinkage void bispe_decblk_mem(u8 *out, const u8 *in);

asmlinkage void bispe_encblk_mem_cbc(u8 *out, const u8 *in, const u8 *previous);
asmlinkage void bispe_decblk_mem_cbc(u8 *out, const u8 *in, const u8 *previous);

asmlinkage void bispe_set_key(const u8 *in);
asmlinkage void bispe_get_key(u8 *out);

asmlinkage bool bispe_check_features(void);

asmlinkage void bispe_dump_regs(u8 *out);

#endif /* _BISPE_CRYPTO_H */
