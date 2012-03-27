/**
 * \file
 * Standalone Scribble parser.
 *
 * \headerfile "st_node.h"
 */
#include <stdio.h>
#include <stdlib.h>

#include "st_node.h"

extern int yyparse();
extern FILE *yyin;
st_tree *tree;

int main(int argc, char *argv[])
{
  if (argc > 1)
    yyin = fopen(argv[1], "r");

  tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
  yyparse();
  st_tree_print(tree);
  free(tree);

  if (argc > 1)
    fclose(yyin);
  return EXIT_SUCCESS;
}
