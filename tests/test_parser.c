#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "st_node.h"

#include <CUnit/CUnit.h>
#include <CUnit/Console.h>

extern int yyparse();
extern FILE *yyin;
st_tree *tree;

int setup_parsersuite(void)
{
  return 0;
}


int teardown_parsersuite(void)
{
  return 0;
}


void test_empty_global(void)
{
  char *filename = "examples/parser/Global.spr";
  CU_ASSERT(NULL != (yyin = fopen(filename, "r")));

  tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
  yyparse();
  CU_ASSERT(tree->info->global == 1);

  st_node *root = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);
  CU_ASSERT(st_node_compare_r(tree->root, root) ? 1 : st_node_compare_r_error(tree->root, root, 0));
  free(root);

  free(tree);
  fclose(yyin);
}


void test_empty_local(void)
{
  char *filename = "examples/parser/Global_Local.spr";
  CU_ASSERT(NULL != (yyin = fopen(filename, "r")));

  tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
  yyparse();
  CU_ASSERT(tree->info->global == 0);

  st_node *root = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);
  CU_ASSERT(st_node_compare_r(tree->root, root) ? 1 : st_node_compare_r_error(tree->root, root, 0));
  free(root);

  free(tree);
  CU_ASSERT(0 == fclose(yyin));
}


int main(int argc, char *argv[])
{
  CU_pSuite parsersuite = NULL;

  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  parsersuite = CU_add_suite("Session C parser", setup_parsersuite, teardown_parsersuite);

  if (NULL == parsersuite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if ((NULL == CU_add_test(parsersuite, "Empty Global protocol", &test_empty_global)) ||
      (NULL == CU_add_test(parsersuite, "Empty Local protocol",  &test_empty_local))) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_console_run_tests();
  CU_cleanup_registry();

  return CU_get_error();
}
