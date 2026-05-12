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
#include <iostream>
using namespace std;

// -----------------------------------------------------------------------------
// Pretty printer
// -----------------------------------------------------------------------------
// Note: Used AI assistance to help implement the struct definitions, but teacher allows it as long as I cite
inline void ast_line(ostream& os, string prefix, bool last, string label) {
  os << prefix << (last ? "└── " : "├── ") << label << "\n";
}

// TODO: Define and Implement structures to hold each data node!
// Tip: Build with the root of your tree as the lowest struct in this file
//      Implement each higher node in the tree HIGHER up in this file than its children
//      i.e. The root struct at the bottom of the file
//           The leaves of the tree toward the top of the file

// abstract base class for all statements
struct Statement {
  virtual void print_tree(ostream& os, string prefix, bool last) = 0;
  virtual void interpret(ostream& out) = 0;
  virtual ~Statement() = default;
};

// write statement - prints a string
struct WriteStmt : public Statement {
  string value;

  void print_tree(ostream& os, string prefix, bool last) override {
    string indent = prefix + (last ? "    " : "│   ");
    ast_line(os, prefix, last, "Write Statement");
    ast_line(os, indent, true, "Output: " + value);
  }

  void interpret(ostream& out) override {
    out << value << "\n";
  }
};

// compound statement - holds a list of statements inside BEGIN END
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

// TODO: Finish this struct for Block
struct Block {
  // TODO: Declare Any Member Variables
  unique_ptr<CompoundStmt> compound;

  // Member Function to Print
  void print_tree(ostream& os, string prefix, bool last) {
    string indent = prefix + (last ? "    " : "│   ");
    ast_line(os, prefix, last, "Block");
    if (compound) compound->print_tree(os, indent, true);
  }

  // Member Function to Interpret
  void interpret(ostream& out) {
    if (compound) compound->interpret(out);
  }
};

// You do not need to edit this struct, but can if you choose
struct Program {
  string name;
  unique_ptr<Block> block;

  void print_tree(ostream& os) {
    cout << "Program\n";
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