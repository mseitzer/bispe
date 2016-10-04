/***************************************************************************
 * bispe_state.h
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

#ifndef _BISPE_STATE_H
#define _BISPE_STATE_H

#include <linux/kernel.h>
#include <linux/types.h>

extern uint32_t *bispe_code_seg_bp;
extern uint32_t *bispe_stack_seg_bp;
extern uint32_t *bispe_call_seg_bp;
extern uint32_t *bispe_print_seg_bp;

extern size_t bispe_code_seg_size;
extern size_t bispe_stack_seg_size;
extern size_t bispe_call_seg_size;
extern size_t bispe_print_seg_size;

extern size_t bispe_argc;
extern uint32_t *bispe_argv;

extern uint64_t bispe_instr_per_cycle;

void bispe_set_instr_ptr(uint32_t *ptr);
void bispe_set_stack_ptr(uint32_t *ptr);
void bispe_set_call_ptr(uint32_t *ptr);
void bispe_set_print_ptr(uint32_t *ptr);

uint32_t *bispe_get_print_ptr(void);

bool bispe_chk_halt(void);
bool bispe_chk_error(void);
uint8_t bispe_get_error(void);

void bispe_reset_flags(void);
void bispe_cycle_entry(void);

#endif /* _BISPE_STATE_H */
