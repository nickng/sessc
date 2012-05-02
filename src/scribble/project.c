/**
 * \file
 * This file contains an implementation of projection algorithm
 * from global Scribble into endpoint Scribble.
 *
 * \headerfile "st_node.h"
 * \headerfile "scribble/project.h"
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "st_node.h"
#include "scribble/project.h"


st_node *scribble_project_root(st_node *node, char *projectrole)
{
  assert(node != NULL && node->type == ST_NODE_ROOT);
  st_node *local = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);

  int i = 0;
  st_node *child_node = NULL;
  for (i=0; i<node->nchild; ++i) {
    child_node = scribble_project_node(node->children[i], projectrole);
    if (child_node != NULL) {
      st_node_append(local, child_node);
    }
  }

  return local;
}


st_node *scribble_project_message(st_node *node, char *projectrole)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_SENDRECV);
  st_node *local = (st_node *)malloc(sizeof(st_node));

  if (strcmp(node->interaction->from, projectrole) == 0) {
    local = st_node_init(local, ST_NODE_SEND);
    local->interaction->from = NULL;
    local->interaction->nto = node->interaction->nto;
    local->interaction->to = (char **)malloc(sizeof(char *) * local->interaction->nto);
    for (i=0; i<local->interaction->nto; ++i) {
      local->interaction->to[i] = strdup(node->interaction->to[i]);
    }
    local->interaction->msgsig.op = strdup(node->interaction->msgsig.op);
    local->interaction->msgsig.payload = strdup(node->interaction->msgsig.payload);

    return local;
  }

  for (i=0; i<node->interaction->nto; ++i) {
    if (strcmp(node->interaction->to[i], projectrole) == 0) {
      local = st_node_init(local, ST_NODE_RECV);
      local->interaction->from = (char *)calloc(sizeof(char), strlen(node->interaction->from)+1);
      local->interaction->from = strdup(node->interaction->from);
      local->interaction->nto = 0;
      local->interaction->to = NULL;
      local->interaction->msgsig.op = strdup(node->interaction->msgsig.op);
      local->interaction->msgsig.payload = strdup(node->interaction->msgsig.payload);

      return local;
    }
  }

  return NULL;
}


st_node *scribble_project_choice(st_node *node, char *projectrole)
{
  assert(node != NULL && node->type == ST_NODE_CHOICE);
  st_node *local = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CHOICE);

  local->choice->at = (char *)calloc(sizeof(char), strlen(node->choice->at)+1);
  local->choice->at = strdup(node->choice->at);

  int i = 0;
  st_node *child_node = NULL;
  for (i=0; i<node->nchild; ++i) {
    child_node = scribble_project_node(node->children[i], projectrole);
    if (child_node != NULL) {
      st_node_append(local, child_node);
    }
  }

  return local;
}


st_node *scribble_project_parallel(st_node *node, char *projectrole)
{
  assert(node != NULL && node->type == ST_NODE_PARALLEL);
  st_node *local = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_PARALLEL);

  int i = 0;
  st_node *child_node = NULL;
  for (i=0; i<node->nchild; ++i) {
    child_node = scribble_project_node(node->children[i], projectrole);
    if (child_node != NULL) {
      st_node_append(local, child_node);
    }
  }

  return local;
}


st_node *scribble_project_recur(st_node *node, char *projectrole)
{
  assert(node != NULL && node->type == ST_NODE_RECUR);
  st_node *local = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECUR);

  local->recur->label = (char *)calloc(sizeof(char), strlen(node->recur->label)+1);
  local->recur->label = strdup(node->recur->label);

  int i = 0;
  st_node *child_node = NULL;
  for (i=0; i<node->nchild; ++i) {
    child_node = scribble_project_node(node->children[i], projectrole);
    if (child_node != NULL) {
      st_node_append(local, child_node);
    }
  }

  return local;
}


st_node *scribble_project_continue(st_node *node, char *projectrole)
{
  return node;
}


st_node *scribble_project_node(st_node *node, char *projectrole)
{
  switch (node->type) {
    case ST_NODE_ROOT:
      return scribble_project_root(node, projectrole);
      break;
    case ST_NODE_SENDRECV:
      return scribble_project_message(node, projectrole);
      break;
    case ST_NODE_CHOICE:
      return scribble_project_choice(node, projectrole);
      break;
    case ST_NODE_PARALLEL:
      return scribble_project_parallel(node, projectrole);
      break;
    case ST_NODE_RECUR:
      return scribble_project_recur(node, projectrole);
      break;
    case ST_NODE_CONTINUE:
      return scribble_project_continue(node, projectrole);
      break;
    case ST_NODE_SEND:
    case ST_NODE_RECV:
    default:
      fprintf(stderr, "%s:%d %s Unknown node type: %d\n", __FILE__, __LINE__, __FUNCTION__, node->type);
      return NULL;
      break;
  }
}


st_tree *scribble_project(st_tree *global, char *projectrole)
{
  int i;

  if (!global->info->global) {
    fprintf(stderr, "Warn: Not projecting for endpoint protocol.\n");
    return global;
  }


  st_tree *local = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
  local->info->myrole = (char *)calloc(sizeof(char), strlen(projectrole)+1);
  strcpy(local->info->myrole, projectrole);

  st_tree_set_name(local, global->info->name);
  local->info->global = 0;
  // Copy imports over.
  for (i=0; i<global->info->nimport; ++i) {
    st_tree_add_import(local, *(global->info->imports[i]));
  }
  // Copy roles over.
  for (i=0; i<global->info->nrole; ++i) {
    if (strcmp(global->info->roles[i], projectrole) == 0) {
      continue;
    }
    st_tree_add_role(local, global->info->roles[i]);
  }
  assert(global->info->nrole == local->info->nrole+1);

  if (global->root != NULL) {
    assert(global->root->type == ST_NODE_ROOT);
    local->root = scribble_project_root(global->root, projectrole);
  }
  return local;
}
