/*****************************************************************************
 *   rules.l - Flex Rules for TIPS Language Subset
 *   MSU CSE 4714/6714 Capstone Project (Fall 2025)
 *   Author: Derek Willis
*****************************************************************************/
%option noyywrap nodefault nounput
%option yylineno

%{
#include "lexer.h"
%}

%%
 /* DEFINE YOUR RULES STARTING HERE */
 /* Note: Used AI assistance to help implement these rules but teacher said I could as long as I cite*/

[ \t\r\n]+           { /* skip whitespace */ }

"PROGRAM"            { return PROGRAM;   }
"BEGIN"              { return TOK_BEGIN; }
"END"                { return END;       }
"WRITE"              { return WRITE;     }

[A-Z][A-Z0-9]{8,}   { return UNKNOWN; }
[A-Z][A-Z0-9]*      { return IDENT;   }

'[^'\n]{81,}'        { return UNKNOWN;   }
'[^'\n]*'            { return STRINGLIT; }

";"                  { return SEMICOLON;  }
"("                  { return OPENPAREN;  }
")"                  { return CLOSEPAREN; }

.                    { return UNKNOWN; }

%%


