/***************************************************************************
 * parser.c
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "ast.h"
#include "lexer.h"

#define ARR_SIZE(A) (sizeof(A)/sizeof(sym_t))

#define TRY_MATCHING(PROD, TYPE) \
	node = match_prod(TYPE, PROD, ARR_SIZE(PROD));\
	if(node != NULL) {\
		return node;\
	}\
	token = token_saved;
	

typedef struct sym_t {
	bool terminal;
	int type;
	union func {
		node_t *(*check) (void);
		node_t *(*create) (token_t *token); // returns a node containing the token
	} func;
} sym_t;

/* check function prototypes */
static node_t *check_type(void);
static node_t *check_var_def(void);
static node_t *check_var_assign(void);
static node_t *check_print(void);
static node_t *check_return(void);
static node_t *check_expr(void);
static node_t *check_expr1(void);
static node_t *check_cond(void);
static node_t *check_numop(void);
static node_t *check_boolop(void);

static node_t *check_statement(void);
static node_t *check_sequence(void);
static node_t *check_branch(void);
static node_t *check_for_loop_head(void);
static node_t *check_loop(void);

static node_t *check_fcall_arglist(void);
static node_t *check_fcall(void);
static node_t *check_ret_fcall(void);

static node_t *check_argdef(void);
static node_t *check_argdeflist(void);
static node_t *check_func(void);
static node_t *check_program(void);

// currently processed token
static token_t *token = NULL;

static node_t *match_prod(ast_type type, sym_t prod[], size_t len) {
	bool matching = true;

	size_t children_len = 0;
	node_t *children[len];

	for(int i = 0; i < len; i++) {
		if(!matching) {
			break;
		}

		if(prod[i].terminal && token != NULL) {
			matching &= token->type == prod[i].type;
			if(matching && prod[i].func.create != NULL) {
				children[children_len] = (*prod[i].func.create)(token);
				children_len++;
			}

			token = token->next;
		} else {
			if(token == NULL) {
				matching = false;
				break;
			}

			children[children_len] = (*prod[i].func.check)();
			if(children[children_len] == NULL) {
				matching = false;
			}
			children_len++;
		}
	}

	node_t *res = NULL;
	if(matching) {
		res = create_node(type, children, children_len);
	} 
	if(res == NULL) {
		for(int i = 0; i < children_len; i++) {
			cleanup_node(children[i]);
		}
	}
	return res;
}

static node_t *check_type(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{true, TOK_VOID, {.create = create_type}},
	};
	TRY_MATCHING(prod, AST_TYPE)

	sym_t prod2[] = {
		{true, TOK_INT, {.create = create_type}},
	};
	TRY_MATCHING(prod2, AST_TYPE)

	return NULL;
}

static node_t *check_var_def(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{false, 0, {.check = check_type}},
		{true, TOK_IDENTIFIER, {.create = create_identifier}},
		{true, TOK_EQ, {.create = NULL}},
		{false, 0, {.check = check_expr}},
	};
	TRY_MATCHING(prod, AST_VAR_DEF)

	sym_t prod2[] = {
		{false, 0, {.check = check_type}},
		{true, TOK_IDENTIFIER, {.create = create_identifier}},
	};
	TRY_MATCHING(prod2, AST_VAR_DEF)

	return NULL;
}

static node_t *check_var_assign(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{true, TOK_IDENTIFIER, {.create = create_identifier}},
		{true, TOK_EQ, {.create = NULL}},
		{false, 0, {.check = check_expr}},
	};
	TRY_MATCHING(prod, AST_VAR_ASSIGN)

	return NULL;
}

static node_t *check_print(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{true, TOK_PRINT, {.create = NULL}},
		{false, 0, {.check = check_expr}},
	};
	TRY_MATCHING(prod, AST_PRINT)

	return NULL;
}

static node_t *check_return(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{true, TOK_RETURN, {.create = NULL}},
		{false, 0, {.check = check_expr}},
	};
	TRY_MATCHING(prod, AST_RETURN)

	sym_t prod2[] = {
		{true, TOK_RETURN, {.create = NULL}},
	};
	TRY_MATCHING(prod2, AST_RETURN)

	return NULL;
}

static node_t *check_expr(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{false, 0, {.check = check_expr1}},
		{false, 0, {.check = check_numop}},
		{false, 0, {.check = check_expr}},
	};
	TRY_MATCHING(prod, AST_EXPRESSION)

	sym_t prod2[] = {
		{false, 0, {.check = check_expr1}},
	};
	TRY_MATCHING(prod2, AST_EXPRESSION)

	return NULL;
}

static node_t *check_expr1(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{false, 0, {.check = check_ret_fcall}},
	};
	TRY_MATCHING(prod, AST_EXPRESSION1)

	sym_t prod2[] = {
		{true, TOK_IDENTIFIER, {.create = create_identifier}},
	};
	TRY_MATCHING(prod2, AST_EXPRESSION1)

	sym_t prod3[] = {
		{true, TOK_INTLITERAL, {.create = create_intliteral}},
	};
	TRY_MATCHING(prod3, AST_EXPRESSION1)

	sym_t prod4[] = {
		{true, TOK_MINUS, {.create = NULL}},
		{true, TOK_INTLITERAL, {.create = create_neg_intliteral}},
	};
	TRY_MATCHING(prod4, AST_EXPRESSION1)

	sym_t prod5[] = {
		{true, TOK_OPENPA, {.create = NULL}},
		{false, 0, {.check = check_expr}},
		{true, TOK_CLOSEPA, {.create = NULL}},
	};
	TRY_MATCHING(prod5, AST_EXPRESSION1)

	return NULL;
}

static node_t *check_cond(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{false, 0, {.check = check_expr}},
		{false, 0, {.check = check_boolop}},
		{false, 0, {.check = check_expr}},
	};
	TRY_MATCHING(prod, AST_CONDITION)

	return NULL;
}

static node_t *check_numop(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{true, TOK_PLUS, {.create = create_numop}},
	};
	TRY_MATCHING(prod, AST_NUMOP)

	sym_t prod2[] = {
		{true, TOK_MINUS, {.create = create_numop}},
	};
	TRY_MATCHING(prod2, AST_NUMOP)	

	sym_t prod3[] = {
		{true, TOK_STAR, {.create = create_numop}},
	};
	TRY_MATCHING(prod3, AST_NUMOP)

	sym_t prod4[] = {
		{true, TOK_SLASH, {.create = create_numop}},
	};
	TRY_MATCHING(prod4, AST_NUMOP)

	sym_t prod5[] = {
		{true, TOK_PERCENT, {.create = create_numop}},
	};
	TRY_MATCHING(prod5, AST_NUMOP)

	return NULL;
}

static node_t *check_boolop(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{true, TOK_EQEQ, {.create = create_boolop}},
	};
	TRY_MATCHING(prod, AST_BOOLOP)

	sym_t prod2[] = {
		{true, TOK_NEQ, {.create = create_boolop}},
	};
	TRY_MATCHING(prod2, AST_BOOLOP)

	sym_t prod3[] = {
		{true, TOK_GT, {.create = create_boolop}},
	};
	TRY_MATCHING(prod3, AST_BOOLOP)

	sym_t prod4[] = {
		{true, TOK_LT, {.create = create_boolop}},
	};
	TRY_MATCHING(prod4, AST_BOOLOP)

	sym_t prod5[] = {
		{true, TOK_GE, {.create = create_boolop}},
	};
	TRY_MATCHING(prod5, AST_BOOLOP)

	sym_t prod6[] = {
		{true, TOK_LE, {.create = create_boolop}},
	};
	TRY_MATCHING(prod6, AST_BOOLOP)

	return NULL;
}

static node_t *check_statement(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{false, 0, {.check = check_var_def}},
		{true, TOK_SEMICL, {.create = NULL}},
	};
	TRY_MATCHING(prod, AST_STATEMENT)

	sym_t prod2[] = {
		{false, 0, {.check = check_var_assign}},
		{true, TOK_SEMICL, {.create = NULL}},
	};
	TRY_MATCHING(prod2, AST_STATEMENT)

	sym_t prod3[] = {
		{false, 0, {.check = check_fcall}},
		{true, TOK_SEMICL, {.create = NULL}},
	};
	TRY_MATCHING(prod3, AST_STATEMENT)

	sym_t prod4[] = {
		{false, 0, {.check = check_print}},
		{true, TOK_SEMICL, {.create = NULL}},
	};
	TRY_MATCHING(prod4, AST_STATEMENT)

	sym_t prod5[] = {
		{false, 0, {.check = check_return}},
		{true, TOK_SEMICL, {.create = NULL}},
	};
	TRY_MATCHING(prod5, AST_STATEMENT)

	return NULL;
}

static node_t *check_sequence(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{false, 0, {.check = check_statement}},
		{false, 0, {.check = check_sequence}},
	};
	TRY_MATCHING(prod, AST_SEQUENCE)

	sym_t prod2[] = {
		{false, 0, {.check = check_branch}},
		{false, 0, {.check = check_sequence}},
	};
	TRY_MATCHING(prod2, AST_SEQUENCE)

	sym_t prod3[] = {
		{false, 0, {.check = check_loop}},
		{false, 0, {.check = check_sequence}},
	};
	TRY_MATCHING(prod3, AST_SEQUENCE)

	return create_empty(); // this accepts the empty word
}

static node_t *check_branch(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{true, TOK_IF, {.create = NULL}},
		{true, TOK_OPENPA, {.create = NULL}},
		{false, 0, {.check = check_cond}},
		{true, TOK_CLOSEPA, {.create = NULL}},
		{true, TOK_OPENBR, {.create = NULL}},
		{false, 0, {.check = check_sequence}},
		{true, TOK_CLOSEBR, {.create = NULL}},
		{true, TOK_ELSE, {.create = NULL}},
		{true, TOK_OPENBR, {.create = NULL}},
		{false, 0, {.check = check_sequence}},
		{true, TOK_CLOSEBR, {.create = NULL}},
	};
	TRY_MATCHING(prod, AST_BRANCH)

	sym_t prod2[] = {
		{true, TOK_IF, {.create = NULL}},
		{true, TOK_OPENPA, {.create = NULL}},
		{false, 0, {.check = check_cond}},
		{true, TOK_CLOSEPA, {.create = NULL}},
		{true, TOK_OPENBR, {.create = NULL}},
		{false, 0, {.check = check_sequence}},
		{true, TOK_CLOSEBR, {.create = NULL}},
	};
	TRY_MATCHING(prod2, AST_BRANCH)

	return NULL;
}

static node_t *check_for_loop_head(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{false, 0, {.check = check_var_def}},
		{true, TOK_SEMICL, {.create = NULL}},
		{false, 0, {.check = check_cond}},
		{true, TOK_SEMICL, {.create = NULL}},
		{false, 0, {.check = check_var_assign}},
	};
	TRY_MATCHING(prod, AST_FOR_LOOP_HEAD)

	sym_t prod2[] = {
		{false, 0, {.check = check_var_assign}},
		{true, TOK_SEMICL, {.create = NULL}},
		{false, 0, {.check = check_cond}},
		{true, TOK_SEMICL, {.create = NULL}},
		{false, 0, {.check = check_var_assign}},
	};
	TRY_MATCHING(prod2, AST_FOR_LOOP_HEAD)

	return NULL;
}

static node_t *check_loop(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{true, TOK_WHILE, {.create = NULL}},
		{true, TOK_OPENPA, {.create = NULL}},
		{false, 0, {.check = check_cond}},
		{true, TOK_CLOSEPA, {.create = NULL}},
		{true, TOK_OPENBR, {.create = NULL}},
		{false, 0, {.check = check_sequence}},
		{true, TOK_CLOSEBR, {.create = NULL}},
	};
	TRY_MATCHING(prod, AST_LOOP)

	sym_t prod2[] = {
		{true, TOK_FOR, {.create = NULL}},
		{true, TOK_OPENPA, {.create = NULL}},
		{false, 0, {.check = check_for_loop_head}},
		{true, TOK_CLOSEPA, {.create = NULL}},
		{true, TOK_OPENBR, {.create = NULL}},
		{false, 0, {.check = check_sequence}},
		{true, TOK_CLOSEBR, {.create = NULL}},
	};
	TRY_MATCHING(prod2, AST_LOOP)

	sym_t prod3[] = {
		{true, TOK_DO, {.create = NULL}},
		{true, TOK_OPENBR, {.create = NULL}},
		{false, 0, {.check = check_sequence}},
		{true, TOK_CLOSEBR, {.create = NULL}},
		{true, TOK_WHILE, {.create = NULL}},
		{true, TOK_OPENPA, {.create = NULL}},
		{false, 0, {.check = check_cond}},
		{true, TOK_CLOSEPA, {.create = NULL}},
		{true, TOK_SEMICL, {.create = NULL}},
	};
	TRY_MATCHING(prod3, AST_LOOP)

	return NULL;
}

static node_t *check_fcall_arglist(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{false, 0, {.check = check_expr}},
		{true, TOK_COMMA, {.create = NULL}},
		{false, 0, {.check = check_fcall_arglist}},
	};
	TRY_MATCHING(prod, AST_FCALL_ARGLIST)

	sym_t prod2[] = {
		{false, 0, {.check = check_expr}},
	};
	TRY_MATCHING(prod2, AST_FCALL_ARGLIST)

	return create_empty(); // this accepts the empty word;
}

static node_t *check_ret_fcall(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{true, TOK_IDENTIFIER, {.create = create_identifier}},
		{true, TOK_OPENPA, {.create = NULL}},
		{false, 0, {.check = check_fcall_arglist}},
		{true, TOK_CLOSEPA, {.create = NULL}},
	};
	TRY_MATCHING(prod, AST_RET_FCALL)

	return NULL;
}

static node_t *check_fcall(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{true, TOK_IDENTIFIER, {.create = create_identifier}},
		{true, TOK_OPENPA, {.create = NULL}},
		{false, 0, {.check = check_fcall_arglist}},
		{true, TOK_CLOSEPA, {.create = NULL}},
	};
	TRY_MATCHING(prod, AST_FCALL)

	return NULL;
}

static node_t *check_argdef(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{false, 0, {.check = check_type}},
		{true, TOK_IDENTIFIER, {.create = create_identifier}},
	};
	TRY_MATCHING(prod, AST_ARGDEF)

	return NULL;
}

static node_t *check_argdeflist(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{false, 0, {.check = check_argdef}},
		{true, TOK_COMMA, {.create = NULL}},
		{false, 0, {.check = check_argdeflist}},
	};
	TRY_MATCHING(prod, AST_ARGDEFLIST)

	sym_t prod2[] = {
		{false, 0, {.check = check_argdef}},
	};
	TRY_MATCHING(prod2, AST_ARGDEFLIST)

	sym_t prod3[] = {
		{true, TOK_VOID, {.create = NULL}},
	};
	TRY_MATCHING(prod3, AST_ARGDEFLIST)

	return NULL;
}

static node_t *check_func(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	set_max_arg_count(0);

	sym_t prod[] = {
		{false, 0, {.check = check_type}},
		{true, TOK_IDENTIFIER, {.create = create_identifier}},
		{true, TOK_OPENPA, {.create = NULL}},
		{false, 0, {.check = check_argdeflist}},
		{true, TOK_CLOSEPA, {.create = NULL}},
		{true, TOK_OPENBR, {.create = NULL}},
		{false, 0, {.check = check_sequence}},
		{true, TOK_CLOSEBR, {.create = NULL}},
	};
	TRY_MATCHING(prod, AST_FUNCTION)

	sym_t prod2[] = {
		{false, 0, {.check = check_type}},
		{true, TOK_IDENTIFIER, {.create = create_identifier}},
		{true, TOK_OPENPA, {.create = NULL}},
		{false, 0, {.check = check_argdeflist}},
		{true, TOK_CLOSEPA, {.create = NULL}},
		{true, TOK_SEMICL, {.create = NULL}},
	};
	TRY_MATCHING(prod2, AST_FUNCTION_PROTO)

	return NULL;
}

static node_t *check_program(void) {
	token_t *token_saved = token;
	node_t *node = NULL;

	sym_t prod[] = {
		{false, 0, {.check = check_func}},
		{false, 0, {.check = check_program}},
	};
	TRY_MATCHING(prod, AST_INNER);

	return create_empty(); // this accepts the empty word
}

static node_t *check_start(void) {
	sym_t prod[] = {
		{false, 0, {.check = check_program}},
		{true, TOK_EOF, {.create = NULL}},
	};

	node_t *node = match_prod(AST_HEAD, prod, ARR_SIZE(prod));
	return node;
}

node_t *parse_tokenlist(token_t *first) {
	token = first;
	return check_start();
}
