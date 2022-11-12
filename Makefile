CC                  = gcc
CFLAGS              = -std=c11 -Wpedantic -Werror
SOURCES             = main.c cnf.c dpll.c trivector.c
TEST_DIR            = tests
OUT_DIR				= out
RELEASE_TARGET      = $(OUT_DIR)/release/dpll
DEBUG_TARGET        = $(OUT_DIR)/debug/dpll

.PHONY: default
default: all

.PHONY: all
all: release debug

.PHONY: release
release: $(SOURCES)
	mkdir -p $(shell dirname $(RELEASE_TARGET))
	$(CC) $(CFLAGS) -DNDEBUG -O2 $(SOURCES) -o $(RELEASE_TARGET)

.PHONY: debug
debug: $(SOURCES)
	mkdir -p $(shell dirname $(DEBUG_TARGET))
	$(CC) $(CFLAGS) -DDEBUG -g $(SOURCES) -o $(DEBUG_TARGET)

.PHONY: test
test: testleak testsat testunsat

.PHONY: testleak
testleak: debug
	$(TEST_DIR)/memory-leakage/run-all-tests.sh $(shell pwd)/$(DEBUG_TARGET)

.PHONY: testsat
testsat: release
	$(TEST_DIR)/sat/run-all-tests.sh $(shell pwd)/$(RELEASE_TARGET)

.PHONY: testunsat
testunsat: release
	$(TEST_DIR)/unsat/run-all-tests.sh $(shell pwd)/$(RELEASE_TARGET)

.PHONY: clean
clean:
	rm -rf $(OUT_DIR)
