/***************************************************************************
 * compiler.c
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
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ast.h"
#include "generator.h"
#include "lexer.h"
#include "parser.h"
#include "symbols.h"

#define FILE_BUFSIZE 256

static const char *backend_crypto = "/sys/kernel/bispe/crypto";

/* struct in which information about a buffer is passed */
struct buf_info {
	void *ptr;
	size_t size;
};

static void print_usage(void) {
	printf("usage: ./compiler [-u] [-s[op]] [-o <outfile>] <infile>\n");
}

/* returns an newly allocated string containing the infile string
	with the extension changed to '.scle' */
static char *build_outfile_name(const char *infile) {
	const char *extension = ".scle";
	int pos = 0;
	int last_dot = -1;
	for(; infile[pos] != '\0'; pos++) {
		if(infile[pos] == '.') {
			last_dot = pos;
		}
	}
	if(last_dot == -1) {
		last_dot = pos;
	}

	char *str = malloc(last_dot + 6);
	if(str == NULL) {
		fprintf(stderr, "error: could not allocate memory for filename string\n");
		return NULL;
	}

	strncpy(str, infile, last_dot);
	strncpy(str+last_dot, extension, 6);
	return str;
}

/* reads file into newly allocated buffer */
static char *read_file(FILE *fp) {
	int ch, pos = 0;
	int size = FILE_BUFSIZE + 1;

	char *buf = (char *) malloc(FILE_BUFSIZE+1);
	if(buf == NULL) {
		fprintf(stderr, "error: could not allocate %i bytes for file buffer\n", size);
		return NULL;
	}

	while((ch = fgetc(fp)) != EOF) {
		buf[pos++] = ch;

		if(pos == size) {
			size = size * 2;
			char *tmp = realloc(buf, size);
			if(tmp == NULL) {
				free(buf);
				fprintf(stderr, "error: could not allocate %i bytes for file buffer\n", size);
				return NULL;
			}

			buf = tmp;
		}
	}
	if(ferror(fp)) {
		free(buf);
		fprintf(stderr, "error: failed reading from file stream\n");
		return NULL;
	}

	buf[pos] = '\0';
	return buf;
}

static size_t write_sys(const char *sys, void *buf, size_t size) {
	int fd = open(sys, O_WRONLY);
	if(fd == -1) {
		fprintf(stderr, "error: could not open %s\n", sys);
		return -1;
	}

	size_t res = write(fd, buf, size);

	close(fd);

	return res;
}

/* writes the output_buf to the file specified by filename */
static int write_outfile(const char *filename, uint32_t *output_buf, size_t output_len) {
	FILE *fp = fopen(filename, "w");
	if(fp == NULL) {
		fprintf(stderr, "error: could not open '%s' to write output file\n", filename);
		return 1;
	}

	fwrite(output_buf, sizeof(uint32_t), output_len, fp);
	fclose(fp);
	return 0;
}

int main(int argc, char *argv[]) {
	int res = 1;
	char *infile, *outfile;

	// parse command line arguments
	int opt;
	int has_o_flag = 0;
	int show_mnemonics = 0;
	int show_opcodes = 0;
	int unencrypted = 0;
	while((opt = getopt(argc, argv, "s::uo:")) != -1) {
		switch (opt) {
			case 'o':
				outfile = optarg;
				has_o_flag = 1;
				break;
			case 's':
				if(optarg != NULL) {
					if(strcmp(optarg, "op") == 0
						|| strcmp(optarg, "opcodes") == 0) {
						show_opcodes = 1;
					} else {
						show_mnemonics = 1;
					}
				} else {
					show_mnemonics = 1;
				}
				break;
			case 'u':
				unencrypted = 1;
				printf("WARNING: your code will be saved unencrypted!\n");
				break;
			default:
				print_usage();
				goto out;
		}
	}
	if(optind >= argc) {
		print_usage();
		goto out;
	}
	infile = argv[optind];

	// open infile and read to string buffer
	FILE *fp = fopen(infile, "r");
	if(fp == NULL) {
		fprintf(stderr, "error: could not open %s\n", argv[1]);
		goto out;
	}

	char *infile_str = read_file(fp);
	fclose(fp);

	if(infile_str == NULL) {
		goto out;
	}

	// tokenize input
	token_t *token_head = tokenize_string(infile_str);
	free(infile_str);

	if(token_head == NULL) {
		fprintf(stderr, "lexing failed. exiting.\n");	
		goto out;
	}

	// generate ast from token list
	node_t *ast_head = parse_tokenlist(token_head);
	if(ast_head == NULL) {
		fprintf(stderr, "parsing failed. check the syntax of file '%s'. exiting.\n", infile);
		goto parser_out;
	}

	// generate code from ast
	code_t *code_head = generate_code(ast_head);
	if(code_head == NULL) {
		fprintf(stderr, "generating code failed. exiting.\n");
		goto generator_out;
	}

	// print generator output
	if(show_mnemonics || show_opcodes) {
		int mode = show_mnemonics ? 1 : show_opcodes ? 2 : 0;
		print_code(code_head, mode);
		goto buf_build_out;
	}

	// build output buffer with opcodes
	size_t output_len = 0;
	uint32_t *output_buf = build_codebuf(code_head, &output_len, unencrypted);

	if(output_buf == NULL) {
		fprintf(stderr, "building output buffer failed. exiting.\n");
		goto buf_build_out;
	}

	// send code buffer to kernel module for encryption
	if(!unencrypted) {
		struct buf_info buf_info = {
			output_buf,
			output_len << 2
		};
		size_t result = write_sys(backend_crypto, &buf_info, sizeof(buf_info));
		
		if(result == -1) { // could not communicate with backend
			fprintf(stderr, "check if the backend was loaded correctly\n");
			goto outfile_out;
		} else if (result == 0) { // backend error during encryption
			fprintf(stderr, "backend error. check dmesg for more information\n");
			goto outfile_out;
		}
	}

	// build outfile name if needed
	if(!has_o_flag) {
		outfile = build_outfile_name(infile);
		if(outfile == NULL) {
			fprintf(stderr, "building outfile name failed. exiting.\n");
			goto outfile_out;
		}
	}

	// write outfile
	int result = write_outfile(outfile, output_buf, output_len);
	if(!has_o_flag) {
		free(outfile);
	}

	if(result > 0) {
		fprintf(stderr, "writing outfile '%s' failed (%d %s). exiting.\n",
			outfile, errno, strerror(errno));
		goto outfile_out;
	}

	// report compiler success
	res = 0;

outfile_out:
	free(output_buf);
buf_build_out:
	cleanup_code(code_head);
generator_out:
	cleanup_node(ast_head);
parser_out:
	cleanup_symbols();
	cleanup_tokens(token_head);
out:
	return res;
}
