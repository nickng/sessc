/**
 * \file
 * Thie file contains the tree representation of (multiparty) session
 * according to the Scribble language specification and provides functions
 * to build and manipulate session type trees.
 * 
 * \headerfile "st_node.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "st_node.h"


st_tree *st_tree_init(st_tree *tree)
{
  tree->info = (st_info *)malloc(sizeof(st_info));
  tree->info->nrole   = 0;
  tree->info->nimport = 0;
  tree->root = NULL;

  return tree;
}


st_tree *st_tree_set_name(st_tree *tree, const char *name)
{
  tree->info->name = (char *)calloc(sizeof(char), strlen(name)+1);
  strcpy(tree->info->name, name);

  return tree;
}


st_tree *st_tree_add_role(st_tree *tree, const char *role)
{
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

  return node;
}


st_node *st_node_append(st_node *node, st_node *child)
{
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
    st_node_printr(tree->root, 0);
  } else {
    printf("Protocol tree is empty\n");
  }

  printf("--------------------\n\n");
}


void st_node_printr(const st_node *node, int indent)
{
  int i;

  st_node_print(node, indent);
  for (i=0; i<node->nchild; ++i) {
    st_node_printr(node->children[i], indent+1);
  }
}


void st_node_print(const st_node *node, int indent)
{
  int i;
  if (node != NULL) {
    printf("%d|", indent);
    for (i=indent; i>0; --i) printf("  ");
    switch (node->type) {
      case ST_NODE_ROOT:
        printf("Node { type: root }\n");
        break;
      case ST_NODE_SENDRECV:
        printf("Node { type: interaction, from: %s, to: %s msgsig: { op: %s, payload: %s }}\n", node->interaction->from, node->interaction->to[0], node->interaction->msgsig.op, node->interaction->msgsig.payload);
        break;
      case ST_NODE_SEND:
        printf("Node { type: send, to: %s, msgsig: { op: %s, payload: %s }}\n", node->interaction->to[0], node->interaction->msgsig.op, node->interaction->msgsig.payload);
        break;
      case ST_NODE_RECV:
        printf("Node { type: recv, from: %s, msgsig: { op: %s, payload: %s }}\n", node->interaction->from, node->interaction->msgsig.op, node->interaction->msgsig.payload);
        break;
      case ST_NODE_CHOICE:
        printf("Node { type: choice }\n");
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
