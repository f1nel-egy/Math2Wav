BUILD_DIR=../build
BUILD_DIR_REALPATH=$(shell realpath $(BUILD_DIR))
OBJS_DIR_REALPATH=$(BUILD_DIR_REALPATH)/objs

SRC_DIR=./
SRC_DIR_REALPATH = $(shell realpath $(SRC_DIR))
LIBS_DIR=$(SRC_DIR)/libs

ARCH_BITS=64

CC=gcc
CXX=g++
AS=nasm
AS_GNU=as
LD=g++

CFLAGS=-m$(ARCH_BITS) -fstack-protector -fPIC -I$(SRC_DIR_REALPATH) -I$(LIBS_DIR) -O3
CXXFLAGS=$(CFLAGS)
ASFLAGS=-f elf$(ARCH_BITS)
LDFLAGS=$(CFLAGS) -lm -lc -lunwind -unwind=libunwind -pie -fuse-ld=lld -Wl,-O3,--sort-common,--as-needed,-z,relro,-z,now

SOURCE_SUBDIRS = \
	$(LIBS_DIR)

SUBDIRS := $(SOURCE_SUBDIRS)

.PHONY: $(SUBDIRS) renderer clean

$(SUBDIRS):
	$(MAKE) -C $@ \
		CC="$(CC)" \
		CXX="$(CXX)" \
		AS="$(AS)" \
		LD="$(LD)" \
		CFLAGS="$(CFLAGS)" \
		CXXFLAGS="$(CXXFLAGS)" \
		ASFLAGS="$(ASFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		BUILD_DIR="$(OBJS_DIR_REALPATH)" \
		build

build: $(SUBDIRS)
	echo "  CC      $(OBJS_DIR_REALPATH)/main.o"
	$(CC) $(CFLAGS) -c main.c -o $(OBJS_DIR_REALPATH)/main.o
	echo "  LD      $(BUILD_DIR_REALPATH)/math2wav"
	$(LD) $(LDFLAGS) $(OBJS_DIR_REALPATH)/*.o -o $(BUILD_DIR_REALPATH)/math2wav
	echo "  LD      $(BUILD_DIR_REALPATH)/libmath2wav.so"
	$(LD) $(LDFLAGS) -shared $(OBJS_DIR_REALPATH)/*.o -o $(BUILD_DIR_REALPATH)/libmath2wav.so
	echo "Math2Wav at: $(BUILD_DIR_REALPATH)/math2wav is ready"

run:
	$(BUILD_DIR_REALPATH)/math2wav -h

clean:
	rm -rf $(BUILD_DIR_REALPATH)/*
	mkdir $(BUILD_DIR_REALPATH)/objs
