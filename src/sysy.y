%code requires {
	#include <memory>
	#include <string>
}


%{

#include <iostream>
#include <memory>
#include <string>
#include "ast_defs.hpp"

int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

template<typename T>
std::unique_ptr<T> cast_ast(BaseAST *p){
   return std::unique_ptr<T>((T*)p);
}


%}

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
	std::string *str_val;
	int int_val;
	BaseAST* ast_val;
}

%token IF ELSE
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%token VOID INT 
%token RETURN CONST
%token ADD SUB MUL DIV MOD
%token LE LT GE GT EQ NEQ
// logic and & or & not
%token LNOT LAND LOR
%token WHILE
%token BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast_val> CompUnit FuncDef FuncDefParams FuncDefParam FuncCallParams
%type <ast_val> Block BlockItemList BlockItem Stmt 
%type <ast_val> If
%type <ast_val> Decl ConstDecl VarDecl Type ConstDef ConstDefList ConstInitVal VarDef VarDefList InitVal LVal
%type <ast_val> PrimaryExp ConstExp Exp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp

// BINARY operators.
%type <ast_val> AddOp SubOp MulOp DivOp ModOp
%type <ast_val> LeOp LtOp GeOp GtOp EqOp NeqOp
%type <ast_val> LAndOp LOrOp

%type <ast_val> UnaryOp Lv0Op Lv1Op Lv2Op Lv3Op Lv4Op Lv5Op
%type <int_val> Number

%%

Compile_loader:
	CompUnit {
		ast = cast_ast<CompUnitAST>($1);
	}
	;

CompUnit:
	{
		$$ = new CompUnitAST();
	} | CompUnit Decl {
		auto ast = (CompUnitAST*)$1;
		ast->decls.push_back(cast_ast<DeclAST>($2));
		$$ = ast;
	} | CompUnit FuncDef {
		auto ast = (CompUnitAST*)$1;
		ast->decls.push_back(cast_ast<FuncDefAST>($2));
		$$ = ast;
	}
	;

FuncDef
	: Type IDENT '(' FuncDefParams ')' Block {
		auto ast = new FuncDefAST();
		ast->func_typ = cast_ast<TypeAST>($1);
		ast->ident = *std::unique_ptr<std::string>($2);
		ast->params = cast_ast<FuncDefParamsAST>($4);
		ast->block = cast_ast<BlockAST>($6);
		$$ = ast;
	} | Type IDENT '(' ')' Block {
		auto ast = new FuncDefAST();
		ast->func_typ = cast_ast<TypeAST>($1);
		ast->ident = *std::unique_ptr<std::string>($2);
		ast->block = cast_ast<BlockAST>($5);
		$$ = ast;
	}
	;

Type
	: INT {
		auto ast = new TypeAST();
		ast->typ = "i32";
		$$ = ast;
	} | VOID {
		auto ast = new TypeAST();
		ast->typ = "void";
		ast->is_void = true;
		$$ = ast;
	}
	;

FuncDefParams:
	FuncDefParams ',' FuncDefParam {
		auto ast = (FuncDefParamsAST*)$1;
		auto param = (FuncDefParamsAST*)$3;
		ast->params.splice(ast->params.end(), param->params);
		delete param;
		$$ = ast;
	} | FuncDefParam {
		$$ = $1;
	}
	;

FuncDefParam:
	Type IDENT {
		auto ast = new FuncDefParamsAST();
		ast->params.push_back({cast_ast<TypeAST>($1), *$2});
		$$ = ast;
	}
	;

FuncCallParams:
	FuncCallParams ',' Exp {
		auto ast = (FuncCallParamsAST *)$1;
		ast->params.push_back(cast_ast<ExpAST>($3));
		$$ = ast;
	} | Exp {
		auto ast = new FuncCallParamsAST();
		ast->params.push_back(cast_ast<ExpAST>($1));
		$$ = ast;
	}
	;

Block
	: '{' BlockItemList '}' {
		$$ = $2;
	} | '{' '}' {
		$$ = new BlockAST();
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
	ConstDecl {
		auto ast = new DeclAST();
		ast->decl = cast_ast<ConstDeclAST>($1);
		$$ = ast;
	} | VarDecl {
		auto ast = new DeclAST();
		ast->decl = cast_ast<VarDeclAST>($1);
		$$ = ast;
	};

ConstDecl:
	CONST Type ConstDefList ';' {
		((ConstDeclAST*)$3)->typ = cast_ast<TypeAST>($2);
		$$ = $3;
	};

ConstDefList:
	ConstDefList ',' ConstDef {
		auto ast = (ConstDeclAST*)$1;
		ast->defs.push_back(cast_ast<ConstDefAST>($3));
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
	ConstExp {
		auto ast = new ConstInitValAST();
		ast->exp = cast_ast<ConstExpAST>($1);
		$$ = ast;
	};

ConstExp:
	Exp {
		auto ast = new ConstExpAST();
		ast->exp = cast_ast<ExpAST>($1);
		$$ = ast;
	};

VarDecl:
	Type VarDefList ';' {
	// TODO
		auto ast = (VarDeclAST*)$2;
		ast->typ = cast_ast<TypeAST>($1);
		$$ = ast;
	};

VarDefList:
	VarDefList ',' VarDef {
		auto ast = (VarDeclAST*)$1;
		ast->defs.push_back(cast_ast<VarDefAST>($3));
		$$ = ast;
	} | VarDef {
		auto ast = new VarDeclAST();
		ast->defs.push_back(cast_ast<VarDefAST>($1));
		$$ = ast;
	};

VarDef:
	IDENT {
		auto ast = new VarDefAST();
		ast->ident = *$1;
		$$ = ast;
	} | IDENT '=' InitVal {
		auto ast = new VarDefAST();
		ast->ident = *$1;
		ast->val = cast_ast<InitValAST>($3);
		$$ = ast;
	};

InitVal: 
	Exp {
		auto ast = new InitValAST();
		ast->exp = cast_ast<ExpAST>($1);
		$$ = ast;
	};


Stmt:
	RETURN ';' {
		auto ast = new StmtAST();
		auto ret_ast = std::make_unique<ReturnAST>();
		ret_ast->exp = std::make_unique<OptionalExpAST>();
		ast->val = std::move(ret_ast);
		$$ = ast;
	} | RETURN Exp ';' {
		auto ast = new StmtAST();
		auto ret_ast = std::make_unique<ReturnAST>();
		ret_ast->exp = std::make_unique<OptionalExpAST>();
		ret_ast->exp->exp = cast_ast<ExpAST>($2);
		ast->val = std::move(ret_ast);
		$$ = ast;
	} | LVal '=' Exp ';' {
		auto ast = new StmtAST();
		auto assign_ast = std::make_unique<LValAssignAST>();
		assign_ast->lval = cast_ast<LValAST>($1);
		assign_ast->exp = cast_ast<ExpAST>($3);
		ast->val = std::move(assign_ast);
		$$ = ast;
	} | ';' {
		auto ast = new StmtAST();
		ast->val = std::make_unique<OptionalExpAST>();
		$$ = ast;
	} | Exp ';' {
		auto ast = new StmtAST();
		ast->val = cast_ast<OptionalExpAST>($1);
		$$ = ast;
	} | Block {
		auto ast = new StmtAST();
		ast->val = cast_ast<BlockAST>($1);
		$$ = ast;
	} | If {
		auto ast = new StmtAST();
		ast->val = cast_ast<IfAST>($1);
		$$ = ast;
	} | WHILE '(' Exp ')' Stmt {
		auto ast = new StmtAST();
		auto while_ast = new WhileAST();
		while_ast->cond = cast_ast<ExpAST>($3);
		while_ast->stmt = cast_ast<StmtAST>($5);
		ast->val = cast_ast<WhileAST>(while_ast);
		$$ = ast;
	} | BREAK ';' {
		auto ast = new StmtAST;
		ast->val = std::make_unique<BreakAST>();
		$$ = ast;
	} | CONTINUE ';' {
		auto ast = new StmtAST;
		ast->val = std::make_unique<ContinueAST>();
		$$ = ast;
	}; 

If:
	IF '(' Exp ')' Stmt %prec LOWER_THAN_ELSE{
		auto ast = new IfAST();
		ast->cond = cast_ast<ExpAST>($3);
		ast->if_stmt = cast_ast<StmtAST>($5);
		ast->set_if_cnt();
		$$ = ast;
	} | IF '(' Exp ')' Stmt ELSE Stmt {
		auto ast = new IfAST();
		ast->cond = cast_ast<ExpAST>($3);
		ast->if_stmt = cast_ast<StmtAST>($5);
		ast->else_stmt = cast_ast<StmtAST>($7);
		ast->set_if_cnt();
		$$ = ast;
	};



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
		ast->unary_exp = cast_ast<PrimaryExpAST>($1);
		$$ = ast;
	} | IDENT '(' ')' {
		auto ast = new FuncCallAST();
		ast->func = *$1;
		ast->params = std::make_unique<FuncCallParamsAST>();
		$$ = ast;
	} | IDENT '(' FuncCallParams ')' {
		auto ast = new FuncCallAST();
		ast->func = *$1;
		ast->params = cast_ast<FuncCallParamsAST>($3);
		$$ = ast;
	} | UnaryOp UnaryExp{
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
	MulExp {
		auto ast = new AddExpAST();
		ast->nxt_level = cast_ast<MulExpAST>($1);
		$$ = ast;
	}
	| AddExp Lv1Op MulExp {
		auto ast = new AddExpAST();
		ast->now_level = cast_ast<AddExpAST>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<MulExpAST>($3);
		$$ = ast;
	} 
	;

RelExp:
	AddExp {
		auto ast = new RelExpAST();
		ast->nxt_level = cast_ast<AddExpAST>($1);
		$$ = ast;
	}
	| RelExp Lv2Op AddExp {
		auto ast = new RelExpAST();
		ast->now_level = cast_ast<RelExpAST>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<AddExpAST>($3);
		$$ = ast;
	}
	;

EqExp:
	RelExp {
		auto ast = new EqExpAST();
		ast->nxt_level = cast_ast<RelExpAST>($1);
		$$ = ast;
	}
	| EqExp Lv3Op RelExp {
		auto ast = new EqExpAST();
		ast->now_level = cast_ast<EqExpAST>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<RelExpAST>($3);
		$$ = ast;
	};

LAndExp:
	EqExp {
		auto ast = new LAndExpAST();
		ast->nxt_level = cast_ast<EqExpAST>($1);
		$$ = ast;
	}
	| LAndExp Lv4Op EqExp {
		auto ast = new LAndExpAST();
		ast->now_level = cast_ast<LAndExpAST>($1);
		ast->binary_op = cast_ast<BinaryOpAST>($2);
		ast->nxt_level = cast_ast<EqExpAST>($3);
		$$ = ast;
	};

LOrExp:
	LAndExp {
		auto ast = new LOrExpAST();
		ast->nxt_level = cast_ast<LAndExpAST>($1);
		$$ = ast;
	}
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
