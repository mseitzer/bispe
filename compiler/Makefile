CC      = gcc
CFLAGS  = -std=c99 -pedantic -D_XOPEN_SOURCE=600 -Wall -Werror -g
CPPFLAGS= -D_GNU_SOURCE
LDFLAGS = 
RM      = rm -f

BIN_DIR      = ../bin
OUT_DIR      = .

PROJ_INCL_DIR    = ../include
LOC_INCL_DIR     = ./include

CFLAGS += -I$(PROJ_INCL_DIR) -I$(LOC_INCL_DIR)

.PHONY: all bin clean

all: compiler

bin: OUT_DIR = $(BIN_DIR)
bin: compiler

compiler: compiler.o lexer.o parser.o ast.o generator.o symbols.o
	$(CC) -o $(OUT_DIR)/$@ $(LDFLAGS) $^

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

compiler.o: compiler.c ${LOC_INCL_DIR}/lexer.h ${LOC_INCL_DIR}/parser.h ${LOC_INCL_DIR}/ast.h ${LOC_INCL_DIR}/generator.h

lexer.o: lexer.c ${LOC_INCL_DIR}/lexer.h

parser.o: parser.c ${LOC_INCL_DIR}/parser.h ${LOC_INCL_DIR}/lexer.h ${LOC_INCL_DIR}/ast.h

ast.o: ast.c ${LOC_INCL_DIR}/ast.h ${LOC_INCL_DIR}/symbols.h ${LOC_INCL_DIR}/lexer.h

generator.o: generator.c ${LOC_INCL_DIR}/generator.h ${LOC_INCL_DIR}/instructions.h ${LOC_INCL_DIR}/ast.h

symbols.o: symbols.c ${LOC_INCL_DIR}/symbols.h ${LOC_INCL_DIR}/sglib.h ${LOC_INCL_DIR}/types.h ${LOC_INCL_DIR}/lexer.h

clean:
	$(RM) compiler $(BIN_DIR)/compiler *.o