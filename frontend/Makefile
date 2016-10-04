CC      = gcc
CFLAGS  = -std=c99 -pedantic -D_XOPEN_SOURCE=600 -Wall -Werror -g
CPPFLAGS= -D_GNU_SOURCE
LDFLAGS = 
RM      = rm -f

BIN_DIR      = ../bin
OUT_DIR      = .

PROJ_INCL_DIR    = ../include

CFLAGS += -I$(PROJ_INCL_DIR)

.PHONY: all clean bin

all: bispe

bin: OUT_DIR = $(BIN_DIR)
bin: bispe

bispe: bispe.o
	$(CC) -o $(OUT_DIR)/$@ $(LDFLAGS) $^

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

bispe.o: $(PROJ_INCL_DIR)/bispe_comm.h

clean:
	$(RM) bispe $(BIN_DIR)/bispe *.o