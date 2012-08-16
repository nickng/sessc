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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Set whether scribble pretty printer uses colour.
 *
 * @param[in] colour_mode Colour mode to be set.
 *                        Use -1 to query without modifying colour_mode settings.
 *
 * \returns current colour mode.
 */
int scribble_colour_mode(int colour_mode);

void scribble_fprint_expr(FILE *stream, st_expr_t *expr);

void scribble_fprint_root(FILE *stream, st_node *node, int indent);
void scribble_fprint_message(FILE *stream, st_node *node, int indent);
void scribble_fprint_send(FILE *stream, st_node *node, int indent);
void scribble_fprint_recv(FILE *stream, st_node *node, int indent);
void scribble_fprint_choice(FILE *stream, st_node *node, int indent);
void scribble_fprint_parallel(FILE *stream, st_node *node, int indent);
void scribble_fprint_recur(FILE *stream, st_node *node, int indent);
void scribble_fprint_continue(FILE *stream, st_node *node, int indent);
void scribble_fprint_for(FILE *stream, st_node *node, int indent);
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

#ifdef __cplusplus
}
#endif

#endif // SCRIBBLE__PRINT__H__
