/**
 * \file
 * This file contains a series of validation checks for a given
 * Scribble protocol.
 *
 * \headerfile "st_node.h"
 * \headerfile "scribble/check.h"
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "st_node.h"
#include "scribble/check.h"


static int scribble_check_defined_roles_r(st_node *node, parametrised_role_t **decl_roles, int ndecl_role)
{
  int i, j;
  int found = 0;

  int error = 0;

  assert(node != NULL);
  // TODO: Extend for parametrised roles
  switch (node->type) {
    case ST_NODE_SEND:
      for (i=0; i<node->interaction->nto; ++i) {
        found = 0;
        for (j=0; j<ndecl_role; ++j) {
          if (strcmp(decl_roles[j]->name, node->interaction->to[i]) == 0)
            found = 1;
        }
        if (!found) {
          fprintf(stderr, "%s: To #%d (%s) not declared\n", __FUNCTION__, i, node->interaction->to[i]);
          node->marked = 1;
          error = 1;
        }
      }
      break;
    case ST_NODE_RECV:
      found = 0;
      for (j=0; j<ndecl_role; ++j) {
        if (strcmp(decl_roles[j]->name, node->interaction->from) == 0)
          found = 1;
      }
      if (!found) {
        fprintf(stderr, "%s: From (%s) not declared\n", __FUNCTION__, node->interaction->from);
        node->marked = 1;
        error = 1;
      }
      break;
    case ST_NODE_SENDRECV:
      for (j=0; j<ndecl_role; ++j) {
        if (strcmp(decl_roles[j]->name, node->interaction->from) == 0)
          found = 1;
      }
      if (!found) {
        fprintf(stderr, "%s: From (%s) not declared\n", __FUNCTION__, node->interaction->from);
        node->marked = 1;
        error = 1;
      }
      for (i=0; i<node->interaction->nto; ++i) {
        found = 0;
        for (j=0; j<ndecl_role; ++j) {
          if (strcmp(decl_roles[j]->name, node->interaction->to[i]) == 0)
            found = 1;
        }
        if (!found) {
          fprintf(stderr, "%s: To #%d (%s) not declared\n", __FUNCTION__, i, node->interaction->to[i]);
          node->marked = 1;
          error = 1;
        }
      }
      break;
    case ST_NODE_CONTINUE:
      assert(node->nchild == 0);
      break;
    case ST_NODE_CHOICE:
      found = 0;
      for (j=0; j<ndecl_role; ++j) {
        if (strcmp(decl_roles[j]->name, node->choice->at) == 0)
          found = 1;
      }
      if (!found) {
        fprintf(stderr, "%s: Choice role (%s) not declared\n", __FUNCTION__, node->choice->at);
        node->marked = 1;
        error = 1;
      }
      // nobreak
    case ST_NODE_ROOT:
    case ST_NODE_PARALLEL:
    case ST_NODE_RECUR:
      for (i=0; i<node->nchild; ++i) {
        error |= scribble_check_defined_roles_r(node->children[i], decl_roles, ndecl_role);
      }
      break;
    default:
      fprintf(stderr, "%s:%d %s Unknown node type: %d\n", __FILE__, __LINE__, __FUNCTION__, node->type);
      break;
  }

  return error;
}


int scribble_check_defined_roles(st_tree *tree)
{
  assert(tree != NULL);
  assert(tree->root != NULL);

  return scribble_check_defined_roles_r(tree->root, tree->info->roles, tree->info->nrole);
}


int scribble_check(st_tree *tree)
{
  int error = 0;

  error = scribble_check_defined_roles(tree);

  if (error) {
    fprintf(stderr, "Error: Not all roles referenced are defined\n");
  }

  return error;
}
