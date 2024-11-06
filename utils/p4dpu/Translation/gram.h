#ifndef _TRANS_GRAM_H_
#define _TRANS_GRAM_H_

#include <stdint.h>

/* Rule numbers.  */
typedef int rule_number;
#define RULE_NUMBER_MAX INT_MAX

/*--------.
| Rules.  |
`--------*/

typedef struct {
    /* The number of the rule in the source.  It is usually the index in
        RULES too, except if there are useless rules.  */
    rule_number code;

    /* The index in RULES.  Usually the rule number in the source,
        except if some rules are useless.  */
    rule_number number;

    sym_content *lhs;
    item_number *rhs;

    /* This symbol provides both the associativity, and the precedence. */
    sym_content *prec;

    int dprec;
    int merger;

    /* This symbol was attached to the rule via %prec. */
    sym_content *precsym;

    /* Location of the rhs.  */
    location location;
    bool useful;
    bool is_predicate;

    /* Counts of the numbers of expected conflicts for this rule, or -1 if none
        given. */
    int expected_sr_conflicts;
    int expected_rr_conflicts;

    const char *action;
    location action_loc;
} rule;

#endif  /* _TRANS_GRAM_H_ */