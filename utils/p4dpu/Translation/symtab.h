#ifndef _TRANS_SYMTAB_H_
#define _TRANS_SYMTAB_H_

/** Symbol classes.  */
typedef enum
{
    /** Undefined.  */
    unknown_sym,
    /** Declared with %type: same as Undefined, but triggered a Wyacc if applied to a terminal. */
    pct_type_sym,
    /** Terminal. */
    token_sym,
    /** Nonterminal. */
    nterm_sym
} symbol_class;

/** Internal token numbers. */
typedef int symbol_number;
# define SYMBOL_NUMBER_MAXIMUM  INT_MAX

typedef struct symbol symbol;
typedef struct sym_content sym_content;

struct symbol {
    /** The key, name of the symbol.  */
    uniqstr tag;

    /** All the info about the pointed symbol is there. */
    sym_content *content;
};

struct sym_content {
    /** The main symbol that denotes this content (it contains the possible alias). */
    symbol *symbol;

    /** Its \c \%type.

        Beware that this is the type_name as was entered by the user,
        including silly things such as "]" if she entered "%token <]> t".
        Therefore, when outputting type_name to M4, be sure to escape it
        into "@}".  See quoted_output for instance.  */
    uniqstr type_name;

    symbol_number number;

    symbol_class class;
};

#endif  /* _TRANS_SYMTAB_H_ */