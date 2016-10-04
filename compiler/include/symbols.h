/***************************************************************************
 * symbols.h
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

#ifndef SYMBOLS_H 
#define SYMBOLS_H

#include "lexer.h"
#include "types.h"

#define HASHTABLE_SIZE 33

typedef struct var_info {
	char *name; // identifier
	dtype type; // type
	int pos; // local address

	struct var_info *next;
} var_info;

typedef struct func_info {
	char *name; // identifier
	dtype ret_type; // return type
	int prototyped; // was this function previously defined by a prototype
	int addr; // address in code

	int arg_count; // count of arguments
	int var_count; // count of local variables
	int max_call_size; // maximum amount of arguments from functions called by this function

	var_info *var_table[HASHTABLE_SIZE]; // variables declared in this function

	struct func_info *next;
} func_info;

func_info *add_func(char *name, dtype ret_type);
func_info *get_func(char *name);

var_info *add_var(char *name, dtype ret_type, func_info *func);
var_info *get_var(char *name, func_info *func);

void cleanup_symbols(void);
void print_function_table(void);

dtype tok_type_to_dtype(token_type type);

#endif /* SYMBOLS_H */
