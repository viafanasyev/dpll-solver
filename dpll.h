#pragma once
#include "cnf.h"

typedef enum {
    SAT,
    UNSAT,
    ERROR,
} DpllResult;

DpllResult dpll_check_sat(const CNF* cnf);

