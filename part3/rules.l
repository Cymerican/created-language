/*****************************************************************************
 *   rules.l - Flex Rules for Cymerica Language (Part 3)
 *   MSU CSE 4714/6714 Capstone Project (Spring 2026)
 *   Author: Derek Willis / caj514
*****************************************************************************/
%option noyywrap nodefault nounput
%option yylineno

%{
#include "lexer.h"
%}

%%
 /* DEFINE YOUR RULES STARTING HERE */

[ \t\r\n]+           { /* skip whitespace */ }

"PROGRAM"            { return PROGRAM;     }
"BEGIN"              { return TOK_BEGIN;   }
"END"                { return END;         }
"WRITE"              { return WRITE;       }
"READ"               { return READ;        }
"VAR"                { return VAR;         }
"INTEGER"            { return INTEGER;     }
"REAL"               { return REAL;        }
"MOD"                { return MOD;         }
"CYMERICA"           { return CUSTOM;      }
"SPAWN"              { return SPAWN;       }

[A-Z][A-Z0-9]{8,}   { return UNKNOWN; }
[A-Z][A-Z0-9]*      { return IDENT;   }

'[^'\n]{81,}'        { return UNKNOWN;   }
'[^'\n]*'            { return STRINGLIT; }

[0-9]+\.[0-9]+       { return FLOATLIT;  }
[0-9]+               { return INTLIT;    }

":="                 { return ASSIGN;       }
"+="                 { return PLUS_ASSIGN;  }
"-="                 { return MINUS_ASSIGN; }
"*="                 { return MULT_ASSIGN;  }
"/="                 { return DIV_ASSIGN;   }
";"                  { return SEMICOLON;    }
":"                  { return COLON;        }
"("                  { return OPENPAREN;    }
")"                  { return CLOSEPAREN;   }
"+"                  { return PLUS;         }
"-"                  { return MINUS;        }
"*"                  { return MULTIPLY;     }
"/"                  { return DIVIDE;       }

.                    { return UNKNOWN; }

%%
