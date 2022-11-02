#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "cnf.h"
#include "debug.h"
#include "dpll.h"
#include "trivector.h"

#define DPLL_ERROR(msg) do { \
    fprintf(stderr, "DPLL Error: " msg "\n"); \
} while (0)

#define DPLL_ERROR_F(fmt, ...) do { \
    fprintf(stderr, "DPLL Error: " fmt "\n", ##__VA_ARGS__); \
} while (0)

typedef struct DpllStateStack {
    TriVector* vars_states;
    struct DpllStateStack* previous;
} DpllStateStack;

static DpllStateStack* push_dpll_state(DpllStateStack* stack, TriVector* vars_states) {
    // stack may be null
    assert(vars_states != NULL);

    DpllStateStack* new_stack = (DpllStateStack*) calloc(1, sizeof(DpllStateStack));
    if (new_stack == NULL) {
        DPLL_ERROR("Insufficient memory");
        return NULL;
    }

    new_stack->vars_states = vars_states;
    new_stack->previous = stack;
    return new_stack;
}

static DpllStateStack* pop_dpll_state(DpllStateStack* stack) {
    assert(stack != NULL);

    DpllStateStack* new_stack = stack->previous;
    free(stack);
    return new_stack;
}

static inline size_t var_to_index(signed int var) {
    assert(var != 0);
    return (var > 0 ? var : -var) - 1;
}

static bool is_definitely_sat_clause(
    const Clause* clause,
    const TriVector* vars_states
) { 
    size_t vars_num = clause->len;
    signed int* vars = clause->vars;
    for (size_t var_num = 0; var_num < vars_num; ++var_num) {
        signed int var = vars[var_num];
        size_t var_index = var_to_index(var);
        if (var > 0) {
            if (is_set_positive(vars_states, var_index)) {
                return true;
            }
        } else {
            if (is_set_negative(vars_states, var_index)) {
                return true;
            }
        }
    }
    return false; 
}

static bool is_definitely_unsat_clause(
    const Clause* clause,
    const TriVector* vars_states
) {
    size_t vars_num = clause->len;
    signed int* vars = clause->vars;
    for (size_t var_num = 0; var_num < vars_num; ++var_num) {
        signed int var = vars[var_num];
        size_t var_index = var_to_index(var);
        if (var > 0) {
            if (!is_set_negative(vars_states, var_index)) {
                return false;
            }
        } else {
            if (!is_set_positive(vars_states, var_index)) {
                return false;
            }
        }
    }
    return true;
}

static bool is_definitely_sat(
    const CNF* cnf,
    const TriVector* vars_states
) {
    for (size_t clause_num = 0, clauses_num = cnf->clauses_num; clause_num < clauses_num; ++clause_num) {
        if (!is_definitely_sat_clause(cnf->clauses[clause_num], vars_states)) {
           return false; 
        }
    }
    return true;
}

static bool is_definitely_unsat(
    const CNF* cnf,
    const TriVector* vars_states
) {
    for (size_t clause_num = 0, clauses_num = cnf->clauses_num; clause_num < clauses_num; ++clause_num) {
        if (is_definitely_unsat_clause(cnf->clauses[clause_num], vars_states)) {
            return true;
        }
    }
    return false;
}

static bool has_contradictions(
    const CNF* cnf,
    const TriVector* vars_states
) {
    assert(cnf != NULL);
    assert(vars_states != NULL);

    return is_definitely_unsat(cnf, vars_states);
}

static signed int get_single_undecided_var_or_zero(
    const Clause* clause,
    const TriVector* vars_states
) {
    size_t len = clause->len;
    signed int* vars = clause->vars;
    signed int undecided_var = 0;
    for (size_t var_num = 0; var_num < len; ++var_num) {
        signed int var = vars[var_num];
        assert(var != 0);
        size_t var_index = var_to_index(var);
        if (var > 0) {
            if (is_set_positive(vars_states, var_index)) {
                // This clause is already SAT
                return 0;
            }
            if (!is_set_negative(vars_states, var_index)) {
                if (undecided_var != 0) {
                    // There are at least two undecided vars
                    return 0;
                }
                undecided_var = var;
            }
        } else {
            if (is_set_negative(vars_states, var_index)) {
                // This clause is already SAT
                return 0;
            }
            if (!is_set_positive(vars_states, var_index)) {
                if (undecided_var != 0) {
                    // There are at least two undecided vars
                    return 0;
                }
                undecided_var = var;
            }
        }
    }
    return undecided_var;
}

static void propagate_all_units(
    const CNF* cnf,
    TriVector* vars_states
) {
    assert(cnf != NULL);
    assert(vars_states != NULL);

    bool any_changes = false;
    do {
        any_changes = false;
        for (size_t clause_num = 0, clauses_num = cnf->clauses_num; clause_num < clauses_num; ++clause_num) {
            signed int undecided_var = get_single_undecided_var_or_zero(cnf->clauses[clause_num], vars_states);
            if (undecided_var != 0) {
                size_t var_index = var_to_index(undecided_var);
                assert(is_not_set(vars_states, var_index));

                set_state(vars_states, var_index, undecided_var > 0);
                any_changes = true;
            }
        }
    } while (any_changes);
}

size_t choose_var(
    const CNF* cnf,
    const TriVector* vars_states
) {
    assert(cnf != NULL);
    assert(vars_states != NULL);

    size_t var = get_index_of_non_set(vars_states);

    assert(var >= cnf->vars_num || is_not_set(vars_states, var));
    return var;
}

DpllResult dpll_check_sat(const CNF* cnf) {
    assert(cnf != NULL);

    TriVector* vars_states = create_tri_vector(cnf->vars_num);
    if (vars_states == NULL) {
        DPLL_ERROR("Insufficient memory");
        return ERROR;
    }

    DpllStateStack* cur_state = NULL;
    cur_state = push_dpll_state(cur_state, vars_states);
    if (cur_state == NULL) {
        DPLL_ERROR("Insufficient memory");
        free(vars_states->states);
        free(vars_states);
        return ERROR;
    }

    TriVector* vars_states_left = NULL;
    TriVector* vars_states_right = NULL;
    DpllStateStack* new_state = NULL;
    DpllResult result = ERROR;

    while (cur_state != NULL) {
        vars_states = cur_state->vars_states;
        cur_state = pop_dpll_state(cur_state);

        propagate_all_units(cnf, vars_states);

        if (is_definitely_sat(cnf, vars_states)) {
            result = SAT;
            goto exit;
        }

        if (has_contradictions(cnf, vars_states)) {
            free(vars_states->states);
            free(vars_states);
            vars_states = NULL;
            continue;
        }

        size_t toggled_var = choose_var(cnf, vars_states);
        if (toggled_var >= cnf->vars_num) {
            result = SAT;
            goto exit;
        }

        vars_states_left = clone_tri_vector(vars_states);
        if (vars_states_left == NULL) {
            DPLL_ERROR("Insufficient memory");
            result = ERROR;
            goto exit;
        }
        set_state(vars_states_left, toggled_var, false);
        new_state = push_dpll_state(cur_state, vars_states_left);
        if (new_state == NULL) {
            DPLL_ERROR("Insufficient memory");
            result = ERROR;
            goto exit;
        }
        cur_state = new_state;
        new_state = NULL;
        vars_states_left = NULL;

        vars_states_right = clone_tri_vector(vars_states);
        if (vars_states_right == NULL) {
            DPLL_ERROR("Insufficient memory");
            result = ERROR;
            goto exit;
        }
        set_state(vars_states_right, toggled_var, true);
        new_state = push_dpll_state(cur_state, vars_states_right);
        if (new_state == NULL) {
            DPLL_ERROR("Insufficient memory");
            result = ERROR;
            goto exit;
        }
        cur_state = new_state;
        new_state = NULL;
        vars_states_right = NULL;

        free(vars_states->states);
        free(vars_states);
        vars_states = NULL;
    }

    result = UNSAT;

exit:
    if (vars_states_left != NULL) {
        free(vars_states_left->states);
        free(vars_states_left);
    }
    if (vars_states_right != NULL) {
        free(vars_states_right->states);
        free(vars_states_right);
    }
    if (vars_states != NULL) {
        free(vars_states->states);
        free(vars_states);
    }
    while (cur_state != NULL) {
        free(cur_state->vars_states->states);
        free(cur_state->vars_states);
        cur_state = pop_dpll_state(cur_state);
    }
    return result;
}

