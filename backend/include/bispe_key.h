/***************************************************************************
 * bispe_key.h
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

#ifndef _BISPE_KEY_H
#define _BISPE_KEY_H

#include <linux/kobject.h>

ssize_t hash_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

ssize_t password_store(struct kobject *kobj, struct kobj_attribute *attr,
					   const char *buf, size_t count);

#endif /* _BISPE_KEY_H */