#ifndef SCRIBBLE__PROJECT__H__
#define SCRIBBLE__PROJECT__H__
/**
 * \file
 * This file contains the implmentation of projection algorithm
 * from global Scribble into endpoint Scribble.
 *
 */

#include "st_node.h"

#ifdef __cplusplus
extern "C" {
#endif

st_node *scribble_project_root(st_node *node, char *projectrole);
st_node *scribble_project_choice(st_node *node, char *projectrole);
st_node *scribble_project_parallel(st_node *node, char *projectrole);
st_node *scribble_project_recur(st_node *node, char *projectrole);
st_node *scribble_project_continue(st_node *node, char *projectrole);
st_node *scribble_project_node(st_node *node, char *projectrole);

/**
 * \brief Project a global st_tree to endpoint st_tree.
 * 
 * @param[in] global Global st_tree.
 * @param[in] role   Role to project against.
 *
 * \returns Projected st_tree.
 */
st_tree *scribble_project(st_tree *global, char *role);

#ifdef __cplusplus
}
#endif

#endif // SCRIBBLE__PROJECT__H__
