/***************************************************************************
 * bispe_interpreter.h
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

#ifndef _BISPE_INTERPRETER_H
#define _BISPE_INTERPRETER_H

extern struct runtime_ctx runtime_ctx;

uint32_t *bispe_get_print_seg_bp(void);
size_t bispe_get_print_count(void);

int start_interpreter(struct runtime_ctx *runtime_ctx, int threaded);

struct runtime_ctx *init_interpreter(struct invoke_ctx *invoke_ctx);
struct runtime_ctx *init_interpreter_intern(struct invoke_ctx *invoke_ctx);

void cleanup_runtime_ctx(struct runtime_ctx *runtime_ctx);

#ifdef TESTS
void run_interpreter_tests(void);
#endif

#endif /* _BISPE_INTERPRETER_H */
