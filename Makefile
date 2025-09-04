CC = gcc
CFLAGS = -g -Isrc -Wall -Wextra

DIRS = src/Lexer src/Parser src/Semantics src/IR src/RegAlloc src/Codegen
SOURCES = $(shell find $(DIRS) -name "*.c") src/main.c src/types.c src/symbols.c src/compilercontext.c src/bumpallocator.c src/errors.c

EXECUTABLES_AND_ASM_FILES = $(shell find tests -type f ! -name "*.z")

OBJECTS = $(SOURCES:.c=.o)
all: zxal

zxal: $(OBJECTS)
	$(CC) -o zxal $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLES_AND_ASM_FILES)  zxal 

.PHONY: all clean