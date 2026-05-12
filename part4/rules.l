/*****************************************************************************
 *   rules.l - Flex Rules for Cymerica Language (Part 4)
 *   MSU CSE 4714/6714 Capstone Project (Spring 2026)
 *   Author: Derek Willis / caj514
 *   Part 4 additions:
 *     - IF, THEN, ELSE, WHILE keywords
 *     - Relational operators: <, >, =, <>, !=
 *     - Logical operators: AND, OR, NOT
 *     - Line comments: ## ... (no newline)
 *     - Block comments: (* ... *) (my design choice)
 *   Note: Used AI assistance to help implement comment rules and new tokens,
 *   but teacher said I could as long as I cite
*****************************************************************************/
%option noyywrap nodefault nounput
%option yylineno

%{
#include <string.h>
#include "lexer.h"

// driver sets this before scanning; we only read it.
extern const char* gSkinC;

// small helper for skin equality
static int skin_is(const char* s) {
  return (gSkinC && s && strcmp(gSkinC, s) == 0);
}
%}

%x BLOCK_COMMENT

%%
 /* -------------------------------------------------------------------------
  * Whitespace
  * ---------------------------------------------------------------------- */
[ \t\r\n]+           { /* skip whitespace */ }

 /* -------------------------------------------------------------------------
  * Line comments: ## anything up to end of line
  * ---------------------------------------------------------------------- */
"##"[^\n]*           { /* skip line comment */ }

 /* -------------------------------------------------------------------------
  * Block comments: (* ... *)  -- my design choice for Part 4
  * ---------------------------------------------------------------------- */
"(*"                 { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*)"  { BEGIN(INITIAL); }
<BLOCK_COMMENT>\n    { /* keep yylineno counting */ }
<BLOCK_COMMENT>.     { /* consume block comment body */ }

 /* -------------------------------------------------------------------------
  * Keywords  (checked before IDENT so they take priority)
  * ---------------------------------------------------------------------- */
"PROGRAM"            { return PROGRAM;     }
"BEGIN"              { return TOK_BEGIN;   }
"END"                { return END;         }
"WRITE"              { return WRITE;       }
"READ"               { return READ;        }
"VAR"                { return VAR;         }
"INTEGER"            { return INTEGER;     }
"REAL"               { return REAL;        }
"MOD"                { return MOD;         }
"AND"                { return TOK_AND;     }
"OR"                 { return TOK_OR;      }
"NOT"                { return TOK_NOT;     }
"IF"                 { return IF;          }
"THEN"               { return THEN;        }
"ELSE"               { return ELSE;        }
"WHILE"              { return WHILE;       }
"CYMERICA"           { return CUSTOM;      }
"SPAWN"              { return SPAWN;       }

 /* -------------------------------------------------------------------------
  * Identifiers (after all keywords)
  * ---------------------------------------------------------------------- */
[A-Z][A-Z0-9]{8,}   { return UNKNOWN; }
[A-Z][A-Z0-9]*      { return IDENT;   }

 /* -------------------------------------------------------------------------
  * String literals
  * ---------------------------------------------------------------------- */
'[^'\n]{81,}'        { return UNKNOWN;   }
'[^'\n]*'            { return STRINGLIT; }

 /* -------------------------------------------------------------------------
  * Numeric literals
  * ---------------------------------------------------------------------- */
[0-9]+\.[0-9]+       { return FLOATLIT;  }
[0-9]+               { return INTLIT;    }

 /* -------------------------------------------------------------------------
  * Operators -- compound assignment (must come before single-char ops)
  * ---------------------------------------------------------------------- */
":="                 { return ASSIGN;       }
"+="                 { return PLUS_ASSIGN;  }
"-="                 { return MINUS_ASSIGN; }
"*="                 { return MULT_ASSIGN;  }
"/="                 { return DIV_ASSIGN;   }

 /* -------------------------------------------------------------------------
  * Relational operators (multi-char before single-char)
  * ---------------------------------------------------------------------- */
"<>"                 { return NOTEQUALTO;   }
"!="                 { return NOTEQUALTO;   }
"<"                  { return LESSTHAN;     }
">"                  { return GREATERTHAN;  }
"="                  { return EQUALTO;      }

 /* -------------------------------------------------------------------------
  * Punctuation
  * ---------------------------------------------------------------------- */
";"                  { return SEMICOLON;    }
":"                  { return COLON;        }
"("                  { return OPENPAREN;    }
")"                  { return CLOSEPAREN;   }

 /* -------------------------------------------------------------------------
  * Arithmetic operators
  * ---------------------------------------------------------------------- */
"+"                  { return PLUS;         }
"-"                  { return MINUS;        }
"*"                  { return MULTIPLY;     }
"/"                  { return DIVIDE;       }

 /* -------------------------------------------------------------------------
  * Catch-all
  * ---------------------------------------------------------------------- */
.                    { return UNKNOWN; }

%%
