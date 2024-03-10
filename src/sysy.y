%code requires {
	#include <memory>
	#include <string>
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast_defs.hpp"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
	std::string *str_val;
	int int_val;
	BaseAST* ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast_val> FuncDef FuncType Block Stmt
%type <int_val> Number

%%

CompUnit
	: FuncDef {
		auto comp_unit = std::make_unique<CompUnitAST>();
		comp_unit->func_def = std::unique_ptr<BaseAST>($1);
		ast = std::move(comp_unit);
	}
	;

FuncDef
	: FuncType IDENT '(' ')' Block {
		auto ast = new FuncDefAST();
		ast->func_typ = std::unique_ptr<BaseAST>($1);
		ast->ident = *std::unique_ptr<std::string>($2);
		ast->block = std::unique_ptr<BaseAST>($5);
		$$ = ast;
	}
	;

FuncType
	: INT {
		auto ast = new TypAST();
		ast->typ = "InT";
		$$ = ast;
	}
	;

Block
	: '{' Stmt '}' {
		auto ast = new BlockAST();
		ast->stmt = std::unique_ptr<BaseAST>($2);
		$$ = ast;
	}
	;

Stmt
	: RETURN Number ';' {
		auto ast = new StmtAST();
		ast->ret_val = $2;
		$$ = ast;
	}
	;

Number
	: INT_CONST {
		$$ = $1;
	}
	;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
	std::cerr << "error: " << s << std::endl;
}
