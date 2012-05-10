#ifndef SCRIBBLE__PRINT__H__
#define SCRIBBLE__PRINT__H__
/**
 * \file
 * Header file for Scribble pretty printer.
 *
 * \headerfile "st_node.h"
 */

#include <stdio.h>
#include "st_node.h"

#ifdef COLOUR_SUPPORT
  #define GREEN "\033[1;32m"
  #define RED "\033[41m"
  #define RESET "\033[0m"
#else
  #define GREEN ""
  #define RED ""
  #define RESET ""
#endif

void scribble_fprint_root(FILE *stream, st_node *node, int indent);
void scribble_fprint_message(FILE *stream, st_node *node, int indent);
void scribble_fprint_send(FILE *stream, st_node *node, int indent);
void scribble_fprint_recv(FILE *stream, st_node *node, int indent);
void scribble_fprint_choice(FILE *stream, st_node *node, int indent);
void scribble_fprint_parallel(FILE *stream, st_node *node, int indent);
void scribble_fprint_recur(FILE *stream, st_node *node, int indent);
void scribble_fprint_continue(FILE *stream, st_node *node, int indent);
void scribble_fprint_node(FILE *stream, st_node *node, int indent);

/**
 * \brief Pretty print a st_tree into Scribble to an output stream.
 * 
 * @param[out] stream Output stream.
 * @param[in]  tree   st_tree instance to print.
 */
void scribble_fprint(FILE *stream, st_tree *tree);


/**
 * \brief Pretty print a st_tree into Scribble.
 * 
 * @param[in]  tree   st_tree instance to print.
 */
void scribble_print(st_tree *tree);


#endif // SCRIBBLE__PRINT__H__
