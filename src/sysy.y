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

%token INT RETURN CONST
%token ADD SUB MUL DIV MOD
%token LE LT GE GT EQ NEQ
// logic and & or & not
%token LNOT LAND LOR
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast_val> FuncDef FuncType Block BlockItemList BlockItem Stmt 
%type <ast_val> Decl ConstDecl BType ConstDef ConstDefList ConstInitVal LVal
%type <ast_val> PrimaryExp ConstExp Exp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp

// All below are BINARY operators.
%type <ast_val> AddOp SubOp MulOp DivOp ModOp
%type <ast_val> LeOp LtOp GeOp GtOp EqOp NeqOp
%type <ast_val> LAndOp LOrOp

%type <ast_val> UnaryOp Lv0Op Lv1Op Lv2Op Lv3Op Lv4Op Lv5Op
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
	: '{' BlockItemList '}' {
		$$ = $2;
	}
	;

BlockItemList
	: BlockItemList BlockItem {
		((BlockAST*)$1)->items.push_back(cast_ast<BlockItemAST>($2));
		$$ = $1;
	} | BlockItem {
		auto ast = new BlockAST();
		ast->items.push_back(cast_ast<BlockItemAST>($1));
		$$ = ast;
	}
	;

BlockItem:
	Decl {
		auto ast = new BlockItemAST();
		ast->item = cast_ast<DeclAST>($1);
		$$ = ast;
	} | Stmt {
		auto ast = new BlockItemAST();
		ast->item = cast_ast<StmtAST>($1);
		$$ = ast;
	};
	
Decl:
	ConstDecl;

ConstDecl:
	CONST BType ConstDefList ';' {
		((ConstDeclAST*)$3)->typ = cast_ast<BTypeAST>($2);
		$$ = $3;
	};

ConstDefList:
	ConstDefList ConstDef {
		auto ast = (ConstDeclAST*)$1;
		ast->defs.push_back(cast_ast<ConstDefAST>($2));
		$$ = ast;
	} | ConstDef {
		auto ast = new ConstDeclAST();
		ast->defs.push_back(cast_ast<ConstDefAST>($1));
		$$ = ast;
	};

ConstDef:
	IDENT '=' ConstInitVal {
		auto ast = new ConstDefAST();
		ast->ident = *$1;
		ast->val = cast_ast<ConstInitValAST>($3);
		$$ = ast;
	};

ConstInitVal:
	ConstExp;

ConstExp:
	Exp;

BType:
	INT {
		auto ast = new BTypeAST();
		$$ = ast;
	};

Stmt
	: RETURN Exp ';' {
		auto ast = new StmtAST();
		ast->ret_val = cast_ast<ExpAST>($2);
		$$ = ast;
	}
	;

LVal:
	IDENT {
		auto ast = new LValAST();
		ast->ident = *$1;
		$$ = ast;
	};

Exp:
	LOrExp {
		auto ast = new ExpAST();
		ast->binary_exp = cast_ast<LOrExpAST>($1);
		$$ = ast;
	}
	;

PrimaryExp:
	'(' Exp ')' { 
		auto ast = new PrimaryExpAST();
		ast->inside_exp = cast_ast<ExpAST>($2);
		$$ = ast;
	}
	| LVal {
		auto ast = new PrimaryExpAST();
		ast->inside_exp = cast_ast<LValAST>($1);
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

AddOp: ADD		{ $$ = new BinaryOpAST(OP_ADD); } ;
SubOp: SUB		{ $$ = new BinaryOpAST(OP_SUB); } ;
MulOp: MUL		{ $$ = new BinaryOpAST(OP_MUL); } ;
DivOp: DIV		{ $$ = new BinaryOpAST(OP_DIV); } ;
ModOp: MOD		{ $$ = new BinaryOpAST(OP_MOD); } ;
GtOp: GT 		{ $$ = new BinaryOpAST(OP_GT); } ;
GeOp: GE 		{ $$ = new BinaryOpAST(OP_GE); } ;
LtOp: LT 		{ $$ = new BinaryOpAST(OP_LT); } ;
LeOp: LE 		{ $$ = new BinaryOpAST(OP_LE); } ;
EqOp: EQ 		{ $$ = new BinaryOpAST(OP_EQ); } ;
NeqOp: NEQ 		{ $$ = new BinaryOpAST(OP_NEQ); } ;
LAndOp: LAND 	{ $$ = new BinaryOpAST(OP_LAND); } ;
LOrOp: LOR 		{ $$ = new BinaryOpAST(OP_LOR); } ;


UnaryOp: 
	ADD 	{ $$ = new UnaryOpAST(OP_ADD); }
	| SUB 	{ $$ = new UnaryOpAST(OP_SUB); }
	| LNOT 	{ $$ = new UnaryOpAST(OP_LNOT); }
	;

Lv0Op: MulOp | DivOp | ModOp ;
Lv1Op: AddOp | SubOp ;
Lv2Op: GeOp | GtOp | LeOp | LtOp ;
Lv3Op: EqOp | NeqOp ;
Lv4Op: LAndOp ;
Lv5Op: LOrOp ;

MulExp:
	UnaryExp {
		auto ast = new MulExpAST();
		ast->nxt_level = cast_ast<UnaryExpAST>($1);
		$$ = ast;
	}
	| MulExp Lv0Op UnaryExp {
		auto ast = new MulExpAST();
		ast->now_level = cast_ast<MulExpAST>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<UnaryExpAST>($3);
		$$ = ast;
	}
	;

AddExp:
	MulExp
	| AddExp Lv1Op MulExp {
		auto ast = new AddExpAST();
		ast->now_level = cast_ast<AddExpAST>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<MulExpAST>($3);
		$$ = ast;
	} 
	;

RelExp:
	AddExp
	| RelExp Lv2Op AddExp {
		auto ast = new RelExpAST();
		ast->now_level = cast_ast<RelExpAST>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<AddExpAST>($3);
		$$ = ast;
	}
	;

EqExp:
	RelExp
	| EqExp Lv3Op RelExp {
		auto ast = new EqExpAST();
		ast->now_level = cast_ast<EqExpAST>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<RelExpAST>($3);
		$$ = ast;
	};

LAndExp:
	EqExp
	| LAndExp Lv4Op EqExp {
		auto ast = new LAndExpAST();
		ast->now_level = cast_ast<LAndExpAST>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<EqExpAST>($3);
		$$ = ast;
	};

LOrExp:
	LAndExp
	| LOrExp Lv5Op LAndExp {
		auto ast = new LOrExpAST();
		ast->now_level = cast_ast<LOrExpAST>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<LAndExpAST>($3);
		$$ = ast;
	};


Number
	: INT_CONST
	;

%%

void yyerror(std::unique_ptr<BaseAST> &ast, const char *err_info) {
	std::cerr << "yyerror: " << err_info << std::endl;
}
