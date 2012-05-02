/**
 * \file
 * This file contains a pretty printer of (multiparty) session
 * in form of a Scribble protocol.
 *
 * \headerfile "st_node.h"
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "st_node.h"

#include "scribble/print.h"


void scribble_fprint_node(FILE *stream, st_node *node, int indent)
{
  switch (node->type) {
    case ST_NODE_ROOT:
      scribble_fprint_root(stream, node, indent);
      break;
    case ST_NODE_SENDRECV:
      scribble_fprint_message(stream, node, indent);
      break;
    case ST_NODE_SEND:
      scribble_fprint_send(stream, node, indent);
      break;
    case ST_NODE_RECV:
      scribble_fprint_recv(stream, node, indent);
      break;
    case ST_NODE_CHOICE:
      scribble_fprint_choice(stream, node, indent);
      break;
    case ST_NODE_PARALLEL:
      scribble_fprint_parallel(stream, node, indent);
      break;
    case ST_NODE_RECUR:
      scribble_fprint_recur(stream, node, indent);
      break;
    case ST_NODE_CONTINUE:
      scribble_fprint_continue(stream, node, indent);
      break;
    default:
      fprintf(stderr, "%s:%d %s Unknown node type: %d\n", __FILE__, __LINE__, __FUNCTION__, node->type);
  }
}


void scribble_fprint_message(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_SENDRECV);
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "%s(%s) from %s to ",
      node->interaction->msgsig.op == NULL? "" : node->interaction->msgsig.op,
      node->interaction->msgsig.payload == NULL? "" : node->interaction->msgsig.payload,
      node->interaction->from);
  for (i=0; i<node->interaction->nto; ++i) {
    fprintf(stream, "%s", node->interaction->to[i]);
    if (i != node->interaction->nto-1) {
      fprintf(stream, ", ");
    }
  }
  fprintf(stream, ";\n");
}


void scribble_fprint_send(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_SEND);
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "%s(%s) to ",
      node->interaction->msgsig.op == NULL? "" : node->interaction->msgsig.op,
      node->interaction->msgsig.payload == NULL? "" : node->interaction->msgsig.payload);
  for (i=0; i<node->interaction->nto; ++i) {
    fprintf(stream, "%s", node->interaction->to[i]);
    if (i != node->interaction->nto-1) {
      fprintf(stream, ", ");
    }
  }
  fprintf(stream, ";\n");
}


void scribble_fprint_recv(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_RECV);
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "%s(%s) from %s",
      node->interaction->msgsig.op == NULL? "" : node->interaction->msgsig.op,
      node->interaction->msgsig.payload == NULL? "" : node->interaction->msgsig.payload,
      node->interaction->from);
  fprintf(stream, ";\n");
}


void scribble_fprint_choice(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_CHOICE);
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "choice at %s ", node->choice->at);
  for (i=0; i<node->nchild; ++i) {
    scribble_fprint_node(stream, node->children[i], indent);
    if (i != node->nchild-1) {
      fprintf(stream, " or ");
    }
  }
  fprintf(stream, "\n");
}

void scribble_fprint_parallel(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_PARALLEL);
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "par ");
  for (i=0; i<node->nchild; ++i) {
    scribble_fprint_node(stream, node->children[i], indent);
    if (i != node->nchild-1) {
      fprintf(stream, " and ");
    }
  }
  fprintf(stream, "\n");
}


void scribble_fprint_recur(FILE *stream, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_RECUR);
  int i;
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "rec %s {\n", node->recur->label);
  for (i=0; i<node->nchild; ++i) {
    scribble_fprint_node(stream, node->children[i], indent+1);
  }

  for (i=0; i<indent; ++i) fprintf(stream, "  ");
  fprintf(stream, "}\n");
}


void scribble_fprint_continue(FILE *stream, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_CONTINUE);
  int i;
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "continue %s;\n", node->cont->label);
}


void scribble_fprint_root(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_ROOT);
  fprintf(stream, "{\n");

  for (i=0; i<node->nchild; ++i) {
    scribble_fprint_node(stream, node->children[i], indent+1);
  }

  for (i=0; i<indent; ++i) fprintf(stream, "  ");
  fprintf(stream, "}");
}


void scribble_fprint(FILE *stream, st_tree *tree)
{
  int i;
  for (i=0; i<tree->info->nimport; ++i) {
    fprintf(stream, "import %s", tree->info->imports[i]->name);
    if (tree->info->imports[i]->from != NULL) {
      fprintf(stream, " from %s", tree->info->imports[i]->from);
    }
    if (tree->info->imports[i]->as != NULL) {
      fprintf(stream, " as %s", tree->info->imports[i]->as);
    }
    fprintf(stream, ";\n");
  }

  if (tree->info->global) {
    fprintf(stream, "global protocol %s ", tree->info->name);
  } else {
    fprintf(stream, "local protocol %s at %s ", tree->info->name, tree->info->myrole);
  }
  fprintf(stream, "(");
  for (i=0; i<tree->info->nrole; ++i) {
    fprintf(stream, "role %s", tree->info->roles[i]);
    if (i != tree->info->nrole-1) {
      fprintf(stream, ", ");
    }
  }
  fprintf(stream, ")");

  scribble_fprint_root(stream, tree->root, 0);

  fprintf(stream, "\n");
}


void scribble_print(st_tree *tree)
{
  scribble_fprint(stdout, tree);
}
