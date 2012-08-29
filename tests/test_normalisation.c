#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "st_node.h"
#include "canonicalise.h"

#include <CUnit/CUnit.h>
#include <CUnit/Console.h>

extern int yyparse();
extern FILE *yyin;
st_tree *tree;

int setup_suite(void)
{
  return 0;
}


int teardown_suite(void)
{
  return 0;
}


void test_nestedemptychoice(void)
{
  st_node *root = st_node_init(malloc(sizeof(st_node)), ST_NODE_ROOT);
  st_node *tmp, *tmp2;

  // Build an example node tree
  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_CHOICE);
  tmp->choice->at = "_";
  st_node_append(root, tmp);
  st_node_append(root->children[0], st_node_init(malloc(sizeof(st_node)), ST_NODE_ROOT));

  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_CHOICE);
  tmp->choice->at = "__";
  st_node_append(root->children[0]->children[0], tmp);
  st_node_append(root->children[0]->children[0]->children[0], st_node_init(malloc(sizeof(st_node)), ST_NODE_ROOT));
  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_CHOICE);
  tmp->choice->at = "___";
  st_node_append(root->children[0]->children[0]->children[0]->children[0], tmp);


  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_CHOICE);
  tmp->choice->at = "__";
  st_node_append(root->children[0]->children[0], tmp);
  st_node_append(root->children[0]->children[0]->children[1], st_node_init(malloc(sizeof(st_node)), ST_NODE_ROOT));
  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_CHOICE);
  tmp->choice->at = "___";
  st_node_append(root->children[0]->children[0]->children[1]->children[0], tmp);

  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_CHOICE);
  tmp->choice->at = "__";
  st_node_append(root->children[0]->children[0], tmp);
  st_node_append(root->children[0]->children[0]->children[2], st_node_init(malloc(sizeof(st_node)), ST_NODE_ROOT));
  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_CHOICE);
  tmp->choice->at = "___";
  st_node_append(root->children[0]->children[0]->children[2]->children[0], tmp);

  st_node_append(root->children[0], st_node_init(malloc(sizeof(st_node)), ST_NODE_ROOT));

  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_RECV);
  tmp->interaction->from = "X";
  tmp->interaction->msgsig.op = "Label";
  tmp->interaction->msgsig.payload = "int";
  st_node_append(root->children[0]->children[1], tmp);
  st_node_canonicalise(root);

  // We should get the same as this:
  tmp2 = st_node_init(malloc(sizeof(st_node)), ST_NODE_ROOT);
  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_CHOICE);
  tmp->choice->at = "_";
  st_node_append(tmp2, tmp);
  st_node_append(tmp2->children[0], st_node_init(malloc(sizeof(st_node)), ST_NODE_ROOT));

  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_RECV);
  tmp->interaction->from = "X";
  tmp->interaction->msgsig.op = "Label";
  tmp->interaction->msgsig.payload = "int";
  st_node_append(tmp2->children[0]->children[0], tmp);

  CU_ASSERT(1 == st_node_compare_r(root, tmp2));

  st_node_free(root);
  st_node_free(tmp2);
}

void test_simpleemptychoice(void)
{
  st_node *root = st_node_init(malloc(sizeof(st_node)), ST_NODE_ROOT);
  st_node *tmp, *tmp2;

  // Build an example node tree
  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_SEND);
  tmp->interaction->nto = 1;
  tmp->interaction->to = calloc(sizeof(char *), tmp->interaction->nto);
  tmp->interaction->to[0] = "X";
  tmp->interaction->msgsig.op = "Label";
  tmp->interaction->msgsig.payload = "int";
  st_node_append(root, tmp);

  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_CHOICE);
  tmp->choice->at = "_";
  st_node_append(root, tmp);
  st_node_append(root->children[1], st_node_init(malloc(sizeof(st_node)), ST_NODE_ROOT));

  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_CHOICE);
  tmp->choice->at = "__";
  st_node_append(root->children[1]->children[0], tmp);

  st_node_canonicalise(root);

  // We should get the same as this:
  tmp2 = st_node_init(malloc(sizeof(st_node)), ST_NODE_ROOT);
  tmp = st_node_init(malloc(sizeof(st_node)), ST_NODE_SEND);
  tmp->interaction->nto = 1;
  tmp->interaction->to = calloc(sizeof(char *), tmp->interaction->nto);
  tmp->interaction->to[0] = "X";
  tmp->interaction->msgsig.op = "Label";
  tmp->interaction->msgsig.payload = "int";
  st_node_append(tmp2, tmp);

  CU_ASSERT(1 == st_node_compare_r(root, tmp2));

  st_node_free(root);
  st_node_free(tmp2);
}

int main(int argc, char *argv[])
{
  CU_pSuite suite = NULL;

  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  suite = CU_add_suite("Session C node normalisation", setup_suite, teardown_suite);

  if (NULL == suite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (NULL == CU_add_test(suite, "Nested Empty Choice", &test_nestedemptychoice)
      || NULL == CU_add_test(suite, "Simple Empty Choice", &test_simpleemptychoice)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_console_run_tests();
  CU_cleanup_registry();

  return CU_get_error();
}
