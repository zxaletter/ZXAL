CC = gcc
CFLAGS = -g -Isrc

DIRS = src/Lexer src/Parser src/Semantics src/IR src/RegAlloc

SOURCES = $(shell find $(DIRS) -name "*.c") src/RegAlloc/regalloc.c src/IR/cfg.c src/IR/tac.c src/IR/dag.c src/Semantics/types.c src/main.c src/bumpallocator.c src/compilercontext.c

OUTPUT = program

INCLUDES = -I src -I src/Lexer -I src/Parser -I src/Semantics -I src/IR -I src/RegAlloc

all: $(OUTPUT)

$(OUTPUT): $(SOURCES)
	gcc -g  $(INCLUDES) -o $@ $^

clean:
	rm -f $(OUTPUT)

# OBJECTS = $(SOURCES:.c=.o)
# all: zxal

# zxal: $(OBJECTS)
# 	$(CC) -o zxal $(OBJECTS)

# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# clean:
# 	rm -f $(OBJECTS) zxal

# .PHONY: all clean