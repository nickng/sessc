#ifndef PARSER_TYPES__H__
#define PARSER_TYPES__H__

#include "st_node.h"

typedef struct {
  int count;
  st_node **nodes;
} st_nodes;

typedef struct {
  char *name;
  st_expr_t *expr;
} symbol_t;

typedef struct {
  int count;
  symbol_t **symbols;
} symbol_table_t;

#endif // PARSER_TYPES__H__
