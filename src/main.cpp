#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
// #include <argp.h>
#include "koopa.h"
#include <getopt.h>
#include <unistd.h>

extern FILE *yyin;

#include "ast_defs.hpp"
#include "ir.hpp"
#include "sysy.tab.hpp"

extern char *optarg;
extern int optind, opterr, optopt;
static const struct option long_opt_args[] = {
	{"koopa", no_argument, NULL, 1001},
	{"k", no_argument, NULL, 1001},
	{"riscv", no_argument, NULL, 1002},
	{"r", no_argument, NULL, 1002},
	{"output", required_argument, NULL, 1003},
	{"o", required_argument, NULL, 1003},
	{0, 0, 0, 0}};

bool output_koopa = false;

int main(int argc, char **argv) {
	int now_opt = 0;
	std::string outp;
	while((now_opt = getopt_long_only(argc, argv, "", long_opt_args, NULL)) != -1) {
		switch(now_opt) {
		case 1001:
			output_koopa = true;
			break;
		case 1002:
			output_koopa = false;
			break;
		case 1003:
			outp = optarg;
			break;
		case '?':
			std::cerr << "I can't understand, so...\n";
			std::cerr << "world.execute(me);\n";
			return 114514;
		default:
			assert(0);
		}
	}

	yyin = fopen(argv[optind], "r");
	assert(yyin);

	std::unique_ptr<BaseAST> ast;
	do {
		auto ret = yyparse(ast);
		assert(!ret);
	} while(0);

	std::string outstr;
	std::ostringstream outstrbuf;

	ast->output(outstrbuf, "");
	outstr = outstrbuf.str();

	if(output_koopa) {
		if(outp.empty()) {
			std::cout << outstr;
		} else {
			std::ofstream outp_stream(outp);
			outp_stream << outstr;
		}
		return 0;
	}

	koopa_raw_program_t raw_prog;
	koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
	do {
		koopa_program_t program;
		koopa_error_code_t ret = koopa_parse_from_string(outstr.c_str(), &program);
		assert(ret == KOOPA_EC_SUCCESS);   // 确保解析时没有出错
		raw_prog = koopa_build_raw_program(builder, program);
		koopa_delete_program(program);
	} while(0);

	outstrbuf.str("");
	dfs_ir(raw_prog, outstrbuf);
	outstr = outstrbuf.str();

	if(outp.empty()) {
		std::cout << outstr;
	} else {
		std::ofstream outp_stream(outp);
		outp_stream << outstr;
	}

	koopa_delete_raw_program_builder(builder);
	return 0;
}
