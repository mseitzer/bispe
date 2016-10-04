/***************************************************************************
 * generator.c
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ast.h"
#include "generator.h"
#include "instructions.h"
#include "symbols.h"

struct code_t {
	struct code_t *prev;
	struct code_t *next;
	instr_type instr;
	uint32_t arg;
};

static int error = 0;
static int size = 0; // current size of code segment in 4 bytes

static code_t *first = NULL;
static code_t *current = NULL;

static func_info *current_func = NULL;

static instr_type boolop_to_jmp_instr(token_type type) {
	switch(type) {
		case TOK_EQEQ:
			return INSTR_JEQ;
		case TOK_NEQ:
			return INSTR_JNE;
		case TOK_GT:
			return INSTR_JG;
		case TOK_LT:
			return INSTR_JL;
		case TOK_GE:
			return INSTR_JGE;
		case TOK_LE:
			return INSTR_JLE;
		default:
			return INSTR_NOP; // this should never happen
	}
}

static instr_type neg_jmp_instr(instr_type type) {
	switch(type) {
		case INSTR_JEQ:
			return INSTR_JNE;
		case INSTR_JNE:
			return INSTR_JEQ;
		case INSTR_JG:
			return INSTR_JLE;
		case INSTR_JL:
			return INSTR_JGE;
		case INSTR_JGE:
			return INSTR_JL;
		case INSTR_JLE:
			return INSTR_JG;
		default:
			return INSTR_NOP; // this should never happen
	}
}

static code_t *push_elem(instr_type instr, uint32_t arg) {
	code_t *elem = malloc(sizeof(code_t));
	if(elem == NULL) {
		fprintf(stderr, "generator: fatal error allocating element. exiting.\n");
		cleanup_code(elem);
		exit(EXIT_FAILURE);
	}

	if(first == NULL) {
		first = elem;
	}

	elem->prev = current;
	elem->next = NULL;
	elem->instr = instr;
	elem->arg = arg;

	if(current != NULL) {
		current->next = elem;
	}
	current = elem;

	size += 1;
	if(instr_info[instr].has_arg) {
		size += 1;
	}

	return elem;
}

static void push_func_prolog(func_info *func) {
	int frame_size = func->var_count + func->max_call_size;
	if(frame_size > 0) {
		push_elem(INSTR_PROLOG, frame_size);
	}
}

static void push_func_epilog(func_info *func) {
	int frame_size = func->var_count + func->max_call_size;
	if(frame_size > 0) {
		push_elem(INSTR_EPILOG, frame_size);
	}
}

static void generate_helper(node_t *node) {
	if(node == NULL || node->type == AST_EMPTY) {
		return;
	}

	switch(node->type) {
		case AST_IDENTIFIER:
			push_elem(INSTR_LOAD, node->val.var->pos);
			break;

		case AST_INTLITERAL:
			push_elem(INSTR_PUSH, node->val.number);
			break;

		case AST_VAR_DEF:
		case AST_VAR_ASSIGN:
			if(node->left != NULL) { // ignore pure variable definitions
				generate_helper(node->left); // calculate value of assignment
				push_elem(INSTR_STORE, node->val.var->pos);
			}
			break;

		case AST_PRINT:
			generate_helper(node->left);
			push_elem(INSTR_PRINT, 0);
			break;

		case AST_RETURN:
			generate_helper(node->left);
			push_func_epilog(current_func);
			push_elem(INSTR_RET, 0);
			break;

		case AST_EXPRESSION:
			generate_helper(node->left);
			generate_helper(node->right);
			generate_helper(node->middle); // = operator
			break;

		case AST_NUMOP_LEAF:
			if(node->val.type == TOK_PLUS) {
				push_elem(INSTR_ADD, 0);
			} else if(node->val.type == TOK_MINUS) {
				push_elem(INSTR_SUB, 0);
			} else if(node->val.type == TOK_STAR) {
				push_elem(INSTR_MUL, 0);
			} else if(node->val.type == TOK_SLASH) {
				push_elem(INSTR_DIV, 0);
			} else if(node->val.type == TOK_PERCENT) {
				push_elem(INSTR_MOD, 0);
			}
			break;

		case AST_BOOLOP_LEAF:
			// push the corresponding jump instruction
			// branch or loop will change it if needed
			push_elem(boolop_to_jmp_instr(node->val.type), 0);
			break;

		case AST_CONDITION:
			generate_helper(node->left);
			generate_helper(node->right);
			generate_helper(node->middle); // compare operator
			break;

		case AST_BRANCH:
			generate_helper(node->left); // condition

			code_t *cmp_instr = current;
			// jump if condition is not met
			cmp_instr->instr = neg_jmp_instr(cmp_instr->instr);

			generate_helper(node->middle); // if-branch

			// push jmp instruction if if-else-branch
			code_t *jmp_instr = NULL;
			if(node->right != NULL) {
				jmp_instr = push_elem(INSTR_JMP, 0);
			}

			// set compare jump target to next instruction address
			cmp_instr->arg = size;

			if(node->right != NULL) { // else-branch
				generate_helper(node->right);
				jmp_instr->arg = size;
			}

			break;

		case AST_LOOP:
			; /* empty statement: this has to be here, because the label
				may not be before a declaration (strange C grammar) */

			int loop_type;
			node_t *loop_condition, *loop_body;

			// determine what kind of loop we have
			if(node->left->type == AST_CONDITION) {
				// while-loop
				loop_type = 0;
				loop_condition = node->left;
				loop_body = node->middle; 
			} else if(node->left->type == AST_FOR_LOOP_HEAD) {
				// for-loop
				loop_type = 1;
				loop_condition = node->left->middle;
				loop_body = node->middle;
			} else {
				// do-while-loop
				loop_type = 2;
				loop_condition = node->middle;
				loop_body = node->left;
			}

			// if for-loop, generate loop variable initialization
			if(loop_type == 1) {
				generate_helper(node->left->left);
			}

			code_t *forward_jmp_instr;
			/* push jmp instruction to condition check, if while-loop or for-loop
				(if do-while loop, we execute the body once before condition check) */
			if(loop_type <= 1) {
				forward_jmp_instr = push_elem(INSTR_JMP, 0);
			}
			
			// save address for backjump after condition check
			int back_jmp_target = size;

			// generate sequence
			generate_helper(loop_body);

			// if for-loop, generate loop variable increment assignment
			if(loop_type == 1) {
				generate_helper(node->left->right);
			}

			// set forward jump target to address of condition, if while-loop or for-loop
			if(loop_type <= 1) {
				forward_jmp_instr->arg = size;
			}

			// generate condition
			generate_helper(loop_condition);
			
			// set backwards jump target to address of sequence
			current->arg = back_jmp_target;

			break;

		case AST_FCALL:
		case AST_RET_FCALL:
			generate_helper(node->left); // calculate arguments

			// store arguments from stack to call stack
			for(int i = node->val.func->arg_count-1; i >= 0; i--) {
				push_elem(INSTR_STORE, i);
			}

			push_elem(INSTR_CALL, node->val.func->addr);
			break;

		case AST_FUNCTION:
			current_func = node->val.func;
			current_func->addr = size;

			push_func_prolog(current_func);

			// function body
			generate_helper(node->left);
			
			// if last statement in function body was not return, 
			// add function epilog and return statement
			if(current->instr != INSTR_RET) {
				push_func_epilog(current_func);
				push_elem(INSTR_RET, 0);
			}
			break;

		case AST_HEAD: ;
			// get function handle to main
			func_info *main_func = get_func("main");
			if(main_func == NULL) {
				fprintf(stderr, "error: could not create entry point. "
					"main function not found\n");
				error = 1;
				break;
			}

			// if needed, allocate call stack space for program arguments
			// we do not need to free this space, as the program halts anyway
			if(main_func->arg_count > 0) {
				push_elem(INSTR_PROLOG, main_func->arg_count);
			}

			// generate load instructions for program arguments
			for(int i = 0; i < main_func->arg_count; i++) {
				push_elem(INSTR_ARGLOAD, i);
			}

			// add entry point call to main function
			code_t *entry_point = push_elem(INSTR_CALL, 0);

			// add instruction terminating the program
			push_elem(INSTR_FINISH, 0);

			// generate program
			generate_helper(node->left);

			// now, the address of main is known
			// -> update entry point address to main address
			entry_point->arg = main_func->addr;

			break;

		default:
			generate_helper(node->left);
			generate_helper(node->middle);
			generate_helper(node->right);
	}
}

code_t *generate_code(node_t *head) {
	generate_helper(head);

	if(error) {
		cleanup_code(first);
		return NULL;
	}
	return first;
}

void cleanup_code(code_t *elem) {
	if(elem == NULL) {
		return;
	}

	cleanup_code(elem->next);
	free(elem);
}

/* writes size bytes of random data to buf */
static int write_random_bytes(char *buf, size_t size) {
	int fd = open("/dev/urandom", O_RDONLY);
	if(fd == -1) {
		return 1;
	}

	size_t bytes_written = 0;
	while(1) {
		size_t bytes_read = read(fd, buf+bytes_written, size-bytes_written);
		if(bytes_read == -1) {
			return 1;
		}

		bytes_written += bytes_read;

		if(bytes_written >= size) {
			break;
		}
	}
	close(fd);

	return 0;
}

uint32_t *build_codebuf(code_t *head, size_t *len, int unencrypted) {
	int size = 0;
	code_t *cur = head;

	// count bytecode size
	while(cur != NULL) {
		size++;
		if(instr_info[cur->instr].has_arg) {
			size++;
		}
		cur = cur->next;
	}

	int buf_size = size + (4 - (size % 4)); // round to 128 bits

	// add space for init vector if encrypted
	if(!unencrypted) {
		buf_size += 4;
	}

	*len = buf_size;

	uint32_t *buf = malloc((buf_size * sizeof(uint32_t)));
	if(buf == NULL) {
		fprintf(stderr, "error: could not allocate memory for output string\n");
		return NULL;
	}

	int pos = 0;

	// write init vector to first 16 bytes
	if(!unencrypted) {
		if(write_random_bytes((char *) buf, 16) != 0) {
			fprintf(stderr, "error: could not generate random data\n");
			free(buf);
			return NULL;
		}
		pos = 4;
	}

	cur = head;

	// copy opcodes
	while(cur != NULL) {
		buf[pos++] = instr_info[cur->instr].opcode;

		if(instr_info[cur->instr].has_arg) {
			buf[pos++] = cur->arg;
		}

		cur = cur->next;
	}

	// end with FINISH instruction
	buf[pos++] = instr_info[INSTR_FINISH].opcode;

	// fill up with random padding data
	size_t bytes_to_end = (buf_size - pos) * sizeof(uint32_t);
	if(write_random_bytes((char *) (buf+pos), bytes_to_end) != 0) {
		fprintf(stderr, "error: could not generate random data\n");
		free(buf);
		return NULL;
	}

	return buf;
}

/* Print opcode stream to stdout.
 *   mode = 1: print mnemonics
 *        = 2: print opcodes
 */
void print_code(code_t *head, int mode) {
	uint32_t addr = 0;
	code_t *cur = head;
	while(cur != NULL) {
		printf("%4d: ", addr);
		if(mode == 1) {
			printf("%s", instr_info[cur->instr].name);
		} else if(mode == 2) {
			printf("%08x", instr_info[cur->instr].opcode);
		}
		if(instr_info[cur->instr].has_arg) {
			printf("\t%08x", cur->arg);
			addr += 1;
		}
		printf("\n");
		cur = cur->next;
		addr += 1;
	}
}
