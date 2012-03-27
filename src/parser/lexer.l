%{
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
%}

%option noinput
%option nounput


digit                   [0..9]
letter                  [A-Za-z]
identifier              ({letter}|_)({letter}|{digit}|_)*

string                  \"[^"]*\"
mlcomment_start         "/*"
mlcomment_body          ([^*])|("*"[^/])|{string}
mlcomment_end           "*/"
mlcomment               {mlcomment_start}{mlcomment_body}*{mlcomment_end}

%%

    /* Skip whitespace */
[ \t\n]                 ;

    /* Keywords */
"and"                   { /* printf("Token: AND '%s'\n", yytext); */       return AND;       }
"as"                    { /* printf("Token: AS '%s'\n", yytext); */        return AS;        }
"at"                    { /* printf("Token: AT '%s'\n", yytext); */        return AT;        }
"choice"                { /* printf("Token: CHOICE '%s'\n", yytext); */    return CHOICE;    }
"continue"              { /* printf("Token: CONTINUE '%s'\n", yytext); */  return CONTINUE;  }
"from"                  { /* printf("Token: FROM '%s'\n", yytext); */      return FROM;      }
"global"                { /* printf("Token: GLOBAL '%s'\n", yytext); */    return GLOBAL;    }
"import"                { /* printf("Token: IMPORT '%s'\n", yytext); */    return IMPORT;    }
"local"                 { /* printf("Token: LOCAL '%s'\n", yytext); */     return LOCAL;     }
"or"                    { /* printf("Token: OR '%s'\n", yytext); */        return OR;        }
"par"                   { /* printf("Token: PAR '%s'\n", yytext); */       return PAR;       }
"protocol"              { /* printf("Token: PROTOCOL '%s'\n", yytext); */  return PROTOCOL;  }
"rec"                   { /* printf("Token: REC '%s'\n", yytext); */       return REC;       }
"role"                  { /* printf("Token: ROLE '%s'\n", yytext); */      return ROLE;      }
"to"                    { /* printf("Token: TO '%s'\n", yytext); */        return TO;        }

    /* Symbols */
"("                     { /* printf("Token: LPAREN '%s'\n", yytext); */    return LPAREN;    }
")"                     { /* printf("Token: RPAREN '%s'\n", yytext); */    return RPAREN;    }
"{"                     { /* printf("Token: LBRACE '%s'\n", yytext); */    return LBRACE;    }
"}"                     { /* printf("Token: RBRACE '%s'\n", yytext); */    return RBRACE;    }
","                     { /* printf("Token: COMMA '%s'\n", yytext); */     return COMMA;     }
":"                     { /* printf("Token: COLON '%s'\n", yytext); */     return COLON;     }
";"                     { /* printf("Token: SEMICOLON '%s'\n", yytext); */ return SEMICOLON; }

    /* Variables */
{identifier}            { yylval.str = strdup(yytext);
                          /* printf("Token: IDENT '%s'\n", yytext); */      return IDENT;    }
{mlcomment}             { /* printf("Token: COMMENT '%s'\n", yytext); */                     }

    /* Catch all */
.                       { fprintf(stderr, "Don't know what is '%s'\n", yytext); yyterminate(); }

%%

int yywrap()
{
    return 1; // Continue after line wrapping.
}
