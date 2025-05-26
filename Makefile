CC = gcc
CFLAGS = -Wall -Isrc

# Find all .c files in src and subdirectories
SOURCES = $(shell find src -name "*.c")
OBJECTS = $(SOURCES:.c=.o)

all: zxal

zxal: $(OBJECTS)
	$(CC) -o zxal $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) zxal

.PHONY: all clean