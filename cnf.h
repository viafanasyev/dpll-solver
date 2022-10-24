#pragma once
#include <stdio.h>
#include <stdlib.h>

typedef struct Clause {
    size_t len;
    signed int* vars;
} Clause;

typedef struct CNF {
    size_t vars_num;
    size_t clauses_num;
    Clause** clauses;
} CNF;

int read_dimacs_clause(char* line, size_t max_vars_num, Clause* clause);

CNF* read_dimacs_cnf(FILE* fp);

