/***************************************************************************
 * generator.h
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


#ifndef GENERATOR_H 
#define GENERATOR_H

#include <stdint.h>

#include "ast.h"

typedef struct code_t code_t;

code_t *generate_code(node_t *head);

void cleanup_code(code_t *elem);

uint32_t *build_codebuf(code_t *head, size_t *len, int unencrypted);

void print_code(code_t *head, int mode);

#endif /* GENERATOR_H */
