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
#include <string.h>
#include <stdarg.h>

#include "st_node.h"
#include "scribble/print.h"

static int is_consecutive(long arr[], int count)
{
  int i;
  if (count <= 1) return 0;
  for (i=1; i<count; ++i) {
    if (arr[i] != arr[i-1]+1) return 0;
  }
  return 1;
}

int scribble_colour_mode(int colour_mode)
{
  static int mode = 0;
  if (colour_mode == -1) return mode;
  mode = colour_mode;
  return mode;
}

void scribble_fprintf(FILE *stream, const char *format, ...)
{
  int i = 0;
  char orig_str[2560];
  int found;
  static int in_comment = 0;
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(orig_str, 2560, format, argptr);
  va_end(argptr);

  const char *scribble_keywords[] = {
    "and", "as", "at", "by",
    "catch", "choice", "create", "do",
    "enter", "for", "from", "global",
    "import", "instantiates", "interruptible", "local",
    "or", "par", "protocol", "rec",
    "role", "spawns", "throw", "to",
    "with", "if", "const", "range",
    "for", NULL
  };

  // Colour mode
  //
  if (!scribble_colour_mode(-1)) {
    fprintf(stream, "%s", orig_str);
    return;
  }

  // Not colour mode
  //
  for (i=0; i<strlen(orig_str); ++i) {
    if (orig_str[i] == ' ') {
      fprintf(stream, " ");
    } else {
      break;
    }
  }
  char *token = strtok(orig_str, " ");
  if (token == NULL) return;
  do {
    // Comment detection
    if (!in_comment && NULL != strstr(token, "//")) {
      in_comment = 1;
    }
    if (in_comment) {
      if (NULL != strchr(token, '\n')) {
        fprintf(stream, "\033[1;34m%s\033[0m", token);
        in_comment = 0;
      } else {
        fprintf(stream, "\033[1;34m%s\033[0m ", token);
      }
      continue;
    }

    // Keywords detection
    found = 0;
    i = 0;
    while (scribble_keywords[i] != NULL) {
      if (0 == strcmp(token, scribble_keywords[i])) {
        found = 1;
      }
      i++;
    }
    if (found) {
      fprintf(stream, "\033[1;32m%s\033[0m ", token);
    } else {
      if (NULL != strchr(token, '\n') || NULL != strchr(token, '}')) {
        // w/o trailing space
        fprintf(stream, "%s", token);
      } else {
        fprintf(stream, "%s ", token);
      }
    }
  } while ((token = strtok(NULL, " ")) != NULL);
}

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
    case ST_NODE_PARALLEL: scribble_fprint_parallel(stream, node, indent);
      break;
    case ST_NODE_RECUR:
      scribble_fprint_recur(stream, node, indent);
      break;
    case ST_NODE_CONTINUE:
      scribble_fprint_continue(stream, node, indent);
      break;
    case ST_NODE_FOR:
      scribble_fprint_for(stream, node, indent);
      break;
    default:
      fprintf(stderr, "%s:%d %s Unknown node type: %d\n", __FILE__, __LINE__, __FUNCTION__, node->type);
  }
}


void scribble_fprint_expr(FILE *stream, st_expr_t *expr)
{
  assert(expr != NULL);

  switch (expr->type) {
    case ST_EXPR_TYPE_RANGE:
      st_expr_print(expr->binexpr->left);
      scribble_fprintf(stream, "..");
      st_expr_print(expr->binexpr->right);
      break;
    case ST_EXPR_TYPE_PLUS:
      st_expr_print(expr->binexpr->left);
      scribble_fprintf(stream, "+");
      st_expr_print(expr->binexpr->right);
      break;
    case ST_EXPR_TYPE_TUPLE:
      st_expr_print(expr->binexpr->left);
      scribble_fprintf(stream, "][");
      st_expr_print(expr->binexpr->right);
      break;
    case ST_EXPR_TYPE_MINUS:
      st_expr_print(expr->binexpr->left);
      scribble_fprintf(stream, "-");
      st_expr_print(expr->binexpr->right);
      break;
    case ST_EXPR_TYPE_CONST:
      scribble_fprintf(stream, "%d", expr->constant);
      break;
    case ST_EXPR_TYPE_VAR:
      scribble_fprintf(stream, "%s", expr->variable);
      break;
  };
}


void scribble_fprint_message(FILE *stream, st_node *node, int indent)
{
  int i, j;
  assert(node != NULL && node->type == ST_NODE_SENDRECV);
  for (i=0; i<indent; ++i) scribble_fprintf(stream, "  ");

  // From
  scribble_fprintf(stream, "%s(%s) from ",
      node->interaction->msgsig.op == NULL? "" : node->interaction->msgsig.op,
      node->interaction->msgsig.payload == NULL? "" : node->interaction->msgsig.payload);
  scribble_fprintf(stream, "%s",
      node->interaction->from->name);
  if (NULL != node->interaction->from->param) {
    scribble_fprintf(stream, "[");
    scribble_fprint_expr(stream, node->interaction->from->param);
    scribble_fprintf(stream, "]");
  }

  // To
  scribble_fprintf(stream, " to ");
  for (i=0; i<node->interaction->nto; ++i) {
    scribble_fprintf(stream, "%s",
        node->interaction->to[i]->name);
    if (NULL != node->interaction->to[i]->param) {
      scribble_fprintf(stream, "[");
      scribble_fprint_expr(stream, node->interaction->to[i]->param);
      scribble_fprintf(stream, "]");
    }
    if (i!=node->interaction->nto-1)
      scribble_fprintf(stream, ", ");
  }

  scribble_fprintf(stream, ";%s\n", node->marked ? " // <- HERE" : "");
}


void scribble_fprint_send(FILE *stream, st_node *node, int indent)
{
  int i, j;
  assert(node != NULL && node->type == ST_NODE_SEND);
  for (i=0; i<indent; ++i) scribble_fprintf(stream, "  ");

  scribble_fprintf(stream, "%s(%s) to ",
      node->interaction->msgsig.op == NULL? "" : node->interaction->msgsig.op,
      node->interaction->msgsig.payload == NULL? "" : node->interaction->msgsig.payload);

  // To
  for (i=0; i<node->interaction->nto; ++i) {
    scribble_fprintf(stream, "%s",
        node->interaction->to[i]->name);
    if (NULL != node->interaction->to[i]->param) {
      scribble_fprintf(stream, "[");
      scribble_fprint_expr(stream, node->interaction->to[i]->param);
      scribble_fprintf(stream, "]");
    }
    if (i!=node->interaction->nto-1)
      scribble_fprintf(stream, ", ");
  }

  // If
  if (node->interaction->msg_cond != NULL) {
    scribble_fprintf(stream, " if %s[",
        node->interaction->msg_cond->name);
    scribble_fprint_expr(stream, node->interaction->msg_cond->param);
    scribble_fprintf(stream, "]");
  }

  scribble_fprintf(stream, ";%s\n", node->marked ? " // <- HERE" : "");
}


void scribble_fprint_recv(FILE *stream, st_node *node, int indent)
{
  int i, j;
  assert(node != NULL && node->type == ST_NODE_RECV);
  for (i=0; i<indent; ++i) scribble_fprintf(stream, "  ");

  // From
  scribble_fprintf(stream, "%s(%s) from ",
      node->interaction->msgsig.op == NULL? "" : node->interaction->msgsig.op,
      node->interaction->msgsig.payload == NULL? "" : node->interaction->msgsig.payload);
  scribble_fprintf(stream, "%s",
      node->interaction->from->name);
  if (NULL != node->interaction->from->param) {
    scribble_fprintf(stream, "[");
    scribble_fprint_expr(stream, node->interaction->from->param);
    scribble_fprintf(stream, "]");
  }

  // If
  if (node->interaction->msg_cond != NULL) {
    scribble_fprintf(stream, " if %s[",
        node->interaction->msg_cond->name);
    scribble_fprint_expr(stream, node->interaction->msg_cond->param);
    scribble_fprintf(stream, "]");
  }

  scribble_fprintf(stream, ";%s\n", node->marked ? " // <- HERE" : "");
}


void scribble_fprint_choice(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_CHOICE);
  for (i=0; i<indent; ++i) scribble_fprintf(stream, "  ");

  scribble_fprintf(stream, "choice at %s %s", node->choice->at, node->marked ? "/* HERE */ " : "");
  for (i=0; i<node->nchild; ++i) {
    scribble_fprint_node(stream, node->children[i], indent);
    if (i != node->nchild-1) {
      scribble_fprintf(stream, " or ");
    }
  }
  scribble_fprintf(stream, "\n");
}

void scribble_fprint_parallel(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_PARALLEL);
  for (i=0; i<indent; ++i) scribble_fprintf(stream, "  ");

  scribble_fprintf(stream, "par %s ", node->marked ? "/* HERE */" : "");
  for (i=0; i<node->nchild; ++i) {
    scribble_fprint_node(stream, node->children[i], indent);
    if (i != node->nchild-1) {
      scribble_fprintf(stream, " and ");
    }
  }
  scribble_fprintf(stream, "\n");
}


void scribble_fprint_recur(FILE *stream, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_RECUR);
  int i;
  for (i=0; i<indent; ++i) scribble_fprintf(stream, "  ");

  scribble_fprintf(stream, "rec %s {%s\n", node->recur->label, node->marked ? " // <- HERE" : "");
  for (i=0; i<node->nchild; ++i) {
    scribble_fprint_node(stream, node->children[i], indent+1);
  }

  for (i=0; i<indent; ++i) scribble_fprintf(stream, "  ");
  scribble_fprintf(stream, "}\n");
}


void scribble_fprint_continue(FILE *stream, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_CONTINUE);
  int i;
  for (i=0; i<indent; ++i) scribble_fprintf(stream, "  ");

  scribble_fprintf(stream, "continue %s;%s\n", node->cont->label, node->marked ? " // <- HERE" : "");
}


void scribble_fprint_for(FILE *stream, st_node *node, int indent)
{
  assert(node != NULL && node->type == ST_NODE_FOR);
  int i;
  for (i=0; i<indent; ++i) scribble_fprintf(stream, "  ");

  scribble_fprintf(stream, "for (");
  scribble_fprint_expr(stream, node->forloop->range);
  scribble_fprintf(stream, ") %s ", node->marked ? " /* HERE */" : "");
  for (i=0; i<node->nchild; ++i) {
    scribble_fprint_node(stream, node->children[i], indent);
  }
  scribble_fprintf(stream, "\n");
}


void scribble_fprint_root(FILE *stream, st_node *node, int indent)
{
  int i;
  assert(node != NULL && node->type == ST_NODE_ROOT);
  scribble_fprintf(stream, "{%s\n", node->marked ? " // <- HERE" : "");

  for (i=0; i<node->nchild; ++i) {
    scribble_fprint_node(stream, node->children[i], indent+1);
  }

  for (i=0; i<indent; ++i) scribble_fprintf(stream, "  ");
 scribble_fprintf(stream, "}");
}


void scribble_fprint(FILE *stream, st_tree *tree)
{
  int i;
  for (i=0; i<tree->info->nimport; ++i) {
    scribble_fprintf(stream, "import %s", tree->info->imports[i]->name);
    if (tree->info->imports[i]->from != NULL) {
      scribble_fprintf(stream, " from %s", tree->info->imports[i]->from);
    }
    if (tree->info->imports[i]->as != NULL) {
      scribble_fprintf(stream, " as %s", tree->info->imports[i]->as);
    }
    scribble_fprintf(stream, ";\n");
  }

  if (ST_TYPE_GLOBAL == tree->info->type) {
    scribble_fprintf(stream, "global protocol %s ", tree->info->name);
  } else if (ST_TYPE_LOCAL == tree->info->type || ST_TYPE_PARAMETRISED == tree->info->type) {
    scribble_fprintf(stream, "local protocol %s at %s ", tree->info->name, tree->info->myrole);
  } else assert(1/* unrecognised type */);
  scribble_fprintf(stream, "(");
  for (i=0; i<tree->info->nrole; ++i) {
    scribble_fprintf(stream, "role %s", tree->info->roles[i]->name);
    if (NULL != tree->info->roles[i]->param) {
      scribble_fprintf(stream, "[");
      scribble_fprint_expr(stream, tree->info->roles[i]->param);
      scribble_fprintf(stream, "]");
    }
    if (i != tree->info->nrole-1) {
      scribble_fprintf(stream, ", ");
    }
  }
  scribble_fprintf(stream, ") ");

  scribble_fprint_root(stream, tree->root, 0);

  scribble_fprintf(stream, "\n");
}


void scribble_print(st_tree *tree)
{
  scribble_fprint(stdout, tree);
}
