/***************************************************************************
 * bispe_comm.h
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

#ifndef _BISPE_COMM_H
#define _BISPE_COMM_H

/*
 * This header is shared between front- and backend. 
 * It defines types in which information is exchanged over the sys interface 
 */

/* Holds information about a buffer */
struct buf_info {
	void *ptr;
	size_t size;
};

/* 
 * This struct is used to transfer information over the sys-interface.
 * It contains interpreter settings, input buffers, and output buffers. 
 * Instead of copying buffers directly, only pointers are passed to kernel space.
 */
struct invoke_ctx {
	/* 
	 * User specified interpreter settings: 0 for default value 
	 * stack/call_size are specifying the amount of 128 bit lines used in the segment
	 */
	unsigned int stack_size;
	unsigned int call_size;
	unsigned int ipc;

	/* Userspace buffer containing the program */
	struct buf_info code_buf;

	/* Userspace buffer containing commandline arguments */
	struct buf_info arg_buf;

	/* Pointer to userspace variable to which interpreter result is passed */
	int *result;
	
	/* Userspace buffer to which program output is written */
	struct buf_info out_buf;
};

#endif /* _BISPE_COMM_H */