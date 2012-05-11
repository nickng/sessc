/**
 * \file
 * This file contains the tree representation of (multiparty) session
 * according to the Scribble language specification and provides functions
 * to build and manipulate session type trees.
 * 
 * \headerfile "st_node.h"
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "st_node.h"


st_tree *st_tree_init(st_tree *tree)
{
  assert(tree != NULL);
  tree->info = (st_info *)malloc(sizeof(st_info));
  tree->info->nrole   = 0;
  tree->info->nimport = 0;
  tree->root = NULL;

  return tree;
}


void st_tree_free(st_tree *tree)
{
  assert(tree != NULL);
  if (tree->info != NULL)
    free(tree->info);
  if (tree->root == NULL)
    st_node_free(tree->root);
}


void st_node_free(st_node *node)
{
  int i;
  for (i=0; i<node->nchild; ++i) {
    st_node_free(node->children[i]);
  }
  if (node->nchild > 0) {
    free(node->children);
  }
  node->nchild = 0;

  switch (node->type) {
    case ST_NODE_ROOT:
      break;
    case ST_NODE_SENDRECV:
    case ST_NODE_SEND:
    case ST_NODE_RECV:
      free(node->interaction);
      break;
    case ST_NODE_PARALLEL:
      break;
    case ST_NODE_CHOICE:
      free(node->choice);
      break;
    case ST_NODE_RECUR:
      free(node->recur);
      break;
    case ST_NODE_CONTINUE:
      free(node->recur);
      break;
    default:
      fprintf(stderr, "%s:%d %s Unknown node type: %d\n", __FILE__, __LINE__, __FUNCTION__, node->type);
      break;
  }
  
  free(node);
}


st_tree *st_tree_set_name(st_tree *tree, const char *name)
{
  assert(tree != NULL);
  tree->info->name = (char *)calloc(sizeof(char), strlen(name)+1);
  strcpy(tree->info->name, name);

  return tree;
}


st_tree *st_tree_add_role(st_tree *tree, const char *role)
{
  assert(tree != NULL);
  if (tree->info->nrole == 0) {
    // Allocate for 1 element.
    tree->info->roles = (char **)malloc(sizeof(char *));
  } else if (tree->info->nrole > 0) {
    // Allocate for n+1 element.
    tree->info->roles = (char **)realloc(tree->info->roles, sizeof(char *) * (tree->info->nrole+1));
  }

  tree->info->roles[tree->info->nrole] = (char *)calloc(sizeof(char), strlen(role)+1);
  strcpy(tree->info->roles[tree->info->nrole], role);

  tree->info->nrole++;

  return tree;
}


st_tree *st_tree_add_import(st_tree *tree, st_tree_import_t import)
{
  assert(tree != NULL);
  if (tree->info == NULL) {
    tree->info = malloc(sizeof(st_info));
    tree->info->nrole = 0;
  }

  if (tree->info->nimport == 0) {
    // Allocate for 1 element.
    tree->info->imports = (st_tree_import_t **)malloc(sizeof(st_tree_import_t *));
  } else if (tree->info->nimport > 0) {
    // Allocate for n+1 element.
    tree->info->imports = (st_tree_import_t **)realloc(tree->info->imports, sizeof(st_tree_import_t *) * (tree->info->nimport+1));
  }

  tree->info->imports[tree->info->nimport] = (st_tree_import_t *)malloc(sizeof(st_tree_import_t));
  memcpy(tree->info->imports[tree->info->nimport], &import, sizeof(st_tree_import_t));

  tree->info->nimport++;

  return tree;
}


st_node *st_node_init(st_node *node, int type)
{
  assert(node != NULL);
  node->type = type;
  switch (type) {
    case ST_NODE_ROOT:
      break;
    case ST_NODE_SENDRECV:
    case ST_NODE_SEND:
    case ST_NODE_RECV:
      node->interaction = (st_node_interaction *)malloc(sizeof(st_node_interaction));
      break;
    case ST_NODE_PARALLEL:
      break;
    case ST_NODE_CHOICE:
      node->choice = (st_node_choice *)malloc(sizeof(st_node_choice));
      break;
    case ST_NODE_RECUR:
      node->recur = (st_node_recur *)malloc(sizeof(st_node_recur));
      break;
    case ST_NODE_CONTINUE:
      node->cont = (st_node_continue *)malloc(sizeof(st_node_continue));
      break;
    default:
      fprintf(stderr, "%s:%d %s Unknown node type: %d\n", __FILE__, __LINE__, __FUNCTION__, type);
      break;
  }
  node->nchild = 0;
  node->marked = 0;

  return node;
}


st_node *st_node_append(st_node *node, st_node *child)
{
  assert(node != NULL);
  assert(child != NULL);
  if (node->nchild == 0) {
    // Allocate for 1 node.
    node->children = (st_node **)malloc(sizeof(st_node *));
  } else if (node->nchild > 0) {
    // Allocate for n+1 nodes.
    node->children = (st_node **)realloc(node->children, sizeof(st_node *) * (node->nchild+1));
  }

  node->children[node->nchild++] = child;

  return node;
}


void st_tree_print(const st_tree *tree)
{
  int i;

  if (tree == NULL) {
    fprintf(stderr, "%s:%d %s tree is NULL\n", __FILE__, __LINE__, __FUNCTION__);
  }

  printf("\n-------Summary------\n");

  if (tree->info != NULL) {
    printf("Protocol: %s\n", tree->info->name);
    printf("%s protocol\n", tree->info->global ? "Global" : "Endpoint");
    if (!tree->info->global) {
      printf("Endpoint role: %s\n", tree->info->myrole);
    }
    printf("Imports: [\n");
    for (i=0; i<tree->info->nimport; ++i)
      printf("  { name: %s, as: %s, from: %s }\n", tree->info->imports[i]->name, tree->info->imports[i]->as, tree->info->imports[i]->from);
    printf("]\n");
    printf("Roles: [");
    for (i=0; i<tree->info->nrole; ++i)
      printf("%s ", tree->info->roles[i]);
    printf("]\n");
  } else {
    printf("Protocol info not found\n");
  }

  printf("--------------------\n");

  if (tree->root != NULL) {
    st_node_print_r(tree->root, 0);
  } else {
    printf("Protocol tree is empty\n");
  }

  printf("--------------------\n\n");
}


void st_node_print_r(const st_node *node, int indent)
{
  int i;

  st_node_print(node, indent);
  for (i=0; i<node->nchild; ++i) {
    st_node_print_r(node->children[i], indent+1);
  }
}


void st_node_print(const st_node *node, int indent)
{
  int i;

  if (node != NULL) {
    if (node->marked) {
      printf("%3d *>", indent);
    } else {
      printf("%3d | ", indent);
    }
    for (i=indent; i>0; --i) printf("  ");
    switch (node->type) {
      case ST_NODE_ROOT:
        printf("Node { type: root }\n");
        break;
      case ST_NODE_SENDRECV:
        if (node->interaction->nto > 0) {
          printf("Node { type: interaction, from: %s, to(%d): [%s ..], msgsig: { op: %s, payload: %s }}\n", node->interaction->from, node->interaction->nto, node->interaction->to[0], node->interaction->msgsig.op, node->interaction->msgsig.payload);
        } else {
          printf("Node { type: interaction, from: %s, to(%d): [], msgsig: { op: %s, payload: %s }}\n", node->interaction->from, node->interaction->nto, node->interaction->msgsig.op, node->interaction->msgsig.payload);
        }
        break;
      case ST_NODE_SEND:
        printf("Node { type: send, to: %s, msgsig: { op: %s, payload: %s }}\n", node->interaction->to[0], node->interaction->msgsig.op, node->interaction->msgsig.payload);
        break;
      case ST_NODE_RECV:
        printf("Node { type: recv, from: %s, msgsig: { op: %s, payload: %s }}\n", node->interaction->from, node->interaction->msgsig.op, node->interaction->msgsig.payload);
        break;
      case ST_NODE_CHOICE:
        printf("Node { type: choice, at: %s } %d children \n", node->choice->at, node->nchild);
        break;
      case ST_NODE_PARALLEL:
        printf("Node { type: par }\n");
        break;
      case ST_NODE_RECUR:
        printf("Node { type: recur, label: %s }\n", node->recur->label);
        break;
      case ST_NODE_CONTINUE:
        printf("Node { type: continue, label: %s }\n", node->cont->label);
        break;
      default:
        fprintf(stderr, "%s:%d %s Unknown node type: %d\n", __FILE__, __LINE__, __FUNCTION__, node->type);
        break;
    }
  }
}


void st_node_reset_markedflag(st_node *node)
{
  int i = 0;
  assert(node != NULL);
  node->marked = 0;

  for (i=0; i<node->nchild; ++i) {
    st_node_reset_markedflag(node->children[i]);
  }
}


int st_node_compare_r_error(const st_node *node, const st_node *other, int indent)
{
  int identical = 1;

  if (node != NULL && other != NULL) {
    if (st_node_compare_r(node, other)) { // Identical.
      st_node_print_r(node, indent);
    } else if (st_node_compare(node, other)) { // Not reached diversion point.
      st_node_print(node, indent);
      int i;
      for (i=0; i<node->nchild; ++i) {
        identical &= st_node_compare_r_error(node->children[i], other->children[i], indent+1);
      }
    } else { // Diversion point.
      identical = 0;
      printf("**ERR**");
      st_node_print_r(node, indent);
      printf("--------------------\n");
      st_node_print_r(other, indent);

    }
  }

  return identical;
}


int st_node_compare_r(const st_node *node, const st_node *other)
{
  int identical = 1;
  int i;

  if (node != NULL && other != NULL) {
    identical &= st_node_compare(node, other);

    for (i=0; i<node->nchild; ++i) {
      identical &= st_node_compare(node->children[i], other->children[i]);
    }
  }

  return identical;
}


int st_node_msgsig_compare(const st_node_msgsig_t msgsig, const st_node_msgsig_t other)
{
  return (0 == strcmp(msgsig.op, other.op) && (0 == strcmp(msgsig.payload, other.payload)));
}


int st_node_compare(const st_node *node, const st_node *other)
{
  int identical = 1;
  if (node != NULL && other != NULL) {
    identical = (node->type == other->type && node->nchild == other->nchild);

    switch (node->type) {
      case ST_NODE_ROOT:
        break;
      case ST_NODE_SENDRECV:
        identical &= st_node_msgsig_compare(node->interaction->msgsig, other->interaction->msgsig);
        identical &= (0 == strcmp(node->interaction->from, other->interaction->from));
        identical &= (node->interaction->nto == other->interaction->nto);

        if (identical) {
          int i;
          for (i=0; i<node->interaction->nto; ++i) {
            identical &= (0 ==strcmp(node->interaction->to[i], other->interaction->to[i]));
          }
        }
        break;
      case ST_NODE_SEND:
        identical &= st_node_msgsig_compare(node->interaction->msgsig, other->interaction->msgsig);
        identical &= (node->interaction->nto == other->interaction->nto);

        if (identical) {
          int i;
          for (i=0; i<node->interaction->nto; ++i) {
            identical &= (0 ==strcmp(node->interaction->to[i], other->interaction->to[i]));
          }
        }
        break;
      case ST_NODE_RECV:
        identical &= st_node_msgsig_compare(node->interaction->msgsig, other->interaction->msgsig);
        identical &= (0 == strcmp(node->interaction->from, other->interaction->from));
        identical &= (node->interaction->nto == other->interaction->nto);

        if (identical) {
          int i;
          for (i=0; i<node->interaction->nto; ++i) {
            identical &= (0 ==strcmp(node->interaction->to[i], other->interaction->to[i]));
          }
        }
        break;
      case ST_NODE_CHOICE:
        assert(0 /* compare CHOICE */);
        break;
      case ST_NODE_PARALLEL:
        assert(0 /* compare PARALLEL */);
        break;
      case ST_NODE_RECUR:
        identical &= (0 == strcmp(node->recur->label, other->recur->label));
        break;
      case ST_NODE_CONTINUE:
        identical &= (0 == strcmp(node->recur->label, other->recur->label));
        break;
      default:
        fprintf(stderr, "%s:%d %s Unknown node type: %d\n", __FILE__, __LINE__, __FUNCTION__, node->type);
        break;
    }
  }

  return identical;
}
