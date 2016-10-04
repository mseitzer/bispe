/***************************************************************************
 * lexer.h
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

#ifndef LEXER_H 
#define LEXER_H

/* type defines */
typedef enum {
	TOK_EOF,
	TOK_IDENTIFIER,
	TOK_INTLITERAL,
	TOK_VOID,
	TOK_INT,
	TOK_IF,
	TOK_ELSE,
	TOK_WHILE,
	TOK_FOR,
	TOK_DO,
	TOK_RETURN,
	TOK_PRINT,
	TOK_OPENPA,
	TOK_CLOSEPA,
	TOK_OPENBR,
	TOK_CLOSEBR,
	TOK_COMMA,
	TOK_SEMICL,
	TOK_EQ,
	TOK_EQEQ,
	TOK_NEQ,
	TOK_GT,
	TOK_LT,
	TOK_GE,
	TOK_LE,
	TOK_PLUS,
	TOK_MINUS,
	TOK_STAR,
	TOK_SLASH,
	TOK_PERCENT,
} token_type;

typedef struct token_t {
	struct token_t *next;
	token_type type;
	char *val;
} token_t;

token_t *get_next_token(token_t *token);
int get_token_type(token_t *token);
char *get_token_val(token_t *token);

void cleanup_tokens(token_t *token);
token_t *tokenize_string(char *str);

#endif /* LEXER_H */
