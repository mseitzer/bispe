NAME      := bispe_crypto_test
KERNEL    := $(shell uname -r)
KERN_DIR  ?= /lib/modules/${KERNEL}/build

######################### SOURCES #######################

SOURCES_C   = crypto_test.c

SOURCES_ASM = bispe_crypto_asm.S

OBJS = $(SOURCES_C:%.c=%.o) $(SOURCES_ASM:%.S=%.o)

######################### FLAGS #########################

ccflags-y := -std=gnu99 -Wno-declaration-after-statement -Wall -Werror

#################### KERNEL MODULE ######################

obj-m += ${NAME}.o

${NAME}-y := ${OBJS}

#################### DIRECTIVES #########################

.PHONY: all clean

all:
	make -C ${KERN_DIR} M=${PWD} modules

clean:
	make -C ${KERN_DIR} M=${PWD} clean
