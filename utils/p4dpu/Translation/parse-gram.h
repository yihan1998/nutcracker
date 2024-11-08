#ifndef _TRANS_PARSE_GRAM_H_
#define _TRANS_PARSE_GRAM_H_

#include <stdint.h>

#define DOCAPACT_NINF   (-148)

/* DOCA_NTOKENS -- Number of terminals.  */
#define DOCA_NTOKENS    7
/* DOCA_NNTS -- Number of nonterminals.  */
#define DOCA_NNTS       4
/* DOCA_NRULES -- Number of rules.  */
#define DOCA_NRULES     8
/* DOCA_NSTATES -- Number of states.  */
#define DOCA_NSTATES    14

/** 
 * DOCAGOTO[NTERM-NUM]. 
 */
static const uint8_t doca_goto[DOCA_NSTATES][DOCA_NNTS] =
{

};

static const uint8_t doca_act[DOCA_NSTATES][DOCA_NTOKENS] =
{

};

#endif  /* _TRANS_PARSE_GRAM_H_ */