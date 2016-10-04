/***************************************************************************
 * mem_check.c
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

#define THRESHOLD 1
#define BUF_SIZE 16

static const char usage[] = "usage: ./mem_check <tests> <test_file> <memory_file>";

struct test {
	int size;
	unsigned char buf[BUF_SIZE];
	int common;
	int match_pos;
};

static int read_char(FILE *fp, unsigned char buf[], int *pos) {
	int ch = fgetc(fp);
	if(ch == EOF) {
		return 1;
	}

	buf[*pos] = (unsigned char) ch;
	*pos = (*pos + 1) % BUF_SIZE;

	return 0;
}

static void run_test(struct test *test, unsigned char buf[], int buf_pos) {	
	for(int match_pos = 0; match_pos < test->size; match_pos++) {
		// start in front of buffer
		int pos = buf_pos;

		int i = match_pos;
		for(; i < test->size; i++) {
			if(buf[pos] != test->buf[i]) {
				break;
			}
		
			pos = (pos + 1) % BUF_SIZE;
		}

		if(i - match_pos > test->common) {
			test->common = i - match_pos;
			test->match_pos = match_pos;
		}
	}
}

int main(int argc, char const *argv[]) {
	int res = -1;

	if(argc != 4) {
		puts(usage);
		exit(EXIT_FAILURE);
	}

	/* read in test count */
	char *endptr = NULL;
	int test_cnt = strtol(argv[1], &endptr, 10);
	if(*endptr != '\0' || test_cnt < 1) {
		fprintf(stderr, "test cound must be a positive integer");
		exit(EXIT_FAILURE);
	}

	/* read and allocate tests */
	FILE *fp = fopen(argv[2], "r");
	if(fp == NULL) {
		fprintf(stderr, "error: could not open %s\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	struct test *tests = calloc(2 * test_cnt, sizeof(struct test));
	if(tests == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	for(int i = 0; i < test_cnt; i++) {
		int file_end = 0;
		int pos = 0;
		int ch;
		char buf[32];
		
		for(int j = 0; j < 32; j++) {
			ch = fgetc(fp);
			if(ch == EOF) {
				if(ferror(fp)) {
					perror("fgetc");
					goto error;
				}

				file_end = 1;
				break;
			} else if(ch == '\n') {
				break;
			} else {
				buf[pos++] = (unsigned char) ch;
			}
		}
		// read away line end
		while(file_end == 0 && ch != '\n') {
			ch = fgetc(fp);
			if(ch == EOF) {
				file_end = 1;
			}
		}

		tests[2*i].size = pos / 2;
		for(int j = 0; j < tests[2*i].size; j++) {
			if(sscanf(buf+2*j, "%2hhx", &tests[2*i].buf[j]) != 1) {
				fprintf(stderr, "invalid hex character in test %d\n", i);
				goto error;
			}
		}

		// create same test with other endianess
		tests[2*i+1].size = tests[2*i].size;
		for(int j = 0; j < tests[2*i].size; j++) {
			tests[2*i+1].buf[tests[2*i].size - 1 - j] = tests[2*i].buf[j];
		}

		if(file_end) {
			test_cnt = i+1;
			//printf("found only %d tests in %s\n", i+1, argv[2]);
			break;
		}
	}
	fclose(fp);

	test_cnt *= 2;

	int pos = 0;
	unsigned char buf[BUF_SIZE];

	/* open memory dump*/
	fp = fopen(argv[3], "r");
	if(fp == NULL) {
		fprintf(stderr, "error: could not open %s\n", argv[3]);
		goto error;
	}

	/* fill buffer initially */
	for(int i = 0; i < BUF_SIZE; i++) {
		int ch = fgetc(fp);
		if(ch == EOF) {
			fprintf(stderr, "error: file has less than %d bytes\n", BUF_SIZE);
			goto error;
		}
		buf[i] = (unsigned char) ch;
	}

	/* run tests */
	do {
		for(int i = 0; i < test_cnt; i++) {
			run_test(&tests[i], buf, pos);
		}
	} while(!read_char(fp, buf, &pos));

	/* report results */
	for(int i = 0; i < test_cnt; i++) {
		if(tests[i].common < THRESHOLD) {
			continue;
		}

		printf("test %d: longest match %d, '", i+1, tests[i].common);
		for(int j = 0; j < tests[i].common; j++) {
			printf("%02x", tests[i].buf[tests[i].match_pos+j]);
		}	
		printf("'\n");
	}

	res = 0;
error:
	free(tests);

	if(fp != NULL)
		fclose(fp);

	return res;
}
