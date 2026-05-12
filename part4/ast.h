// =============================================================================
//   ast.h -- Abstract Syntax Tree for Cymerica Language (Part 4)
// =============================================================================
// MSU CSE 4714/6714 Capstone Project (Spring 2026)
// Author: Derek Willis / caj514
// Part 4: proper operator precedence hierarchy (lowest to highest):
//   OR  >  AND  >  relational (=,<>,<,>)  >  +/-  >  */div/mod  >  unary
// Note: Used AI assistance to help implement these structures and interpret
// functions, but teacher said I could as long as I cite
// =============================================================================
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include "lexer.h"
using namespace std;

// -----------------------------------------------------------------------------
// Symbol Table
// -----------------------------------------------------------------------------
inline map<string, variant<int,double>> symbolTable;
inline bool symDeclared(const string& name) { return symbolTable.count(name) > 0; }

// -----------------------------------------------------------------------------
// EPSILON -- for floating-point truth and equality
// -----------------------------------------------------------------------------
static constexpr double EPSILON = 0.000001;

inline bool isTruthy(double v)              { return fabs(v) > EPSILON; }
inline double relEqual(double a, double b)  { return fabs(a-b) <= EPSILON ? 1.0 : 0.0; }
inline double relNotEqual(double a, double b){ return fabs(a-b) >  EPSILON ? 1.0 : 0.0; }
inline double relLess(double a, double b)   { return (b-a) > EPSILON      ? 1.0 : 0.0; }
inline double relGreater(double a, double b){ return (a-b) > EPSILON      ? 1.0 : 0.0; }

// -----------------------------------------------------------------------------
// Pretty printer helper
// -----------------------------------------------------------------------------
inline void ast_line(ostream& os, string prefix, bool last, string label) {
  os << prefix << (last ? "└── " : "├── ") << label << "\n";
}

// =============================================================================
// Expression type
// =============================================================================
using ExprVal = variant<int, double>;

inline double toDouble(const ExprVal& v) {
  return visit([](auto x) -> double { return (double)x; }, v);
}
inline bool isDouble(const ExprVal& v) { return holds_alternative<double>(v); }

// =============================================================================
// Forward declarations for all node types
// =============================================================================
struct ExprNode;   // OR level  (top)
struct AndNode;    // AND level
struct RelNode;    // relational level
struct ValueNode;  // +/- level
struct TermNode;   // */div/mod level
struct FactorNode; // unary level
struct PrimaryNode;// leaf level

// =============================================================================
// PrimaryNode -- FLOATLIT | INTLIT | IDENT | OPENPAREN ExprNode CLOSEPAREN
// =============================================================================
struct PrimaryNode {
  enum Kind { INT_LIT, FLOAT_LIT, IDENT_REF, PAREN_EXPR } kind;
  int    intVal   = 0;
  double floatVal = 0.0;
  string identName;
  unique_ptr<ExprNode> parenExpr;

  void print_tree(ostream& os, string prefix, bool last);
  ExprVal interpret(ostream& out);
};

// =============================================================================
// FactorNode -- [ MINUS | NOT ] primary
// =============================================================================
struct FactorNode {
  bool isNegated = false;
  bool isNot     = false;
  unique_ptr<PrimaryNode> primary;

  void print_tree(ostream& os, string prefix, bool last) {
    if (isNot) {
      ast_line(os, prefix, last, "Factor (NOT)");
      primary->print_tree(os, prefix+(last?"    ":"│   "), true);
    } else if (isNegated) {
      ast_line(os, prefix, last, "Factor (unary -)");
      primary->print_tree(os, prefix+(last?"    ":"│   "), true);
    } else {
      primary->print_tree(os, prefix, last);
    }
  }

  ExprVal interpret(ostream& out) {
    ExprVal val = primary->interpret(out);
    if (isNot) {
      return isTruthy(toDouble(val)) ? (int)0 : (int)1;
    }
    if (isNegated) {
      return isDouble(val) ? ExprVal(-get<double>(val)) : ExprVal(-get<int>(val));
    }
    return val;
  }
};

// =============================================================================
// TermNode -- factor { ( MULTIPLY | DIVIDE | MOD ) factor }
// =============================================================================
struct TermNode {
  unique_ptr<FactorNode>         first;
  vector<Token>                  ops;
  vector<unique_ptr<FactorNode>> rest;

  void print_tree(ostream& os, string prefix, bool last) {
    if (ops.empty()) { first->print_tree(os, prefix, last); return; }
    ast_line(os, prefix, last, string("Binary ") + tokName(ops[0]));
    string ind = prefix + (last ? "    " : "│   ");
    first->print_tree(os, ind, false);
    for (size_t i=0;i<rest.size();i++) rest[i]->print_tree(os,ind,i==rest.size()-1);
  }

  ExprVal interpret(ostream& out) {
    ExprVal result = first->interpret(out);
    for (size_t i=0; i<ops.size(); i++) {
      ExprVal rhs = rest[i]->interpret(out);
      if (ops[i] == MOD) {
        if (isDouble(result)||isDouble(rhs))
          throw runtime_error("Runtime error: MOD requires INTEGER operands");
        int b = get<int>(rhs);
        if (b==0) throw runtime_error("Runtime error: MOD by zero");
        result = get<int>(result) % b;
      } else if (ops[i] == MULTIPLY) {
        if (isDouble(result)||isDouble(rhs)) result = toDouble(result)*toDouble(rhs);
        else result = get<int>(result)*get<int>(rhs);
      } else if (ops[i] == DIVIDE) {
        double d = toDouble(rhs);
        if (fabs(d)<EPSILON) throw runtime_error("Runtime error: division by zero");
        double q = toDouble(result)/d;
        result = (!isDouble(result)&&!isDouble(rhs)) ? ExprVal((int)q) : ExprVal(q);
      }
    }
    return result;
  }
};

// =============================================================================
// ValueNode -- term { ( PLUS | MINUS ) term }
// =============================================================================
struct ValueNode {
  unique_ptr<TermNode>         first;
  vector<Token>                ops;
  vector<unique_ptr<TermNode>> rest;

  void print_tree(ostream& os, string prefix, bool last) {
    if (ops.empty()) { first->print_tree(os, prefix, last); return; }
    ast_line(os, prefix, last, string("Binary ") + tokName(ops[0]));
    string ind = prefix + (last ? "    " : "│   ");
    first->print_tree(os, ind, false);
    for (size_t i=0;i<rest.size();i++) rest[i]->print_tree(os,ind,i==rest.size()-1);
  }

  ExprVal interpret(ostream& out) {
    ExprVal result = first->interpret(out);
    for (size_t i=0; i<ops.size(); i++) {
      ExprVal rhs = rest[i]->interpret(out);
      if (isDouble(result)||isDouble(rhs)) {
        result = (ops[i]==PLUS) ? toDouble(result)+toDouble(rhs)
                                : toDouble(result)-toDouble(rhs);
      } else {
        result = (ops[i]==PLUS) ? get<int>(result)+get<int>(rhs)
                                : get<int>(result)-get<int>(rhs);
      }
    }
    return result;
  }
};

// =============================================================================
// RelNode -- value [ relOp value ]
// Precedence: tighter than AND, looser than +/-
// =============================================================================
struct RelNode {
  unique_ptr<ValueNode> lhs;
  Token                 relOp = 0;
  unique_ptr<ValueNode> rhs;

  void print_tree(ostream& os, string prefix, bool last) {
    if (!relOp) { lhs->print_tree(os, prefix, last); return; }
    ast_line(os, prefix, last, string("RelOp ") + tokName(relOp));
    string ind = prefix + (last ? "    " : "│   ");
    lhs->print_tree(os, ind, false);
    rhs->print_tree(os, ind, true);
  }

  ExprVal interpret(ostream& out) {
    ExprVal lv = lhs->interpret(out);
    if (!relOp) return lv;
    ExprVal rv = rhs->interpret(out);
    double a = toDouble(lv), b = toDouble(rv);
    switch(relOp) {
      case EQUALTO:     return (int)relEqual(a,b);
      case NOTEQUALTO:  return (int)relNotEqual(a,b);
      case LESSTHAN:    return (int)relLess(a,b);
      case GREATERTHAN: return (int)relGreater(a,b);
      default:          return (int)0;
    }
  }
};

// =============================================================================
// AndNode -- rel_expr { AND rel_expr }
// =============================================================================
struct AndNode {
  unique_ptr<RelNode>         first;
  vector<unique_ptr<RelNode>> rest;  // all connected by AND

  void print_tree(ostream& os, string prefix, bool last) {
    if (rest.empty()) { first->print_tree(os, prefix, last); return; }
    ast_line(os, prefix, last, "AND");
    string ind = prefix + (last ? "    " : "│   ");
    first->print_tree(os, ind, false);
    for (size_t i=0;i<rest.size();i++) rest[i]->print_tree(os,ind,i==rest.size()-1);
  }

  ExprVal interpret(ostream& out) {
    ExprVal result = first->interpret(out);
    if (rest.empty()) return result;  // no AND: pass through raw value
    for (size_t i=0; i<rest.size(); i++) {
      if (!isTruthy(toDouble(result))) return (int)0;  // short-circuit
      result = rest[i]->interpret(out);
    }
    return isTruthy(toDouble(result)) ? (int)1 : (int)0;
  }
};

// =============================================================================
// ExprNode -- and_expr { OR and_expr }   (top-level expression)
// =============================================================================
struct ExprNode {
  unique_ptr<AndNode>         first;
  vector<unique_ptr<AndNode>> rest;  // all connected by OR

  void print_tree(ostream& os, string prefix, bool last) {
    if (rest.empty()) { first->print_tree(os, prefix, last); return; }
    ast_line(os, prefix, last, "OR");
    string ind = prefix + (last ? "    " : "│   ");
    first->print_tree(os, ind, false);
    for (size_t i=0;i<rest.size();i++) rest[i]->print_tree(os,ind,i==rest.size()-1);
  }

  ExprVal interpret(ostream& out) {
    ExprVal result = first->interpret(out);
    for (size_t i=0; i<rest.size(); i++) {
      if (isTruthy(toDouble(result))) return (int)1;  // short-circuit
      result = rest[i]->interpret(out);
    }
    // No OR ops: return raw value (so non-boolean expressions pass through)
    if (rest.empty()) return result;
    return isTruthy(toDouble(result)) ? (int)1 : (int)0;
  }
};

// =============================================================================
// PrimaryNode::interpret and print_tree (defined here after ExprNode is complete)
// =============================================================================
inline void PrimaryNode::print_tree(ostream& os, string prefix, bool last) {
  switch(kind) {
    case INT_LIT:   ast_line(os,prefix,last,"Int "+to_string(intVal)); break;
    case FLOAT_LIT: ast_line(os,prefix,last,"Real "+to_string(floatVal)); break;
    case IDENT_REF: ast_line(os,prefix,last,"Ident "+identName); break;
    case PAREN_EXPR:ast_line(os,prefix,last,"Paren"); break;
  }
}

inline ExprVal PrimaryNode::interpret(ostream& out) {
  switch(kind) {
    case INT_LIT:   return intVal;
    case FLOAT_LIT: return floatVal;
    case IDENT_REF: {
      auto& slot = symbolTable[identName];
      return visit([](auto v)->ExprVal{return v;}, slot);
    }
    case PAREN_EXPR: return parenExpr->interpret(out);
  }
  return 0;
}

// =============================================================================
// Statement nodes
// =============================================================================
struct Statement {
  virtual void print_tree(ostream& os, string prefix, bool last) = 0;
  virtual void interpret(ostream& out) = 0;
  virtual ~Statement() = default;
};

// -----------------------------------------------------------------------------
// WriteStmt
// -----------------------------------------------------------------------------
struct WriteStmt : public Statement {
  string strContent, varName;
  Token  tokenType;
  bool   isCymerica = false;

  void print_tree(ostream& os, string prefix, bool last) override {
    string ind = prefix+(last?"    ":"│   ");
    ast_line(os,prefix,last,"Write Statement");
    if (tokenType==STRINGLIT) ast_line(os,ind,true,"content: '"+strContent+"'");
    else                      ast_line(os,ind,true,"content: "+varName);
  }

  void interpret(ostream& out) override {
    if (isCymerica) out << "[CYMERICA]: ";
    if (tokenType==STRINGLIT) {
      out << strContent << "\n";
    } else {
      visit([&](auto&& v){ out << v << "\n"; }, symbolTable[varName]);
    }
  }
};

// -----------------------------------------------------------------------------
// ReadStmt
// -----------------------------------------------------------------------------
struct ReadStmt : public Statement {
  string varName;

  void print_tree(ostream& os, string prefix, bool last) override {
    string ind = prefix+(last?"    ":"│   ");
    ast_line(os,prefix,last,"Read Statement");
    ast_line(os,ind,true,"target: "+varName);
  }

  void interpret(ostream& /*out*/) override {
    visit([](auto& v){ cin >> v; }, symbolTable[varName]);
  }
};

// -----------------------------------------------------------------------------
// AssignStmt
// -----------------------------------------------------------------------------
struct AssignStmt : public Statement {
  string                id;
  Token                 op;
  unique_ptr<ValueNode> rhs;

  void print_tree(ostream& os, string prefix, bool last) override {
    string ind = prefix+(last?"    ":"│   ");
    string opStr;
    switch(op){
      case ASSIGN:       opStr=":="; break;
      case PLUS_ASSIGN:  opStr="+="; break;
      case MINUS_ASSIGN: opStr="-="; break;
      case MULT_ASSIGN:  opStr="*="; break;
      case DIV_ASSIGN:   opStr="/="; break;
      default:           opStr="?";
    }
    ast_line(os,prefix,last,"Assign "+id+" "+opStr);
    rhs->print_tree(os,ind,true);
  }

  void interpret(ostream& out) override {
    ExprVal rhsVal = rhs->interpret(out);
    auto& lhs = symbolTable[id];
    if (op != ASSIGN) {
      ExprVal lhsVal = visit([](auto v)->ExprVal{return v;}, lhs);
      if (op==PLUS_ASSIGN) {
        rhsVal = (isDouble(lhsVal)||isDouble(rhsVal))
          ? ExprVal(toDouble(lhsVal)+toDouble(rhsVal))
          : ExprVal(get<int>(lhsVal)+get<int>(rhsVal));
      } else if (op==MINUS_ASSIGN) {
        rhsVal = (isDouble(lhsVal)||isDouble(rhsVal))
          ? ExprVal(toDouble(lhsVal)-toDouble(rhsVal))
          : ExprVal(get<int>(lhsVal)-get<int>(rhsVal));
      } else if (op==MULT_ASSIGN) {
        rhsVal = (isDouble(lhsVal)||isDouble(rhsVal))
          ? ExprVal(toDouble(lhsVal)*toDouble(rhsVal))
          : ExprVal(get<int>(lhsVal)*get<int>(rhsVal));
      } else if (op==DIV_ASSIGN) {
        double d = toDouble(rhsVal);
        if (fabs(d)<EPSILON) throw runtime_error("Runtime error: division by zero");
        double q = toDouble(lhsVal)/d;
        rhsVal = (!isDouble(lhsVal)&&!isDouble(rhsVal)) ? ExprVal((int)q) : ExprVal(q);
      }
    }
    visit([&](auto& slot){
      using T = decay_t<decltype(slot)>;
      slot = isDouble(rhsVal) ? static_cast<T>(get<double>(rhsVal))
                              : static_cast<T>(get<int>(rhsVal));
    }, lhs);
  }
};

// -----------------------------------------------------------------------------
// SpawnStmt
// -----------------------------------------------------------------------------
struct SpawnStmt : public Statement {
  void print_tree(ostream& os, string prefix, bool last) override {
    ast_line(os,prefix,last,"Spawn Statement");
  }
  void interpret(ostream& out) override {
    out << "-- SPAWN: current variable values --\n";
    for (auto& [name,val] : symbolTable) {
      out << name << " = ";
      holds_alternative<int>(val) ? out<<get<int>(val) : out<<get<double>(val);
      out << "\n";
    }
    out << "------------------------------------\n";
  }
};

// -----------------------------------------------------------------------------
// CompoundStmt
// -----------------------------------------------------------------------------
struct CompoundStmt : public Statement {
  vector<unique_ptr<Statement>> stmts;

  void print_tree(ostream& os, string prefix, bool last) override {
    string ind = prefix+(last?"    ":"│   ");
    ast_line(os,prefix,last,"Compound Statement");
    for (size_t i=0;i<stmts.size();i++)
      stmts[i]->print_tree(os,ind,i==stmts.size()-1);
  }
  void interpret(ostream& out) override {
    for (auto& s:stmts) s->interpret(out);
  }
};

// -----------------------------------------------------------------------------
// IfStmt -- IF expression THEN statement [ ELSE statement ]
// -----------------------------------------------------------------------------
struct IfStmt : public Statement {
  unique_ptr<ExprNode>  condition;
  unique_ptr<Statement> thenBranch;
  unique_ptr<Statement> elseBranch;

  void print_tree(ostream& os, string prefix, bool last) override {
    string ind = prefix+(last?"    ":"│   ");
    ast_line(os,prefix,last,"If Statement");
    ast_line(os,ind,false,"Condition:");
    condition->print_tree(os,ind+"│   ",true);
    bool hasElse = (elseBranch!=nullptr);
    ast_line(os,ind,!hasElse,"Then:");
    thenBranch->print_tree(os,ind+(hasElse?"│   ":"    "),true);
    if (hasElse) {
      ast_line(os,ind,true,"Else:");
      elseBranch->print_tree(os,ind+"    ",true);
    }
  }

  void interpret(ostream& out) override {
    ExprVal condVal = condition->interpret(out);
    if (isTruthy(toDouble(condVal)))
      thenBranch->interpret(out);
    else if (elseBranch)
      elseBranch->interpret(out);
  }
};

// -----------------------------------------------------------------------------
// WhileStmt -- WHILE expression statement
// -----------------------------------------------------------------------------
struct WhileStmt : public Statement {
  unique_ptr<ExprNode>  condition;
  unique_ptr<Statement> body;

  void print_tree(ostream& os, string prefix, bool last) override {
    string ind = prefix+(last?"    ":"│   ");
    ast_line(os,prefix,last,"While Statement");
    ast_line(os,ind,false,"Condition:");
    condition->print_tree(os,ind+"│   ",true);
    ast_line(os,ind,true,"Body:");
    body->print_tree(os,ind+"    ",true);
  }

  void interpret(ostream& out) override {
    while (isTruthy(toDouble(condition->interpret(out))))
      body->interpret(out);
  }
};

// -----------------------------------------------------------------------------
// VarDecl / Block / Program
// -----------------------------------------------------------------------------
struct VarDecl { string name; Token type; };

struct Block {
  vector<VarDecl>          decls;
  unique_ptr<CompoundStmt> compound;

  void print_tree(ostream& os, string prefix, bool last) {
    string ind = prefix+(last?"    ":"│   ");
    ast_line(os,prefix,last,"Block");
    if (!decls.empty()) {
      bool hasComp = (compound!=nullptr);
      ast_line(os,ind,!hasComp,"declarations:");
      string di = ind+(!hasComp?"    ":"│   ");
      for (size_t i=0;i<decls.size();i++) {
        string ts = (decls[i].type==INTEGER)?"INTEGER = 0":"REAL = 0.000000";
        ast_line(os,di,i==decls.size()-1, decls[i].name+" : "+ts);
      }
    }
    if (compound) compound->print_tree(os,ind,true);
  }

  void interpret(ostream& out) {
    for (auto& d:decls)
      symbolTable[d.name] = (d.type==INTEGER) ? ExprVal((int)0) : ExprVal((double)0.0);
    if (compound) compound->interpret(out);
  }
};

struct Program {
  string name;
  unique_ptr<Block> block;

  void print_tree(ostream& os) {
    os << "Program\n";
    ast_line(os,"",false,"name: "+name);
    if (block) block->print_tree(os,"",true);
  }

  void interpret(ostream& out) { if (block) block->interpret(out); }
};
