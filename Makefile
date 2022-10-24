CC                  = gcc
CFLAGS              = -std=c11 -Wpedantic -Werror
SOURCES             = main.c cnf.c
TEST_DIR            = tests
TEST_FAIL_EXIT_CODE = 100500
TARGET              = dpll

default: all

all: build

debug: CFLAGS += -DDEBUG -g
debug: build

build: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

test: testleak

testleak: debug
	$(TEST_DIR)/memory-leakage/run-all-tests.sh $(shell pwd)/$(TARGET)

clean:
	rm -f $(TARGET)
