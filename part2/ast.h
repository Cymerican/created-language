// =============================================================================
//   ast.h 
// =============================================================================
// MSU CSE 4714/6714 Capstone Project (Spring 2026)
// Author: Derek Willis
// =============================================================================
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <iostream>
#include <stdexcept>
using namespace std;

// -----------------------------------------------------------------------------
// Symbol Table — map<string, variant<int,double>> as shown in spec
// -----------------------------------------------------------------------------
// Note: Used AI assistance to help implement these structures, but teacher allows it as long as I cite

inline map<string, variant<int,double>> symbolTable;

// Helper: check if variable is declared
inline bool symDeclared(const string& name) {
  return symbolTable.count(name) > 0;
}

// -----------------------------------------------------------------------------
// Pretty printer helper
// -----------------------------------------------------------------------------
inline void ast_line(ostream& os, string prefix, bool last, string label) {
  os << prefix << (last ? "└── " : "├── ") << label << "\n";
}

// -----------------------------------------------------------------------------
// Abstract base for all statements
// -----------------------------------------------------------------------------
struct Statement {
  virtual void print_tree(ostream& os, string prefix, bool last) = 0;
  virtual void interpret(ostream& out) = 0;
  virtual ~Statement() = default;
};

// -----------------------------------------------------------------------------
// WriteStmt — WRITE( stringlit | ident )
// -----------------------------------------------------------------------------
struct WriteStmt : public Statement {
  string strContent;   // string literal content (no quotes)
  string varName;      // variable name if writing a var
  Token  tokenType;    // STRINGLIT or IDENT
  bool   isCymerica = false;  // true if invoked with CYMERICA keyword

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
      if (isCymerica)
        out << "[CYMERICA]: " << strContent << "\n";
      else
        out << strContent << "\n";
    } else {
      // use visit lambda as shown in spec
      auto& slot = symbolTable[varName];
      if (isCymerica)
        visit([&](auto&& value){ out << "[CYMERICA]: " << value << endl; }, slot);
      else
        visit([&](auto&& value){ out << value << endl; }, slot);
    }
  }
};

// -----------------------------------------------------------------------------
// ReadStmt — READ( ident )
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
    // visit lambda: reads into correct type automatically
    visit([&](auto& value){ cin >> value; }, slot);
  }
};

// -----------------------------------------------------------------------------
// AssignStmt — ident := primary
// primary -> INTLIT | FLOATLIT | IDENT | IDENT + IDENT
// -----------------------------------------------------------------------------
struct AssignStmt : public Statement {
  string id;          // left-hand side variable name
  Token  valueType;   // INTLIT, FLOATLIT, or IDENT
  string value;       // the rhs token text (or left var for IDENT)
  string rhs2;        // right operand for addition (empty if not add)

  void print_tree(ostream& os, string prefix, bool last) override {
    string indent = prefix + (last ? "    " : "│   ");
    ast_line(os, prefix, last, "Assign Statement");
    ast_line(os, indent, false, "id: " + id);
    if (!rhs2.empty())
      ast_line(os, indent, true, "value: " + value + " + " + rhs2);
    else
      ast_line(os, indent, true, "value: " + value);
  }

  void interpret(ostream& /*out*/) override {
    // lhs slot — guaranteed to exist (parser checked)
    auto& lhs = symbolTable[id];

    if (!rhs2.empty()) {
      // addition: both must be in symbol table
      auto& r1 = symbolTable[value];
      auto& r2 = symbolTable[rhs2];
      double sum = visit([](auto v) -> double { return (double)v; }, r1)
                 + visit([](auto v) -> double { return (double)v; }, r2);
      visit([&](auto& slot) {
        using T = decay_t<decltype(slot)>;
        slot = static_cast<T>(sum);
      }, lhs);
    } else if (valueType == INTLIT) {
      visit([&](auto& slot) {
        using T = decay_t<decltype(slot)>;
        slot = static_cast<T>(stoi(value));
      }, lhs);
    } else if (valueType == FLOATLIT) {
      visit([&](auto& slot) {
        using T = decay_t<decltype(slot)>;
        slot = static_cast<T>(stod(value));
      }, lhs);
    } else { // IDENT
      auto& rhs = symbolTable[value];
      visit([&](auto& slot) {
        using T = decay_t<decltype(slot)>;
        slot = static_cast<T>(visit([](auto r) -> double { return (double)r; }, rhs));
      }, lhs);
    }
  }
};

// -----------------------------------------------------------------------------
// CompoundStmt — BEGIN stmt { ; stmt } END
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
// VarDecl — one declaration entry (name + type token)
// -----------------------------------------------------------------------------
struct VarDecl {
  string name;
  Token  type;   // INTEGER or REAL token
};

// -----------------------------------------------------------------------------
// Block — [ VAR decls ] compound
// -----------------------------------------------------------------------------
struct Block {
  vector<VarDecl>          decls;
  unique_ptr<CompoundStmt> compound;

  void print_tree(ostream& os, string prefix, bool last) {
    string indent = prefix + (last ? "    " : "│   ");
    ast_line(os, prefix, last, "Block");
    if (!decls.empty()) {
      // print declarations sub-tree
      bool compExists = (compound != nullptr);
      ast_line(os, indent, !compExists, "declarations");
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
    // Register variables in symbol table
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
    else {
      ast_line(os, "", true, "Block");
      ast_line(os, "    ", true, "(empty)");
    }
  }

  void interpret(ostream& out) {
    if (block) block->interpret(out);
  }
};
