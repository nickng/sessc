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
      node->children = (st_node **)realloc(node->children, sizeof(st_node *) * node->nchild);
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
 * Checks if there is still empty leaf.
 */
int st_node_has_empty_leaf(st_node *node)
{
  int i;
  int has_empty_leaf = 0;
  if (node->nchild == 0 &&
      (node->type == ST_NODE_CHOICE
      || node->type == ST_NODE_PARALLEL
      || node->type == ST_NODE_RECUR
      || node->type == ST_NODE_ROOT)) {
    return 1;
  } else {
    for (i=0; i<node->nchild; ++i) {
      has_empty_leaf |= st_node_has_empty_leaf(node->children[i]);
    }
  }
  return has_empty_leaf;
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
 * Remove duplicated continue for the same loop.
 */
st_node *st_node_recur_remove_dup_continue(st_node *node)
{
  int i, j, k;
  for (i=0; i<node->nchild; ++i) {
    node->children[i] = st_node_recur_remove_dup_continue(node->children[i]);
  }

  if (node->type == ST_NODE_RECUR && node->nchild > 0) {
    for (i=0; i<node->nchild; ++i) {
      if (node->children[i]->type == ST_NODE_CONTINUE) {
        for (j=i+1; j<node->nchild; ++j) {
          if (node->children[j]->type == ST_NODE_CONTINUE
              && strcmp(node->children[i]->cont->label, node->children[j]->cont->label) == 0) {
            // Remove the node
            st_node_free(node->children[j]);
            // Shift nodes up by one
            for (k=j; k<node->nchild-1; ++k) {
              node->children[k] = node->children[k+1];
            }
            st_node_free(node->children[node->nchild-1]);
            node->nchild--;
            node->children = (st_node **)realloc(node->children, sizeof(st_node *) * node->nchild);
          } else {
            break; // j-loop
          }
        }
      }
    }
  }

  return node;
}


/**
 * Move childeren of non-toplevel root to level above.
 */
st_node *st_node_raise_subroot_children(st_node *node)
{
  int i, j, offset;
  st_node *subroot;

  for (i=0; i<node->nchild; ++i) {
    if ((node->type == ST_NODE_ROOT || node->type == ST_NODE_RECUR) && node->children[i]->type == ST_NODE_ROOT) {
      subroot = node->children[i];
      offset = subroot->nchild - 1;
      node->nchild = node->nchild + offset;
      node->children = (st_node **)realloc(node->children, sizeof(st_node *) * node->nchild);

      // Copy old nodes to end
      for (j=node->nchild-1; j>=i+offset+1; --j) {
        node->children[j] = node->children[j-offset];
      }
      // Copy children of root to this level
      for (j=0; j<subroot->nchild; ++j) {
        node->children[i+j] = subroot->children[j];
      }
    }

    node->children[i] = st_node_raise_subroot_children(node->children[i]);
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
 * (4) Move children of non-toplevel root to level above.
 */
st_node *st_node_canonicalise(st_node *node)
{
  node = st_node_recur_simplify(node);
  node = st_node_singleton_leaf_upmerge(node);
  while (st_node_has_empty_leaf(node) && node->nchild != 0) {
    node = st_node_empty_leaf_remove(node);
    node = st_node_singleton_leaf_upmerge(node);
  }
  node = st_node_recur_remove_dup_continue(node);
  node = st_node_raise_subroot_children(node);
  return node;
}


/**
 * Combines label comparison with the next receive.
 */
st_node *st_node_label_recv_merge(st_node *node)
{
  int i, j, k;

  assert(node->type == ST_NODE_CHOICE);

  for (k=0; k<node->nchild; ++k) { // Work on each code-block of choice

    st_node *_choice = node->children[k];

    assert(_choice->type == ST_NODE_ROOT);

    for (i=0; i<_choice->nchild-1; ++i) {
      // If recv(__LOCAL__, (label)__LABEL__), ie. label comparison
      if (_choice->children[i]->type == ST_NODE_RECV
          && strcmp(_choice->children[i]->interaction->msgsig.payload, "__LABEL__") == 0
          && strcmp(_choice->children[i]->interaction->from, "__LOCAL__") == 0) {
        if (_choice->children[i+1]->type == ST_NODE_RECV && _choice->children[i+1]->interaction->msgsig.op == NULL) {
          _choice->children[i+1]->interaction->msgsig.op = (char *)calloc(sizeof(char), strlen(_choice->children[i]->interaction->msgsig.op)+1);
          strcpy(_choice->children[i+1]->interaction->msgsig.op, _choice->children[i]->interaction->msgsig.op);

          st_node_free(_choice->children[i]);
          for (j=i; j<_choice->nchild-1; ++j) {
            _choice->children[j] = _choice->children[j+1];
          }
          _choice->nchild--;
          _choice->children = (st_node **)realloc(_choice->children, sizeof(st_node *) * _choice->nchild);
        }
      }
    }

  }

  return node;
}


/**
 * Search for label probe and apply label-recv merging.
 */
st_node *st_node_label_recv(st_node *node)
{
  int i, j;

  for (i=0; i<node->nchild-1; ++i) {
    // If children[i] = recv(R, __LABEL__) and children[i+1] = choice@R
    if (node->children[i]->type == ST_NODE_RECV
        && strcmp(node->children[i]->interaction->msgsig.payload, "__LABEL__") == 0
        && node->children[i+1]->type == ST_NODE_CHOICE
        && strcmp(node->children[i]->interaction->from, node->children[i+1]->choice->at) == 0) {
      // Apply label-recv merging on children[i+1]
      st_node_free(node->children[i]);
      node->children[i] = st_node_label_recv_merge(node->children[i+1]);
      // Move children up by 1 (removed receive label node)
      for (j=i+1; j<node->nchild-1; ++j) {
        node->children[j] = node->children[j+1];
      }
      node->nchild--;
      node->children = (st_node **)realloc(node->children, sizeof(st_node *) * node->nchild);
    }
  }

  for (i=0; i<node->nchild; ++i) {
    st_node_label_recv(node->children[i]);
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
 * (1) Realign nested choice blocks
 *     obtained by if-then-else-if structure.
 */
st_node *st_node_refactor(st_node *node)
{
  // If-then-else-if, choice role assignments after we remove dummy recv __LOCAL__ nodes.
  node = st_node_choice_realign(node);
  // Labels comparison -> label of next receive
  node = st_node_label_recv(node);
  // Remove empty leaf (choice)
  node = st_node_empty_leaf_remove(node);
  return node;
}
