// =============================================================================
//   ast.h — Abstract Syntax Tree for Cymerica Language (Part 3)
// =============================================================================
// MSU CSE 4714/6714 Capstone Project (Spring 2026)
// Author: Derek Willis / caj514
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

inline bool symDeclared(const string& name) {
  return symbolTable.count(name) > 0;
}

// -----------------------------------------------------------------------------
// Pretty printer helper
// -----------------------------------------------------------------------------
inline void ast_line(ostream& os, string prefix, bool last, string label) {
  os << prefix << (last ? "└── " : "├── ") << label << "\n";
}

// =============================================================================
// Expression nodes — these RETURN variant<int,double>
// =============================================================================

// Forward declarations
struct ValueNode;
struct TermNode;
struct FactorNode;
struct PrimaryNode;

using ExprVal = variant<int, double>;

// Helper: convert ExprVal to double for mixed-type math
inline double toDouble(const ExprVal& v) {
  return visit([](auto x) -> double { return (double)x; }, v);
}

// Helper: is ExprVal a double?
inline bool isDouble(const ExprVal& v) {
  return holds_alternative<double>(v);
}

// -----------------------------------------------------------------------------
// PrimaryNode — FLOATLIT | INTLIT | IDENT | OPENPAREN value CLOSEPAREN
// -----------------------------------------------------------------------------
struct PrimaryNode {
  // exactly one of these is active, based on kind
  enum Kind { INT_LIT, FLOAT_LIT, IDENT_REF, PAREN_EXPR } kind;
  int    intVal   = 0;
  double floatVal = 0.0;
  string identName;
  unique_ptr<ValueNode> parenExpr;  // for ( value )

  void print_tree(ostream& os, string prefix, bool last) {
    switch (kind) {
      case INT_LIT:
        ast_line(os, prefix, last, "Int " + to_string(intVal));
        break;
      case FLOAT_LIT:
        ast_line(os, prefix, last, "Real " + to_string(floatVal));
        break;
      case IDENT_REF:
        ast_line(os, prefix, last, "Ident " + identName);
        break;
      case PAREN_EXPR: {
        ast_line(os, prefix, last, "Paren");
        string indent = prefix + (last ? "    " : "│   ");
        // parenExpr printed by caller chain
        break;
      }
    }
  }

  ExprVal interpret(ostream& out);  // defined after ValueNode
};

// -----------------------------------------------------------------------------
// FactorNode — [ MINUS ] primary
// -----------------------------------------------------------------------------
struct FactorNode {
  bool  isNegated = false;
  unique_ptr<PrimaryNode> primary;

  void print_tree(ostream& os, string prefix, bool last) {
    if (isNegated) {
      ast_line(os, prefix, last, "Factor (unary -)");
      string indent = prefix + (last ? "    " : "│   ");
      primary->print_tree(os, indent, true);
    } else {
      primary->print_tree(os, prefix, last);
    }
  }

  ExprVal interpret(ostream& out) {
    ExprVal val = primary->interpret(out);
    if (!isNegated) return val;
    // negate
    if (isDouble(val))
      return -get<double>(val);
    else
      return -get<int>(val);
  }
};

// -----------------------------------------------------------------------------
// TermNode — factor { ( MULTIPLY | DIVIDE | MOD ) factor }
// -----------------------------------------------------------------------------
struct TermNode {
  unique_ptr<FactorNode>        first;
  vector<Token>                 ops;    // MULTIPLY, DIVIDE, or MOD
  vector<unique_ptr<FactorNode>> rest;

  void print_tree(ostream& os, string prefix, bool last) {
    if (ops.empty()) {
      first->print_tree(os, prefix, last);
      return;
    }
    // build label for first op
    string opLabel = "Binary ";
    opLabel += tokName(ops[0]);
    ast_line(os, prefix, last, opLabel);
    string indent = prefix + (last ? "    " : "│   ");
    first->print_tree(os, indent, false);
    for (size_t i = 0; i < rest.size(); i++)
      rest[i]->print_tree(os, indent, i == rest.size() - 1);
  }

  ExprVal interpret(ostream& out) {
    ExprVal result = first->interpret(out);

    for (size_t i = 0; i < ops.size(); i++) {
      ExprVal rhs = rest[i]->interpret(out);

      if (ops[i] == MOD) {
        // MOD only works on integers
        if (isDouble(result) || isDouble(rhs)) {
          throw runtime_error("Runtime error: MOD operator requires INTEGER operands");
        }
        int a = get<int>(result);
        int b = get<int>(rhs);
        if (b == 0) throw runtime_error("Runtime error: MOD by zero");
        result = a % b;
      } else if (ops[i] == MULTIPLY) {
        if (isDouble(result) || isDouble(rhs))
          result = toDouble(result) * toDouble(rhs);
        else
          result = get<int>(result) * get<int>(rhs);
      } else if (ops[i] == DIVIDE) {
        double denom = toDouble(rhs);
        if (denom == 0.0) throw runtime_error("Runtime error: division by zero");
        double num = toDouble(result);
        double q = num / denom;
        // if both were ints, store as int (integer division)
        if (!isDouble(result) && !isDouble(rhs))
          result = (int)q;
        else
          result = q;
      }
    }
    return result;
  }
};

// -----------------------------------------------------------------------------
// ValueNode — term { ( PLUS | MINUS ) term }
// -----------------------------------------------------------------------------
struct ValueNode {
  unique_ptr<TermNode>        first;
  vector<Token>               ops;   // PLUS or MINUS
  vector<unique_ptr<TermNode>> rest;

  void print_tree(ostream& os, string prefix, bool last) {
    if (ops.empty()) {
      first->print_tree(os, prefix, last);
      return;
    }
    string opLabel = "Binary ";
    opLabel += tokName(ops[0]);
    ast_line(os, prefix, last, opLabel);
    string indent = prefix + (last ? "    " : "│   ");
    first->print_tree(os, indent, false);
    for (size_t i = 0; i < rest.size(); i++)
      rest[i]->print_tree(os, indent, i == rest.size() - 1);
  }

  ExprVal interpret(ostream& out) {
    ExprVal result = first->interpret(out);
    for (size_t i = 0; i < ops.size(); i++) {
      ExprVal rhs = rest[i]->interpret(out);
      if (isDouble(result) || isDouble(rhs)) {
        double a = toDouble(result), b = toDouble(rhs);
        result = (ops[i] == PLUS) ? a + b : a - b;
      } else {
        int a = get<int>(result), b = get<int>(rhs);
        result = (ops[i] == PLUS) ? a + b : a - b;
      }
    }
    return result;
  }
};

// PrimaryNode::interpret defined here (after ValueNode is complete)
inline ExprVal PrimaryNode::interpret(ostream& out) {
  switch (kind) {
    case INT_LIT:   return intVal;
    case FLOAT_LIT: return floatVal;
    case IDENT_REF: {
      auto& slot = symbolTable[identName];
      return visit([](auto v) -> ExprVal { return v; }, slot);
    }
    case PAREN_EXPR:
      return parenExpr->interpret(out);
  }
  return 0; // unreachable
}

// =============================================================================
// Statement nodes — these perform actions (void interpret)
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
  string strContent;
  string varName;
  Token  tokenType;
  bool   isCymerica = false;

  void print_tree(ostream& os, string prefix, bool last) override {
    string indent = prefix + (last ? "    " : "│   ");
    ast_line(os, prefix, last, "Write Statement");
    if (tokenType == STRINGLIT)
      ast_line(os, indent, true, "content: '" + strContent + "'");
    else
      ast_line(os, indent, true, "content: " + varName);
  }

  void interpret(ostream& out) override {
    if (tokenType == STRINGLIT) {
      if (isCymerica) out << "[CYMERICA]: ";
      out << strContent << "\n";
    } else {
      auto& slot = symbolTable[varName];
      if (isCymerica) out << "[CYMERICA]: ";
      visit([&](auto&& value){ out << value << "\n"; }, slot);
    }
  }
};

// -----------------------------------------------------------------------------
// ReadStmt
// -----------------------------------------------------------------------------
struct ReadStmt : public Statement {
  string varName;

  void print_tree(ostream& os, string prefix, bool last) override {
    string indent = prefix + (last ? "    " : "│   ");
    ast_line(os, prefix, last, "Read Statement");
    ast_line(os, indent, true, "target: " + varName);
  }

  void interpret(ostream& /*out*/) override {
    auto& slot = symbolTable[varName];
    visit([&](auto& value){ cin >> value; }, slot);
  }
};

// -----------------------------------------------------------------------------
// AssignStmt — updated for Part 3: RHS is a full ValueNode
//              supports :=  +=  -=  *=  /=
// -----------------------------------------------------------------------------
struct AssignStmt : public Statement {
  string                id;       // LHS variable
  Token                 op;       // ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, MULT_ASSIGN, DIV_ASSIGN
  unique_ptr<ValueNode> rhs;      // full expression

  void print_tree(ostream& os, string prefix, bool last) override {
    string indent = prefix + (last ? "    " : "│   ");
    string opStr;
    switch (op) {
      case ASSIGN:       opStr = ":=";  break;
      case PLUS_ASSIGN:  opStr = "+=";  break;
      case MINUS_ASSIGN: opStr = "-=";  break;
      case MULT_ASSIGN:  opStr = "*=";  break;
      case DIV_ASSIGN:   opStr = "/=";  break;
      default:           opStr = "?";
    }
    ast_line(os, prefix, last, "Assign " + id + " " + opStr);
    rhs->print_tree(os, indent, true);
  }

  void interpret(ostream& out) override {
    ExprVal rhsVal = rhs->interpret(out);
    auto& lhs = symbolTable[id];

    // For compound ops, fetch current LHS value
    if (op != ASSIGN) {
      ExprVal lhsVal = visit([](auto v) -> ExprVal { return v; }, lhs);

      if (op == PLUS_ASSIGN) {
        if (isDouble(lhsVal) || isDouble(rhsVal))
          rhsVal = toDouble(lhsVal) + toDouble(rhsVal);
        else
          rhsVal = get<int>(lhsVal) + get<int>(rhsVal);
      } else if (op == MINUS_ASSIGN) {
        if (isDouble(lhsVal) || isDouble(rhsVal))
          rhsVal = toDouble(lhsVal) - toDouble(rhsVal);
        else
          rhsVal = get<int>(lhsVal) - get<int>(rhsVal);
      } else if (op == MULT_ASSIGN) {
        if (isDouble(lhsVal) || isDouble(rhsVal))
          rhsVal = toDouble(lhsVal) * toDouble(rhsVal);
        else
          rhsVal = get<int>(lhsVal) * get<int>(rhsVal);
      } else if (op == DIV_ASSIGN) {
        double denom = toDouble(rhsVal);
        if (denom == 0.0) throw runtime_error("Runtime error: division by zero");
        double num = toDouble(lhsVal);
        double q = num / denom;
        if (!isDouble(lhsVal) && !isDouble(rhsVal))
          rhsVal = (int)q;
        else
          rhsVal = q;
      }
    }

    // Store result into LHS, converting to the declared type
    visit([&](auto& slot) {
      using T = decay_t<decltype(slot)>;
      if (isDouble(rhsVal))
        slot = static_cast<T>(get<double>(rhsVal));
      else
        slot = static_cast<T>(get<int>(rhsVal));
    }, lhs);
  }
};

// -----------------------------------------------------------------------------
// SpawnStmt — SPAWN custom statement
// Note: Used AI assistance to help implement this, teacher allows as long as cited
// SPAWN dumps all current variable values so you can see the state of the program
// -----------------------------------------------------------------------------
struct SpawnStmt : public Statement {
  void print_tree(ostream& os, string prefix, bool last) override {
    ast_line(os, prefix, last, "Spawn Statement");
  }

  void interpret(ostream& out) override {
    out << "-- SPAWN: current variable values --\n";
    for (auto& [name, val] : symbolTable) {
      out << name << " = ";
      if (holds_alternative<int>(val))
        out << get<int>(val) << "\n";
      else
        out << get<double>(val) << "\n";
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
    string indent = prefix + (last ? "    " : "│   ");
    ast_line(os, prefix, last, "Compound Statement");
    for (size_t i = 0; i < stmts.size(); i++)
      stmts[i]->print_tree(os, indent, i == stmts.size() - 1);
  }

  void interpret(ostream& out) override {
    for (auto& s : stmts)
      s->interpret(out);
  }
};

// -----------------------------------------------------------------------------
// VarDecl
// -----------------------------------------------------------------------------
struct VarDecl {
  string name;
  Token  type;
};

// -----------------------------------------------------------------------------
// Block
// -----------------------------------------------------------------------------
struct Block {
  vector<VarDecl>          decls;
  unique_ptr<CompoundStmt> compound;

  void print_tree(ostream& os, string prefix, bool last) {
    string indent = prefix + (last ? "    " : "│   ");
    ast_line(os, prefix, last, "Block");
    if (!decls.empty()) {
      bool compExists = (compound != nullptr);
      ast_line(os, indent, !compExists, "declarations:");
      string dIndent = indent + (!compExists ? "    " : "│   ");
      for (size_t i = 0; i < decls.size(); i++) {
        string typeStr = (decls[i].type == INTEGER) ? "INTEGER = 0" : "REAL = 0.000000";
        ast_line(os, dIndent, i == decls.size() - 1,
                 decls[i].name + " : " + typeStr);
      }
    }
    if (compound) compound->print_tree(os, indent, true);
  }

  void interpret(ostream& out) {
    for (auto& d : decls) {
      if (d.type == INTEGER)
        symbolTable[d.name] = (int)0;
      else
        symbolTable[d.name] = (double)0.0;
    }
    if (compound) compound->interpret(out);
  }
};

// -----------------------------------------------------------------------------
// Program — root node
// -----------------------------------------------------------------------------
struct Program {
  string name;
  unique_ptr<Block> block;

  void print_tree(ostream& os) {
    os << "Program\n";
    ast_line(os, "", false, "name: " + name);
    if (block) block->print_tree(os, "", true);
  }

  void interpret(ostream& out) {
    if (block) block->interpret(out);
  }
};
