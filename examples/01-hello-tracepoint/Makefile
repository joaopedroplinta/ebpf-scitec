NAME := hello

CC ?= cc
CLANG ?= clang
ARCH := $(shell uname -m | sed -e 's/x86_64/x86/' -e 's/aarch64/arm64/')

BUILD := build
INCLUDE := include

.PHONY: all clean

all: $(BUILD)/$(NAME)

$(INCLUDE)/vmlinux.h:
	../../scripts/gen-vmlinux.sh $(INCLUDE)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/$(NAME).bpf.o: src/$(NAME).bpf.c $(INCLUDE)/vmlinux.h | $(BUILD)
	$(CLANG) -O2 -g -target bpf -D__TARGET_ARCH_$(ARCH) -I$(INCLUDE) -c $< -o $@

$(BUILD)/$(NAME): src/$(NAME).c $(BUILD)/$(NAME).bpf.o
	$(CC) -O2 -g -o $@ $< -lbpf -lelf -lz

clean:
	rm -rf $(BUILD) $(INCLUDE)/vmlinux.h
