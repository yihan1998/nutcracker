%{
#include <stdio.h>
#include <stdlib.h>

void yyerror(const char *s);
int yylex(void);
%}

%token META HEADER_FIELD NUM
%token RSS HAIRPIN PORT MONITOR
%token BINARY_OP UNARY_OP

%%

/* Production rules */

stmts:
      stmts stmt               /* stmts -> stmts stmt */
    | /* empty */              /* stmts -> ε */
    ;

stmt:
      lhs '=' expr ';'         /* stmt -> lhs = expr ; */
    | '{' stmts '}'            /* stmt -> { stmts } */
    ;

expr:
      helper_func '(' optparams ')' /* expr -> helper_func(optparams) */
    | expr BINARY_OP term       /* expr -> expr binary_op term */
    | UNARY_OP expr             /* expr -> unary_op expr */
    | term                      /* expr -> term */
    ;

helper_func:
      RSS                       /* helper_func -> RSS */
    | HAIRPIN                   /* helper_func -> HAIRPIN */
    | PORT                      /* helper_func -> PORT */
    | MONITOR                   /* helper_func -> MONITOR */
    ;

lhs:
      META                      /* lhs -> meta */
    | HEADER_FIELD              /* lhs -> header_field */
    ;

term:
      NUM                       /* term -> num */
    ;

optparams:
      /* define optional parameters if required */
    | /* empty */
    ;

%%

/* Error handling */
void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}

int main(void) {
    printf("Enter your input:\n");
    return yyparse();
}
