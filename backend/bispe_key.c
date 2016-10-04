/***************************************************************************
 * bispe_key.c
 * Provides key setting facilities for user space, derived from TRESOR code
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
 ***************************************************************************/

#include <linux/kernel.h>
#include <linux/smp.h>

#include "bispe_key.h"
#include "bispe_crypto.h"

static unsigned char key_hash[32];

/*
 * Show the SHA256 hash of the key currently in use (adapted from TRESOR)
 * This only works if the key was set through /sys/kernel/bispe/password 
 */
ssize_t hash_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned int i;

	ret = sprintf(&buf[ret], "Key hash:\n");
	for (i = 0; i < 16; i++)
		ret += sprintf(&buf[ret], "%02x ", key_hash[i]);
	ret += sprintf(&buf[ret], "\n");
	for (i = 16; i < 32; i++)
		ret += sprintf(&buf[ret], "%02x ", key_hash[i]);
	ret += sprintf(&buf[ret], "\n");

	return ret;
}

/*
 * Set key function to be called on every cpu (adapted from TRESOR)
 */
static void setkey_current_cpu(void *data)
{
	printk("bispe_password: set key on cpu %d\n", smp_processor_id());
	bispe_set_key((const u8 *)data);
}

/*
 * Set the key using key derivation with SHA256 (adapted from TRESOR)
 */
ssize_t password_store(struct kobject *kobj, struct kobj_attribute *attr,
					   const char *buf, size_t count)
{
	unsigned char password[54], key[32];
	unsigned int i;

	memcpy(password, buf, 54);
	password[53] = '\0';

	/* derive and set key */
	bispe_sha256(password, strlen(password), key);
	for (i = 0; i < BISPE_KDF_ITER; i++) {
		bispe_sha256(key, 32, key_hash);
		bispe_sha256(key_hash, 32, key);
	}

	/* set key on every cpu */
	on_each_cpu(setkey_current_cpu, (void *) key, 1);
	bispe_sha256(key, 32, key_hash);

	/* Reset critical memory chunks */
	memset(password, 0, 54);
	memset(key, 0, 32);

	/* Reset the input buffer (ugly hack, ignoring const) */
	memset((char *)buf, 0, count);
	wbinvd();

	return count;
}
