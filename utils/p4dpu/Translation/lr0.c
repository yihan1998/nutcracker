#include <bitset.h>

#include "closure.h"
#include "lr0.h"
#include "reduce.h"
#include "state.h"

typedef struct state_list
{
  struct state_list *next;
  state *state;
} state_list;

static state_list *first_state = NULL;
static state_list *last_state = NULL;

/* Print CORE for debugging. */
static void
core_print (size_t core_size, item_index *core, FILE *out)
{
  for (int i = 0; i < core_size; ++i)
    {
      item_print (ritem + core[i], NULL, out);
      fputc ('\n', out);
    }
}

/*-----------------------------------------------------------------.
| A state was just discovered by transitioning on SYM from another |
| state.  Queue this state for later examination, in order to find |
| its outgoing transitions.  Return it.                            |
`-----------------------------------------------------------------*/

static state *
state_list_append (symbol_number sym, size_t core_size, item_index *core)
{
  state_list *node = xmalloc (sizeof *node);
  state *res = state_new (sym, core_size, core);

  if (trace_flag & trace_automaton)
    fprintf (stderr, "state_list_append (state = %d, symbol = %d (%s))\n",
             nstates, sym, symbols[sym]->tag);

  node->next = NULL;
  node->state = res;

  if (!first_state)
    first_state = node;
  if (last_state)
    last_state->next = node;
  last_state = node;

  return res;
}

/* Symbols that can be "shifted" (including nonterminals) from the
   current state.  */
bitset shift_symbol;

static rule **redset;
/* For the current state, the list of pointers to states that can be
   reached via a shift/goto.  Could be indexed by the reaching symbol,
   but labels of incoming transitions can be recovered by the state
   itself.  */
static state **shiftset;


/* KERNEL_BASE[symbol-number] -> list of item indices (offsets inside
   RITEM) of length KERNEL_SIZE[symbol-number]. */
static item_index **kernel_base;
static int *kernel_size;

/* A single dimension array that serves as storage for
   KERNEL_BASE.  */
static item_index *kernel_items;


/*-------------------------------------------------------------------.
| Compute the LR(0) parser states (see state.h for details) from the |
| grammar.                                                           |
`-------------------------------------------------------------------*/

void generate_states (void) {
  closure_new (nritems);

  /* Create the initial state, whose accessing symbol (by convention)
     is 0, aka $end.  */
  {
    /* The items of its core: beginning of all the rules of $accept.  */
    kernel_size[0] = 0;
    for (rule_number r = 0; r < nrules && rules[r].lhs->symbol == acceptsymbol; ++r)
      kernel_base[0][kernel_size[0]++] = rules[r].rhs - ritem;
    state_list_append (0, kernel_size[0], kernel_base[0]);
  }

  /* States are queued when they are created; process them all.  */
  for (state_list *list = first_state; list; list = list->next)
    {
      state *s = list->state;
      if (trace_flag & trace_automaton)
        fprintf (stderr, "Processing state %d (reached by %s)\n",
                 s->number,
                 symbols[s->accessing_symbol]->tag);
      /* Set up itemset for the transitions out of this state.  itemset gets a
         vector of all the items that could be accepted next.  */
      closure (s->items, s->nitems);
      /* Record the reductions allowed out of this state.  */
      save_reductions (s);
      /* Find the itemsets of the states that shifts/gotos can reach.  */
      new_itemsets (s);
      /* Find or create the core structures for those states.  */
      append_states (s);

      /* Create the shifts structures for the shifts to those states,
         now that the state numbers transitioning to are known.  */
      state_transitions_set (s, bitset_count (shift_symbol), shiftset);
    }

  /* discard various storage */
  free_storage ();

  /* Set up STATES. */
  set_states ();
}
