// ============================================================================
//  parser.cpp — Recursive-descent parser 
// ----------------------------------------------------------------------------
// MSU CSE 4714/6714 Capstone Project (Spring 2026)
// Author: Derek Willis
// ============================================================================

#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <set>
#include "lexer.h"
#include "ast.h"
#include "debug.h"
using namespace std;

// -----------------------------------------------------------------------------
// One-token lookahead
// -----------------------------------------------------------------------------
bool   havePeek = false;
Token  peekTok  = 0;
string peekLex;

int token = 0;

// Note: Used AI assistance to help implement parsing functions, but teacher said I could as long as I cite

inline const char* tname(Token t) { return tokName(t); }

Token peek() 
{
  if (!havePeek) {
    peekTok = yylex();
    if (peekTok == 0) { peekTok = TOK_EOF; peekLex.clear(); }
    else              { peekLex = yytext ? string(yytext) : string(); }
    dbg::line(string("peek: ") + tname(peekTok) + (peekLex.empty() ? "" : " ["+peekLex+"]")
              + " @ line " + to_string(yylineno));
    havePeek = true;
  }
  return peekTok;
}
Token nextTok() 
{
  Token t = peek();
  dbg::line(string("consume: ") + tname(t));
  havePeek = false;
  return t;
}
Token expect(Token want, const char* msg) 
{
  Token got = nextTok();
  if (got != want) {
    dbg::line(string("expect FAIL: wanted ") + tname(want) + ", got " + tname(got));
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): expected "
        << tname(want) << " — " << msg << ", got " << tname(got)
        << " [" << (yytext ? yytext : "") << "]";
    throw runtime_error(oss.str());
  }
  return got;
}

// strips the quotes off a string literal
string stripQuotes(string s)
{
  return s.substr(1, s.size() - 2);
}

// forward declarations so functions can call each other
unique_ptr<CompoundStmt> parseCompound();
unique_ptr<WriteStmt>    parseWrite();
unique_ptr<Statement>    parseStatement();

// TODO: implement parsing functions for each grammar in your language

// write -> WRITE OPENPAREN STRINGLIT CLOSEPAREN
unique_ptr<WriteStmt> parseWrite()
{
  auto node = make_unique<WriteStmt>();
  expect(WRITE,     "WRITE keyword");
  expect(OPENPAREN, "open paren after WRITE");
  if (peek() != STRINGLIT) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected STRINGLIT inside WRITE, got "
        << tname(peek()) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }
  nextTok();
  node->value = stripQuotes(yytext ? string(yytext) : "");
  expect(CLOSEPAREN, "close paren to end WRITE");
  return node;
}

// statement -> compound | write
unique_ptr<Statement> parseStatement()
{
  if (peek() == TOK_BEGIN)
    return parseCompound();
  else if (peek() == WRITE)
    return parseWrite();
  else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected BEGIN or WRITE, got "
        << tname(peek()) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }
}

// compound -> TOK_BEGIN statement { SEMICOLON statement } END
unique_ptr<CompoundStmt> parseCompound()
{
  auto node = make_unique<CompoundStmt>();
  expect(TOK_BEGIN, "BEGIN keyword");
  node->stmts.push_back(parseStatement());
  while (peek() == SEMICOLON) {
    nextTok();
    if (peek() == END) break;
    node->stmts.push_back(parseStatement());
  }
  expect(END, "END keyword to close block");
  return node;
}

unique_ptr<Block> parseBlock(){
  // Start by creating a pointer to the node we need
  auto node = make_unique<Block>();

  // Step through the grammar, storing anything necessary as member variables
  node->compound = parseCompound();

  // When done with the grammar, return the pointer to our node
  return node;
}

// -----------------------------------------------------------------------------
// Program → PROGRAM IDENT ';' Block EOF
// -----------------------------------------------------------------------------
unique_ptr<Program> parseProgram() {
  // Make a pointer to the node we need to build
  auto p = make_unique<Program>();
  // Step through the grammar, storing anything necessary as member variables
  expect(PROGRAM, "start of program");
  expect(IDENT, "program name");
  // Store the program name 
  p->name  = peekLex;
  expect(SEMICOLON, "after program name");
  // Store a pointer to the appropriate block
  p->block = parseBlock();
  expect(TOK_EOF, "at end of file (no trailing tokens after program)");
  // Nothing left in the grammar so we return our node pointer
  return p;
}

// -----------------------------------------------------------------------------
// Parser entry point (called by driver)
// -----------------------------------------------------------------------------
// *****************************************************
// To test piece-wise change the pointer type below
unique_ptr<Program> parse()
// *****************************************************
{
  // Reset lookahead state for a fresh parse
  havePeek = false;
  peekTok = 0;
  peekLex.clear();
  
  // *****************************************************
  // To test piece-wise change the parser function you set as your root
  auto root = parseProgram();
  // *****************************************************

  // Ensure no extra tokens remain
  if (peek() != TOK_EOF) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): extra tokens after <program>, got "
        << tname(peekTok) << " [" << (yytext ? yytext : "") << "]";
    throw runtime_error(oss.str());
  }

  return root;
}