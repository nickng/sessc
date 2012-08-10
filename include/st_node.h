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
#define ST_NODE_FOR      6

// Endpoint-specific
#define ST_NODE_SEND     7
#define ST_NODE_RECV     8

#define ST_TYPE_LOCAL        0
#define ST_TYPE_GLOBAL       1
#define ST_TYPE_PARAMETRISED 2

// Expression related
#define ST_EXPR_TYPE_CONST 1
#define ST_EXPR_TYPE_VAR   2
#define ST_EXPR_TYPE_RANGE 3
#define ST_EXPR_TYPE_PLUS  4
#define ST_EXPR_TYPE_MINUS 5
#define ST_EXPR_TYPE_MULTIPLY 6
#define ST_EXPR_TYPE_DIVIDE 7
#define ST_EXPR_TYPE_SHL 8
#define ST_EXPR_TYPE_SHR 9
#define ST_EXPR_TYPE_TUPLE 10


// To represent mathematical expressions
struct __st_bin_expr_t {
  struct __st_expr_t *left;
  struct __st_expr_t *right;
};

struct __st_expr_t {
  int type;
  union {
    struct __st_bin_expr_t *binexpr;
    int constant;
    char *variable;
  };
};

typedef struct __st_bin_expr_t st_bin_expr_t;
typedef struct __st_expr_t st_expr_t;


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
  char *name;
  st_expr_t *param;
} st_role_t;

typedef st_role_t msg_cond_t;

typedef struct {
  st_node_msgsig_t msgsig;

  int nto;
  st_role_t **to;

  st_role_t *from;

  msg_cond_t *msg_cond;
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


typedef struct {
  st_expr_t *range;
} st_node_for;


struct __st_node {
  int type;

  union {
    st_node_interaction *interaction;
    st_node_choice *choice;
    st_node_recur *recur;
    st_node_continue *cont;
    st_node_for *forloop;
  };

  int nchild;
  struct __st_node **children;
  int marked;
};


/**
 * Session type tree metadata.
 */
typedef struct {
  char *name;
  int nrole;
  st_role_t **roles;

  int nimport;
  st_tree_import_t **imports;

  int type; // Session Tree type: global/local/parametrised-local
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
 * \brief Cleanup session type tree.
 *
 * @param[in,out] tree Tree to clean up.
 */
void st_tree_free(st_tree *tree);


/**
 * \brief Cleanup session type node (recursive).
 *
 * @param[in,out] node Node to clean up.
 */
void st_node_free(st_node *node);


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
 * \brief Add a role (with parameter) to protocol.
 *
 * @param[in,out] tree  Session type tree of protocol.
 * @param[in]     role  Role name to add.
 * @param[in]     param Expression of parametrised role
 *
 * \returns Updated session types tree.
 */
st_tree *st_tree_add_role_param(st_tree *tree, const char *role, st_expr_t *param);


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
void st_node_print_r(const st_node *node, int indent);


/**
 * \brief Print a single st_node.
 *
 * @param[in] node   Node to print.
 * @param[in] indent Indentation (number of spaces).
 */
void st_node_print(const st_node *node, int indent);


/**
 * \brief Pretty print an expression.
 *
 * @param[in] e   Expression to print.
 */
void st_expr_print(st_expr_t *e);


/**
 * \brief Resets the 'marked' flag on a st_node for debugging purposes.
 *
 * @param[in] node Node to reset flag.
 */
void st_node_reset_markedflag(st_node *node);


/**
 * \brief Compare two st_nodes recursively.
 * Errors will be marked on node->marked.
 *
 * @param[in,out] node  Node (scribble) to compare.
 * @param[in,out] other Node (source code) to compare.
 *
 * \returns 1 if identical, 0 otherwise.
 */
int st_node_compare_r(st_node *node, st_node *other);


/**
 * \brief Compare two message signature.
 *
 * @param[in] msgsig Message signature to compare.
 * @param[in] other  Message signature to compare.
 *
 * \returns 1 if identical, 0 otherwise.
 */
int st_node_compare_msgsig(const st_node_msgsig_t msgsig, const st_node_msgsig_t other);


/**
 * \brief Compare two st_nodes.
 *
 * @param[in] node  Node (scribble) to compare.
 * @param[in] other Node (source code) to compare.
 *
 * \returns 1 if identical, 0 otherwise.
 */
int st_node_compare(st_node *node, st_node *other);


st_expr_t *st_expr_constant(int val);


st_expr_t *st_expr_variable(char *var);


st_expr_t *st_expr_binexpr(st_expr_t *left, int type, st_expr_t *right);



#ifdef __cplusplus
}
#endif

#endif // ST_NODE__H__
