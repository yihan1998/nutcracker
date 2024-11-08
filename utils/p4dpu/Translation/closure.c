#include "closure.h"

void closure_new (int n) {
  itemset = xnmalloc (n, sizeof *itemset);

  ruleset = bitset_create (nrules, BITSET_FIXED);

  set_fderives ();
}