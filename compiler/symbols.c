/***************************************************************************
 * symbols.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "sglib.h"
#include "symbols.h"
#include "types.h"

#define STRLIST_COMPARATOR(e1, e2) strcmp(e1->name, e2->name)

static int func_table_init = 0;
static func_info *func_table[HASHTABLE_SIZE];

static unsigned long djb2_hash(char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

static unsigned int var_info_hash(var_info *var) {
	return djb2_hash(var->name);
}

static unsigned int func_info_hash(func_info *func) {
	return djb2_hash(func->name);
}

SGLIB_DEFINE_LIST_PROTOTYPES(var_info, STRLIST_COMPARATOR, next)
SGLIB_DEFINE_LIST_FUNCTIONS(var_info, STRLIST_COMPARATOR, next)
SGLIB_DEFINE_LIST_PROTOTYPES(func_info, STRLIST_COMPARATOR, next)
SGLIB_DEFINE_LIST_FUNCTIONS(func_info, STRLIST_COMPARATOR, next)
SGLIB_DEFINE_HASHED_CONTAINER_PROTOTYPES(var_info, HASHTABLE_SIZE, var_info_hash)
SGLIB_DEFINE_HASHED_CONTAINER_FUNCTIONS(var_info, HASHTABLE_SIZE, var_info_hash)
SGLIB_DEFINE_HASHED_CONTAINER_PROTOTYPES(func_info, HASHTABLE_SIZE, func_info_hash)
SGLIB_DEFINE_HASHED_CONTAINER_FUNCTIONS(func_info, HASHTABLE_SIZE, func_info_hash)

func_info *add_func(char *name, dtype ret_type) {
	if(func_table_init == 0) {
		sglib_hashed_func_info_init(func_table);
		func_table_init = 1;
	}

	func_info func = { .name = name };
	if(sglib_hashed_func_info_find_member(func_table, &func) != NULL) {
		return NULL;
	}

	func_info *new_func = malloc(sizeof(func_info));
	if(new_func == NULL) {
		fprintf(stderr, "symbols: fatal error allocating function table entry. exiting.\n");
		exit(EXIT_FAILURE);
	}

	new_func->name = name;
	new_func->ret_type = ret_type;
	new_func->prototyped = 0;

	new_func->arg_count = 0;
	new_func->var_count = 0;
	new_func->max_call_size = 0;

	sglib_hashed_var_info_init(new_func->var_table);

	sglib_hashed_func_info_add(func_table, new_func);

	return new_func;
}

var_info *add_var(char *name, dtype ret_type, func_info *func) {
	if(func == NULL) {
		// global scope; not yet supported
		return NULL;
	}

	var_info var = { .name = name };
	if(sglib_hashed_var_info_find_member(func->var_table, &var) != NULL) {
		return NULL;
	}

	var_info *new_var = malloc(sizeof(func_info));
	if(new_var == NULL) {
		fprintf(stderr, "symbols: fatal error allocating symbol table entry. exiting.\n");
		exit(EXIT_FAILURE);
	}

	new_var->name = name;
	new_var->type = ret_type;
	new_var->pos = 0;

	sglib_hashed_var_info_add(func->var_table, new_var);
	return new_var;
}

func_info *get_func(char *name) {
	func_info func = { .name = name };
	return sglib_hashed_func_info_find_member(func_table, &func);
}

var_info *get_var(char *name, func_info *func) {
	var_info var = { .name = name };
	return sglib_hashed_var_info_find_member(func->var_table, &var);
}

/* iterates over function table and the contained variable tables and frees allocated space */
void cleanup_symbols(void) {
	struct sglib_hashed_func_info_iterator func_it;
	for(func_info *func = sglib_hashed_func_info_it_init(&func_it, func_table);
			func != NULL;
			func = sglib_hashed_func_info_it_next(&func_it)) {
		
		struct sglib_hashed_var_info_iterator var_it;
		for(var_info *var = sglib_hashed_var_info_it_init(&var_it, func->var_table);
				var != NULL;
				var = sglib_hashed_var_info_it_next(&var_it)) {
			
			free(var);
		}

		free(func);
	}
}

void print_function_table(void) {
	struct sglib_hashed_func_info_iterator func_it;
	for(func_info *func = sglib_hashed_func_info_it_init(&func_it, func_table);
		func != NULL;
		func = sglib_hashed_func_info_it_next(&func_it)) {
		
		printf("func name: %s, type: %i, argc: %i\n", 
			func->name, func->ret_type, func->arg_count);

		struct sglib_hashed_var_info_iterator var_it;
		for(var_info *var = sglib_hashed_var_info_it_init(&var_it, func->var_table);
		var != NULL;
		var = sglib_hashed_var_info_it_next(&var_it)) {
			printf("\tvar name: %s, type: %i, addr: %i\n", var->name, var->type, var->pos);
		}
	}	
}

dtype tok_type_to_dtype(token_type type) {
	switch(type) {
		case TOK_VOID:
			return DTYPE_VOID;
		case TOK_INT:
			return DTYPE_INT;
		default:
			return DTYPE_VOID; // this should never happen
	}
}
