#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "cnf.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Expected 1 argument, but got %d\n", argc - 1);
        exit(EXIT_FAILURE);
    }

    char* file_name = argv[1];
    FILE* fp = fopen(file_name, "r");
    if (fp == NULL) {
        fprintf(stderr, "fopen() returned NULL for file '%s'\n", file_name);
        exit(EXIT_FAILURE);
    }

    CNF* cnf = read_dimacs_cnf(fp);
    fclose(fp);
    if (cnf == NULL) {
        fprintf(stderr, "Bad CNF syntax in file '%s'\n", file_name);
        exit(EXIT_FAILURE);
    }

    DEBUG_PRINTF("Vars num: %zu", cnf->vars_num);
    DEBUG_PRINTF("Clauses num: %zu", cnf->clauses_num);

    for (size_t i = 0; i < cnf->clauses_num; ++i) {
        free(cnf->clauses[i]->vars);
        free(cnf->clauses[i]);
    }
    free(cnf->clauses);
    free(cnf);

    return 0;
}

