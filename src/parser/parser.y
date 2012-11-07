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

void yyerror(st_tree *tree, const char *s)
{
    fprintf(stderr, "Error: %s\n", s);
}

static symbol_table_t symbol_table;
%}

    /* Keywords */
%token AND AS AT BY CATCH CHOICE CONTINUE CREATE DO ENTER FROM GLOBAL IMPORT INSTANTIATES INTERRUPTIBLE LOCAL OR PAR PROTOCOL REC ROLE SPAWNS THROW TO WITH IF CONST RANGE FOREACH

    /* Symbols */
%token LPAREN RPAREN LBRACE RBRACE LSQUARE RSQUARE LANGLE RANGLE COMMA COLON SEMICOLON EQUAL PLUS MINUS MULTIPLY DIVIDE MODULO SHL SHR NUMRANGE TUPLE

    /* Variables */
%token <str> IDENT COMMENT
%token <num> DIGITS

%type <str> role_name message_operator message_payload type_decl_from type_decl_as
%type <expr> simple_expr role_param range_expr expr_expr tuple_expr
%type <msg_cond> message_condition
%type <node> message choice parallel recursion continue foreach
%type <node> send receive l_choice l_parallel l_recursion l_foreach
%type <node> global_interaction global_interaction_blk and_global_interaction_blk or_global_interaction_blk
%type <node> local_interaction  local_interaction_blk  and_local_interaction_blk  or_local_interaction_blk
%type <node_list> global_interaction_seq local_interaction_seq
%type <msgsig> message_signature

%left PLUS MINUS
%left MULTIPLY DIVIDE MODULO

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
    st_expr_t *expr;
    msg_cond_t *msg_cond;
}

%start scribble_protocol

%%

/* --------------------------- Scribble Protocol Structure --------------------------- */

scribble_protocol           :   const_decls type_decls global_prot_decls { }
                            |   const_decls type_decls local_prot_decls  { }
                            ;

/* --------------------------- Const declarations --------------------------- */

const_decls                 :
                            |   const_decls const_decl
                            ;

const_decl                  :   CONST IDENT EQUAL DIGITS {
                                                           symbol_table.count++;
                                                           symbol_table.symbols = (symbol_t **)realloc(symbol_table.symbols, sizeof(symbol_t *)*symbol_table.count);
                                                           symbol_table.symbols[symbol_table.count-1] = (symbol_t *)malloc(sizeof(symbol_t));
                                                           symbol_table.symbols[symbol_table.count-1]->name = strdup($2);
                                                           symbol_table.symbols[symbol_table.count-1]->expr = st_expr_constant($4);
                                                         }
                            |   RANGE IDENT EQUAL DIGITS NUMRANGE simple_expr { // Register range in symbol table
                                                                                symbol_table.count++;
                                                                                symbol_table.symbols = (symbol_t **)realloc(symbol_table.symbols, sizeof(symbol_t *)*symbol_table.count);
                                                                                symbol_table.symbols[symbol_table.count-1] = (symbol_t *)malloc(sizeof(symbol_t));
                                                                                symbol_table.symbols[symbol_table.count-1]->name = strdup($2);
                                                                                symbol_table.symbols[symbol_table.count-1]->expr = st_expr_binexpr(st_expr_constant($4), ST_EXPR_TYPE_RANGE, $6);
                                                                              }
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

role_decl                   :   ROLE role_name LSQUARE tuple_expr RSQUARE { st_tree_add_role_param(tree, $2, $4); }
                            |   ROLE role_name LSQUARE range_expr RSQUARE { st_tree_add_role_param(tree, $2, $4); }
                            |   ROLE role_name                            { st_tree_add_role(tree, $2);           }
                            ;

role_name                   :   IDENT { $$ = $1; }
                            ;

/* --------------------------- Parametrised roles --------------------------- */

simple_expr                 :   IDENT                            {
                                                                   int i;
                                                                   $$ = st_expr_variable($1);
                                                                   for (i=0; i<symbol_table.count; ++i) {
                                                                     if (0 == strcmp(symbol_table.symbols[i]->name, $1)) {
                                                                        $$ = symbol_table.symbols[i]->expr;
                                                                     }
                                                                   }
                                                                 }
                            |   DIGITS                           { $$ = st_expr_constant($1);                           }
                            |   LPAREN simple_expr RPAREN        { $$ = $2;                                             }
                            |   simple_expr PLUS     simple_expr { $$ = st_expr_binexpr($1, ST_EXPR_TYPE_PLUS, $3);     }
                            |   simple_expr MINUS    simple_expr { $$ = st_expr_binexpr($1, ST_EXPR_TYPE_MINUS, $3);    }
                            |   simple_expr MULTIPLY simple_expr { $$ = st_expr_binexpr($1, ST_EXPR_TYPE_MULTIPLY, $3); }
                            |   simple_expr DIVIDE   simple_expr { $$ = st_expr_binexpr($1, ST_EXPR_TYPE_DIVIDE, $3);   }
                            |   simple_expr MODULO   simple_expr { $$ = st_expr_binexpr($1, ST_EXPR_TYPE_MODULO, $3);   }
                            |   simple_expr SHL      simple_expr { $$ = st_expr_binexpr($1, ST_EXPR_TYPE_SHL, $3);      }
                            |   simple_expr SHR      simple_expr { $$ = st_expr_binexpr($1, ST_EXPR_TYPE_SHR, $3);      }
                            ;

range_expr                  :   IDENT COLON DIGITS NUMRANGE simple_expr {
                                                                          $$ = st_expr_binexpr(st_expr_constant($3), ST_EXPR_TYPE_RANGE, $5);
                                                                          /* Register $1 to global table */
                                                                        }
                            |   simple_expr NUMRANGE simple_expr        {
                                                                          $$ = st_expr_binexpr($1, ST_EXPR_TYPE_RANGE, $3);
                                                                          /* Register $1 to global table */
                                                                        }
                            ;

expr_expr                   :   IDENT COLON simple_expr {
                                                          $$ = st_expr_binexpr(st_expr_variable($1), ST_EXPR_TYPE_BIND, $3);
                                                          // TODO Register $1 to global table
                                                        }
                            |   simple_expr             {
                                                          $$ = $1;
                                                        }

tuple_expr                  :   range_expr TUPLE range_expr { $$ = st_expr_binexpr($1, ST_EXPR_TYPE_TUPLE, $3); }
                            |   range_expr TUPLE expr_expr  { $$ = st_expr_binexpr($1, ST_EXPR_TYPE_TUPLE, $3); }
                            |   expr_expr  TUPLE range_expr { $$ = st_expr_binexpr($1, ST_EXPR_TYPE_TUPLE, $3); }
                            |   expr_expr  TUPLE expr_expr  { $$ = st_expr_binexpr($1, ST_EXPR_TYPE_TUPLE, $3); }
                            ;


role_param                  :                              { $$ = NULL; }
                            |   LSQUARE expr_expr  RSQUARE { $$ = $2;   }
                            |   LSQUARE range_expr RSQUARE { $$ = $2;   }
                            |   LSQUARE tuple_expr RSQUARE { $$ = $2;   }
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
                            |   foreach   { $$ = $1; }
                            ;

/* --------------------------- Point-to-Point Message Transfer --------------------------- */

message                     :   message_signature FROM role_name role_param TO role_name role_param message_condition SEMICOLON {
                                                                                                                                  $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_SENDRECV);
                                                                                                                                  $$->interaction->from = (st_role_t *)malloc(sizeof(st_role_t));
                                                                                                                                  $$->interaction->from->name = strdup($3);
                                                                                                                                  $$->interaction->from->param = $4;

                                                                                                                                  // At the moment, we only accept a single to-role
                                                                                                                                  $$->interaction->nto = 1;
                                                                                                                                  $$->interaction->to = (st_role_t **)calloc(sizeof(st_role_t), $$->interaction->nto);
                                                                                                                                  $$->interaction->to[0] = (st_role_t *)malloc(sizeof(st_role_t));
                                                                                                                                  $$->interaction->to[0]->name = strdup($6);
                                                                                                                                  $$->interaction->to[0]->param = $7;

                                                                                                                                  // Messge signature
                                                                                                                                  $$->interaction->msgsig = $1;

                                                                                                                                  // Message condition
                                                                                                                                  $$->interaction->msg_cond = $8; // Boolean condition
                                                                                                                                }
                            ;

/* --------------------------- For --------------------------- */

foreach                     :    FOREACH LPAREN IDENT COLON range_expr RPAREN global_interaction_blk {
                                                                                                       $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_FOR);
                                                                                                       $$->forloop->range = $5;

                                                                                                       $$->nchild = 1;
                                                                                                       $$->children = (st_node **)calloc(sizeof(st_node *), $$->nchild);
                                                                                                       $$->children[0] = $7;
                                                                                                     }

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
                            |   AND global_interaction_blk and_global_interaction_blk {  $$ = st_node_append($3, $2);                                          }
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
                            |   LOCAL PROTOCOL IDENT AT role_name LSQUARE DIGITS NUMRANGE simple_expr RSQUARE LPAREN role_decl_list RPAREN local_prot_body {
                                                                                                                                                             st_tree_set_name_param(tree, $3, st_expr_binexpr(st_expr_constant($7), ST_EXPR_TYPE_RANGE, $9));
                                                                                                                                                             tree->info->type = ST_TYPE_PARAMETRISED;
                                                                                                                                                             tree->info->myrole = strdup($5);
                                                                                                                                                           }
                            |   LOCAL PROTOCOL IDENT AT role_name LSQUARE DIGITS NUMRANGE simple_expr TUPLE DIGITS NUMRANGE simple_expr RSQUARE LPAREN role_decl_list RPAREN local_prot_body {
                                                                                                                                                             st_tree_set_name_param(tree, $3, st_expr_binexpr(st_expr_binexpr(st_expr_constant($7), ST_EXPR_TYPE_RANGE, $9), ST_EXPR_TYPE_TUPLE, st_expr_binexpr(st_expr_constant($11), ST_EXPR_TYPE_RANGE, $13)));
                                                                                                                                                                                                         tree->info->type = ST_TYPE_PARAMETRISED;
                                                                                                                                                                                                         tree->info->myrole = strdup($5);
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
                            |   l_foreach   { $$ = $1; }
                            |   l_parallel  { $$ = $1; }
                            |   l_choice    { $$ = $1; }
                            |   l_recursion { $$ = $1; }
                            |   continue    { $$ = $1; }
                            ;

/* --------------------------- Point-to-Point Message Transfer --------------------------- */

message_condition           :                           { $$ = NULL; }
                            |   IF role_name role_param {
                                                          assert($2!=NULL /* message_condition cannot be NULL */);
                                                          $$ = (msg_cond_t *)malloc(sizeof(msg_cond_t));
                                                          $$->name = strdup($2);
                                                          $$->param = $3;
                                                        }
                            ;

send                        :   message_signature TO role_name role_param message_condition SEMICOLON {
                                                                                                        $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_SEND);

                                                                                                        // From
                                                                                                        $$->interaction->from = NULL;

                                                                                                        // To
                                                                                                        $$->interaction->nto = 1;
                                                                                                        $$->interaction->to = (st_role_t **)calloc(sizeof(st_role_t *), $$->interaction->nto);
                                                                                                        $$->interaction->to[0] = (st_role_t *)malloc(sizeof(st_role_t));
                                                                                                        $$->interaction->to[0]->name = strdup($3);
                                                                                                        $$->interaction->to[0]->param = $4;

                                                                                                        // Message signature
                                                                                                        $$->interaction->msgsig = $1;

                                                                                                        // Message condition
                                                                                                        $$->interaction->msg_cond = $5; // Role condition
                                                                                                      }
                            ;

receive                     :   message_signature FROM role_name role_param message_condition SEMICOLON {
                                                                                                          $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECV);
                                                                                                          $$->interaction->from = (st_role_t *)malloc(sizeof(st_role_t));
                                                                                                          $$->interaction->from->name = strdup($3);
                                                                                                          $$->interaction->from->param = $4;

                                                                                                          // To
                                                                                                          $$->interaction->nto = 0;
                                                                                                          $$->interaction->to = NULL;

                                                                                                          // Message signature
                                                                                                          $$->interaction->msgsig = $1;

                                                                                                          // Message condition
                                                                                                          $$->interaction->msg_cond = $5; // Role condition
                                                                                                        }
                            ;

/* --------------------------- Choice --------------------------- */

l_foreach                   :    FOREACH LPAREN IDENT COLON range_expr RPAREN local_interaction_blk {
                                                                                                      $$ = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_FOR);
                                                                                                      $$->forloop->var = strdup($3);
                                                                                                      $$->forloop->range = $5;

                                                                                                      $$->nchild = 1;
                                                                                                      $$->children = (st_node **)calloc(sizeof(st_node *), $$->nchild);
                                                                                                      $$->children[0] = $7;
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
