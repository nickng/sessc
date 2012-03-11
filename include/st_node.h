#ifndef ST_NODE__H__
#define ST_NODE__H__
/**
 * \file
 * Thie file contains the tree representation of (multiparty) session
 * according to the Scribble language specification and provides functions
 * to build and manipulate session type trees.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif


#define ST_NODE_ROOT     0
#define ST_NODE_SENDRECV 1
#define ST_NODE_CHOICE   2 
#define ST_NODE_PARALLEL 3
#define ST_NODE_RECUR    4 
#define ST_NODE_CONTINUE 5

// Endpoint-specific
#define ST_NODE_SEND     6
#define ST_NODE_RECV     7


typedef struct {
  char *name;
  char *as;
  char *from;
} st_tree_import_t;


typedef struct {
  char *op;
  char *payload;
} st_node_msgsig_t;


typedef struct {
  st_node_msgsig_t msgsig;
  char **to;
  char *from;
} st_node_interaction;


typedef struct {
  char *label;
} st_node_recur;


typedef struct {
  char *label;
} st_node_continue;


typedef struct {
  char *at;
} st_node_choice;


struct __st_node {
  int type;

  union {
    st_node_interaction *interaction;
    st_node_choice *choice;
    st_node_recur *recur;
    st_node_continue *cont;
  };

  int nchild;
  struct __st_node **children;
};


/**
 * Session type tree metadata.
 */
typedef struct {
  char *name;
  int nrole;
  char **roles;

  int nimport;
  st_tree_import_t **imports;

  int global;
  char *myrole;
} st_info;


/**
 * Session type tree node representation.
 */
typedef struct __st_node st_node;


/**
 * Session type tree represenation.
 */
typedef struct {
  st_info *info;
  st_node *root;
} st_tree;


/**
 * \brief Initialise session type tree.
 *
 * @param[in,out] tree Tree to initialise.
 *
 * \returns Initialised tree.
 */
st_tree *st_tree_init(st_tree *tree);


/**
 * \brief Set name of protocol.
 *
 * @param[in,out] tree Session type tree of protocol.
 * @param[in]     name Name of protocol.
 */
st_tree *st_tree_set_name(st_tree *tree, const char *name);


/**
 * \brief Add a role to protocol.
 *
 * @param[in,out] tree Session type tree of protocol.
 * @param[in]     role Role name to add.
 *
 * \returns Updated session types tree.
 */
st_tree *st_tree_add_role(st_tree *tree, const char *role);


/**
 * \brief Add an import to protocol.
 *
 * @param[in,out] tree   Session type tree of protocol.
 * @param[in]     import Import details to add.
 *
 * \returns Updated session types tree.
 */
st_tree *st_tree_add_import(st_tree *tree, st_tree_import_t import);


/**
 * \brief Initialise session type node.
 *
 * @param[in,out] node Node to initialise.
 * @param[in]     type Type of node.
 *
 * \returns Initialised node.
 */
st_node *st_node_init(st_node *node, int type);


/**
 * \brief Append a node as a child to an existing node.
 *
 * @param[in,out] node  Parent node.
 * @param[in]     child Child node.
 *
 * \returns Updated parent node.
 */
st_node *st_node_append(st_node *node, st_node *child);


/**
 * \brief Print a st_tree with meta information.
 * 
 * @param[in] tree Tree to print.
 */
void st_tree_print(const st_tree *tree);


/**
 * \brief Print a single st_node and all its children recursively.
 *
 * @param[in] node   Node to print.
 * @param[in] indent Indentation (number of spaces).
 */
void st_node_printr(const st_node *node, int indent);


/**
 * \brief Print a single st_node.
 *
 * @param[in] node   Node to print.
 * @param[in] indent Indentation (number of spaces).
 */
void st_node_print(const st_node *node, int indent);


#ifdef __cplusplus
}
#endif

#endif // ST_NODE__H__
