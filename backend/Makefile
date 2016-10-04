NAME      := bispe_km
#KERNEL   := 3.8.2-tresor0.5
KERNEL    := $(shell uname -r)
KERN_DIR  ?= /lib/modules/${KERNEL}/build

BIN_DIR   := ../bin

# toggle debug mode
DEBUG     := 0

# enable to run debug test on module load
RUN_DEBUG_TEST := 0

# enable to run interpreter tests from bispe_tests.c on module load
TESTS     := 0

# enabled: interpreter uses encrypted code and data
# has to be disabled if interpreter tests and debug tests should be run
ENCRYPTION := 1

# enabled: uses custom calling convention to prevent that rip shows up on stack
# this uses a small hack, but should work without problems
RIP_PROTECT := 1

######################### SOURCES #######################

SOURCES_C   = bispe_main.c bispe_interpreter.c bispe_sha.c bispe_key.c

ifeq ($(TESTS),1)
    SOURCES_C += bispe_tests.c
endif

SOURCES_ASM = bispe_cycle_asm.S bispe_crypto_asm.S

OBJS = $(SOURCES_C:%.c=%.o) $(SOURCES_ASM:%.S=%.o)

######################### FLAGS #########################

ccflags-y := -std=gnu99 -Wno-declaration-after-statement -Wall -Werror

# include header files
ccflags-y += -I$(src)/../include
ccflags-y += -I$(src)/include
asflags-y := -I$(src)/include

ifeq ($(DEBUG),1)
    ccflags-y += -g -DDEBUG
    asflags-y += -DDEBUG
endif

ifeq ($(RIP_PROTECT),1)
	asflags-y += -DRIP_PROTECT
endif

ifeq ($(ENCRYPTION),1)
    ccflags-y += -DENCRYPTION
    asflags-y += -DENCRYPTION
else
# interpreter tests and debug test may only be used if encryption is disabled
    ifeq ($(RUN_DEBUG_TEST),1)
        ccflags-y += -DRUN_DEBUG_TEST
    endif

    ifeq ($(TESTS),1)
        ccflags-y += -DTESTS
    endif
endif

#################### KERNEL MODULE ######################

obj-m += ${NAME}.o

${NAME}-y := ${OBJS}

#################### DIRECTIVES #########################

.PHONY: all clean bin

all:
	make -C ${KERN_DIR} M=${PWD} modules

bin: all
	mv ${NAME}.ko ${BIN_DIR}

clean:
	make -C ${KERN_DIR} M=${PWD} clean
	rm -f ${BIN_DIR}/${NAME}.ko
