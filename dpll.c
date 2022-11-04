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

typedef struct ClausesList {
    Clause* clause;
    struct ClausesList* next;
} ClausesList;

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

static void pop_dpll_state(DpllStateStack** stack) {
    assert(stack != NULL);
    assert(*stack != NULL);

    DpllStateStack* new_stack = (*stack)->previous;
    free(*stack);
    *stack = new_stack;
}

static ClausesList* list_prepend(ClausesList* item, Clause* clause) {
    // item may be null
    assert(clause != NULL);

    ClausesList* new_item = calloc(1, sizeof(ClausesList));
    if (new_item == NULL) {
        DPLL_ERROR("Insufficient memory");
        return NULL;
    }

    new_item->clause = clause;
    new_item->next = item;
    return new_item;
}

static void list_drop_current(ClausesList** item) {
    assert(item != NULL);
    assert(*item != NULL);

    ClausesList* next = (*item)->next;
    free(*item);
    *item = next;
}

static inline size_t var_to_index(signed int var) {
    assert(var != 0);
    return (var > 0 ? var : -var) - 1;
}

static ClausesList** create_occurance_list(
    const CNF* cnf,
    bool for_positive_vars
) {
    assert(cnf != NULL);

    size_t vars_num = cnf->vars_num;
    ClausesList** vars_lists = calloc(vars_num, sizeof(ClausesList*));
    if (vars_lists == NULL) {
        DPLL_ERROR("Insufficient memory");
        goto error;
    }

    Clause** clauses = cnf->clauses;
    size_t clauses_num = cnf->clauses_num;
    for (size_t clause_num = 0; clause_num < clauses_num; ++clause_num) {
        Clause* clause = clauses[clause_num];
        size_t len = clause->len;
        signed int* vars = clause->vars;
        signed int undecided_var = 0;
        for (size_t var_num = 0; var_num < len; ++var_num) {
            signed int var = vars[var_num];
            assert(var != 0);
            if (for_positive_vars && var > 0 || !for_positive_vars && var < 0) {
                size_t var_index = var_to_index(var);
                ClausesList* new_var_list = list_prepend(vars_lists[var_index], clause);
                if (new_var_list == NULL) {
                    DPLL_ERROR("Insufficient memory");
                    goto error;
                }
                vars_lists[var_index] = new_var_list;
            }
        }
    }

    return vars_lists;

error:
    if (vars_lists != NULL) {
        for (size_t i = 0; i < vars_num; ++i) {
            while (vars_lists[i] != NULL) {
                list_drop_current(&vars_lists[i]);
            }
        }
        free(vars_lists);
    }
    return NULL;
}

static bool is_definitely_sat_clause(
    const Clause* clause,
    const TriVector* vars_states
) {
    assert(clause != NULL);
    assert(vars_states != NULL);

    signed int* vars = clause->vars;
    for (size_t var_num = 0, len = clause->len; var_num < len; ++var_num) {
        signed int var = vars[var_num];
        assert(var != 0);
        if (var > 0) {
            if (trivector_is_set_true(vars_states, var_to_index(var))) {
                return true;
            }
        } else {
            if (trivector_is_set_false(vars_states, var_to_index(var))) {
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
    assert(clause != NULL);
    assert(vars_states != NULL);

    signed int* vars = clause->vars;
    for (size_t var_num = 0, len = clause->len; var_num < len; ++var_num) {
        signed int var = vars[var_num];
        assert(var != 0);
        if (var > 0) {
            if (!trivector_is_set_false(vars_states, var_to_index(var))) {
                return false;
            }
        } else {
            if (!trivector_is_set_true(vars_states, var_to_index(var))) {
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
    assert(cnf != NULL);
    assert(vars_states != NULL);

    Clause** clauses = cnf->clauses;
    for (size_t clause_num = 0, clauses_num = cnf->clauses_num; clause_num < clauses_num; ++clause_num) {
        if (!is_definitely_sat_clause(clauses[clause_num], vars_states)) {
           return false; 
        }
    }
    return true;
}

static bool is_definitely_unsat(
    const CNF* cnf,
    const TriVector* vars_states
) {
    assert(cnf != NULL);
    assert(vars_states != NULL);

    Clause** clauses = cnf->clauses;
    for (size_t clause_num = 0, clauses_num = cnf->clauses_num; clause_num < clauses_num; ++clause_num) {
        if (is_definitely_unsat_clause(clauses[clause_num], vars_states)) {
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
    assert(clause != NULL);
    assert(vars_states != NULL);

    size_t len = clause->len;
    signed int* vars = clause->vars;
    signed int undecided_var = 0;
    for (size_t var_num = 0; var_num < len; ++var_num) {
        signed int var = vars[var_num];
        assert(var != 0);
        if (var > 0) {
            if (trivector_is_set_true(vars_states, var_to_index(var))) {
                // This clause is already SAT
                return 0;
            }
            if (!trivector_is_set_false(vars_states, var_to_index(var))) {
                if (undecided_var != 0) {
                    // There are at least two undecided vars
                    return 0;
                }
                undecided_var = var;
            }
        } else {
            if (trivector_is_set_false(vars_states, var_to_index(var))) {
                // This clause is already SAT
                return 0;
            }
            if (!trivector_is_set_true(vars_states, var_to_index(var))) {
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

    Clause** clauses = cnf->clauses;
    size_t clauses_num = cnf->clauses_num;

    bool any_changes = false;
    do {
        any_changes = false;
        for (size_t clause_num = 0; clause_num < clauses_num; ++clause_num) {
            signed int undecided_var = get_single_undecided_var_or_zero(clauses[clause_num], vars_states);
            if (undecided_var != 0) {
                size_t var_index = var_to_index(undecided_var);
                assert(trivector_is_not_set(vars_states, var_index));

                trivector_set(vars_states, var_index, undecided_var > 0);
                any_changes = true;
            }
        }
    } while (any_changes);
}

static void propagate_units_for_toggled_var(
    const CNF* cnf,
    TriVector* vars_states,
    ClausesList** positive_occurance_list,
    ClausesList** negative_occurance_list,
    size_t toggled_var_index,
    bool is_positive
) {
    assert(cnf != NULL);
    assert(vars_states != NULL);
    assert(positive_occurance_list != NULL);
    assert(negative_occurance_list != NULL);
    assert(positive_occurance_list != negative_occurance_list);

    size_t vars_num = cnf->vars_num;
    ClausesList** clauses_to_process = (ClausesList**) calloc(vars_num, sizeof(ClausesList*));
    if (clauses_to_process == NULL) {
        DPLL_ERROR("Insufficient memory");
        return; // TODO: Pass error to caller?
    }

    if (is_positive) {
        clauses_to_process[toggled_var_index] = negative_occurance_list[toggled_var_index];
    } else {
        clauses_to_process[toggled_var_index] = positive_occurance_list[toggled_var_index];
    }

    bool any_changes = false;
    do {
        any_changes = false;
        for (size_t i = 0; i < vars_num; ++i) {
            ClausesList* current_list_item = clauses_to_process[i];
            clauses_to_process[i] = NULL;
            while (current_list_item != NULL) {
                signed int undecided_var = get_single_undecided_var_or_zero(current_list_item->clause, vars_states);
                if (undecided_var != 0) {
                    size_t var_index = var_to_index(undecided_var);
                    assert(trivector_is_not_set(vars_states, var_index));

                    if (undecided_var > 0) {
                        trivector_set(vars_states, var_index, true);
                        clauses_to_process[var_index] = negative_occurance_list[var_index];
                    } else {
                        trivector_set(vars_states, var_index, false);
                        clauses_to_process[var_index] = positive_occurance_list[var_index];
                    }
                    any_changes = true;
                }
                current_list_item = current_list_item->next;
            }
        }
    } while (any_changes);

    free(clauses_to_process);
}

static size_t choose_var(
    const CNF* cnf,
    const TriVector* vars_states
) {
    assert(cnf != NULL);
    assert(vars_states != NULL);

    size_t var = trivector_index_of_not_set(vars_states);

    assert(var >= cnf->vars_num || trivector_is_not_set(vars_states, var));
    return var;
}

static DpllStateStack* var_branching(
    const CNF* cnf,
    TriVector* vars_states,
    DpllStateStack* cur_state,
    ClausesList** positive_occurance_list,
    ClausesList** negative_occurance_list,
    size_t toggled_var
) {
    assert(cnf != NULL);
    assert(vars_states != NULL);
    // cur_state may be null
    assert(positive_occurance_list != NULL);
    assert(negative_occurance_list != NULL);
    assert(positive_occurance_list != negative_occurance_list);
    assert(toggled_var < cnf->vars_num);

    DpllStateStack* new_state = NULL;

    TriVector* vars_states_left = clone_trivector(vars_states);
    if (vars_states_left == NULL) {
        DPLL_ERROR("Insufficient memory");
        return NULL;
    }
    trivector_set(vars_states_left, toggled_var, false);
    propagate_units_for_toggled_var(cnf, vars_states_left, positive_occurance_list, negative_occurance_list, toggled_var, false);
    new_state = push_dpll_state(cur_state, vars_states_left);
    if (new_state == NULL) {
        DPLL_ERROR("Insufficient memory");
        free_trivector(vars_states_left);
        return NULL;
    }
    cur_state = new_state;
    new_state = NULL;

    TriVector* vars_states_right = clone_trivector(vars_states);
    if (vars_states_right == NULL) {
        DPLL_ERROR("Insufficient memory");
        free_trivector(vars_states_left);
        pop_dpll_state(&cur_state);
        return NULL;
    }
    trivector_set(vars_states_right, toggled_var, true);
    propagate_units_for_toggled_var(cnf, vars_states_right, positive_occurance_list, negative_occurance_list, toggled_var, true);
    new_state = push_dpll_state(cur_state, vars_states_right);
    if (new_state == NULL) {
        DPLL_ERROR("Insufficient memory");
        free_trivector(vars_states_left);
        free_trivector(vars_states_right);
        pop_dpll_state(&cur_state);
        return NULL;
    }

    return new_state;
}

DpllResult dpll_check_sat(const CNF* cnf) {
    assert(cnf != NULL);

    TriVector* vars_states = NULL;
    DpllStateStack* cur_state = NULL;
    ClausesList** positive_occurance_list = NULL;
    ClausesList** negative_occurance_list = NULL;
    DpllResult result = ERROR;

    vars_states = create_trivector(cnf->vars_num);
    if (vars_states == NULL) {
        DPLL_ERROR("Insufficient memory");
        result = ERROR;
        goto exit;
    }

    positive_occurance_list = create_occurance_list(cnf, true);
    if (positive_occurance_list == NULL) {
        DPLL_ERROR("Insufficient memory");
        result = ERROR;
        goto exit;
    }

    negative_occurance_list = create_occurance_list(cnf, false);
    if (negative_occurance_list == NULL) {
        DPLL_ERROR("Insufficient memory");
        result = ERROR;
        goto exit;
    }

    propagate_all_units(cnf, vars_states);

    cur_state = push_dpll_state(cur_state, vars_states);
    if (cur_state == NULL) {
        DPLL_ERROR("Insufficient memory");
        result = ERROR;
        goto exit;
    }

    while (cur_state != NULL) {
        vars_states = cur_state->vars_states;
        pop_dpll_state(&cur_state);

        if (is_definitely_sat(cnf, vars_states)) {
            result = SAT;
            goto exit;
        }

        if (has_contradictions(cnf, vars_states)) {
            free_trivector(vars_states);
            vars_states = NULL;
            continue;
        }

        size_t toggled_var = choose_var(cnf, vars_states);
        if (toggled_var >= cnf->vars_num) {
            result = SAT;
            goto exit;
        }

        DpllStateStack* new_state = var_branching(cnf, vars_states, cur_state, positive_occurance_list, negative_occurance_list, toggled_var);
        if (new_state == NULL) {
            DPLL_ERROR("Insufficient memory");
            result = ERROR;
            goto exit;
        }
        cur_state = new_state;
        new_state = NULL;

        free_trivector(vars_states);
        vars_states = NULL;
    }

    result = UNSAT;

exit:
    free_trivector(vars_states);
    if (positive_occurance_list != NULL) {
        for (size_t i = 0, vars_num = cnf->vars_num; i < vars_num; ++i) {
            while (positive_occurance_list[i] != NULL) {
                list_drop_current(&positive_occurance_list[i]);
            }
        }
        free(positive_occurance_list);
    }
    if (negative_occurance_list != NULL) {
        for (size_t i = 0, vars_num = cnf->vars_num; i < vars_num; ++i) {
            while (negative_occurance_list[i] != NULL) {
                list_drop_current(&negative_occurance_list[i]);
            }
        }
        free(negative_occurance_list);
    }
    while (cur_state != NULL) {
        free_trivector(cur_state->vars_states);
        pop_dpll_state(&cur_state);
    }
    return result;
}

