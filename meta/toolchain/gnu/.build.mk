export PATH := $(shell meta/toolchain/gnu/use.sh):$(PATH)

# --- Host compiler ---------------------------------------------------------- #

HOST_CC=cc

HOST_CFLAGS= \
	$(CFLAGS_STD) \
	$(CFLAGS_OPT) \
	$(CFLAGS_WARN) \
	$(HOST_CFLAGS_INC)

HOST_LD=ld
HOST_LDFLAGS=

# --- Cross-Compiler --------------------------------------------------------- #

CROSS_AS=nasm
CROSS_ASFLAGS=-f elf64

CROSS_CC=$(CONFIG_ARCH)-elf-gcc
CROSS_CFLAGS= \
	-MD \
	$(CFLAGS_STD) \
	$(CFLAGS_OPT) \
	$(CFLAGS_WARN) \
	$(CROSS_CFLAGS_INC) \
	-ffreestanding \
	-D__brutal__=1

CROSS_KCFLAGS= \
	$(CROSS_CFLAGS) \
	-fno-pic \
	-fno-stack-protector \
	-mno-80387 \
	-mno-mmx \
	-mno-3dnow \
	-mno-sse \
	-mno-sse2 \
	-mno-red-zone \
	-mcmodel=kernel \
	-D__kernel__=1

CROSS_UCFLAGS= \
	$(CROSS_CFLAGS) \
	-fno-pic \
	-fno-stack-protector \
	-nostdlib

CROSS_LD=$(CONFIG_ARCH)-elf-ld
CROSS_KLDFLAGS= \
	-Tsources/archs/$(CONFIG_ARCH)/kernel.ld \
	-z max-page-size=0x1000

CROSS_ULDFLAGS= \
	-Tsources/archs/$(CONFIG_ARCH)/userspace.ld \
	-z max-page-size=0x1000

CROSS_AR=$(CONFIG_ARCH)-elf-ar
CROSS_ARFLAGS=rcsv
