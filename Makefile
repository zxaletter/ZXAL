CC = gcc
CFLAGS = -g -Isrc -Wall -Wextra

DIRS = src/Lexer src/Parser src/Semantics src/IR src/RegAlloc src/Codegen
# src/RegAlloc src/Codegen
SOURCES = $(shell find $(DIRS) -name "*.c") src/main.c src/types.c src/symbols.c src/compilercontext.c src/bumpallocator.c
# src/RegAlloc/regalloc.c src/IR/cfg.c src/IR/tac.c src/Semantics/types.c
# OUTPUT = program

# INCLUDES = -I src -I src/Lexer -I src/Parser -I src/Semantics 
# # -I src/Semantics -I src/IR -I src/RegAlloc 

# all: $(OUTPUT)

# $(OUTPUT): $(SOURCES)
# 	gcc -g  $(INCLUDES) -o $@ $^

# clean:
# 	rm -f $(OUTPUT)

OBJECTS = $(SOURCES:.c=.o)
all: zxal

zxal: $(OBJECTS)
	$(CC) -o zxal $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) prac.asm zxal

.PHONY: all clean