CC = gcc
CFLAGS = -g -Isrc

DIRS = src/Lexer src/Parser src/Semantics src/IR 

SOURCES = $(shell find $(DIRS) -name "*.c") src/main.c

OUTPUT = program

INCLUDES = -I src -I src/Lexer -I src/Parser -I src/Semantics -I src/IR

all: $(OUTPUT)

$(OUTPUT): $(SOURCES)
	gcc -g $(INCLUDES) -o $@ $^

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