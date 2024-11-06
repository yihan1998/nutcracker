#include "gram.h"
#include "reader.h"

int gram_parse(void) {
    int result;

    /* The state stack: array, bottom, top.  */
    uint8_t stack[YYINITDEPTH];
    uint8_t *stack_bottom = stack;
    uint8_t *stack_top = stack_bottom;

    return result;
}