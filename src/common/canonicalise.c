/**
 * \file
 * This file contains the canonicalisation functions of (multiparty) session
 * type nodes (st_node).
 * Each function here represents a separate pass on the node and are recursive.
 * 
 * \headerfile "st_node.h"
 * \headerfile "canonicalise.h"
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "st_node.h"
#include "canonicalise.h"


/**
 * Remove redundant continue calls inside
 * if the only operation in a recur block is continue.
 */
st_node *st_node_recur_simplify(st_node *node) 
{
  int i, j;
  for (i=0; i<node->nchild; ++i) {
    node->children[i] = st_node_recur_simplify(node->children[i]);
  }

  int only_continue = 1;
  if (node->type == ST_NODE_RECUR) {
    for (i=0; i<node->nchild; ++i) {
      only_continue &= (node->children[i]->type == ST_NODE_CONTINUE);
    }

    if (only_continue) {
      // Remove all children (ie. continue nodes)
      for (i=0; i<node->nchild; ++i) {
        st_node_free(node->children[i]);
        for (j=i; j<node->nchild-1; ++j) {
            node->children[j] = node->children[j+1];
        }
        node->nchild--;
      }
      assert(node->nchild == 0);
      free(node->children);
    }
  }

  return node;
}


/**
 * Single child in par and root blocks 
 * are merged to the parent node as this
 * do not change the semantics of the tree.
 */
st_node *st_node_singleton_leaf_upmerge(st_node *node)
{
  int i;
  for (i=0; i<node->nchild; ++i) {
    node->children[i] = st_node_singleton_leaf_upmerge(node->children[i]);
  }

  if (node->type == ST_NODE_PARALLEL
      || node->type == ST_NODE_ROOT) {
    if (node->nchild == 1 && node->children[0]->type == ST_NODE_ROOT) {
      node->type = ST_NODE_ROOT;
      node->nchild = node->children[0]->nchild;
      node->children = (st_node **)realloc(node->children, sizeof(st_node *) * (node->nchild));
      st_node *oldchild = node->children[0];
      for (i=0; i<node->nchild; ++i) {
        node->children[i] = oldchild->children[i];
      }
      oldchild->nchild = 0;
      free(oldchild->children);
      st_node_free(oldchild);
    }
  }
  return node;
}


/**
 * Remove leaf nodes with no children
 * (choice, par, recur, root)
 */
st_node *st_node_empty_leaf_remove(st_node *node)
{
  int i, j;
  for (i=0; i<node->nchild; ++i) {
    node->children[i] = st_node_empty_leaf_remove(node->children[i]);
  }

  for (i=0; i<node->nchild; ++i) {
    if (node->children[i]->type == ST_NODE_CHOICE
        || node->children[i]->type == ST_NODE_PARALLEL
        || node->children[i]->type == ST_NODE_RECUR
        || node->children[i]->type == ST_NODE_ROOT) {
      if (node->children[i]->nchild == 0) {
        // Free the node.
        st_node_free(node->children[i]);
        for (j=i; j<node->nchild-1; ++j) {
            node->children[j] = node->children[j+1];
        }
        node->children = (st_node **)realloc(node->children, sizeof(st_node *) * (node->nchild-1));
        node->nchild--;
      }
    }
  }

  return node;
}


/**
 * Create canonicalised version of st_node.
 * (1) Remove redundant continue calls inside
 *     if the only operation in a recur block is continue.
 * (2) Single child in par and root blocks 
 *     are merged to the parent node as this
 *     do not change the semantics of the tree.
 * (3) Remove leaf nodes with no children
 *     (choice, par, recur, root)
 */
st_node *st_node_canonicalise(st_node *node)
{
  node = st_node_recur_simplify(node);
  node = st_node_singleton_leaf_upmerge(node);
  node = st_node_empty_leaf_remove(node);
  node = st_node_singleton_leaf_upmerge(node);
  return node;
}


/**
 * Add implicit 'continue' at end of rec-loop,
 * converting ordinary looping code to recursion style.
 */
st_node *st_node_recur_add_implicit_continue(st_node *node)
{
  int i;
  for (i=0; i<node->nchild; ++i) {
    node->children[i] = st_node_recur_add_implicit_continue(node->children[i]);
  }

  if (node->type == ST_NODE_RECUR && node->nchild > 0) {
    if (node->children[node->nchild-1]->type != ST_NODE_CONTINUE) {
      st_node *child = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CONTINUE);
      child->cont->label = (char *)calloc(sizeof(char), strlen(node->recur->label)+1);
      strcpy(child->cont->label, node->recur->label);
      st_node_append(node, child);
    }
  }

  return node;
}


/**
 * Realign nested choice blocks
 * obtained by if-then-else-if structure.
 */
st_node *st_node_choice_realign(st_node *node)
{
  int i, j;
  for (i=0; i<node->nchild; ++i) {
    node->children[i] = st_node_choice_realign(node->children[i]);
  }

  if (node->type == ST_NODE_CHOICE) {
    for (i=0; i<node->nchild; ++i) {
      assert(node->children[i]->type == ST_NODE_ROOT);
      if (node->children[i]->nchild == 1  // has one node in branch block (if-then-else-if)
          && node->children[i]->children[0]->type == ST_NODE_CHOICE // is a choice
          && strcmp(node->choice->at, node->children[i]->children[0]->choice->at) == 0) { // has same choice role
        for (j=0; j<node->children[i]->children[0]->nchild; ++j) {
          st_node_append(node, node->children[i]->children[0]->children[j]); // move branch blocks of this choice to parent choice
        }
        // So that st_node_free won't free our copied nodes
        node->children[i]->children[0]->nchild = 0;
        free(node->children[i]->children[0]->children);
      }
    }
  }

  return node;
}


/**
 * Refactor the st_node.
 * Note that most of the operations here are very AST-specific,
 * using this on arbitrary Scribble protocol may change its meaning!
 * (1) Add implicit 'continue' at end of rec-loop,
 *     converting ordinary looping code to recursion style.
 * (2) Realign nested choice blocks
 *     obtained by if-then-else-if structure.
 */
st_node *st_node_refactor(st_node *node)
{
  // While loops.
  node = st_node_recur_add_implicit_continue(node);
  // If-then-else-if.
  node = st_node_choice_realign(node);
  // Remove empty leaf (choice)
  node = st_node_empty_leaf_remove(node);
  return node;
}
