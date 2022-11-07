# DPLL Solver

Simple SAT solver that uses [DPLL algorithm](https://en.wikipedia.org/wiki/DPLL_algorithm) with unit-propagation rule, but without pure-literal-elimination. It parses CNF files in [DIMACS format](https://logic.pdmi.ras.ru/~basolver/dimacs.html) and tells whether the given CNF is satisfiable.

### Environment

Tools needed for building:
* gcc with C11 support
* GNU Make
* valgrind (for testing only)

Solver was tested on:
* Ubuntu 20.04.5 LTS x86\_64 
* gcc (Ubuntu 10.3.0-1ubuntu1~20.04) 10.3.0
* GNU Make 4.2.1
* valgrind-3.15.0

### Build

Just clone the repository and run:
```shell
make
```

Or, if you want to build only release/debug target:
```shell
make release # for release target
make debug   # for debug   target
```

Binaries are stored in `out/release/` and `out/debug/` directories.

### Run

Program expects single argument - file with CNF in DIMACS format.
```shell
out/.../dpll input.cnf
```

Program will print 'SAT' to stdin, if CNF is satisfiable, and 'UNSAT' otherwise.

### Test

There are three kinds of test groups:
* memory leakage tests using valgrind (`tests/memory-leakage`);
* solver tests for SAT / UNSAT (`tests/sat`, `tests/unsat`).

You can run all tests by running:
```shell
make test
```

Or you can run each test group separately:
```shell
make testleak  # memory leakage tests
make testsat   # solver SAT tests
make testunsat # solver UNSAT tests
```

To add a new test, just put \*.cnf file into test group folder. See `tests/.../run-all-tests.sh` and `tests/.../run-single-test.sh` scripts for more details.

