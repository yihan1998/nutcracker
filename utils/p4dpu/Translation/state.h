#ifndef _TRANS_STATE_H_
#define _TRANS_STATE_H_

#include <stdbool.h>

#include <bitset.h>

#include "gram.h"
#include "symtab.h"


/*-------------------.
| Numbering states.  |
`-------------------*/

typedef int state_number;
#define STATE_NUMBER_MAXIMUM INT_MAX

/* Be ready to map a state_number to an int.  */
static inline int
state_number_as_int (state_number s)
{
  return s;
}


typedef struct state state;

/*--------------.
| Transitions.  |
`--------------*/

typedef struct
{
  int num;            /** Size of destination STATES.  */
  state *states[1];
} transitions;


/* What is the symbol labelling the transition to
   TRANSITIONS->states[Num]?  Can be a token (amongst which the error
   token), or nonterminals in case of gotos.  */

# define TRANSITION_SYMBOL(Transitions, Num) \
  (Transitions->states[Num]->accessing_symbol)

/* Is the TRANSITIONS->states[Num] a shift? (as opposed to gotos).  */

# define TRANSITION_IS_SHIFT(Transitions, Num) \
  (ISTOKEN (TRANSITION_SYMBOL (Transitions, Num)))

/* Is the TRANSITIONS->states[Num] a goto?. */

# define TRANSITION_IS_GOTO(Transitions, Num) \
  (!TRANSITION_IS_SHIFT (Transitions, Num))

/* Is the TRANSITIONS->states[Num] labelled by the error token?  */

# define TRANSITION_IS_ERROR(Transitions, Num) \
  (TRANSITION_SYMBOL (Transitions, Num) == errtoken->content->number)

/* When resolving a SR conflicts, if the reduction wins, the shift is
   disabled.  */

# define TRANSITION_DISABLE(Transitions, Num) \
  (Transitions->states[Num] = NULL)

# define TRANSITION_IS_DISABLED(Transitions, Num) \
  (Transitions->states[Num] == NULL)


/* Iterate over each transition over a token (shifts).  */
# define FOR_EACH_SHIFT(Transitions, Iter)                      \
  for (Iter = 0;                                                \
       Iter < Transitions->num                                  \
         && (TRANSITION_IS_DISABLED (Transitions, Iter)         \
             || TRANSITION_IS_SHIFT (Transitions, Iter));       \
       ++Iter)                                                  \
    if (!TRANSITION_IS_DISABLED (Transitions, Iter))


/* The destination of the transition (shift/goto) from state S on
   label SYM (term or nterm).  Abort if none found.  */
struct state *transitions_to (state *s, symbol_number sym);


/*-------.
| Errs.  |
`-------*/

typedef struct
{
  int num;
  symbol *symbols[1];
} errs;

errs *errs_new (int num, symbol **tokens);


/*-------------.
| Reductions.  |
`-------------*/

typedef struct
{
  int num;
  bitset *lookaheads;
  /* Sorted ascendingly on rule number.  */
  rule *rules[1];
} reductions;



/*---------.
| states.  |
`---------*/

struct state_list;

struct state
{
   state_number number;
   symbol_number accessing_symbol;
   transitions *transitions;
   reductions *reductions;
   errs *errs;

   /* When an includer (such as ielr.c) needs to store states in a list, the
      includer can define struct state_list as the list node structure and can
      store in this member a reference to the node containing each state.  */
   struct state_list *state_list;

   /* Whether no lookahead sets on reduce actions are needed to decide
      what to do in state S.  */
   bool consistent;

   /* If some conflicts were solved thanks to precedence/associativity,
      a human readable description of the resolution.  */
   const char *solved_conflicts;
   const char *solved_conflicts_xml;

   /* Its items.  Must be last, since ITEMS can be arbitrarily large.  Sorted
      ascendingly on item index in RITEM, which is sorted on rule number.  */
   size_t nitems;
   item_index items[1];
};

extern state_number nstates;
extern state *final_state;

/* Create a new state with ACCESSING_SYMBOL for those items.  */
state *state_new (symbol_number accessing_symbol,
                  size_t core_size, item_index *core);
/* Create a new state with the same kernel as S (same accessing
   symbol, transitions, reductions, consistency and items).  */
state *state_new_isocore (state const *s);

/* Record that from S we can reach all the DST states (NUM of them).  */
void state_transitions_set (state *s, int num, state **dst);

/* Print the transitions of state s for debug.  */
void state_transitions_print (const state *s, FILE *out);

/* Set the reductions of STATE.  */
void state_reductions_set (state *s, int num, rule **reds);

/* The index of the reduction of state S that corresponds to rule R.
   Aborts if there is no reduction of R in S.  */
int state_reduction_find (state const *s, rule const *r);

/* Set the errs of STATE.  */
void state_errs_set (state *s, int num, symbol **errors);

/* Print on OUT all the lookahead tokens such that this STATE wants to
   reduce R.  */
void state_rule_lookaheads_print (state const *s, rule const *r, FILE *out);
void state_rule_lookaheads_print_xml (state const *s, rule const *r,
                                      FILE *out, int level);

/* Create/destroy the states hash table.  */
void state_hash_new (void);
void state_hash_free (void);

/* Find the state associated to the CORE, and return it.  If it does
   not exist yet, return NULL.  */
state *state_hash_lookup (size_t core_size, const item_index *core);

/* Insert STATE in the state hash table.  */
void state_hash_insert (state *s);

/* Remove unreachable states, renumber remaining states, update NSTATES, and
   write to OLD_TO_NEW a mapping of old state numbers to new state numbers such
   that the old value of NSTATES is written as the new state number for removed
   states.  The size of OLD_TO_NEW must be the old value of NSTATES.  */
void state_remove_unreachable_states (state_number old_to_new[]);

/* All the states, indexed by the state number.  */
extern state **states;

/* Free all the states.  */
void states_free(void);

#endif /* _TRANS_STATE_H_ */
