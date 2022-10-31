#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include "bitvector.h"
#include "cnf.h"
#include "debug.h"
#include "dpll.h"

#define DPLL_ERROR(msg) do { \
    fprintf(stderr, "DPLL Error: " msg "\n"); \
} while (0)

#define DPLL_ERROR_F(fmt, ...) do { \
    fprintf(stderr, "DPLL Error: " fmt "\n", ##__VA_ARGS__); \
} while (0)

typedef struct DpllStateStack {
    BitVector* true_variables;
    BitVector* false_variables;
    struct DpllStateStack* previous;
} DpllStateStack;

static DpllStateStack* push_dpll_state(DpllStateStack* stack, BitVector* true_variables, BitVector* false_variables) {
    // stack may be null
    assert(true_variables != NULL);
    assert(false_variables != NULL);
    assert(true_variables != false_variables);

    DpllStateStack* new_stack = (DpllStateStack*) calloc(1, sizeof(DpllStateStack));
    if (new_stack == NULL) {
        DPLL_ERROR("Insufficient memory");
        return NULL;
    }

    new_stack->true_variables = true_variables;
    new_stack->false_variables = false_variables;
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
    const BitVector* true_variables,
    const BitVector* false_variables
) { 
    size_t vars_num = clause->len;
    signed int* vars = clause->vars;
    for (size_t var_num = 0; var_num < vars_num; ++var_num) {
        signed int var = vars[var_num];
        size_t var_index = var_to_index(var);
        if (var > 0) {
            if (get_bit_state(true_variables, var_index)) {
                return true;
            }
        } else {
            if (get_bit_state(false_variables, var_index)) {
                return true;
            }
        }
    }
    return false; 
}

static bool is_definitely_unsat_clause(
    const Clause* clause,
    const BitVector* true_variables,
    const BitVector* false_variables
) {
    size_t vars_num = clause->len;
    signed int* vars = clause->vars;
    for (size_t var_num = 0; var_num < vars_num; ++var_num) {
        signed int var = vars[var_num];
        size_t var_index = var_to_index(var);
        if (var > 0) {
            if (!get_bit_state(false_variables, var_index)) {
                return false;
            }
        } else {
            if (!get_bit_state(true_variables, var_index)) {
                return false;
            }
        }
    }
    return true;
}

static bool is_definitely_sat(
    const CNF* cnf,
    const BitVector* true_variables,
    const BitVector* false_variables
) {
    for (size_t clause_num = 0, clauses_num = cnf->clauses_num; clause_num < clauses_num; ++clause_num) {
        if (!is_definitely_sat_clause(cnf->clauses[clause_num], true_variables, false_variables)) {
           return false; 
        }
    }
    return true;
}

static bool is_definitely_unsat(
    const CNF* cnf,
    const BitVector* true_variables,
    const BitVector* false_variables
) {
    for (size_t clause_num = 0, clauses_num = cnf->clauses_num; clause_num < clauses_num; ++clause_num) {
        if (is_definitely_unsat_clause(cnf->clauses[clause_num], true_variables, false_variables)) {
            return true;
        }
    }
    return false;
}

static bool has_contradictions(
    const CNF* cnf,
    const BitVector* true_variables,
    const BitVector* false_variables
) {
    assert(cnf != NULL);
    assert(true_variables != NULL);
    assert(false_variables != NULL);
    assert(true_variables != false_variables);

    if (have_intersection(true_variables, false_variables)) {
        return true;
    }

    return is_definitely_unsat(cnf, true_variables, false_variables);
}

static signed int get_single_undecided_var_or_zero(
    const Clause* clause,
    const BitVector* true_variables,
    const BitVector* false_variables
) {
    size_t len = clause->len;
    signed int* vars = clause->vars;
    signed int undecided_var = 0;
    for (size_t var_num = 0; var_num < len; ++var_num) {
        signed int var = vars[var_num];
        assert(var != 0);
        size_t var_index = var_to_index(var);
        if (var > 0) {
            if (get_bit_state(true_variables, var_index)) {
                // This clause is already SAT
                return 0;
            }
            if (!get_bit_state(false_variables, var_index)) {
                if (undecided_var != 0) {
                    // There are at least two undecided vars
                    return 0;
                }
                undecided_var = var;
            }
        } else {
            if (get_bit_state(false_variables, var_index)) {
                // This clause is already SAT
                return 0;
            }
            if (!get_bit_state(true_variables, var_index)) {
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
    BitVector* true_variables,
    BitVector* false_variables
) {
    assert(cnf != NULL);
    assert(true_variables != NULL);
    assert(false_variables != NULL);
    assert(true_variables != false_variables);

    bool any_changes = false;
    do {
        any_changes = false;
        for (size_t clause_num = 0, clauses_num = cnf->clauses_num; clause_num < clauses_num; ++clause_num) {
            signed int undecided_var = get_single_undecided_var_or_zero(cnf->clauses[clause_num], true_variables, false_variables);
            if (undecided_var != 0) {
                if (undecided_var > 0) {
                    size_t var_index = var_to_index(undecided_var);
                    assert(!get_bit_state(true_variables, var_index));
                    assert(!get_bit_state(false_variables, var_index));

                    invert_bit(true_variables, var_index);
                } else {
                    size_t var_index = var_to_index(undecided_var);
                    assert(!get_bit_state(true_variables, var_index));
                    assert(!get_bit_state(false_variables, var_index));

                    invert_bit(false_variables, var_index);
                }
                any_changes = true;
            }
        }
    } while (any_changes);
}

DpllResult dpll_check_sat(const CNF* cnf) {
    assert(cnf != NULL);

    BitVector* true_variables = create_bit_vector(cnf->vars_num);
    if (true_variables == NULL) {
        DPLL_ERROR("Insufficient memory");
        return ERROR;
    }

    BitVector* false_variables = create_bit_vector(cnf->vars_num);
    if (false_variables == NULL) {
        DPLL_ERROR("Insufficient memory");
        free(true_variables->bytes);
        free(true_variables);
        return ERROR;
    }

    DpllStateStack* cur_state = NULL;
    cur_state = push_dpll_state(cur_state, true_variables, false_variables);
    if (cur_state == NULL) {
        DPLL_ERROR("Insufficient memory");
        free(true_variables->bytes);
        free(true_variables);
        free(false_variables->bytes);
        free(false_variables);
        return ERROR;
    }

    BitVector* true_variables_left = NULL;
    BitVector* false_variables_left = NULL;
    BitVector* true_variables_right = NULL;
    BitVector* false_variables_right = NULL;
    DpllStateStack* new_state = NULL;
    DpllResult result = ERROR;

    while (cur_state != NULL) {
        true_variables = cur_state->true_variables;
        false_variables = cur_state->false_variables;
        cur_state = pop_dpll_state(cur_state);

        propagate_all_units(cnf, true_variables, false_variables);

        if (is_definitely_sat(cnf, true_variables, false_variables)) {
            result = SAT;
            goto exit;
        }

        if (has_contradictions(cnf, true_variables, false_variables)) {
            free(true_variables->bytes);
            free(true_variables);
            free(false_variables->bytes);
            free(false_variables);
            true_variables = NULL;
            false_variables = NULL;
            continue;
        }

        size_t toggled_var = get_index_both_zero(true_variables, false_variables);
        if (toggled_var >= cnf->vars_num) {
            result = SAT;
            goto exit;
        }

        true_variables_left = clone_bit_vector(true_variables);
        if (true_variables_left == NULL) {
            DPLL_ERROR("Insufficient memory");
            result = ERROR;
            goto exit;
        }
        false_variables_left = clone_bit_vector(false_variables);
        if (false_variables_left == NULL) {
            DPLL_ERROR("Insufficient memory");
            result = ERROR;
            goto exit;
        }
        invert_bit(false_variables_left, toggled_var);
        new_state = push_dpll_state(cur_state, true_variables_left, false_variables_left);
        if (new_state == NULL) {
            DPLL_ERROR("Insufficient memory");
            result = ERROR;
            goto exit;
        }
        cur_state = new_state;
        new_state = NULL;
        true_variables_left = NULL;
        false_variables_left = NULL;

        true_variables_right = clone_bit_vector(true_variables);
        if (true_variables_right == NULL) {
            DPLL_ERROR("Insufficient memory");
            result = ERROR;
            goto exit;
        }
        false_variables_right = clone_bit_vector(false_variables);
        if (false_variables_right == NULL) {
            DPLL_ERROR("Insufficient memory");
            result = ERROR;
            goto exit;
        }
        invert_bit(true_variables_right, toggled_var);
        new_state = push_dpll_state(cur_state, true_variables_right, false_variables_right);
        if (new_state == NULL) {
            DPLL_ERROR("Insufficient memory");
            result = ERROR;
            goto exit;
        }
        cur_state = new_state;
        new_state = NULL;
        true_variables_right = NULL;
        false_variables_right = NULL;

        free(true_variables->bytes);
        free(true_variables);
        free(false_variables->bytes);
        free(false_variables);
        true_variables = NULL;
        false_variables = NULL;
    }

    result = UNSAT;

exit:
    if (true_variables_left != NULL) {
        free(true_variables_left->bytes);
        free(true_variables_left);
    }
    if (false_variables_left != NULL) {
        free(false_variables_left->bytes);
        free(false_variables_left);
    }
    if (true_variables_right != NULL) {
        free(true_variables_right->bytes);
        free(true_variables_right);
    }
    if (false_variables_right != NULL) {
        free(false_variables_right->bytes);
        free(false_variables_right);
    }
    if (true_variables != NULL) {
        free(true_variables->bytes);
        free(true_variables);
    }
    if (false_variables != NULL) {
        free(false_variables->bytes);
        free(false_variables);
    }
    while (cur_state != NULL) {
        free(cur_state->true_variables->bytes);
        free(cur_state->true_variables);
        free(cur_state->false_variables->bytes);
        free(cur_state->false_variables);
        cur_state = pop_dpll_state(cur_state);
    }
    return result;
}

