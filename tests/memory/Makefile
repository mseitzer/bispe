CC      = gcc
CFLAGS  = -std=c99 -pedantic -D_XOPEN_SOURCE=600 -Wall -Werror -O2
CPPFLAGS= -D_GNU_SOURCE
LDFLAGS = 
RM      = rm -f

.PHONY: all bin clean

all: mem_check

mem_check: mem_check.o
	$(CC) -o $@ $(LDFLAGS) $^

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

clean:
	$(RM) mem_check *.o