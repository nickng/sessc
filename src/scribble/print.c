/**
 * \file
 * This file contains a pretty printer of (multiparty) session
 * in form of a Scribble protocol.
 *
 * \headerfile "st_node.h"
 * \headerfile "scribble/print.h"
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

  fprintf(stream, "%s(%s) %sfrom%s %s %sto%s ",
      node->interaction->msgsig.op == NULL? "" : node->interaction->msgsig.op,
      node->interaction->msgsig.payload == NULL? "" : node->interaction->msgsig.payload,
      GREEN, RESET,
      node->interaction->from,
      GREEN, RESET
      );
  for (i=0; i<node->interaction->nto; ++i) {
    fprintf(stream, "%s", node->interaction->to[i]);
    if (i != node->interaction->nto-1) {
      fprintf(stream, ", ");
    }
  }
  fprintf(stream, ";%s%s%s\n", RED, node->marked ? " // <- HERE" : "", RESET);
}


void scribble_fprint_send(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_SEND);
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "%s(%s) %sto%s ",
      node->interaction->msgsig.op == NULL? "" : node->interaction->msgsig.op,
      node->interaction->msgsig.payload == NULL? "" : node->interaction->msgsig.payload,
      GREEN, RESET);
  for (i=0; i<node->interaction->nto; ++i) {
    fprintf(stream, "%s", node->interaction->to[i]);
    if (i != node->interaction->nto-1) {
      fprintf(stream, ", ");
    }
  }
  fprintf(stream, ";%s%s%s\n", GREEN, node->marked ? " // <- HERE" : "", RESET);
}


void scribble_fprint_recv(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_RECV);
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "%s(%s) %sfrom%s %s",
      node->interaction->msgsig.op == NULL? "" : node->interaction->msgsig.op,
      node->interaction->msgsig.payload == NULL? "" : node->interaction->msgsig.payload,
      GREEN, RESET,
      node->interaction->from);
  fprintf(stream, ";%s\n", node->marked ? " // <- HERE" : "");
}


void scribble_fprint_choice(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_CHOICE);
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "%schoice at%s %s %s%s%s", GREEN, RESET, node->choice->at, RED, node->marked ? "/* HERE */ " : "", RESET);
  for (i=0; i<node->nchild; ++i) {
    scribble_fprint_node(stream, node->children[i], indent);
    if (i != node->nchild-1) {
      fprintf(stream, " %sor%s ", GREEN, RESET);
    }
  }
  fprintf(stream, "\n");
}

void scribble_fprint_parallel(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_PARALLEL);
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "%spar%s %s%s%s ", GREEN, RESET, RED, node->marked ? "/* HERE */" : "", RESET);
  for (i=0; i<node->nchild; ++i) {
    scribble_fprint_node(stream, node->children[i], indent);
    if (i != node->nchild-1) {
      fprintf(stream, " %sand%s ", GREEN, RESET);
    }
  }
  fprintf(stream, "\n");
}


void scribble_fprint_recur(FILE *stream, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_RECUR);
  int i;
  for (i=0; i<indent; ++i) fprintf(stream, "  ");

  fprintf(stream, "%srec%s %s {%s%s%s\n", GREEN, RESET, node->recur->label, RED, node->marked ? " // <- HERE" : "", RESET);
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

  fprintf(stream, "%scontinue%s %s;%s%s%s\n", GREEN, RESET, node->cont->label, RED, node->marked ? " // <- HERE" : "", RESET);
}


void scribble_fprint_root(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_ROOT);
  fprintf(stream, "{%s%s%s\n", RED, node->marked ? " // <- HERE" : "", RESET);

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
    fprintf(stream, "%simport%s %s", GREEN, RESET, tree->info->imports[i]->name);
    if (tree->info->imports[i]->from != NULL) {
      fprintf(stream, " %sfrom%s %s", GREEN, RESET, tree->info->imports[i]->from);
    }
    if (tree->info->imports[i]->as != NULL) {
      fprintf(stream, " %sas%s %s", GREEN, RESET, tree->info->imports[i]->as);
    }
    fprintf(stream, ";\n");
  }

  if (tree->info->global) {
    fprintf(stream, "%sglobal protocol%s %s ", GREEN, RESET, tree->info->name);
  } else {
    fprintf(stream, "%slocal protocol%s %s at %s ", GREEN, RESET, tree->info->name, tree->info->myrole);
  }
  fprintf(stream, "(");
  for (i=0; i<tree->info->nrole; ++i) {
    fprintf(stream, "%srole%s %s", GREEN, RESET, tree->info->roles[i]);
    if (i != tree->info->nrole-1) {
      fprintf(stream, ", ");
    }
  }
  fprintf(stream, ") ");

  scribble_fprint_root(stream, tree->root, 0);

  fprintf(stream, "\n");
}


void scribble_print(st_tree *tree)
{
  scribble_fprint(stdout, tree);
}
