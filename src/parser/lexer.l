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
digits                  {digit}{digit}*
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
[ \t\r\n]               ;

    /* Keywords */
"and"                   { DEBUG_PRINT("Token: AND '%s'\n");           return AND;           }
"as"                    { DEBUG_PRINT("Token: AS '%s'\n");            return AS;            }
"at"                    { DEBUG_PRINT("Token: AT '%s'\n");            return AT;            }
"by"                    { DEBUG_PRINT("Token: BY '%s'\n");            return BY;            }
"catch"                 { DEBUG_PRINT("Tokebn: CATCH '%s'\n");        return CATCH;         }
"choice"                { DEBUG_PRINT("Token: CHOICE '%s'\n");        return CHOICE;        }
"continue"              { DEBUG_PRINT("Token: CONTINUE '%s'\n");      return CONTINUE;      }
"create"                { DEBUG_PRINT("Token: CREATE '%s'\n");        return CREATE;        }
"do"                    { DEBUG_PRINT("Token: DO '%s'\n");            return DO;            }
"enter"                 { DEBUG_PRINT("Token: ENTER '%s'\n");         return ENTER;         }
"from"                  { DEBUG_PRINT("Token: FROM '%s'\n");          return FROM;          }
"global"                { DEBUG_PRINT("Token: GLOBAL '%s'\n");        return GLOBAL;        }
"import"                { DEBUG_PRINT("Token: IMPORT '%s'\n");        return IMPORT;        }
"instantiates"          { DEBUG_PRINT("Token: INSTANTIATES '%s'\n");  return INSTANTIATES;  }
"interruptible"         { DEBUG_PRINT("Token: INTERRUPTIBLE '%s'\n"); return INTERRUPTIBLE; }
"local"                 { DEBUG_PRINT("Token: LOCAL '%s'\n");         return LOCAL;         }
"or"                    { DEBUG_PRINT("Token: OR '%s'\n");            return OR;            }
"par"                   { DEBUG_PRINT("Token: PAR '%s'\n");           return PAR;           }
"protocol"              { DEBUG_PRINT("Token: PROTOCOL '%s'\n");      return PROTOCOL;      }
"rec"                   { DEBUG_PRINT("Token: REC '%s'\n");           return REC;           }
"role"                  { DEBUG_PRINT("Token: ROLE '%s'\n");          return ROLE;          }
"spawns"                { DEBUG_PRINT("Token: SPAWNS '%s'\n");        return SPAWNS;        }
"throw"                 { DEBUG_PRINT("Tokens: THROW '%s'\n");        return THROW;         }
"to"                    { DEBUG_PRINT("Token: TO '%s'\n");            return TO;            }
"with"                  { DEBUG_PRINT("Token: WITH '%s'\n");          return WITH;          }

    /* Role parametrised Scribble */
"if"                    { DEBUG_PRINT("Token: IF '%s'\n");        return IF;        }
"const"                 { DEBUG_PRINT("Token: CONST '%s'\n");     return CONST;     }
"range"                 { DEBUG_PRINT("Token: RANGE '%s'\n");     return RANGE;     }

    /* Symbols */
"("                     { DEBUG_PRINT("Token: LPAREN '%s'\n");    return LPAREN;    }
")"                     { DEBUG_PRINT("Token: RPAREN '%s'\n");    return RPAREN;    }
"{"                     { DEBUG_PRINT("Token: LBRACE '%s'\n");    return LBRACE;    }
"}"                     { DEBUG_PRINT("Token: RBRACE '%s'\n");    return RBRACE;    }
"["                     { DEBUG_PRINT("Token: LSQUARE '%s'\n");   return LSQUARE;   }
"]"                     { DEBUG_PRINT("Token: RSQUARE '%s'\n");   return RSQUARE;   }
"<"                     { DEBUG_PRINT("Token: LANGLE '%s'\n");    return LANGLE;    }
">"                     { DEBUG_PRINT("Token: RANGLE '%s'\n");    return RANGLE;    }
","                     { DEBUG_PRINT("Token: COMMA '%s'\n");     return COMMA;     }
":"                     { DEBUG_PRINT("Token: COLON '%s'\n");     return COLON;     }
";"                     { DEBUG_PRINT("Token: SEMICOLON '%s'\n"); return SEMICOLON; }
"="                     { DEBUG_PRINT("Token: EQUAL '%s'\n");     return EQUAL;     }
"+"                     { DEBUG_PRINT("Token: PLUS '%s'\n");      return PLUS;      }
"-"                     { DEBUG_PRINT("Token: MINUS '%s'\n");     return MINUS;     }
"*"                     { DEBUG_PRINT("Token: MULTIPLY '%s'\n");  return MULTIPLY;  }
"/"                     { DEBUG_PRINT("Token: DIVIDE '%s'\n");    return DIVIDE;    }
"%"                     { DEBUG_PRINT("Token: MODULO '%s'\n");    return MODULO;    }
".."                    { DEBUG_PRINT("Token: NUMRANGE '%s'\n");  return NUMRANGE;  }

    /* Variables */
{digits}                { yylval.num = atol(yytext); // Unsigned
                          DEBUG_PRINT("Token: DIGITS '%s'\n");    return DIGITS;    }
{identifier}            { yylval.str = strdup(yytext);
                          DEBUG_PRINT("Token: IDENT '%s'\n");     return IDENT;     }
{mlcomment}             { DEBUG_PRINT("Token: COMMENT '%s'\n");                     }
{comment}               { DEBUG_PRINT("Token: COMMENT '%s'\n");                     }

    /* Catch all */
.                       { fprintf(stderr, "lexer: Don't know what is '%s'\n", yytext); yyterminate(); }

%%

int yywrap()
{
    return 1; // Continue after line wrapping.
}

#undef DEBUG_PRINT
