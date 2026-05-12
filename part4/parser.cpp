// ============================================================================
//  parser.cpp -- Recursive-descent parser for Cymerica Language (Part 4)
// ----------------------------------------------------------------------------
// MSU CSE 4714/6714 Capstone Project (Spring 2026)
// Author: Derek Willis / caj514
// Part 4: proper precedence hierarchy
//   parseExpression (OR) -> parseAndExpr (AND) -> parseRelExpr (relOp)
//   -> parseValue (+/-) -> parseTerm (*/div/mod) -> parseFactor (NOT/-) -> parsePrimary
// Note: Used AI assistance to help implement the new parse functions and
// precedence hierarchy, but teacher said I could as long as I cite
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
int    token    = 0;

inline const char* tname(Token t) { return tokName(t); }

Token peek() {
  if (!havePeek) {
    peekTok = yylex();
    if (peekTok == 0) { peekTok = TOK_EOF; peekLex.clear(); }
    else              { peekLex = yytext ? string(yytext) : string(); }
    dbg::line(string("peek: ")+tname(peekTok)
              +(peekLex.empty()?"":" ["+peekLex+"]")
              +" @ line "+to_string(yylineno));
    havePeek = true;
  }
  return peekTok;
}

Token nextTok() {
  Token t = peek();
  dbg::line(string("consume: ")+tname(t));
  havePeek = false;
  return t;
}

Token expect(Token want, const char* msg) {
  Token got = nextTok();
  if (got != want) {
    dbg::line(string("expect FAIL: wanted ")+tname(want)+", got "+tname(got));
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): expected "
        << tname(want) << " -- " << msg << ", got " << tname(got)
        << " [" << (yytext ? yytext : "") << "]";
    throw runtime_error(oss.str());
  }
  return got;
}

string stripQuotes(const string& s) { return s.substr(1, s.size()-2); }

void requireDeclared(const string& name) {
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
unique_ptr<ExprNode>     parseExpression();
unique_ptr<ValueNode>    parseValue();

// =============================================================================
// parsePrimary -- FLOATLIT | INTLIT | IDENT | OPENPAREN ExprNode CLOSEPAREN
// =============================================================================
unique_ptr<PrimaryNode> parsePrimary() {
  auto node = make_unique<PrimaryNode>();

  if (peek() == INTLIT) {
    string lex = peekLex; nextTok();
    node->kind   = PrimaryNode::INT_LIT;
    node->intVal = stoi(lex.empty() ? (yytext?string(yytext):"0") : lex);
  }
  else if (peek() == FLOATLIT) {
    string lex = peekLex; nextTok();
    node->kind     = PrimaryNode::FLOAT_LIT;
    node->floatVal = stod(lex.empty() ? (yytext?string(yytext):"0") : lex);
  }
  else if (peek() == IDENT) {
    string lex = peekLex; nextTok();
    node->kind      = PrimaryNode::IDENT_REF;
    node->identName = lex.empty() ? (yytext?string(yytext):"") : lex;
    requireDeclared(node->identName);
  }
  else if (peek() == OPENPAREN) {
    nextTok(); // consume '('
    node->kind      = PrimaryNode::PAREN_EXPR;
    node->parenExpr = parseExpression(); // full expression inside parens
    expect(CLOSEPAREN, "')' to close parenthesized expression");
  }
  else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected primary (INTLIT, FLOATLIT, IDENT, or '('), got "
        << tname(peek()) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }

  return node;
}

// =============================================================================
// parseFactor -- [ MINUS | NOT ] primary
// =============================================================================
unique_ptr<FactorNode> parseFactor() {
  auto node = make_unique<FactorNode>();
  if (peek() == MINUS) {
    nextTok(); node->isNegated = true;
    dbg::line("parseFactor: unary MINUS");
  } else if (peek() == TOK_NOT) {
    nextTok(); node->isNot = true;
    dbg::line("parseFactor: NOT");
  }
  node->primary = parsePrimary();
  return node;
}

// =============================================================================
// parseTerm -- factor { ( MULTIPLY | DIVIDE | MOD ) factor }
// =============================================================================
unique_ptr<TermNode> parseTerm() {
  auto node = make_unique<TermNode>();
  node->first = parseFactor();
  while (peek()==MULTIPLY || peek()==DIVIDE || peek()==MOD) {
    node->ops.push_back(nextTok());
    node->rest.push_back(parseFactor());
  }
  return node;
}

// =============================================================================
// parseValue -- term { ( PLUS | MINUS ) term }
// =============================================================================
unique_ptr<ValueNode> parseValue() {
  auto node = make_unique<ValueNode>();
  node->first = parseTerm();
  while (peek()==PLUS || peek()==MINUS) {
    node->ops.push_back(nextTok());
    node->rest.push_back(parseTerm());
  }
  return node;
}

// =============================================================================
// parseRelExpr -- value [ relOp value ]
// relOp: LESSTHAN | GREATERTHAN | EQUALTO | NOTEQUALTO
// =============================================================================
unique_ptr<RelNode> parseRelExpr() {
  dbg::line("parseRelExpr");
  auto node = make_unique<RelNode>();
  node->lhs = parseValue();
  Token t = peek();
  if (t==LESSTHAN||t==GREATERTHAN||t==EQUALTO||t==NOTEQUALTO) {
    node->relOp = nextTok();
    dbg::line(string("parseRelExpr: relOp=")+tname(node->relOp));
    node->rhs = parseValue();
  }
  return node;
}

// =============================================================================
// parseAndExpr -- rel_expr { AND rel_expr }
// =============================================================================
unique_ptr<AndNode> parseAndExpr() {
  dbg::line("parseAndExpr");
  auto node = make_unique<AndNode>();
  node->first = parseRelExpr();
  while (peek() == TOK_AND) {
    nextTok();
    node->rest.push_back(parseRelExpr());
  }
  return node;
}

// =============================================================================
// parseExpression -- and_expr { OR and_expr }
// =============================================================================
unique_ptr<ExprNode> parseExpression() {
  dbg::line("parseExpression");
  auto node = make_unique<ExprNode>();
  node->first = parseAndExpr();
  while (peek() == TOK_OR) {
    nextTok();
    node->rest.push_back(parseAndExpr());
  }
  return node;
}

// =============================================================================
// parseWrite
// =============================================================================
unique_ptr<WriteStmt> parseWrite(bool cymerica=false) {
  auto node = make_unique<WriteStmt>();
  node->isCymerica = cymerica;
  if (cymerica) expect(CUSTOM,"CYMERICA keyword");
  else          expect(WRITE,"WRITE keyword");
  expect(OPENPAREN,"open paren after WRITE");

  if (peek()==STRINGLIT) {
    string lex=peekLex; nextTok();
    node->strContent = stripQuotes(lex.empty()?(yytext?string(yytext):""):lex);
    node->tokenType  = STRINGLIT;
  } else if (peek()==IDENT) {
    string lex=peekLex; nextTok();
    node->varName   = lex.empty()?(yytext?string(yytext):""):lex;
    node->tokenType = IDENT;
    requireDeclared(node->varName);
  } else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected STRINGLIT or IDENT inside WRITE, got "
        << tname(peek()) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }
  expect(CLOSEPAREN,"close paren to end WRITE");
  return node;
}

// =============================================================================
// parseRead
// =============================================================================
unique_ptr<ReadStmt> parseRead() {
  auto node = make_unique<ReadStmt>();
  expect(READ,"READ keyword");
  expect(OPENPAREN,"open paren after READ");
  if (peek()!=IDENT) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected IDENT inside READ, got "
        << tname(peek()) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }
  string lex=peekLex; nextTok();
  node->varName = lex.empty()?(yytext?string(yytext):""):lex;
  requireDeclared(node->varName);
  expect(CLOSEPAREN,"close paren to end READ");
  return node;
}

// =============================================================================
// parseAssign -- IDENT ( := | += | -= | *= | /= ) value
// =============================================================================
unique_ptr<AssignStmt> parseAssign() {
  auto node = make_unique<AssignStmt>();
  string lex=peekLex; nextTok();
  node->id = lex.empty()?(yytext?string(yytext):""):lex;
  requireDeclared(node->id);

  Token op = peek();
  if (op==ASSIGN||op==PLUS_ASSIGN||op==MINUS_ASSIGN||op==MULT_ASSIGN||op==DIV_ASSIGN) {
    nextTok(); node->op = op;
  } else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected assignment operator after '" << node->id << "', got "
        << tname(op) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }
  node->rhs = parseValue();
  return node;
}

// =============================================================================
// parseSpawn
// =============================================================================
unique_ptr<SpawnStmt> parseSpawn() {
  expect(SPAWN,"SPAWN keyword");
  return make_unique<SpawnStmt>();
}

// =============================================================================
// parseIf -- IF expression THEN statement [ ELSE statement ]
// Dangling-else: ELSE binds to nearest IF (greedy match)
// =============================================================================
unique_ptr<IfStmt> parseIf() {
  dbg::line("parseIf: parsing IF statement");
  auto node = make_unique<IfStmt>();
  expect(IF,"IF keyword");
  dbg::line("parseIf: parsing condition");
  node->condition  = parseExpression();
  expect(THEN,"THEN after IF condition");
  dbg::line("parseIf: parsing THEN branch");
  node->thenBranch = parseStatement();
  if (peek()==ELSE) {
    nextTok();
    dbg::line("parseIf: parsing ELSE branch");
    node->elseBranch = parseStatement();
  }
  dbg::line("parseIf: done");
  return node;
}

// =============================================================================
// parseWhile -- WHILE expression statement
// =============================================================================
unique_ptr<WhileStmt> parseWhile() {
  dbg::line("parseWhile: parsing WHILE statement");
  auto node = make_unique<WhileStmt>();
  expect(WHILE,"WHILE keyword");
  dbg::line("parseWhile: parsing condition");
  node->condition = parseExpression();
  dbg::line("parseWhile: parsing body");
  node->body      = parseStatement();
  dbg::line("parseWhile: done");
  return node;
}

// =============================================================================
// parseStatement
// =============================================================================
unique_ptr<Statement> parseStatement() {
  Token t = peek();
  dbg::line(string("parseStatement: ")+tname(t));
  if      (t==TOK_BEGIN) return parseCompound();
  else if (t==IF)        return parseIf();
  else if (t==WHILE)     return parseWhile();
  else if (t==WRITE)     return parseWrite(false);
  else if (t==CUSTOM)    return parseWrite(true);
  else if (t==READ)      return parseRead();
  else if (t==SPAWN)     return parseSpawn();
  else if (t==IDENT)     return parseAssign();
  else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected statement (BEGIN/IF/WHILE/WRITE/READ/SPAWN/IDENT), got "
        << tname(t) << " [" << peekLex << "]";
    throw runtime_error(oss.str());
  }
}

// =============================================================================
// parseCompound -- BEGIN stmt { ; stmt } END
// =============================================================================
unique_ptr<CompoundStmt> parseCompound() {
  dbg::line("parseCompound: BEGIN");
  auto node = make_unique<CompoundStmt>();
  expect(TOK_BEGIN,"BEGIN keyword");
  node->stmts.push_back(parseStatement());
  while (peek()==SEMICOLON) {
    nextTok();
    if (peek()==END) break;
    node->stmts.push_back(parseStatement());
  }
  expect(END,"END keyword to close compound statement");
  dbg::line("parseCompound: END");
  return node;
}

// =============================================================================
// parseDeclaration
// =============================================================================
VarDecl parseDeclaration() {
  VarDecl d;
  expect(IDENT,"variable name in declaration");
  d.name = yytext ? string(yytext) : "";
  if (symDeclared(d.name)) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): duplicate variable declaration '" << d.name << "'";
    throw runtime_error(oss.str());
  }
  expect(COLON,"':' after variable name");
  Token typeT = nextTok();
  if      (typeT==INTEGER) d.type = INTEGER;
  else if (typeT==REAL)    d.type = REAL;
  else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno
        << "): expected INTEGER or REAL type, got " << tname(typeT);
    throw runtime_error(oss.str());
  }
  symbolTable[d.name] = (d.type==INTEGER) ? ExprVal((int)0) : ExprVal((double)0.0);
  return d;
}

// =============================================================================
// parseVarBlock
// =============================================================================
vector<VarDecl> parseVarBlock() {
  vector<VarDecl> decls;
  expect(VAR,"VAR keyword");
  decls.push_back(parseDeclaration());
  while (peek()==SEMICOLON) {
    nextTok();
    if (peek()==TOK_BEGIN) break;
    decls.push_back(parseDeclaration());
  }
  return decls;
}

// =============================================================================
// parseBlock / parseProgram / parse
// =============================================================================
unique_ptr<Block> parseBlock() {
  auto node = make_unique<Block>();
  if (peek()==VAR) node->decls = parseVarBlock();
  node->compound = parseCompound();
  return node;
}

unique_ptr<Program> parseProgram() {
  auto p = make_unique<Program>();
  expect(PROGRAM,"start of program");
  expect(IDENT,"program name");
  p->name  = yytext ? string(yytext) : "";
  expect(SEMICOLON,"after program name");
  p->block = parseBlock();
  expect(TOK_EOF,"at end of file");
  return p;
}

unique_ptr<Program> parse() {
  havePeek = false; peekTok = 0; peekLex.clear();
  symbolTable.clear();
  auto root = parseProgram();
  if (peek()!=TOK_EOF) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): extra tokens after program, got "
        << tname(peekTok) << " [" << (yytext?yytext:"") << "]";
    throw runtime_error(oss.str());
  }
  return root;
}
