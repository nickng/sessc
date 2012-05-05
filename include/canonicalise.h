#ifndef CANONICALISE__H__
#define CANONICALISE__H__
/**
 * \file
 * This file contains the canonicalisation functions of (multiparty) session
 * type nodes (st_node).
 *
 */

#ifdef __cplusplus
extern "C" {
#endif


/**
 * \brief Canonicalise the node obtained by protocol-parsing or code-parsing.
 * 
 * @param[in,out] node Node to canonicalise.
 *
 * \returns Canonicalised node.
 */
st_node *st_node_canonicalise(st_node *node);


/**
 * \brief Refactor the node obtained by code-parsing.
 *
 * @param[in,out] node Node to refactor.
 *
 * \returns Refactored node.
 */
st_node *st_node_refactor(st_node *node);


#ifdef __cplusplus
}
#endif

#endif // CANONICALISE__H__ 
