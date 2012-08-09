#include <stdio.h>
#include <stdlib.h>

#include "st_node.h"
#include "scribble/project.h"
#include "scribble/print.h"

extern int yyparse(st_tree *tree);
extern FILE *yyin;

int main(int argc, char *argv[])
{
  char *rolename;
  if (argc > 2) {
    yyin = fopen(argv[1], "r");
    rolename = argv[2];
  } else {
    fprintf(stderr, "Usage: %s scribble_file role\n", argv[0]);
    return EXIT_FAILURE;
  }

  st_tree *g = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
  if (yyin == NULL) perror("fopen");
  yyparse(g);
  //st_tree *l = scribble_project(g, rolename);

  //scribble_print(l);

  st_tree_free(g);
  //st_tree_free(l);

  if (argc > 1)
    fclose(yyin);
  return EXIT_SUCCESS;
}
