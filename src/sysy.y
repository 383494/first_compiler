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

template<typename T>
std::unique_ptr<T> cast_ast(BaseAST *p){
   return std::unique_ptr<T>((T*)p);
}

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

%token INT RETURN
%token ADD SUB MUL DIV MOD NOT
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast_val> FuncDef FuncType Block Stmt
%type <ast_val> PrimaryExp Exp UnaryExp AddExp MulExp

// All of them below are BINARY operators.
%type <ast_val> AddOp SubOp MulOp DivOp ModOp

%type <ast_val> UnaryOp Lv0Op Lv1Op
%type <int_val> Number

%%

CompUnit
	: FuncDef {
		auto comp_unit = std::make_unique<CompUnitAST>();
		comp_unit->func_def = cast_ast<FuncDefAST>($1);
		ast = std::move(comp_unit);
	}
	;

FuncDef
	: FuncType IDENT '(' ')' Block {
		auto ast = new FuncDefAST();
		ast->func_typ = cast_ast<TypAST>($1);
		ast->ident = *std::unique_ptr<std::string>($2);
		ast->block = cast_ast<BlockAST>($5);
		$$ = ast;
	}
	;

FuncType
	: INT {
		auto ast = new TypAST();
		ast->typ = "i32";
		$$ = ast;
	}
	;

Block
	: '{' Stmt '}' {
		auto ast = new BlockAST();
		ast->stmt = cast_ast<StmtAST>($2);
		$$ = ast;
	}
	;

Stmt
	: RETURN Exp ';' {
		auto ast = new StmtAST();
		ast->ret_val = cast_ast<ExpAST>($2);
		$$ = ast;
	}
	;

Exp:
	AddExp {
		auto ast = new ExpAST();
		ast->binary_exp = cast_ast<BinaryExpAST<1>>($1);
		$$ = ast;
	}
	;

PrimaryExp:
	'(' Exp ')' { 
		auto ast = new PrimaryExpAST();
		ast->inside_exp = cast_ast<ExpAST>($2);
		$$ = ast;
	}
	| Number {
		auto ast = new PrimaryExpAST();
		ast->inside_exp = $1;
		$$ = ast;
	};

UnaryExp:
	PrimaryExp {
		auto ast = new UnaryExpAST();
		ast->unary_exp = cast_ast<UnaryExpAST>($1);
		$$ = ast;
	}
	| UnaryOp UnaryExp{
		auto ast = new UnaryExpAST();
		ast->unary_op = cast_ast<UnaryOpAST>($1);
		ast->unary_exp = cast_ast<UnaryExpAST>($2);
		$$ = ast;
	}
	;

AddOp: ADD	{ $$ = new BinaryOpAST(OP_ADD); } ;
SubOp: SUB	{ $$ = new BinaryOpAST(OP_SUB); } ;
MulOp: MUL	{ $$ = new BinaryOpAST(OP_MUL); } ;
DivOp: DIV	{ $$ = new BinaryOpAST(OP_DIV); } ;
ModOp: MOD	{ $$ = new BinaryOpAST(OP_MOD); } ;

UnaryOp: 
	ADD 	{ $$ = new UnaryOpAST(OP_ADD); }
	| SUB 	{ $$ = new UnaryOpAST(OP_SUB); }
	| NOT 	{ $$ = new UnaryOpAST(OP_NOT); }
	;

Lv1Op: AddOp | SubOp ;
Lv0Op: MulOp | DivOp | ModOp ;

MulExp:
	UnaryExp {
		auto ast = new BinaryExpAST<0>();
		ast->nxt_level = cast_ast<UnaryExpAST>($1);
		$$ = ast;
	}
	| MulExp Lv0Op UnaryExp {
		auto ast = new BinaryExpAST<0>();
		ast->now_level = cast_ast<BinaryExpAST<0>>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<UnaryExpAST>($3);
		$$ = ast;
	}
	;

AddExp:
	MulExp
	| AddExp Lv1Op MulExp {
		auto ast = new BinaryExpAST<1>();
		ast->now_level = cast_ast<BinaryExpAST<1>>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<BinaryExpAST<0>>($3);
		$$ = ast;
	} 
	;

Number
	: INT_CONST
	;

%%

void yyerror(std::unique_ptr<BaseAST> &ast, const char *err_info) {
	std::cerr << "yyerror: " << err_info << std::endl;
}
