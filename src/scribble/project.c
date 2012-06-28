/**
 * \file
 * This file contains an implementation of projection algorithm
 * from global Scribble into endpoint Scribble.
 *
 * \headerfile "st_node.h"
 * \headerfile "scribble/project.h"
 */

#include <stdio.h>
#include <stdlib.h>
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

  assert(ST_ROLE_NORMAL == node->interaction->from_type||ST_ROLE_PARAMETRISED == node->interaction->from_type
         ||ST_ROLE_NORMAL == node->interaction->to_type||ST_ROLE_PARAMETRISED == node->interaction->to_type);

  // Parametrised from
  if (ST_ROLE_PARAMETRISED == node->interaction->from_type) {
    if (0 == strcmp(node->interaction->p_from->name, projectrole)) { // Rule 1, 3
      local = st_node_init(local, ST_NODE_SEND);

      local->interaction->from_type = ST_ROLE_NORMAL;
      local->interaction->from = NULL;

      local->interaction->nto = node->interaction->nto;
      if (ST_ROLE_PARAMETRISED == node->interaction->to_type) { // Rule 1, parametrised -> parametrised

        local->interaction->to_type = ST_ROLE_PARAMETRISED;
        local->interaction->p_to = (parametrised_role_t **)calloc(sizeof(parametrised_role_t *), local->interaction->nto);
        for (i=0; i<local->interaction->nto; ++i) {
          local->interaction->p_to[i] = (parametrised_role_t *)malloc(sizeof(parametrised_role_t));
          local->interaction->p_to[i]->name = strdup(node->interaction->p_to[i]->name);
          local->interaction->p_to[i]->bindvar = strdup(node->interaction->p_to[i]->bindvar);
          local->interaction->p_to[i]->idxcount = node->interaction->p_to[i]->idxcount;
          local->interaction->p_to[i]->indices = (long *)calloc(sizeof(long), local->interaction->p_to[i]->idxcount);
          memcpy(local->interaction->p_to[i]->indices, node->interaction->p_to[i]->indices, sizeof(long) * local->interaction->p_to[i]->idxcount);
        }

      } else if (ST_ROLE_NORMAL == node->interaction->to_type) { // Rule 3, parametrised -> non-parametirsed

        local->interaction->to_type = ST_ROLE_NORMAL;
        local->interaction->to = (char **)calloc(sizeof(char *), local->interaction->nto);
        for (i=0; i<local->interaction->nto; ++i) {
          local->interaction->to[i] = strdup(node->interaction->to[i]);
        }
      }

      // Message condition
      local->interaction->msg_cond = (msg_cond_t *)malloc(sizeof(msg_cond_t));
      local->interaction->msg_cond->name = strdup(node->interaction->p_from->name);
      local->interaction->msg_cond->bindvar = strdup(node->interaction->p_from->bindvar);
      local->interaction->msg_cond->idxcount = node->interaction->p_from->idxcount;
      local->interaction->msg_cond->indices = (long *)calloc(sizeof(long), local->interaction->msg_cond->idxcount);
      memcpy(local->interaction->msg_cond->indices, node->interaction->p_from->indices, sizeof(long) * local->interaction->msg_cond->idxcount);

      // Message signature
      local->interaction->msgsig.op = node->interaction->msgsig.op == NULL ? NULL : strdup(node->interaction->msgsig.op);
      local->interaction->msgsig.payload = node->interaction->msgsig.payload == NULL ? NULL : strdup(node->interaction->msgsig.payload);

      return local;
    }
  }


  // Parametrised to
  if (ST_ROLE_PARAMETRISED == node->interaction->to_type) {
    for (i=0; i<node->interaction->nto; ++i) {
      if (0 == strcmp(node->interaction->p_to[i]->name, projectrole)) { // Rule 2, 4
        local = st_node_init(local, ST_NODE_RECV);

        if (ST_ROLE_PARAMETRISED == node->interaction->from_type) { // Rule 2, parametrised -> parametrised

          local->interaction->from_type = ST_ROLE_PARAMETRISED;
          local->interaction->p_from = (parametrised_role_t *)malloc(sizeof(parametrised_role_t));
          local->interaction->p_from->name = strdup(node->interaction->p_from->name);
          local->interaction->p_from->bindvar = strdup(node->interaction->p_from->bindvar);
          local->interaction->p_from->idxcount = node->interaction->p_from->idxcount;
          local->interaction->p_from->indices = (long *)calloc(sizeof(long), local->interaction->p_from->idxcount);
          memcpy(local->interaction->p_from->indices, node->interaction->p_from->indices, sizeof(long) * local->interaction->p_from->idxcount);

        } else if (ST_ROLE_NORMAL == node->interaction->from_type) { // Rule 4, non-parametrised -> parametrised

          local->interaction->from_type = ST_ROLE_NORMAL;
          local->interaction->from = strdup(node->interaction->from);

        }

        // Message condition (XXX: only copy to[0])
        local->interaction->msg_cond = (msg_cond_t *)malloc(sizeof(msg_cond_t));
        local->interaction->msg_cond->name = strdup(node->interaction->p_to[0]->name);
        local->interaction->msg_cond->bindvar = strdup(node->interaction->p_to[0]->bindvar);
        local->interaction->msg_cond->idxcount = node->interaction->p_to[0]->idxcount;
        local->interaction->msg_cond->indices = (long *)calloc(sizeof(long), local->interaction->msg_cond->idxcount);
        memcpy(local->interaction->msg_cond->indices, node->interaction->p_to[0]->indices, sizeof(long) * local->interaction->msg_cond->idxcount);

        // Message signature
        local->interaction->msgsig.op = node->interaction->msgsig.op == NULL ? NULL : strdup(node->interaction->msgsig.op);
        local->interaction->msgsig.payload = node->interaction->msgsig.payload == NULL ? NULL : strdup(node->interaction->msgsig.payload);

        return local;
      }
    }
  }


  // Non-parametrised from
  if (ST_ROLE_NORMAL == node->interaction->from_type) { // Rule 5
    if (0 == strcmp(node->interaction->from, projectrole)) {
      local = st_node_init(local, ST_NODE_SEND);
      local->interaction->from = NULL;

      local->interaction->nto = node->interaction->nto;
      if (ST_ROLE_PARAMETRISED == node->interaction->to_type) { // Rule 5, parametrised -> non-parametrised

        local->interaction->to_type = ST_ROLE_PARAMETRISED;
        local->interaction->p_to = (parametrised_role_t **)calloc(sizeof(parametrised_role_t *), local->interaction->nto);
        for (i=0; i<local->interaction->nto; ++i) {
          local->interaction->p_to[i] = (parametrised_role_t *)malloc(sizeof(parametrised_role_t));
          local->interaction->p_to[i]->name = strdup(node->interaction->p_to[i]->name);
          local->interaction->p_to[i]->bindvar = strdup(node->interaction->p_to[i]->bindvar);
          local->interaction->p_to[i]->idxcount = node->interaction->p_to[i]->idxcount;
          local->interaction->p_to[i]->indices = (long *)calloc(sizeof(long), local->interaction->p_to[i]->idxcount);
          memcpy(local->interaction->p_to[i]->indices, node->interaction->p_to[i]->indices, sizeof(long) * local->interaction->p_to[i]->idxcount);
        }

      } else if (ST_ROLE_NORMAL == node->interaction->to_type) { // Rule 5.0, non-parametrised -> non-parametirsed

        local->interaction->to_type = ST_ROLE_NORMAL;
        local->interaction->to = (char **)calloc(sizeof(char *), local->interaction->nto);
        for (i=0; i<local->interaction->nto; ++i) {
          local->interaction->to[i] = strdup(node->interaction->to[i]);
        }

      }

      // Note: No msg_cond
      local->interaction->msg_cond = NULL;

      // Message signature
      local->interaction->msgsig.op = node->interaction->msgsig.op == NULL ? NULL : strdup(node->interaction->msgsig.op);
      local->interaction->msgsig.payload = node->interaction->msgsig.payload == NULL ? NULL : strdup(node->interaction->msgsig.payload);

      return local;
    }
  }


  // Non-parametrised to
  if (ST_ROLE_NORMAL == node->interaction->to_type) { // Rule 6
    for (i=0; i<node->interaction->nto; ++i) {
      if (0 == strcmp(node->interaction->to[i], projectrole)) {
        local = st_node_init(local, ST_NODE_RECV);

        if (ST_ROLE_PARAMETRISED == node->interaction->from_type) { // Rule 6 non-parametrised -> parametrised

          local->interaction->from_type = ST_ROLE_PARAMETRISED;
          local->interaction->p_from = (parametrised_role_t *)malloc(sizeof(parametrised_role_t));
          local->interaction->p_from->name = strdup(node->interaction->p_from->name);
          local->interaction->p_from->bindvar = strdup(node->interaction->p_from->bindvar);
          local->interaction->p_from->idxcount = node->interaction->p_from->idxcount;
          local->interaction->p_from->indices = (long *)calloc(sizeof(long), local->interaction->p_from->idxcount);
          memcpy(local->interaction->p_from->indices, node->interaction->p_from->indices, sizeof(long) * local->interaction->p_from->idxcount);

        } else if (ST_ROLE_NORMAL == node->interaction->from_type) {  // Rule 6.0 non-parametrised -> non-parametrised

          local->interaction->from = strdup(node->interaction->from);

        }

        local->interaction->nto = 0;
        local->interaction->to = NULL;

        // Note: No msg_cond
        local->interaction->msg_cond = NULL;

        // Message signature
        local->interaction->msgsig.op = node->interaction->msgsig.op == NULL ? NULL : strdup(node->interaction->msgsig.op);
        local->interaction->msgsig.payload = node->interaction->msgsig.payload == NULL ? NULL : strdup(node->interaction->msgsig.payload);

        return local;
      }
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
