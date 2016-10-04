/***************************************************************************
 * bispe.c
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
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "bispe_comm.h"

#define FILE_BUFSIZE 256
#define ARR_SIZE(A) ((sizeof A) / (sizeof A[0]))

static const char *backend_invoke = "/sys/kernel/bispe/invoke";
static const char *backend_password = "/sys/kernel/bispe/password";

/* string representations corresponding to error codes returned by interpreter */
static const char *error_code_strings[] = {
	NULL,
	"illegal opcode",
	"invalid jump target",
	"stack overflow",
	"stack underflow",
	"call stack overflow",
	"call stack underflow",
	"division by zero",
	"argument out of range",
};

/* help messages corresponding to error codes returned by interpreter */
static const char *error_help_strings[] = {
	NULL,
	"Check if the correct password is set.",
	NULL,
	"Try to increase the stack size with --stack-size.",
	NULL,
	"Try to increase the call stack size with --call-size.",
	NULL,
	NULL,
	"Try to supply more command line arguments."
};

/* struct specifying the command line options for getopt */
static struct option cmd_options[] = {
    {"stack-size", required_argument, NULL, 0x1},
    {"call-size", required_argument, NULL, 0x2},
    {"print-size", required_argument, NULL, 0x3},
    {"instr-per-cycle", required_argument, NULL, 0x4},
    {NULL, 0, NULL, 0}
};

static void print_usage(void) {
	char *args[] = {
		"[-p]",
		"[--stack-size=<size>]",
		"[--call-size=<size>]",
		"[--print-size=<size>]",
		"[--instr-per-cycle=<instr_per_cycle>]",
		"<executable>"
	};
	printf("usage: ./bispe ");
	for(int i = 0; i < sizeof(args) / sizeof(args[0]); i++) {
		printf("%s ", args[i]);
	}
	printf("\n");
}

/* reads file into newly allocated buffer */
static char *read_file(FILE *fp, size_t *size) {
	int ch, pos = 0;
	int buf_size = FILE_BUFSIZE + 1;

	char *buf = (char *) malloc(FILE_BUFSIZE+1);
	if(buf == NULL) {
		fprintf(stderr, "error: could not allocate %i bytes for file buffer\n", buf_size);
		return NULL;
	}

	while((ch = fgetc(fp)) != EOF) {
		buf[pos++] = ch;

		if(pos == buf_size) {
			buf_size = buf_size * 2;
			char *tmp = realloc(buf, buf_size);
			if(tmp == NULL) {
				free(buf);
				fprintf(stderr, "error: could not allocate %i bytes for file buffer\n", buf_size);
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

	*size = pos;
	return buf;
}

/* opens sys file and writes size bytes from buf to it */
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

static char *read_password(char *buf, size_t size) {
	char ch;
	int pos = 0;
	static struct termios oldSettings, newSettings;
	
	/* copy old stdin settings */
	tcgetattr(STDIN_FILENO, &oldSettings);
	newSettings = oldSettings;

	/* do not display characters */
	newSettings.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

	while((ch = fgetc(stdin)) != EOF) {
		if(ch == '\n') {
			break;
		}
		buf[pos++] = ch;

		if(pos >= size) {
			break;
		}
	}
	if(ferror(stdin)) {
		memset(buf, 0, size);
		return NULL;
	}

	/* restore old settings */
	tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);

	return buf;
}

int main(int argc, char *argv[]) {
	/* file to execute */
	char *infile;

	/* if a password should be set */
	int setPassword = 0;

	/* interpreter settings, zero denotes to use the default value */
	size_t stack_size = 0;
	size_t call_size = 0;
	size_t print_size = 40;
	size_t instr_per_cycle = 0;

	/* parse command line arguments */
	int opt;
	while((opt = getopt_long(argc, argv, "+pi:", cmd_options, NULL)) != -1) {
		char *endptr = NULL;
		switch (opt) {
			case 'p':
				setPassword = 1;
				break;
			case 0x1:
				stack_size = strtoul(optarg, &endptr, 10);
				if(*endptr != '\0') {
					printf("stack-size contains invalid character(s)\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 0x2:
				call_size = strtoul(optarg, &endptr, 10);
				if(*endptr != '\0') {
					printf("call-size contains invalid character(s)\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 0x3:
				print_size = strtoul(optarg, &endptr, 10);
				if(*endptr != '\0') {
					printf("print-size contains invalid character(s)\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'i':
			case 0x4:
				instr_per_cycle = strtoul(optarg, &endptr, 10);
				if(*endptr != '\0') {
					printf("instr-per-cycle contains invalid character(s)\n");
					exit(EXIT_FAILURE);
				}
				break;
			default:
				print_usage();
				exit(EXIT_FAILURE);
		}
	}
	if(optind >= argc && !setPassword) {
		print_usage();
		exit(EXIT_FAILURE);
	}
	infile = argv[optind];

	/* 
	 * Build command-line argument array for interpreter:
	 * As there currently is only the 32 bit integer data type,
	 * this is also the only kind of data we accept from the command line
	 */
	int interpr_argc = argc - optind - 1;
	uint32_t interpr_argv[interpr_argc];

	for(int i = 0; i < interpr_argc; i++) {
		char *endptr = NULL;
		const char *arg = argv[optind+1+i];

		errno = 0; // set errno to zero before to detect overflows
		long int number = strtol(arg, &endptr, 0);

		if(errno == ERANGE || number > UINT32_MAX || number < INT32_MIN) {
			fprintf(stderr, "Overflow in interpreter argument '%s'. "
				"Arguments must 32 bit integers.\n", arg);
			exit(EXIT_FAILURE);
		} else if(*endptr != '\0') {
			fprintf(stderr, "Can not parse interpreter argument '%s': "
				"Invalid characters.\n", arg);
			exit(EXIT_FAILURE);
		}

		/* 
		 * The cast does not matter, as the interpreter will treat 
		 * the integer as signed again.
		 */
		interpr_argv[i] = (uint32_t) number;
	}

	/* read password from user */
	if(setPassword) {
		char buf[3], pw[54];
		printf("Do you want to set a new password? WARNING: old password will be overwritten! "
			"If you use TRESOR, this WILL lead to corruption of your data!\n");
		printf("y/n: ");
		
		if(fgets(buf, sizeof(buf), stdin) == NULL) {
			fprintf(stderr, "reading from stdin failed (%d %s). exiting.\n",
				errno, strerror(errno));
			exit(EXIT_FAILURE);
		}

		if(strncmp(buf, "y", 1) != 0) {
			exit(EXIT_FAILURE);
		}

		/* read password from stdin */
		if(read_password(pw, sizeof(pw)) == NULL) {
			fprintf(stderr, "reading from stdin failed (%d %s). exiting.\n",
				errno, strerror(errno));
		}

		/* send password to backend */
		if(write_sys(backend_password, pw, sizeof(pw)) == -1) {
			memset(pw, 0, sizeof(pw));
			fprintf(stderr, "check if backend was loaded correctly\n");
			exit(EXIT_FAILURE);
		}

		memset(pw, 0, sizeof(pw));

		/* if no file to execute was specified, exit now */
		if(infile == NULL) {
			exit(EXIT_SUCCESS);
		}
	}

	/* open infile and read to buffer */
	FILE *fp = fopen(infile, "r");
	if(fp == NULL) {
		fprintf(stderr, "error: could not open executable '%s'\n", infile);
		exit(EXIT_FAILURE);
	}

	size_t infile_size;
	char *infile_buf = read_file(fp, &infile_size);
	if(infile_buf == NULL) {
		fclose(fp);
		exit(EXIT_FAILURE);
	}
	fclose(fp);

	/* allocate print buffer */
	size_t print_buf_size = print_size * sizeof(uint32_t);
	uint32_t *print_buf = malloc(print_buf_size);
	if(print_buf == NULL) {
		fprintf(stderr, "error: could not allocate %zu bytes for print buffer\n", print_buf_size);
		exit(EXIT_FAILURE);
	}

	/* set interpreter result to -1 to catch backend errors */
	int interpr_result = -1;

	struct invoke_ctx invoke_ctx = {
		.stack_size = stack_size,
		.call_size = call_size, 
		.ipc = instr_per_cycle,
		.code_buf = { (void *) infile_buf, infile_size },
		.arg_buf = { (void *) interpr_argv, interpr_argc * sizeof(uint32_t) },
		.result = &interpr_result,
		.out_buf = { (void *) print_buf, print_buf_size }
	};

	/* send infile to kernel backend for execution */
	size_t print_bytes_written = write_sys(backend_invoke,
											&invoke_ctx,
											sizeof(struct invoke_ctx));
	
	/* could not communicate with backend */
	if(print_bytes_written == -1) {
		fprintf(stderr, "check if backend was loaded correctly\n");
		free(print_buf);
		free(infile_buf);
		exit(EXIT_FAILURE);
	}

	/* report backend error */
	if(interpr_result == -1) {
		fprintf(stderr, "backend error. check dmesg for more information\n");
		free(print_buf);
		free(infile_buf);
		exit(EXIT_FAILURE);	
	}

	/* execution completed. check for runtime errors */
	if(interpr_result > 0) {
		if(interpr_result < ARR_SIZE(error_code_strings)) {
			printf("interpreter runtime error %d: %s\n", 
				interpr_result, error_code_strings[interpr_result]);
			/* display help message if there exists one for this error */
			if(error_help_strings[interpr_result] != NULL) {
				printf("%s\n", error_help_strings[interpr_result]);
			}
		} else {
			printf("interpreter runtime error %d: unknown error code.\n",
				interpr_result);
		}
	}

	/* print print buffer */
	if(print_bytes_written > 0) {
		for(int i = 0; i < print_bytes_written >> 2; i++) {
			printf("%d\n", *(print_buf+i));
		}
	}

	free(print_buf);
	free(infile_buf);
	return 0;
}
