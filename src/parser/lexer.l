%{
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

#ifdef __DEBUG__
#define DEBUG_PRINT(s) do { fprintf(stderr, (s), yytext); } while (0)
#else
#define DEBUG_PRINT(s) do {} while (0)
#endif

%}

%option noinput
%option nounput


digit                   [0-9]
letter                  [A-Za-z]
identifier              ({letter}|_)({letter}|{digit}|_)*

string                  \"[^"]*\"
mlcomment_start         "/*"
mlcomment_body          ([^*])|("*"[^/])|{string}
mlcomment_end           "*/"
mlcomment               {mlcomment_start}{mlcomment_body}*{mlcomment_end}
comment                 "//"([^\n])*\n

%%

    /* Skip whitespace */
[ \t\n]                 ;

    /* Keywords */
"and"                   { DEBUG_PRINT("Token: AND '%s'\n");       return AND;       }
"as"                    { DEBUG_PRINT("Token: AS '%s'\n");        return AS;        }
"at"                    { DEBUG_PRINT("Token: AT '%s'\n");        return AT;        }
"choice"                { DEBUG_PRINT("Token: CHOICE '%s'\n");    return CHOICE;    }
"continue"              { DEBUG_PRINT("Token: CONTINUE '%s'\n");  return CONTINUE;  }
"from"                  { DEBUG_PRINT("Token: FROM '%s'\n");      return FROM;      }
"global"                { DEBUG_PRINT("Token: GLOBAL '%s'\n");    return GLOBAL;    }
"import"                { DEBUG_PRINT("Token: IMPORT '%s'\n");    return IMPORT;    }
"local"                 { DEBUG_PRINT("Token: LOCAL '%s'\n");     return LOCAL;     }
"or"                    { DEBUG_PRINT("Token: OR '%s'\n");        return OR;        }
"par"                   { DEBUG_PRINT("Token: PAR '%s'\n");       return PAR;       }
"protocol"              { DEBUG_PRINT("Token: PROTOCOL '%s'\n");  return PROTOCOL;  }
"rec"                   { DEBUG_PRINT("Token: REC '%s'\n");       return REC;       }
"role"                  { DEBUG_PRINT("Token: ROLE '%s'\n");      return ROLE;      }
"to"                    { DEBUG_PRINT("Token: TO '%s'\n");        return TO;        }

    /* Symbols */
"("                     { DEBUG_PRINT("Token: LPAREN '%s'\n");    return LPAREN;    }
")"                     { DEBUG_PRINT("Token: RPAREN '%s'\n");    return RPAREN;    }
"{"                     { DEBUG_PRINT("Token: LBRACE '%s'\n");    return LBRACE;    }
"}"                     { DEBUG_PRINT("Token: RBRACE '%s'\n");    return RBRACE;    }
","                     { DEBUG_PRINT("Token: COMMA '%s'\n");     return COMMA;     }
":"                     { DEBUG_PRINT("Token: COLON '%s'\n");     return COLON;     }
";"                     { DEBUG_PRINT("Token: SEMICOLON '%s'\n"); return SEMICOLON; }

    /* Variables */
{identifier}            { yylval.str = strdup(yytext);
                          DEBUG_PRINT("Token: IDENT '%s'\n");     return IDENT;    }
{mlcomment}             { DEBUG_PRINT("Token: COMMENT '%s'\n");                    }
{comment}               { DEBUG_PRINT("Token: COMMENT '%s'\n");                    }

    /* Catch all */
.                       { fprintf(stderr, "lexer: Don't know what is '%s'\n", yytext); yyterminate(); }

%%

int yywrap()
{
    return 1; // Continue after line wrapping.
}

#undef DEBUG_PRINT
