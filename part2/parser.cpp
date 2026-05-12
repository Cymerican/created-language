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
string stripQuotes(const string& s)
{
  return s.substr(1, s.size() - 2);
}

// helper: throw parse error for undeclared variable
void requireDeclared(const string& name)
{
  if (!symDeclared(name)) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): undeclared variable '" << name << "'";
    throw runtime_error(oss.str());
  }
}

// forward declarations
unique_ptr<CompoundStmt> parseCompound();
unique_ptr<Statement>    parseStatement();

// -----------------------------------------------------------------------------
// parseWrite — WRITE ( stringlit | ident )
// -----------------------------------------------------------------------------
unique_ptr<WriteStmt> parseWrite(bool cymerica = false)
{
  auto node = make_unique<WriteStmt>();
  node->isCymerica = cymerica;
  if (cymerica)
    expect(CUSTOM, "CYMERICA keyword");
  else
    expect(WRITE, "WRITE keyword");
  expect(OPENPAREN, "open paren after WRITE");

  if (peek() == STRINGLIT) {
    nextTok();
    node->strContent = stripQuotes(yytext ? string(yytext) : "");
    node->tokenType  = STRINGLIT;
  } else if (peek() == IDENT) {
    nextTok();
    node->varName   = yytext ? string(yytext) : "";
    node->tokenType = IDENT;
    requireDeclared(node->varName);
  } else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected STRINGLIT or IDENT inside WRITE, got "
        << tname(peek()) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }

  expect(CLOSEPAREN, "close paren to end WRITE");
  return node;
}

// -----------------------------------------------------------------------------
// parseRead — READ ( ident )
// -----------------------------------------------------------------------------
unique_ptr<ReadStmt> parseRead()
{
  auto node = make_unique<ReadStmt>();
  expect(READ, "READ keyword");
  expect(OPENPAREN, "open paren after READ");
  if (peek() != IDENT) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected IDENT inside READ, got "
        << tname(peek()) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }
  nextTok();
  node->varName = yytext ? string(yytext) : "";
  requireDeclared(node->varName);
  expect(CLOSEPAREN, "close paren to end READ");
  return node;
}

// -----------------------------------------------------------------------------
// parseAssign — ident := primary
// primary -> INTLIT | FLOATLIT | IDENT [ + IDENT ]
// -----------------------------------------------------------------------------
unique_ptr<AssignStmt> parseAssign()
{
  auto node = make_unique<AssignStmt>();

  // consume the ident
  nextTok();
  node->id = yytext ? string(yytext) : "";
  requireDeclared(node->id);

  expect(ASSIGN, "':=' in assignment");

  if (peek() == INTLIT) {
    nextTok();
    node->value     = yytext ? string(yytext) : "0";
    node->valueType = INTLIT;
  } else if (peek() == FLOATLIT) {
    nextTok();
    node->value     = yytext ? string(yytext) : "0";
    node->valueType = FLOATLIT;
  } else if (peek() == IDENT) {
    nextTok();
    string firstName = yytext ? string(yytext) : "";
    requireDeclared(firstName);
    if (peek() == PLUS) {
      nextTok(); // consume +
      if (peek() != IDENT) {
        ostringstream oss;
        oss << "Parse error (line " << yylineno
            << "): expected IDENT after '+', got "
            << tname(peek()) << " [" << peekLex << "]";
        throw runtime_error(oss.str());
      }
      nextTok();
      string secName = yytext ? string(yytext) : "";
      requireDeclared(secName);
      node->value     = firstName;
      node->rhs2      = secName;
      node->valueType = IDENT;
    } else {
      node->value     = firstName;
      node->valueType = IDENT;
    }
  } else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected expression after ':=', got "
        << tname(peek()) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }

  return node;
}

// -----------------------------------------------------------------------------
// parseStatement — compound | write | read | assign | CYMERICA (like write)
// -----------------------------------------------------------------------------
unique_ptr<Statement> parseStatement()
{
  if (peek() == TOK_BEGIN)
    return parseCompound();
  else if (peek() == WRITE)
    return parseWrite(false);
  else if (peek() == CUSTOM)
    return parseWrite(true);
  else if (peek() == READ)
    return parseRead();
  else if (peek() == IDENT)
    return parseAssign();
  else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected statement, got "
        << tname(peek()) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }
}

// -----------------------------------------------------------------------------
// parseCompound — BEGIN stmt { ; stmt } END
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// parseDeclaration — IDENT : TYPE  (checks for duplicates)
// -----------------------------------------------------------------------------
VarDecl parseDeclaration()
{
  VarDecl d;
  expect(IDENT, "variable name in declaration");
  d.name = yytext ? string(yytext) : "";

  // check for duplicate declaration
  if (symDeclared(d.name)) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): duplicate variable declaration '" << d.name << "'";
    throw runtime_error(oss.str());
  }

  expect(COLON, "':' after variable name");
  Token typeT = nextTok();
  if (typeT == INTEGER)
    d.type = INTEGER;
  else if (typeT == REAL)
    d.type = REAL;
  else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected INTEGER or REAL type, got " << tname(typeT);
    throw runtime_error(oss.str());
  }

  // Insert immediately so subsequent declarations can check for duplicates
  if (d.type == INTEGER)
    symbolTable[d.name] = (int)0;
  else
    symbolTable[d.name] = (double)0.0;

  return d;
}

// -----------------------------------------------------------------------------
// parseVarBlock — VAR decl { ; decl }
// -----------------------------------------------------------------------------
vector<VarDecl> parseVarBlock()
{
  vector<VarDecl> decls;
  expect(VAR, "VAR keyword");

  decls.push_back(parseDeclaration());

  while (peek() == SEMICOLON) {
    nextTok();
    if (peek() == TOK_BEGIN) break;  // end of var section
    decls.push_back(parseDeclaration());
  }

  return decls;
}

// -----------------------------------------------------------------------------
// parseBlock — [ VAR ... ] compound
// -----------------------------------------------------------------------------
unique_ptr<Block> parseBlock()
{
  auto node = make_unique<Block>();

  if (peek() == VAR)
    node->decls = parseVarBlock();

  node->compound = parseCompound();
  return node;
}

// -----------------------------------------------------------------------------
// parseProgram — PROGRAM IDENT ; Block EOF
// -----------------------------------------------------------------------------
unique_ptr<Program> parseProgram()
{
  auto p = make_unique<Program>();
  expect(PROGRAM, "start of program");
  expect(IDENT, "program name");
  p->name  = peekLex;
  expect(SEMICOLON, "after program name");
  p->block = parseBlock();
  expect(TOK_EOF, "at end of file (no trailing tokens after program)");
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
  peekTok  = 0;
  peekLex.clear();

  // Reset symbol table for fresh run
  symbolTable.clear();

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
