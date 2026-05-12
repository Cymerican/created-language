# =============================================================================
# Makefile — TIPS interpreter skeleton
# Author: Derek Willis (MSU CSE Fall 2025)
#
# Builds `parse` from:
#   • rules.l -> (flex) -> lex.yy.c -> lex.yy.o
#   • parser.cpp -> parser.o
#   • driver.cpp -> driver.o
#   • debug.cpp  -> debug.o
# Usage: `make` to build, `make clean` to remove outputs.
# Tip: swap -O2 for -Og -g in CXXFLAGS for GNU debug builds.
# =============================================================================

CXX      := g++
CXXFLAGS := -std=gnu++17 -Wall -Wextra -O2

.PHONY: all clean
all: parse

# Generate scanner source with Flex
lex.yy.c: rules.l lexer.h
	flex rules.l

# Compile objects
lex.yy.o: lex.yy.c lexer.h
	$(CXX) $(CXXFLAGS) -c lex.yy.c -o $@

parser.o: parser.cpp lexer.h ast.h debug.h
	$(CXX) $(CXXFLAGS) -c parser.cpp -o $@

driver.o: driver.cpp lexer.h ast.h debug.h
	$(CXX) $(CXXFLAGS) -c driver.cpp -o $@

# Link executable
parse: lex.yy.o parser.o driver.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Clean build artifacts
clean:
	rm -f parse *.o lex.yy.c
