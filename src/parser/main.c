/**
 * \file
 * Standalone Scribble parser.
 *
 * \headerfile "st_node.h"
 */
#include <stdio.h>
#include <stdlib.h>

#include "st_node.h"

extern int yyparse(st_tree *tree);
extern FILE *yyin;

int main(int argc, char *argv[])
{
  if (argc > 1) {
    yyin = fopen(argv[1], "r");
  } else {
    fprintf(stderr, "Warning: reading from stdin\n");
  }

  st_tree *tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
  yyparse(tree);
  st_tree_print(tree);
  st_tree_free(tree);

  if (argc > 1)
    fclose(yyin);
  return EXIT_SUCCESS;
}
