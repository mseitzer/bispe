/***************************************************************************
 * ast.c
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "symbols.h"

// maximum amount of arguments from functions called by the currently processed function
static int max_arg_count = 0;

void set_max_arg_count(int i) {
	max_arg_count = i;
}

static node_t *alloc_node(void) {
	node_t *node = malloc(sizeof(node_t));
	if(node == NULL) {
		fprintf(stderr, "ast: fatal error allocating node. exiting.\n");
		exit(EXIT_FAILURE);
	}
	node->left = NULL;
	node->middle = NULL;
	node->right = NULL;
	return node;
}

node_t *create_identifier(token_t *token) {
	node_t *node = alloc_node();
	node->type = AST_IDENTIFIER;
	node->val.str = token->val;
	return node;
}

node_t *create_intliteral(token_t *token) {
	node_t *node = alloc_node();
	node->type = AST_INTLITERAL;
	
	// convert string to integer
	errno = 0; // set errno to zero before to detect overflows
	long int number = strtol(token->val, NULL, 0);
	if(errno == ERANGE || number > UINT32_MAX || number < INT32_MIN) {
		fprintf(stderr, "parser error: overflow in int literal '%s'\n", token->val);
		free(node);
		exit(EXIT_FAILURE);
	}
	node->val.number = number;

	return node;
}

node_t *create_neg_intliteral(token_t *token) {
	node_t *node = create_intliteral(token);
	node->val.number = (-1) * node->val.number;
	return node;
}

node_t *create_numop(token_t *token) {
	node_t *node = alloc_node();
	node->type = AST_NUMOP_LEAF;
	node->val.type = token->type;
	return node;
}

node_t *create_boolop(token_t *token) {
	node_t *node = alloc_node();
	node->type = AST_BOOLOP_LEAF;
	node->val.type = token->type;
	return node;
}

node_t *create_type(token_t *token) {
	node_t *node = alloc_node();
	node->type = AST_TYPELEAF;
	node->val.type = token->type;
	return node;
}

static int check_return_validity(func_info *func, node_t *node) {
	if(node == NULL) {
		return 1;
	} else if(node->type != AST_RETURN) {
		if(check_return_validity(func, node->left)
			&& check_return_validity(func, node->middle)
			&& check_return_validity(func, node->right)) {
			return 1;
		} else {
			return 0;
		}
	}

	if(node->left != NULL && func->ret_type == DTYPE_VOID) {
		fprintf(stderr, "error: return with a value, in function '%s' returning void.\n", 
			func->name);
		return 0;
	} else if(node->left == NULL && func->ret_type != DTYPE_VOID) {
		fprintf(stderr, "error: return with no value, in function '%s' returning non-void.\n", 
			func->name);
		return 0;
	}
	return 1;
}

static int register_loc_vars(func_info *func, node_t *node) {
	if(node == NULL) {
		return 1;
	}
	
	if(node->type == AST_VAR_DEF) {
		dtype type = tok_type_to_dtype(node->left->left->val.type);
		char *name = node->middle->val.str;
		
		var_info *var = add_var(name, type, func);
		if(var == NULL) {
			fprintf(stderr, 
				"error: duplicate declaration of local variable '%s' in function '%s'.\n",
				name, func->name);
			return 0;
		}
		node->val.var = var;
		
		// calculate address of variable in the function's stack frame
		var->pos = func->max_call_size + func->var_count;
		
		func->var_count = func->var_count + 1;

		// restructure and cleanup node
		cleanup_node(node->left);
		cleanup_node(node->middle);
		node->left = node->right;
		node->middle = NULL;
		node->right = NULL;

		// parse variable identifiers used in assignment expression
		if(!register_loc_vars(func, node->left)) {
			return 0;
		}

	} else if(node->type == AST_VAR_ASSIGN) {
		char *name = node->left->val.str;
		
		var_info *var = get_var(name, func);
		if(var == NULL) {
			fprintf(stderr, 
				"error: assignment of previously undeclared variable '%s' in function '%s'\n",
				name, func->name);
			return 0;
		}

		node->val.var = var;

		// restructure and cleanup node
		cleanup_node(node->left);
		node->left = node->middle;
		node->middle = NULL;

		// parse variable identifiers used in assignment expression
		if(!register_loc_vars(func, node->left)) {
			return 0;
		}
	} else if(node->type == AST_IDENTIFIER) {
		char *name = node->val.str;

		var_info *var = get_var(name, func);
		if(var == NULL) {
			fprintf(stderr, 
				"error: use of previously undeclared variable '%s' in function '%s'\n",
				name, func->name);
			return 0;
		}

		node->val.var = var;
	} else {
		if(!(register_loc_vars(func, node->left)
			&& register_loc_vars(func, node->middle)
			&& register_loc_vars(func, node->right))) {
			return 0;
		}
	}
	return 1;
}

static void calculate_arg_addresses(func_info *func, node_t *node) {
	if(node == NULL) {
		return;
	}

	if(node->type == AST_ARGDEF) {
		char *name = node->middle->val.str;
		var_info *var = get_var(name, func);

		var->pos = var->pos + func->var_count + func->max_call_size;

	} else { // recursive parsing of arg tree
		calculate_arg_addresses(func, node->left);
		calculate_arg_addresses(func, node->middle);
	}
}

static int register_args(func_info *func, node_t *node) {
	if(node == NULL) {
		return 1;
	}

	// register new argument to var table
	if(node->type == AST_ARGDEF) {
		dtype type = tok_type_to_dtype(node->left->left->val.type);
		char *name = node->middle->val.str;

		var_info *var = add_var(name, type, func);
		if(var == NULL) {
			fprintf(stderr, "error: duplicate declaration of argument '%s' in function '%s'.\n",
				name, func->name);
			return 0;
		}

		// set argument position relative to the first argument
		// leftmost function argument has position 0
		// absolute stack frame address will be calculated later if argument count is known
		var->pos = func->arg_count + 1;
		func->arg_count += 1;

	} else { // recursive parsing of arg tree
		if(!(register_args(func, node->left)
			&& register_args(func, node->middle))) {
			return 0;
		}
	}
	return 1;
}

static node_t *register_function_prototype(node_t *node, node_t *children[]) {
	dtype ret_type = tok_type_to_dtype(children[0]->left->val.type);
	char *name = children[1]->val.str;
	node_t *arglist = children[2];

	// register new function in function table
	func_info *func = add_func(name, ret_type);
	if(func == NULL) {
		fprintf(stderr, "error: duplicate declaration of function '%s'.\n", name);
		return NULL;
	}

	func->prototyped = 1;

	// add arguments to function var table
	if(register_args(func, arglist) == 0) {
		return NULL;
	}

	// cleanup no longer referenced nodes
	cleanup_node(children[0]);
	cleanup_node(children[1]);
	cleanup_node(children[2]);

	// change node type to dummy type
	node->type = AST_INNER;
	return node;
}

static node_t *register_function(node_t *node, node_t *children[]) {
	dtype ret_type = tok_type_to_dtype(children[0]->left->val.type);
	char *name = children[1]->val.str;
	node_t *arglist = children[2];

	func_info *func = get_func(name);
	if(func == NULL) {
		// register new function in function table
		func = add_func(name, ret_type);

		// add arguments to function's var table
		if(register_args(func, arglist) == 0) {
			return NULL;
		}
	} else if(!func->prototyped) {
		fprintf(stderr, "error: duplicate declaration of function '%s'.\n", name);
		return NULL;
	} else if(func->prototyped) {
		func->prototyped = 0;
	}
	
	func->max_call_size = max_arg_count;

	// register local vars
	if(register_loc_vars(func, children[3]) == 0) {
		return NULL;
	}

	// calculate addresses of argument variables
	calculate_arg_addresses(func, arglist);

	// check for well-formed return statements
	if(check_return_validity(func, children[3]) == 0) {
		return NULL;
	}

	node->val.func = func;
	node->left = children[3]; // function body

	// cleanup no longer referenced nodes
	cleanup_node(children[0]);
	cleanup_node(children[1]);
	cleanup_node(children[2]);

	return node;
}

static node_t *create_fcall(node_t *node, node_t *children[], int must_return) {
	char *name = children[0]->val.str;

	func_info *func = get_func(name);
	if(func == NULL) {
		fprintf(stderr, "error: call of undeclared function '%s'.\n", name);
		return NULL;
	}

	if(must_return && func->ret_type == DTYPE_VOID) {
		fprintf(stderr, "error: void-returning call of function '%s' in expression.\n", name);
		return NULL;
	}

	// count arguments of this function call
	int arg_count = 0;
	node_t *it = children[1];
	while(it != NULL && it->type == AST_FCALL_ARGLIST) {
		arg_count++;
		it = it->middle;
	}

	// too few/many arguments
	if(arg_count != func->arg_count) {
		fprintf(stderr, "error: argument count in call of function '%s' not matching. \
expected %d, but got %d.\n", name, func->arg_count, arg_count);
		return NULL;
	}

	// update global argument count if needed
	if(arg_count > max_arg_count) {
		max_arg_count = arg_count;
	}

	node->val.func = func;
	node->left = children[1]; // argument list
	
	cleanup_node(children[0]);

	return node;
}

node_t *create_node(ast_type type, node_t *children[], size_t len) {
	node_t *node = alloc_node();
	node->type = type;

	node_t *res = NULL;	
	switch(type) {
		case AST_FUNCTION:
			res = register_function(node, children);
			break;

		case AST_FUNCTION_PROTO:
			res = register_function_prototype(node, children);
			break;

		case AST_FCALL:
			res = create_fcall(node, children, 0);
			break;

		case AST_RET_FCALL:
			res = create_fcall(node, children, 1);
			break;

		default:
			node->left = (len >= 1) ? children[0] : NULL;
			node->middle = (len >= 2) ? children[1] : NULL;
			node->right = (len >= 3) ? children[2] : NULL;
			res = node;
	}

	if(res == NULL) {
		free(node);
	}
	return res;
}

node_t *create_empty(void) {
	node_t *node = alloc_node();
	node->type = AST_EMPTY;
	return node;
}

void cleanup_node(node_t *node) {
	if(node == NULL) {
		return;
	}

	cleanup_node(node->left);
	cleanup_node(node->middle);
	cleanup_node(node->right);

	free(node);
}

void print_ast(node_t *node, int level) {
	if(node == NULL || node->type == AST_EMPTY) {
		return;
	} 

	for(int i = 0; i < level; i++) {
		printf("\t");
	}

	switch(node->type) {
		case AST_IDENTIFIER:
			printf("LEAF: %s\n", node->val.str);
			break;
		case AST_INTLITERAL:
			printf("LEAF: %i\n", node->val.number);
			break;
		case AST_NUMOP_LEAF:
		case AST_BOOLOP_LEAF:
			printf("LEAF: %i\n", node->val.type);
			break;
		default:
			printf("INNER_NODE: %i\n", node->type);
			print_ast(node->left, level+1);
			print_ast(node->middle, level+1);
			print_ast(node->right, level+1);
	}
}
