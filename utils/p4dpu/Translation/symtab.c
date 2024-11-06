/*----------------------------------------------------------------.
| Find the symbol named KEY, and return it.  If it does not exist |
| yet, create it.                                                 |
`----------------------------------------------------------------*/

symbol * symbol_get (const char *key, location loc) {
    return symbol_from_uniqstr (uniqstr_new (key), loc);
}

/*-------------------------------.
| Create the symbol hash table.  |
`-------------------------------*/

void symbols_new(void) {
  symbol_table = hash_xinitialize(HT_INITIAL_CAPACITY, NULL,
                                   hash_symbol_hasher,
                                   hash_symbol_comparator,
                                   symbol_free);

  /* Construct the acceptsymbol symbol. */
  acceptsymbol = symbol_get("$accept", empty_loc);
  acceptsymbol->content->class = nterm_sym;
  acceptsymbol->content->number = nnterms++;

  /* Construct the YYerror/"error" token */
  errtoken = symbol_get ("YYerror", empty_loc);
  errtoken->content->class = token_sym;
  errtoken->content->number = ntokens++;
  {
    symbol *alias = symbol_get ("error", empty_loc);
    symbol_class_set (alias, token_sym, empty_loc, false);
    symbol_make_alias (errtoken, alias, empty_loc);
  }

  /* Construct the YYUNDEF/"$undefined" token that represents all
     undefined literal tokens.  It is always symbol number 2.  */
  undeftoken = symbol_get ("YYUNDEF", empty_loc);
  undeftoken->content->class = token_sym;
  undeftoken->content->number = ntokens++;
  {
    symbol *alias = symbol_get ("$undefined", empty_loc);
    symbol_class_set (alias, token_sym, empty_loc, false);
    symbol_make_alias (undeftoken, alias, empty_loc);
  }

  semantic_type_table = hash_xinitialize (HT_INITIAL_CAPACITY,
                                          NULL,
                                          hash_semantic_type_hasher,
                                          hash_semantic_type_comparator,
                                          free);
}