// ============================================================================
//  parser.cpp — Recursive-descent parser for Cymerica Language (Part 3)
// ----------------------------------------------------------------------------
// MSU CSE 4714/6714 Capstone Project (Spring 2026)
// Author: Derek Willis / caj514
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

string stripQuotes(const string& s)
{
  return s.substr(1, s.size() - 2);
}

void requireDeclared(const string& name)
{
  if (!symDeclared(name)) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): undeclared variable '" << name << "'";
    throw runtime_error(oss.str());
  }
}

// Forward declarations
unique_ptr<CompoundStmt> parseCompound();
unique_ptr<Statement>    parseStatement();
unique_ptr<ValueNode>    parseValue();   // NEW

// =============================================================================
// Expression parsers (Part 3)
// Grammar:
//   value   -> term { ( PLUS | MINUS ) term }
//   term    -> factor { ( MULTIPLY | DIVIDE | MOD ) factor }
//   factor  -> [ MINUS ] primary
//   primary -> FLOATLIT | INTLIT | IDENT | OPENPAREN value CLOSEPAREN
// =============================================================================

// -----------------------------------------------------------------------------
// parsePrimary
// -----------------------------------------------------------------------------
unique_ptr<PrimaryNode> parsePrimary()
{
  auto node = make_unique<PrimaryNode>();

  if (peek() == INTLIT) {
    nextTok();
    node->kind   = PrimaryNode::INT_LIT;
    node->intVal = stoi(yytext ? string(yytext) : "0");
  }
  else if (peek() == FLOATLIT) {
    nextTok();
    node->kind     = PrimaryNode::FLOAT_LIT;
    node->floatVal = stod(yytext ? string(yytext) : "0");
  }
  else if (peek() == IDENT) {
    nextTok();
    node->kind      = PrimaryNode::IDENT_REF;
    node->identName = yytext ? string(yytext) : "";
    requireDeclared(node->identName);
  }
  else if (peek() == OPENPAREN) {
    nextTok(); // consume '('
    node->kind      = PrimaryNode::PAREN_EXPR;
    node->parenExpr = parseValue();  // recurse into value
    expect(CLOSEPAREN, "')' to close parenthesized expression");
  }
  else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected primary expression (INTLIT, FLOATLIT, IDENT, or '('), got "
        << tname(peek()) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }

  return node;
}

// -----------------------------------------------------------------------------
// parseFactor — [ MINUS ] primary
// -----------------------------------------------------------------------------
unique_ptr<FactorNode> parseFactor()
{
  auto node = make_unique<FactorNode>();

  if (peek() == MINUS) {
    nextTok(); // consume unary '-'
    node->isNegated = true;
  }

  node->primary = parsePrimary();
  return node;
}

// -----------------------------------------------------------------------------
// parseTerm — factor { ( MULTIPLY | DIVIDE | MOD ) factor }
// -----------------------------------------------------------------------------
unique_ptr<TermNode> parseTerm()
{
  auto node  = make_unique<TermNode>();
  node->first = parseFactor();

  while (peek() == MULTIPLY || peek() == DIVIDE || peek() == MOD) {
    node->ops.push_back(nextTok());
    node->rest.push_back(parseFactor());
  }

  return node;
}

// -----------------------------------------------------------------------------
// parseValue — term { ( PLUS | MINUS ) term }
// -----------------------------------------------------------------------------
unique_ptr<ValueNode> parseValue()
{
  auto node  = make_unique<ValueNode>();
  node->first = parseTerm();

  while (peek() == PLUS || peek() == MINUS) {
    node->ops.push_back(nextTok());
    node->rest.push_back(parseTerm());
  }

  return node;
}

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
// parseAssign — IDENT ( := | += | -= | *= | /= ) value
// -----------------------------------------------------------------------------
unique_ptr<AssignStmt> parseAssign()
{
  auto node = make_unique<AssignStmt>();

  // consume the identifier
  nextTok();
  node->id = yytext ? string(yytext) : "";
  requireDeclared(node->id);

  // consume assignment operator
  Token op = peek();
  if (op == ASSIGN || op == PLUS_ASSIGN || op == MINUS_ASSIGN ||
      op == MULT_ASSIGN || op == DIV_ASSIGN) {
    nextTok();
    node->op = op;
  } else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected assignment operator after '" << node->id << "', got "
        << tname(op) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }

  // parse full RHS expression
  node->rhs = parseValue();
  return node;
}

// -----------------------------------------------------------------------------
// parseSpawn — SPAWN  (custom statement: prints memory snapshot)
// -----------------------------------------------------------------------------
unique_ptr<SpawnStmt> parseSpawn()
{
  expect(SPAWN, "SPAWN keyword");
  return make_unique<SpawnStmt>();
}

// -----------------------------------------------------------------------------
// parseStatement
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
  else if (peek() == SPAWN)
    return parseSpawn();
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
// parseDeclaration
// -----------------------------------------------------------------------------
VarDecl parseDeclaration()
{
  VarDecl d;
  expect(IDENT, "variable name in declaration");
  d.name = yytext ? string(yytext) : "";

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

  // Insert immediately so subsequent declarations can check duplicates
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
    if (peek() == TOK_BEGIN) break;
    decls.push_back(parseDeclaration());
  }

  return decls;
}

// -----------------------------------------------------------------------------
// parseBlock
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
// parseProgram
// -----------------------------------------------------------------------------
unique_ptr<Program> parseProgram()
{
  auto p = make_unique<Program>();
  expect(PROGRAM, "start of program");
  expect(IDENT, "program name");
  p->name  = peekLex;
  expect(SEMICOLON, "after program name");
  p->block = parseBlock();
  expect(TOK_EOF, "at end of file");
  return p;
}

// -----------------------------------------------------------------------------
// Parser entry point
// -----------------------------------------------------------------------------
unique_ptr<Program> parse()
{
  havePeek = false;
  peekTok  = 0;
  peekLex.clear();
  symbolTable.clear();

  auto root = parseProgram();

  if (peek() != TOK_EOF) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): extra tokens after <program>, got "
        << tname(peekTok) << " [" << (yytext ? yytext : "") << "]";
    throw runtime_error(oss.str());
  }

  return root;
}
