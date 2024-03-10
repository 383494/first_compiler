#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>

extern FILE *yyin;

#include "ast_defs.hpp"
#include "sysy.tab.hpp"

int main(int argc, const char *argv[]) {
	assert(argc == 5);
	auto inp = argv[2];

	yyin = fopen(inp, "r");
	assert(yyin);

	// parse input file
	std::unique_ptr<BaseAST> ast;
	auto ret = yyparse(ast);
	assert(!ret);

	ast->output("");
	// dump AST
	return 0;
}
