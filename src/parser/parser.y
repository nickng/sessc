%{
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "st_node.h"
#include "parser_types.h"

#ifdef __DEBUG__
#define YYDEBUG 1
#endif 

extern int yylex();
extern FILE *yyin;

// Local variables.
static st_tree_import_t import;
static role_params_t role_params;

void yyerror(st_tree *tree, const char *s)
{
    fprintf(stderr, "Error: %s\n", s);
}
%}

    /* Keywords */
%token AND AS AT CHOICE CONTINUE FROM GLOBAL IMPORT LOCAL OR PAR PROTOCOL REC ROLE TO IF

    /* Symbols */
%token LBRACE RBRACE LPAREN RPAREN SEMICOLON COLON COMMA LSQUARE RSQUARE PLUS NUMRANGE

    /* Variables */
%token <str> IDENT COMMENT
%token <num> DIGITS

%type <str> role_name message_operator message_payload type_decl_from type_decl_as
%type <role_param> role_param role_param_binder role_param_range role_param_add role_param_var role_param_set role_indices
%type <msg_cond> message_condition
%type <node> message choice parallel recursion continue
%type <node> send receive l_choice l_parallel l_recursion
%type <node> global_interaction global_interaction_blk and_global_interaction_blk or_global_interaction_blk
%type <node> local_interaction  local_interaction_blk  and_local_interaction_blk  or_local_interaction_blk
%type <node_list> global_interaction_seq local_interaction_seq
%type <msgsig> message_signature

%code requires {
#include "st_node.h"
#include "parser_types.h"
}

%parse-param {st_tree *tree}

%union {
    unsigned long num;
    char *str;
    st_node *node;
    st_nodes *node_list;
    st_node_msgsig_t msgsig;
    role_param_t *role_param;
    msg_cond_t *msg_cond;
}

%start scribble_protocol

%%

/* --------------------------- Scribble Protocol Structure --------------------------- */

scribble_protocol           :   type_decls global_prot_decls { }
                            |   type_decls local_prot_decls  { }
                            ;

/* --------------------------- Message Type Declarations --------------------------- */

type_decls                  : 
                            |   type_decls type_decl
                            ;

type_decl                   :   IMPORT IDENT type_decl_from type_decl_as SEMICOLON {
                                                                                        import.name = strdup($2);
                                                                                        import.from = ($3 == NULL) ? NULL : strdup($3);
                                                                                        import.as   = ($4 == NULL) ? NULL : strdup($4);
                                                                                        st_tree_add_import(tree, import);
                                                                                   }
                            ;

type_decl_from              :              { $$ = NULL; }
                            |   FROM IDENT { $$ = $2;   }
                            ;

type_decl_as                :            { $$ = NULL; }
                            |   AS IDENT { $$ = $2;   }
                            ;

/* --------------------------- Message Signature --------------------------- */

message_signature           :   message_operator message_payload {
                                                                    $$.op = ($1 == NULL) ? NULL : strdup($1);
                                                                    assert($2 != NULL /* Payload is never NULL */);
                                                                    $$.payload = strdup($2);
                                                                 }
                            ;

message_operator            :         { $$ = NULL; }
                            |   IDENT { $$ = $1;   }
                            ;

message_payload             :   LPAREN RPAREN       { $$ = ""; }
                            |   LPAREN IDENT RPAREN { $$ = $2; }
                            ;

/* --------------------------- Global Protocol --------------------------- */

global_prot_decls           :   GLOBAL PROTOCOL IDENT LPAREN role_decl_list RPAREN global_prot_body { st_tree_set_name(tree, $3); tree->info->type = ST_TYPE_GLOBAL; }
                            ;

role_decl_list              :
                            |   role_decl                         /* XXX: Shift-reduce */
                            |   role_decl_list COMMA role_decl  
                            ;

role_decl                   :   ROLE role_name role_param_range { st_tree_add_role_param(tree, $2, $3->params[0], $3->params[$3->count-1]); }
                            |   ROLE role_name                  { st_tree_add_role(tree, $2); }
                            ;

role_name                   :   IDENT { $$ = $1; }
                            ;

role_param_binder           :   role_param_range { $$ = $1;   }
                            |   role_param_set   { $$ = $1;   }
                            |                    { $$ = NULL; }
                            ;

role_param                  :   role_param_range { $$ = $1;   }
                            |   role_param_set   { $$ = $1;   }
                            |   role_param_add   { $$ = $1;   }
                            |   role_param_var   { $$ = $1;   }
                            |                    { $$ = NULL; }
                            ;

/* --------------------------- Parametrised roles --------------------------- */

role_param_range            : LSQUARE IDENT COLON DIGITS NUMRANGE DIGITS RSQUARE {
                                                                                    $$ = (role_param_t *)malloc(sizeof(role_param_t));
                                                                                    $$->count = 0;
                                                                                    long i;
                                                                                    $$->params = (long *)calloc(sizeof(long), ($6-$4+1));
                                                                                    for (i=$4; i<=$6; ++i) {
                                                                                        $$->bindvar = strdup($2);
                                                                                        $$->params[$$->count++] = i;
                                                                                    }

                                                                                    // Register role-parameter bindings 
                                                                                    role_params.params_ptrs = (role_param_t **)malloc(sizeof(role_param_t *) * (role_params.count + 1));
                                                                                    role_params.params_ptrs[role_params.count++] = $$;
                                                                                 }
                            ;

role_param_var              : LSQUARE IDENT COLON IDENT RSQUARE {
                                                                    $$ = (role_param_t *)malloc(sizeof(role_param_t));
                                                                    $$->count = 0;
                                                                    int i, j;
                                                                    for (i=0; i<role_params.count; ++i) {
                                                                        if (0 == strcmp(role_params.params_ptrs[i]->bindvar, $4)) {
                                                                            $$->params = (long *)calloc(sizeof(long), role_params.params_ptrs[i]->count);
                                                                            for (j=0; j<role_params.params_ptrs[i]->count; ++j) {
                                                                                $$->bindvar = strdup($2);
                                                                                $$->params[$$->count++] = role_params.params_ptrs[i]->params[j];
                                                                            }

                                                                            // Register role-parameter bindings
                                                                            role_params.params_ptrs = (role_param_t **)malloc(sizeof(role_param_t *) * (role_params.count + 1));
                                                                            role_params.params_ptrs[role_params.count++] = $$;
                                                                            break;
                                                                        }
                                                                    }
                                                                }
                            ;

role_param_add              : LSQUARE IDENT COLON IDENT PLUS DIGITS RSQUARE {
                                                                                $$ = (role_param_t *)malloc(sizeof(role_param_t));
                                                                                $$->count = 0;
                                                                                int i, j;
                                                                                for (i=0; i<role_params.count; ++i) {
                                                                                    if (0 == strcmp(role_params.params_ptrs[i]->bindvar, $4)) {
                                                                                        $$->params = (long *)calloc(sizeof(long), role_params.params_ptrs[i]->count);
                                                                                        for (j=0; j<role_params.params_ptrs[i]->count; ++j) {
                                                                                            $$->bindvar = strdup($2);
                                                                                            $$->params[$$->count++] = role_params.params_ptrs[i]->params[j] + $6;
                                                                                        }

                                                                                        // Register role-parameter bindings
                                                                                        role_params.params_ptrs = (role_param_t **)malloc(sizeof(role_param_t *) * (role_params.count + 1));
                                                                                        role_params.params_ptrs[role_params.count++] = $$;
                                                                                        break;
                                                                                    }
                                                                                }
                                                                            }
                            | LSQUARE IDENT COLON DIGITS PLUS IDENT RSQUARE {
                                                                                $$ = (role_param_t *)malloc(sizeof(role_param_t));
                                                                                $$->count = 0;
                                                                                int i, j;
                                                                                for (i=0; i<role_params.count; ++i) {
                                                                                    if (0 == strcmp(role_params.params_ptrs[i]->bindvar, $6)) {
                                                                                        $$->params = (long *)calloc(sizeof(long), role_params.params_ptrs[i]->count);
                                                                                        for (j=0; j<role_params.params_ptrs[i]->count; ++j) {
                                                                                            $$->bindvar = strdup($2);
                                                                                            $$->params[$$->count++] = role_params.params_ptrs[i]->params[j] + $4;
                                                                                        }

                                                                                        // Register role-parameter bindings
                                                                                        role_params.params_ptrs = (role_param_t **)malloc(sizeof(role_param_t *) * (role_params.count + 1));
                                                                                        role_params.params_ptrs[role_params.count++] = $$;
                                                                                        break;
                                                                                    }
                                                                                }
                                                                            }
                            ;

role_indices                : DIGITS                    {
                                                            $$ = (role_param_t *)malloc(sizeof(role_param_t));
                                                            $$->count = 1;
                                                            $$->params = (long *)malloc(sizeof(long));
                                                            $$->params[0] = $1;
                                                        }
                            | role_indices COMMA DIGITS {
                                                            $$ = $1;
                                                            $$->params = (long *)realloc($$->params, sizeof(long) * ($$->count + 1));
                                                            $$->params[$$->count++] = $3;
                                                        }
                            ;

role_param_set              : LSQUARE IDENT COLON role_indices RSQUARE {
                                                                            $$ = $4; // Copy indices information over
                                                                            $$->bindvar = strdup($2);

                                                                            // Register role-parameter bindings
                                                                            role_params.params_ptrs = (role_param_t **)malloc(sizeof(role_param_t *) * (role_params.count + 1));
                                                                            role_params.params_ptrs[role_params.count++] = $$;
                                                                       }
                            ;

/* --------------------------- Global Interaction Blocks and Sequences --------------------------- */

global_prot_body            :   global_interaction_blk { tree->root = $1; }
                            ;


global_interaction_blk      :   LBRACE global_interaction_seq RBRACE {
                                                                        $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);

                                                                        $$->nchild = $2->count;
                                                                        $$->children = (st_node **)calloc(sizeof(st_node *), $$->nchild);
                                                                        int i;
                                                                        for (i=0; i<$$->nchild; ++i) {
                                                                            $$->children[i] = $2->nodes[$2->count-1-i];
                                                                        }
                                                                     }
                            ;

global_interaction_seq      :                                               {   $$ = memset((st_nodes *)malloc(sizeof(st_nodes)), 0, sizeof(st_nodes)); }
                            |   global_interaction global_interaction_seq   {
                                                                                $$ = $2;
                                                                                $$->nodes = (st_node **)realloc($$->nodes, sizeof(st_node *) * ($$->count+1));
                                                                                $$->nodes[$$->count++] = $1;
                                                                            }
                            ;

global_interaction          :   message   { $$ = $1; }
                            |   choice    { $$ = $1; } 
                            |   parallel  { $$ = $1; }
                            |   recursion { $$ = $1; }
                            |   continue  { $$ = $1; }
                            ;

/* --------------------------- Point-to-Point Message Transfer --------------------------- */

message                     :   message_signature FROM role_name role_param_binder TO role_name role_param SEMICOLON {
                                                                                                                        $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_SENDRECV);
                                                                                                                        if (NULL == $4) {
                                                                                                                            $$->interaction->from_type = ST_ROLE_NORMAL;
                                                                                                                            $$->interaction->from = strdup($3);
                                                                                                                        } else { // Parametrised
                                                                                                                            $$->interaction->from_type = ST_ROLE_PARAMETRISED;
                                                                                                                            $$->interaction->p_from = (parametrised_role_t *)malloc(sizeof(parametrised_role_t));
                                                                                                                            $$->interaction->p_from->name = strdup($3);
                                                                                                                            $$->interaction->p_from->bindvar = strdup($4->bindvar);
                                                                                                                            $$->interaction->p_from->idxcount = $4->count;
                                                                                                                            $$->interaction->p_from->indices = (long *)calloc(sizeof(long), $$->interaction->p_from->idxcount);
                                                                                                                            memcpy($$->interaction->p_from->indices, $4->params, sizeof(long) * $$->interaction->p_from->idxcount);
                                                                                                                        }

                                                                                                                        // At the moment, we only accept a single to-role
                                                                                                                        $$->interaction->nto = 1;
                                                                                                                        if (NULL == $7) {
                                                                                                                            $$->interaction->to_type = ST_ROLE_NORMAL;
                                                                                                                            $$->interaction->to = (char **)calloc(sizeof(char *), $$->interaction->nto);
                                                                                                                            $$->interaction->to[0] = strdup($6);
                                                                                                                        } else {
                                                                                                                            $$->interaction->to_type = ST_ROLE_PARAMETRISED;
                                                                                                                            $$->interaction->p_to = (parametrised_role_t **)calloc(sizeof(parametrised_role_t *), $$->interaction->nto);
                                                                                                                            $$->interaction->p_to[0] = (parametrised_role_t *)malloc(sizeof(parametrised_role_t));
                                                                                                                            $$->interaction->p_to[0]->name = strdup($6);
                                                                                                                            $$->interaction->p_to[0]->bindvar = strdup($7->bindvar);
                                                                                                                            $$->interaction->p_to[0]->idxcount = $7->count;
                                                                                                                            $$->interaction->p_to[0]->indices = (long *)calloc(sizeof(long), $$->interaction->p_to[0]->idxcount);
                                                                                                                            memcpy($$->interaction->p_to[0]->indices, $7->params, sizeof(long) * $$->interaction->p_to[0]->idxcount);
                                                                                                                        }

                                                                                                                        $$->interaction->msgsig = $1;

                                                                                                                        // Release role-parameter bindings
                                                                                                                        role_params.count = 0;
                                                                                                                        free(role_params.params_ptrs);
                                                                                                                     }
                            ;

/* --------------------------- Choice --------------------------- */

choice                      :   CHOICE AT role_name global_interaction_blk or_global_interaction_blk {
                                                                                                        $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CHOICE);
                                                                                                        $$->choice->at = strdup($3);                                                                                                       

                                                                                                        $$->nchild = $5->nchild + 1;
                                                                                                        $$->children = (st_node **)calloc(sizeof(st_node *), $$->nchild);
                                                                                                        $$->children[0] = $4; // First or-block
                                                                                                        int i;
                                                                                                        for (i=0; i<$5->nchild; ++i) {
                                                                                                            $$->children[1+i] = $5->children[i];
                                                                                                        }
                                                                                                     }
                            ;

or_global_interaction_blk   :                                                       {  $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);  }
                            |   OR global_interaction_blk or_global_interaction_blk {  $$ = st_node_append($3, $2);  }
                            ;

/* --------------------------- Parallel --------------------------- */

parallel                    :   PAR global_interaction_blk and_global_interaction_blk {
                                                                                        $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_PARALLEL);

                                                                                        $$->nchild = 1 + $3->nchild;
                                                                                        $$->children = (st_node **)calloc(sizeof(st_node *), $$->nchild);
                                                                                        $$->children[0] = $2;
                                                                                        int i;
                                                                                        for (i=0; i<$3->nchild; ++i) {
                                                                                            $$->children[1+i] = $3->children[i];
                                                                                        }
                                                                                      }
                            ;

and_global_interaction_blk  :                                                         {  $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);  }
                            |   AND global_interaction_blk and_global_interaction_blk {  $$ = st_node_append($3, $2);  } 
                            ;

/* --------------------------- Recursion --------------------------- */

recursion                   :   REC IDENT global_interaction_blk {  
                                                                    $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECUR);
                                                                    $$->recur->label = strdup($2);

                                                                    $$->nchild = $3->nchild;
                                                                    $$->children = (st_node **)calloc(sizeof(st_node *), $$->nchild);
                                                                    memcpy($$->children, $3->children, sizeof(st_node *) * $$->nchild);
                                                                 } 
                            ;

continue                    :   CONTINUE IDENT SEMICOLON {
                                                            $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CONTINUE);
                                                            $$->cont->label = strdup($2);
                                                         }
                            ;

/* --------------------------- Local Protocols --------------------------- */

local_prot_decls            :   LOCAL PROTOCOL IDENT AT role_name LPAREN role_decl_list RPAREN local_prot_body  { 
                                                                                                                  st_tree_set_name(tree, $3);
                                                                                                                  tree->info->type = ST_TYPE_LOCAL;
                                                                                                                  tree->info->myrole = strdup($5);
                                                                                                                }
                            |   LOCAL PROTOCOL IDENT AT role_name LSQUARE DIGITS NUMRANGE DIGITS RSQUARE LPAREN role_decl_list RPAREN local_prot_body  { 
                                                                                                                                   st_tree_set_name(tree, $3);
                                                                                                                                   tree->info->type = ST_TYPE_PARAMETRISED;
                                                                                                                                   assert($7<1000 && $9<1000);
                                                                                                                                   tree->info->myrole = (char *)calloc(sizeof(char), strlen($5)+4+6+1);
                                                                                                                                   sprintf(tree->info->myrole, "%s[%lu..%lu]", $5, $7, $9);
                                                                                                                                 }
                            ;

/* --------------------------- Local Interaction Blocks and Sequences --------------------------- */

local_prot_body             :   local_interaction_blk  { tree->root = $1; }
                            ;

local_interaction_blk       :   LBRACE local_interaction_seq RBRACE {
                                                                      $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);

                                                                      $$->nchild = $2->count;
                                                                      $$->children = (st_node **)calloc(sizeof(st_node *), $$->nchild);
                                                                      int i;
                                                                      for (i=0; i<$$->nchild; ++i) {
                                                                        $$->children[i] = $2->nodes[$2->count-1-i];
                                                                      }
                                                                    }
                            ;

local_interaction_seq       :                                             {
                                                                            $$ = memset((st_nodes *)malloc(sizeof(st_nodes)), 0, sizeof(st_nodes));
                                                                          }
                            |   local_interaction local_interaction_seq   {
                                                                            $$ = $2;
                                                                            $$->nodes = (st_node **)realloc($$->nodes, sizeof(st_node *) * ($$->count+1));
                                                                            $$->nodes[$$->count++] = $1;
                                                                          }
                            ;

local_interaction           :   send        { $$ = $1; }
                            |   receive     { $$ = $1; }
                            |   l_parallel  { $$ = $1; }
                            |   l_choice    { $$ = $1; }
                            |   l_recursion { $$ = $1; }
                            |   continue    { $$ = $1; }
                            ;

/* --------------------------- Point-to-Point Message Transfer --------------------------- */

message_condition           :                                 { $$ = NULL; }
                            |  IF role_name role_param_binder {
                                                                assert($3!=NULL /* message_condition cannot be NULL */);
                                                                $$ = (msg_cond_t *)malloc(sizeof(msg_cond_t));
                                                                $$->name = strdup($2);
                                                                $$->bindvar = strdup($3->bindvar);
                                                                $$->idxcount = $3->count;
                                                                $$->indices = (long *)calloc(sizeof(long), $$->idxcount);
                                                                memcpy($$->indices, $3->params, sizeof(long) * $$->idxcount);
                                                              }

send                        :   message_signature TO role_name role_param_binder message_condition SEMICOLON {
                                                                                                                $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_SEND);

                                                                                                                // From
                                                                                                                $$->interaction->from = NULL;

                                                                                                                // To
                                                                                                                $$->interaction->nto = 1;
                                                                                                                if (NULL == $4) {
                                                                                                                    $$->interaction->to_type = ST_ROLE_NORMAL;
                                                                                                                    $$->interaction->to = (char **)calloc(sizeof(char *), $$->interaction->nto);
                                                                                                                    $$->interaction->to[0] = strdup($3);
                                                                                                                } else {
                                                                                                                    $$->interaction->to_type = ST_ROLE_PARAMETRISED;
                                                                                                                    $$->interaction->p_to = (parametrised_role_t **)calloc(sizeof(parametrised_role_t *), $$->interaction->nto);
                                                                                                                    $$->interaction->p_to[0] = (parametrised_role_t *)malloc(sizeof(parametrised_role_t));
                                                                                                                    $$->interaction->p_to[0]->name = strdup($3);
                                                                                                                    $$->interaction->p_to[0]->bindvar = strdup($4->bindvar);
                                                                                                                    $$->interaction->p_to[0]->idxcount = $4->count;
                                                                                                                    $$->interaction->p_to[0]->indices = (long *)calloc(sizeof(long), $$->interaction->p_to[0]->idxcount);
                                                                                                                    memcpy($$->interaction->p_to[0]->indices, $4->params, sizeof(long) * $$->interaction->p_to[0]->idxcount);
                                                                                                                }

                                                                                                                // Message signature
                                                                                                                $$->interaction->msgsig = $1;

                                                                                                                // Message condition
                                                                                                                $$->interaction->msg_cond = $5;
                                                                                                             }
                            ;

receive                     :   message_signature FROM role_name role_param_binder message_condition SEMICOLON {
                                                                                                                 $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECV);

                                                                                                                 // From
                                                                                                                 if (NULL == $4) {
                                                                                                                     $$->interaction->from_type = ST_ROLE_NORMAL;
                                                                                                                     $$->interaction->from = strdup($3);
                                                                                                                 } else { // Parametrised
                                                                                                                     $$->interaction->from_type = ST_ROLE_PARAMETRISED;
                                                                                                                     $$->interaction->p_from = (parametrised_role_t *)malloc(sizeof(parametrised_role_t));
                                                                                                                     $$->interaction->p_from->name = strdup($3);
                                                                                                                     $$->interaction->p_from->bindvar = strdup($4->bindvar);
                                                                                                                     $$->interaction->p_from->idxcount = $4->count;
                                                                                                                     $$->interaction->p_from->indices = (long *)calloc(sizeof(long), $$->interaction->p_from->idxcount);
                                                                                                                     memcpy($$->interaction->p_from->indices, $4->params, sizeof(long) * $$->interaction->p_from->idxcount);
                                                                                                                 }

                                                                                                                 // To
                                                                                                                 $$->interaction->nto = 0;
                                                                                                                 $$->interaction->to = NULL;

                                                                                                                 // Message signature
                                                                                                                 $$->interaction->msgsig = $1;

                                                                                                                 // Message condition
                                                                                                                 $$->interaction->msg_cond = $5;
                                                                                                               }
                            ;

/* --------------------------- Choice --------------------------- */

l_choice                    :   CHOICE AT role_name local_interaction_blk or_local_interaction_blk {  $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CHOICE);  }
                            ;

or_local_interaction_blk    :                                                     {  $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);  }
                            |   OR local_interaction_blk or_local_interaction_blk {  $$ = st_node_append($3, $2);  }
                            ;

/* --------------------------- Parallel --------------------------- */

l_parallel                  :   PAR local_interaction_blk and_local_interaction_blk {
                                                                                        $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_PARALLEL);
                                                                                        $$->nchild = 1 + $3->nchild;
                                                                                        $$->children = (st_node **)calloc(sizeof(st_node *), $$->nchild);
                                                                                        $$->children[0] = $2;
                                                                                        int i;
                                                                                        for (i=0; i<$3->nchild; ++i) {
                                                                                            $$->children[1+i] = $3->children[i];
                                                                                        }
                                                                                    }
                            ;

and_local_interaction_blk   :                                                       {  $$= st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT); }
                            |   AND local_interaction_blk and_local_interaction_blk {  $$ = st_node_append($3, $2);  } 
                            ;

/* --------------------------- Recursion --------------------------- */

l_recursion                 :   REC IDENT local_interaction_blk {  
                                                                    $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECUR);
                                                                    $$->recur->label = strdup($2);

                                                                    $$->nchild = $3->nchild;
                                                                    $$->children = (st_node **)calloc(sizeof(st_node *), $$->nchild);
                                                                    memcpy($$->children, $3->children, sizeof(st_node *) * $$->nchild);
                                                                }
                            ;

%%
