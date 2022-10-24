CC                  = gcc
CFLAGS              = -std=c11 -Wpedantic -Werror
SOURCES             = main.c cnf.c
TEST_DIR            = tests
TEST_FAIL_EXIT_CODE = 100500
OUT_DIR				= out
RELEASE_TARGET      = $(OUT_DIR)/release/dpll
DEBUG_TARGET        = $(OUT_DIR)/debug/dpll

default: all

all: release debug

release: $(SOURCES)
	mkdir -p $(shell dirname $(RELEASE_TARGET))
	$(CC) $(CFLAGS) -O2 $(SOURCES) -o $(RELEASE_TARGET)

debug: $(SOURCES)
	mkdir -p $(shell dirname $(DEBUG_TARGET))
	$(CC) $(CFLAGS) -DDEBUG -g $(SOURCES) -o $(DEBUG_TARGET)

test: testleak

testleak: debug
	$(TEST_DIR)/memory-leakage/run-all-tests.sh $(shell pwd)/$(DEBUG_TARGET)

clean:
	rm -rf $(OUT_DIR)
