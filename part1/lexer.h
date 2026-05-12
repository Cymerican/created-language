// *****************************************************************************
//   lexer.h - Token constants and shared declarations for TIPS Language Subset
// *****************************************************************************
// MSU CSE 4714/6714 Capstone Project (Fall 2025)
// Author: Derek Willis
// *****************************************************************************
#pragma once
#include <cstdio>
// ---------------------------------------------------------------------------
// Keywords
// ---------------------------------------------------------------------------
#define TOK_BEGIN   1000
#define ELSE        1004
#define END         1005
#define IF          1007
#define PROGRAM     1009
#define READ        1010
#define THEN        1012
#define VAR         1014
#define WHILE       1015
#define WRITE       1016
#define CUSTOM      1017
// ---------------------------------------------------------------------------
// Datatype Specifiers
// ---------------------------------------------------------------------------
#define INTEGER     1100
#define REAL        1101
// ---------------------------------------------------------------------------
// Punctuation
// ---------------------------------------------------------------------------
#define SEMICOLON   2000
#define COLON       2001
#define OPENPAREN   2002
#define CLOSEPAREN  2003
// ---------------------------------------------------------------------------
// Operators
// ---------------------------------------------------------------------------
#define PLUS        3000
#define MINUS       3001
#define MULTIPLY    3002
#define DIVIDE      3003
#define ASSIGN      3004
#define EQUALTO     3005
#define LESSTHAN    3006
#define GREATERTHAN 3007
#define NOTEQUALTO  3008
#define MOD         3009
#define TOK_NOT     3010
#define TOK_OR      3011
#define TOK_AND     3012
#define CUSTOM_OPER 3013
// ---------------------------------------------------------------------------
// Useful abstractions
// ---------------------------------------------------------------------------
#define IDENT       4000  // identifier
#define INTLIT      4001  // integer literal
#define FLOATLIT    4002  // floating point literal
#define STRINGLIT   4003  // string literal
// ---------------------------------------------------------------------------
// Special Tokens
// ---------------------------------------------------------------------------
#define TOK_EOF        0  // end of file
#define UNKNOWN     6000  // unknown lexeme

// For convenience: refer to TOKENS as Token's vs. int's
using Token = int;

// Flex globals
int yylex(void);
extern FILE* yyin;
extern char* yytext;        // the string contents of a TOKEN
extern int   yylineno;

// Optional “current token” symbol (defined in parser.cpp)
extern int token;

// Friendly names for dumps/errors
inline const char* tokName(Token t) {
  switch (t) {
    case TOK_BEGIN:     return "BEGIN";
    case ELSE:          return "ELSE";
    case END:           return "END";
    case IF:            return "IF";
    case PROGRAM:       return "PROGRAM";
    case READ:          return "READ";
    case THEN:          return "THEN";
    case VAR:           return "VAR";
    case WHILE:         return "WHILE";
    case WRITE:         return "WRITE";
    case CUSTOM:        return "CUSTOM KEYWORD";      
    case INTEGER:       return "INTEGER";
    case REAL:          return "REAL";
    case SEMICOLON:     return "SEMICOLON";
    case COLON:         return "COLON";
    case OPENPAREN:     return "OPENPAREN";
    case CLOSEPAREN:    return "CLOSEPAREN";
    case PLUS:          return "PLUS";
    case MINUS:         return "MINUS";
    case MULTIPLY:      return "MULTIPLY";
    case DIVIDE:        return "DIVIDE";
    case ASSIGN:        return "ASSIGN";
    case EQUALTO:       return "EQUALTO";
    case LESSTHAN:      return "LESSTHAN";
    case GREATERTHAN:   return "GREATERTHAN";
    case NOTEQUALTO:    return "NOTEQUALTO";
    case MOD:           return "MOD";
    case TOK_NOT:       return "NOT";
    case TOK_OR:        return "OR";
    case TOK_AND:       return "AND";
    case CUSTOM_OPER:   return "CUSTOM OPERATOR";     
    case IDENT:         return "IDENT";
    case INTLIT:        return "INTLIT";
    case FLOATLIT:      return "FLOATLIT";
    case STRINGLIT:     return "STRINGLIT";
    case TOK_EOF:       return "EOF";
    case UNKNOWN:       return "UNKNOWN";
    default:            return "?";
  }
}
