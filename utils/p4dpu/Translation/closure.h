#ifndef _TRANS_CLOSURE_H_
#define _TRANS_CLOSURE_H_

/* Allocates the itemset and ruleset vectors, and precomputes useful
   data so that closure can be called.  n is the number of elements to
   allocate for itemset.  */
void closure_new (int n);

#endif  /* _TRANS_CLOSURE_H_ */