%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "st_node.h"
#include "parser_types.h"

extern int yylex();
extern FILE *yyin;
//extern st_tree *tree;

// Local variables.
int i;
st_node *node;
st_node *parent;
st_nodes *node_list;
st_node_msgsig_t msgsig;
st_tree_import_t import;

void yyerror(st_tree *tree, const char *s)
{
    fprintf(stderr, "Error: %s\n", s);
}
%}

    /* Keywords */
%token AND AS AT CHOICE CONTINUE FROM GLOBAL IMPORT LOCAL OR PAR PROTOCOL REC ROLE TO

    /* Symbols */
%token LBRACE RBRACE LPAREN RPAREN SEMICOLON COLON COMMA

    /* Variables */
%token <str> IDENT COMMENT

%type <str> role_name message_operator message_payload type_decl_from type_decl_as
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
    char *str;
    st_node *node;
    st_nodes *node_list;
    st_node_msgsig_t msgsig;
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
                                                                                    import.name = (char *)calloc(sizeof(char), strlen($2)+1);
                                                                                    strcpy(import.name, $2);
                                                                                    if ($3 != NULL) {
                                                                                        import.from = (char *)calloc(sizeof(char), strlen($3)+1);
                                                                                        strcpy(import.from, $3);
                                                                                    } else {
                                                                                        import.from = NULL;
                                                                                    }
                                                                                    if ($4 != NULL) {
                                                                                        import.as = (char *)calloc(sizeof(char), strlen($4)+1);
                                                                                        strcpy(import.as, $4);
                                                                                    } else {
                                                                                        import.as = NULL;
                                                                                    }
                                                                                    st_tree_add_import(tree, import);
                                                                                   }
                            ;

type_decl_from              :              { $$ = NULL; }
                            |   FROM IDENT { $$ = $2; }
                            ;

type_decl_as                :            { $$ = NULL; }
                            |   AS IDENT { $$ = $2; }
                            ;

/* --------------------------- Message Signature --------------------------- */

message_signature           :   message_operator message_payload {
                                                                    msgsig.op = (char *)calloc(sizeof(char), strlen($1)+1);
                                                                    strcpy(msgsig.op, $1);
                                                                    msgsig.payload = (char *)calloc(sizeof(char), strlen($2)+1);
                                                                    strcpy(msgsig.payload, $2);
                                                                    $$ = msgsig;
                                                                 }
                            ;

message_operator            :   IDENT { $$ = $1; }
                            ;

message_payload             :   LPAREN RPAREN       { $$ = ""; }
                            |   LPAREN IDENT RPAREN { $$ = $2; }
                            ;

/* --------------------------- Global Protocol --------------------------- */

global_prot_decls           :   GLOBAL PROTOCOL IDENT LPAREN role_decl_list RPAREN global_prot_body { st_tree_set_name(tree, $3); tree->info->global = 1; }
                            ;

role_decl_list              :
                            |   role_decl                         /* XXX: Shift-reduce */
                            |   role_decl_list COMMA role_decl  
                            ;

role_decl                   :   ROLE role_name  { st_tree_add_role(tree, $2); }
                            ;

role_name                   :   IDENT { $$ = $1; }
                            ;

/* --------------------------- Global Interaction Blocks and Sequences --------------------------- */

global_prot_body            :   global_interaction_blk { tree->root = $1; }
                            ;


global_interaction_blk      :   LBRACE global_interaction_seq RBRACE {
                                                                        parent = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);

                                                                        parent->nchild = $2->count;
                                                                        parent->children = (st_node **)calloc(sizeof(st_node *), parent->nchild);
                                                                        for (i=0; i<parent->nchild; ++i) {
                                                                            parent->children[i] = $2->nodes[$2->count-1-i];
                                                                        }

                                                                        $$ = parent;
                                                                     }
                            ;

global_interaction_seq      :                                               {   $$ = (st_nodes *)malloc(sizeof(st_nodes));  }
                            |   global_interaction global_interaction_seq   {
                                                                                $2->nodes = (st_node **)realloc($2->nodes, sizeof(st_node *) * ($2->count+1));
                                                                                $2->nodes[$2->count++] = $1;
                                                                                $$ = $2;
                                                                            }
                            ;

global_interaction          :   message   { $$ = $1; }
                            |   choice    { $$ = $1; } 
                            |   parallel  { $$ = $1; }
                            |   recursion { $$ = $1; }
                            |   continue  { $$ = $1; }
                            ;

/* --------------------------- Point-to-Point Message Transfer --------------------------- */

message                     :   message_signature FROM role_name TO role_name SEMICOLON {
                                                                                            node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_SENDRECV);
                                                                                            node->interaction->from = (char *)calloc(sizeof(char), (strlen($3)+1));
                                                                                            strcpy(node->interaction->from, $3);

                                                                                            node->interaction->nto = 1;
                                                                                            node->interaction->to = (char **)calloc(sizeof(char *), node->interaction->nto);
                                                                                            node->interaction->to[0] = (char *)calloc(sizeof(char), (strlen($5)+1));
                                                                                            strcpy(node->interaction->to[0], $5);

                                                                                            node->interaction->msgsig = $1;

                                                                                            $$ = node;
                                                                                        }
                            ;

/* --------------------------- Choice --------------------------- */

choice                      :   CHOICE AT role_name global_interaction_blk or_global_interaction_blk {
                                                                                                        node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CHOICE);

                                                                                                        $$ = node;
                                                                                                      }
                            ;

or_global_interaction_blk   :                                                       { node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT); $$ = node; }
                            |   OR global_interaction_blk or_global_interaction_blk { $$ = st_node_append($3, $2); }
                            ;

/* --------------------------- Parallel --------------------------- */

parallel                    :   PAR global_interaction_blk and_global_interaction_blk {
                                                                                        node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_PARALLEL);
                                                                                        node->nchild = 1 + $3->nchild;
                                                                                        node->children = (st_node **)calloc(sizeof(st_node *), node->nchild);
                                                                                        node->children[0] = $2;
                                                                                        for (i=0; i<$3->nchild; ++i) {
                                                                                            node->children[1+i] = $3->children[i];
                                                                                        }

                                                                                        $$ = node;
                                                                                      }
                            ;

and_global_interaction_blk  :                                                         { node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT); $$ = node; }
                            |   AND global_interaction_blk and_global_interaction_blk { $$ = st_node_append($3, $2); } 
                            ;

/* --------------------------- Recursion --------------------------- */

recursion                   :   REC IDENT global_interaction_blk {  
                                                                    node = (st_node *)malloc(sizeof(st_node));
                                                                    st_node_init(node, ST_NODE_RECUR);
                                                                    node->recur->label = (char *)calloc(sizeof(char), strlen($2)+1);
                                                                    strcpy(node->recur->label, $2);

                                                                    node->nchild = $3->nchild;
                                                                    node->children = (st_node **)calloc(sizeof(st_node *), node->nchild);
                                                                    memcpy(node->children, $3->children, sizeof(st_node *) * node->nchild);

                                                                    $$ = node;
                                                                  }
                            ;

continue                    :   CONTINUE IDENT SEMICOLON {
                                                                node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CONTINUE);
                                                                node->cont->label = (char *)calloc(sizeof(char), strlen($2)+1);
                                                                strcpy(node->cont->label, $2);

                                                                $$ = node;
                                                          }
                            ;

/* --------------------------- Local Protocols --------------------------- */

local_prot_decls            :   LOCAL PROTOCOL IDENT AT role_name LPAREN role_decl_list RPAREN local_prot_body  {
                                                                                                                    st_tree_set_name(tree, $3);
                                                                                                                    tree->info->global = 0;
                                                                                                                    tree->info->myrole = (char *)calloc(sizeof(char), strlen($5)+1);
                                                                                                                    strcpy(tree->info->myrole, $5);
                                                                                                                }
                            ;

/* --------------------------- Local Interaction Blocks and Sequences --------------------------- */

local_prot_body             :   local_interaction_blk  { tree->root = $1; }
                            ;

local_interaction_blk       :   LBRACE local_interaction_seq RBRACE {
                                                                        parent = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);

                                                                        parent->nchild = $2->count;
                                                                        parent->children = (st_node **)calloc(sizeof(st_node *), parent->nchild);
                                                                        for (i=0; i<parent->nchild; ++i) {
                                                                            parent->children[i] = $2->nodes[$2->count-1-i];
                                                                        }

                                                                        $$ = parent;
                                                                      }
                            ;

local_interaction_seq       :                                               {
                                                                                $$ = (st_nodes *)malloc(sizeof(st_nodes));
                                                                            }
                            |   local_interaction local_interaction_seq   {
                                                                                $2->nodes = (st_node **)realloc($2->nodes, sizeof(st_node *) * ($2->count+1));
                                                                                $2->nodes[$2->count++] = $1;
                                                                                $$ = $2;
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

send                        :   message_signature TO role_name SEMICOLON {
                                                                            node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_SEND);
                                                                            node->interaction->from = NULL;

                                                                            node->interaction->nto = 1;
                                                                            node->interaction->to = (char **)calloc(sizeof(char *), node->interaction->nto);
                                                                            node->interaction->to[0] = (char *)calloc(sizeof(char), (strlen($3)+1));
                                                                            strcpy(node->interaction->to[0], $3);

                                                                            node->interaction->msgsig = $1;

                                                                            $$ = node;
                                                                         }
                            ;

receive                     :   message_signature FROM role_name SEMICOLON {
                                                                            node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECV);
                                                                            node->interaction->from = (char *)calloc(sizeof(char), (strlen($3)+1));
                                                                            strcpy(node->interaction->from, $3);

                                                                            node->interaction->nto = 0;
                                                                            node->interaction->to = NULL;

                                                                            node->interaction->msgsig = $1;

                                                                            $$ = node;
                                                                           }
                            ;

/* --------------------------- Choice --------------------------- */

l_choice                    :   CHOICE AT role_name local_interaction_blk or_local_interaction_blk {
                                                                                                        node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CHOICE);

                                                                                                        $$ = node;
                                                                                                      }
                            ;

or_local_interaction_blk    :                                                     { node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT); $$ = node; }
                            |   OR local_interaction_blk or_local_interaction_blk { $$ = st_node_append($3, $2); }
                            ;

/* --------------------------- Parallel --------------------------- */

l_parallel                  :   PAR local_interaction_blk and_local_interaction_blk {
                                                                                        node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_PARALLEL);
                                                                                        node->nchild = 1 + $3->nchild;
                                                                                        node->children = (st_node **)calloc(sizeof(st_node *), node->nchild);
                                                                                        node->children[0] = $2;
                                                                                        for (i=0; i<$3->nchild; ++i) {
                                                                                            node->children[1+i] = $3->children[i];
                                                                                        }

                                                                                        $$ = node;
                                                                                    }
                            ;

and_local_interaction_blk   :                                                       { node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT); $$ = node; }
                            |   AND local_interaction_blk and_local_interaction_blk { $$ = st_node_append($3, $2); } 
                            ;

/* --------------------------- Recursion --------------------------- */

l_recursion                 :   REC IDENT local_interaction_blk {  
                                                                    node = (st_node *)malloc(sizeof(st_node));
                                                                    st_node_init(node, ST_NODE_RECUR);
                                                                    node->recur->label = (char *)calloc(sizeof(char), strlen($2)+1);
                                                                    strcpy(node->recur->label, $2);

                                                                    node->nchild = $3->nchild;
                                                                    node->children = (st_node **)calloc(sizeof(st_node *), node->nchild);
                                                                    memcpy(node->children, $3->children, sizeof(st_node *) * node->nchild);

                                                                    $$ = node;
                                                                }
                            ;

%%
