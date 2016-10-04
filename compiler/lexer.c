/***************************************************************************
 * lexer.c
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

struct keyword_t {
	const char *keyword;
	const token_type type;
};

static const struct keyword_t keywords[] = {
	{"void", TOK_VOID},
	{"int", TOK_INT},
	{"if", TOK_IF},
	{"else", TOK_ELSE},
	{"while", TOK_WHILE},
	{"for", TOK_FOR},
	{"do", TOK_DO},
	{"return", TOK_RETURN},
	{"print", TOK_PRINT},
};

void cleanup_tokens(token_t *token) {
	while(token != NULL) {
		if(token->val != NULL) {
			free(token->val);
		}
		token_t *next = token->next;
		free(token);
		token = next;
	}
}

static token_t *create_next_token(token_t *last) {
	token_t *tmp = malloc(sizeof(token_t));
	if(tmp == NULL) {
		fprintf(stderr, "error: could not allocate memory for token list\n");
		return NULL;
	}
	
	tmp->next = NULL;
	tmp->type = -1;
	tmp->val = NULL;

	if(last != NULL) {
		last->next = tmp;
	}

	return tmp;
}

static int scan_intliteral(char *str, int pos, token_t *next, int line) {
	int i = pos;
	int is_hex = 0;
	int is_oct = 0;

	// scan for prefix
	if(str[i] == '0') {
		if(str[i+1] == 'x') { // allow hexadecimal notation
			is_hex = 1;
			i = i+2;
		} else if(str[i+1] != '\0') { // allow octal notation
			is_oct = 1;
			i = i+1;
		}
	}

	do {
		char ch = str[i];
		if(is_oct && (ch == '8' || ch == '9')) {
			fprintf(stderr, 
				"lexer: illegal character '%c' in int literal (base 8!) at line %i\n",
				ch, line);
			return -1;
		} else if(ch >= '0' && ch <= '9') {
			i++;
		} else if(is_hex && ((ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))) {
			i++;
		} else if((ch >= 'A' && ch <= 'Z') 
			|| (ch >= 'a' && ch <= 'z')
		 	|| ch == '_') {
			fprintf(stderr, "lexer: illegal character '%c' in int literal at line %i\n",
				ch, line);
			return -1;
		} else {
			next->val = strndup(str+pos, i-pos);
			if(next->val == NULL) {
				fprintf(stderr, "error: could not allocate memory for string in token list\n");
				return -1;
			}
			next->type = TOK_INTLITERAL;
			return i - 1;
		}
	} while(1);
	return -1;
}

static int check_ident_type(token_t *next) {
	for(int i = 0; i < sizeof(keywords) / sizeof(struct keyword_t); i++) {
		if(strcmp(keywords[i].keyword, next->val) == 0) {
			return keywords[i].type;
		}
	}

	return TOK_IDENTIFIER;
}

static int scan_ident(char *str, int pos, token_t *next) {
	int i = pos;
	do {
		char ch = str[i];
		if( (ch >= 'A' && ch <= 'Z')
		 || (ch >= 'a' && ch <= 'z')
		 || (ch >= '0' && ch <= '9')
		 ||  ch == '_') {
			i++;
		} else {
			next->val = strndup(str+pos, i-pos);
			if(next->val == NULL) {
				fprintf(stderr, "error: could not allocate memory for string in token list\n");
				return -1;
			}
			next->type = check_ident_type(next);
			return i - 1;
		}
	} while(1);
	return -1;
}

token_t *tokenize_string(char *str) {
	token_t *first = create_next_token(NULL);
	if(first == NULL) {
		return NULL;
	}

	token_t *next = first;

	int pos = 0;
	int line = 1;
	char ch = str[0];
	while(ch != '\0') {
		switch(ch) {
		// ignore separators
		case '\n':
			line++;
		case ' ': case '\t': case '\r':
			ch = str[++pos];
			continue;

		// identifier
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
		case 's': case 't': case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z': case '_':
			pos = scan_ident(str, pos, next);
			if(pos == -1) {
				return NULL;
			}
			break;

		// int literals
		case '0': case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			pos = scan_intliteral(str, pos, next, line);
			if(pos == -1) {
				return NULL;
			}
			break;

		// brackets and separators
		case '(':
			next->type = TOK_OPENPA;
			break;
		case ')':
			next->type = TOK_CLOSEPA;
			break;
		case '{':
			next->type = TOK_OPENBR;
			break;
		case '}':
			next->type = TOK_CLOSEBR;
			break;
		case ',':
			next->type = TOK_COMMA;
			break;
		case ';':
			next->type = TOK_SEMICL;
			break;

		// bool operators
		case '=':
			if(str[pos+1] == '=') {
				next->type = TOK_EQEQ;
				pos++;
			} else {
				next->type = TOK_EQ;
			}
			break;

		case '!':
			if(str[pos+1] == '=') {
				next->type = TOK_NEQ;
				pos++;
			} else {
				fprintf(stderr, "lexer: illegal character '%c' at line %i\n", ch, line);
				cleanup_tokens(first);
				return NULL;
			}
			break;

		case '<':
			if(str[pos+1] == '=') {
				next->type = TOK_LE;
				pos++;
			} else {
				next->type = TOK_LT;
			}
			break;

		case '>':
			if(str[pos+1] == '=') {
				next->type = TOK_GE;
				pos++;
			} else {
				next->type = TOK_GT;
			}
			break;

		// numeric operators
		case '+':
			next->type = TOK_PLUS;
			break;

		case '-':
			next->type = TOK_MINUS;
			break;

		case '*':
			next->type = TOK_STAR;
			break;

		case '/':
			if(str[pos+1] == '/') { // read away comment
				while(str[pos] != '\0' && str[pos] != '\n') {
					pos++;
				}
				ch = str[pos];
				continue;
			} else {
				next->type = TOK_SLASH;
			}
			break;

		case '%':
			next->type = TOK_PERCENT;
			break;

		default:
			fprintf(stderr, "lexer: illegal character '%c' at line %i\n", ch, line);
			cleanup_tokens(first);
			return NULL;
		}

		next = create_next_token(next);
		if(next == NULL) {
			cleanup_tokens(first);
			return NULL;
		}
		ch = str[++pos];
	}
	next->type = TOK_EOF;

	return first;
}
