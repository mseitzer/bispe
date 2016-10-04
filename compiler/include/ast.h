/***************************************************************************
 * ast.h
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

#ifndef AST_H 
#define AST_H

#include <stdint.h>

#include "lexer.h"
#include "symbols.h"

typedef enum {
	AST_HEAD,
	AST_EMPTY,
	AST_INNER, // dummy node type
	AST_IDENTIFIER,
	AST_INTLITERAL,
	AST_TYPE,
	AST_TYPELEAF,
	AST_VAR_DEF,
	AST_VAR_ASSIGN,
	AST_PRINT,
	AST_RETURN,
	AST_EXPRESSION,
	AST_EXPRESSION1, // unary expression
	AST_NUMOP,
	AST_NUMOP_LEAF,
	AST_BOOLOP,
	AST_BOOLOP_LEAF,
	AST_CONDITION,
	AST_STATEMENT,
	AST_SEQUENCE,
	AST_BRANCH,
	AST_FOR_LOOP_HEAD,
	AST_LOOP,
	AST_FCALL_ARGLIST,
	AST_FCALL,
	AST_RET_FCALL, // function call returning sth.
	AST_ARGDEF,
	AST_ARGDEFLIST,
	AST_FUNCTION,
	AST_FUNCTION_PROTO,
} ast_type;

typedef struct node_t {
	ast_type type;
	union val {
		char *str;
		token_type type;
		int32_t number;
		func_info *func;
		var_info *var;
	} val;
	struct node_t *left;
	struct node_t *middle;
	struct node_t *right;
} node_t;

void set_max_arg_count(int i);

node_t *create_identifier(token_t *token);
node_t *create_intliteral(token_t *token);
node_t *create_neg_intliteral(token_t *token);
node_t *create_numop(token_t *token);
node_t *create_boolop(token_t *token);
node_t *create_type(token_t *token);

node_t *create_node(ast_type type, node_t *children[], size_t len);
node_t *create_empty(void);
void cleanup_node(node_t *node);
void print_ast(node_t *node, int level);

#endif /* AST_H */
