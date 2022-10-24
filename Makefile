CC      = gcc
CFLAGS  = -std=c11 -Wpedantic -Werror
SOURCES = main.c cnf.c
TARGET  = dpll

default: all

all: build

debug: CFLAGS += -DDEBUG -g
debug: build

build: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
