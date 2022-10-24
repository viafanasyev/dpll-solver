#define  _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "cnf.h"

#define CLAUSE_PARSE_ERROR(msg) do { \
    fprintf(stderr, "Clause Parse Error: " msg "\n"); \
} while (0)

#define CLAUSE_PARSE_ERROR_F(fmt, ...) do { \
    fprintf(stderr, "Clause Parse Error: " fmt "\n", ##__VA_ARGS__); \
} while (0)

#define CNF_PARSE_ERROR(msg) do { \
    fprintf(stderr, "CNF Parse Error: " msg "\n"); \
} while (0)

#define CNF_PARSE_ERROR_F(fmt, ...) do { \
    fprintf(stderr, "CNF Parse Error: " fmt "\n", ##__VA_ARGS__); \
} while (0)

int read_dimacs_clause(char* line, size_t max_vars_num, Clause* clause) {
    assert(line != NULL);
    assert(clause != NULL);

    signed int* vars = (signed int*) calloc(max_vars_num, sizeof(signed int));
    if (vars == NULL) {
        CLAUSE_PARSE_ERROR("Insufficient memory");
        return -1;
    }

    size_t real_vars_num = 0;
    char* token = strtok(line, " ");
    signed int var = 0; 
    while (token != NULL) {
        var = atoi(token);
        token = strtok(NULL, " ");
        if (var == 0) {
            break;
        }
        if ((var > 0 && var > max_vars_num) || (var < 0 && -var > max_vars_num)) {
            CLAUSE_PARSE_ERROR_F("Expected variables in [-%zu; %zu], but got %d", max_vars_num, max_vars_num, var);
            free(vars);
            return -1;
        }
        vars[real_vars_num++] = var;
    }
    if (var != 0 || token != NULL) {
        CLAUSE_PARSE_ERROR("Variables should be terminated with zero");
        free(vars);
        return -1;
    }

    signed int* shrinked_vars = (signed int*) realloc(vars, real_vars_num * sizeof(signed int));
    if (shrinked_vars == NULL) {
        CLAUSE_PARSE_ERROR("Insufficient memory");
        free(vars);
        return -1;
    }
    clause->len = real_vars_num;
    clause->vars = shrinked_vars; // TODO: Validate, that all vars are unique
    return 0;
}

CNF* read_dimacs_cnf(FILE* fp) {
    assert(fp != NULL);

    char* line = NULL;
    size_t len = 0;
    ssize_t read = -1;
    size_t vars_num = 0;
    size_t clauses_num = 0;
    Clause** clauses = NULL;
    size_t current_clause_num = 0;
    size_t line_num = 0;
    CNF* cnf = NULL;
    while ((read = getline(&line, &len, fp)) != -1) {
        ++line_num;
        if (read < 2) {
            // Empty line
            continue;
        }
        if (line[0] == 'c') {
            // Comment
            continue;
        }
        if (read > sizeof("p cnf ") - 1 && !strncmp(line, "p cnf ", sizeof("p cnf ") - 1)) {
            // Number of variables and clauses
            if (vars_num != 0 && clauses_num != 0) {
                CNF_PARSE_ERROR_F("Number of vars and clauses is set twice (line #%zu)", line_num);
                goto error;
            }
            if (sscanf(line + sizeof("p cnf ") - 1, "%zu %zu\n", &vars_num, &clauses_num) != 2) {
                CNF_PARSE_ERROR_F("Bad syntax for number of vars and clauses (line #%zu)", line_num);
                goto error;
            }

            clauses = (Clause**) calloc(clauses_num, sizeof(Clause));
            if (clauses == NULL) {
                CNF_PARSE_ERROR_F("Insufficient memory (line #%zu)", line_num);
                goto error;
            }
            for (size_t i = 0; i < clauses_num; ++i) {
                clauses[i] = (Clause*) calloc(1, sizeof(Clause));
                if (clauses[i] == NULL) {
                    CNF_PARSE_ERROR_F("CNF Parse Error: Insufficient memory (line #%zu)", line_num);
                    goto error;
                }
            }
        } else {
            // Clause
            if (vars_num == 0 || clauses_num == 0) {
                CNF_PARSE_ERROR_F("Clause is met, but number of clauses was not defined previously (line #%zu)", line_num);
                goto error;
            }
            if (current_clause_num == clauses_num) {
                CNF_PARSE_ERROR_F("Too many clauses (line #%zu)", line_num);
                goto error;
            }

            Clause* current_clause = clauses[current_clause_num++];
            if (read_dimacs_clause(line, vars_num, current_clause) != 0) {
                CNF_PARSE_ERROR_F("Bad clause syntax (line #%zu)", line_num);
                goto error;
            }
        }
    }
    free(line);
    line = NULL;

    if (current_clause_num != clauses_num) {
        CNF_PARSE_ERROR_F("Expected %zu clauses, but got %zu", clauses_num, current_clause_num);
        goto error;
    }

    cnf = (CNF*) calloc(1, sizeof(CNF));
    if (cnf == NULL) {
        CNF_PARSE_ERROR("Insufficient memory");
        goto error;
    }
    cnf->vars_num = vars_num;
    cnf->clauses_num = clauses_num;
    cnf->clauses = clauses;
    return cnf;

error:
    free(line);
    if (clauses != NULL) {
        for (size_t i = 0; i < clauses_num; ++i) {
            if (clauses[i] != NULL) {
                free(clauses[i]->vars);
                free(clauses[i]);
            }
        }
        free(clauses);
    }
    free(cnf);
    return NULL;
}

