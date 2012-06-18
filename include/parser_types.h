#ifndef PARSER_TYPES__H__
#define PARSER_TYPES__H__

typedef struct {
  int count;
  st_node **nodes;
} st_nodes;

typedef struct {
  int count;
  char *name; // Name of the variable bound to the parameter(s)
  unsigned long *params;
} role_param_t;

typedef struct {
  int count;
  role_param_t **params_ptrs;
} role_params_t;

#endif // PARSER_TYPES__H__
