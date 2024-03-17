#pragma once
#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>

namespace Ast_Base{

constexpr const char* INDENT = "  ";
using Ost = std::ostringstream;

extern int var_count;

namespace Ast_Defs {

enum Op_type{
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_NOT,
};


class BaseAST{
public:
	virtual ~BaseAST() = default;
	virtual void output(Ost&, std::string) const = 0;
};

class CompUnitAST: public BaseAST{
public:
	std::unique_ptr<BaseAST> func_def;
	void output(Ost &outstr, std::string prefix) const override{
		//outstr << prefix << "CompUnit{\n";
		func_def->output(outstr, prefix);
		//outstr << '\n' << prefix << "}";
	}
};

class FuncDefAST: public BaseAST{
public:
	std::unique_ptr<BaseAST> func_typ;
	std::string ident;
	std::unique_ptr<BaseAST> block;
	void output(Ost &outstr, std::string prefix) const override{
		outstr << prefix << "fun @" << ident << "():";
		func_typ->output(outstr, "");
		outstr << "{\n";
		block->output(outstr, prefix + INDENT);
		outstr << "\n" << prefix << "}\n";
	}
};

class TypAST: public BaseAST{
public:
	std::string typ;
	void output(Ost &outstr, std::string) const override{
		outstr << typ;
	}
};

class BlockAST: public BaseAST{
public:
	std::unique_ptr<BaseAST> stmt;
	void output(Ost &outstr, std::string prefix) const override{
		outstr << prefix << "%entry:\n";
		stmt->output(outstr, prefix + INDENT);
		//outstr << '\n' << prefix << "}";
	}
};

class StmtAST: public BaseAST{
public:
	std::unique_ptr<BaseAST> ret_val;
	void output(Ost &outstr, std::string prefix) const override{
		ret_val->output(outstr, prefix);
		outstr << prefix << "ret %" << var_count-1;
	}
};

class ExpAST: public BaseAST{
public:
	std::unique_ptr<BaseAST> unary_exp;
	void output(Ost& outstr, std::string prefix) const override{
		unary_exp->output(outstr, prefix);
	}
};

class UnaryExpAST: public BaseAST{
public:
	std::optional<std::unique_ptr<BaseAST>> unary_op;
	std::unique_ptr<BaseAST> unary_exp;
	void output(Ost &outstr, std::string prefix) const override{
		if(unary_op.has_value()){
			unary_exp->output(outstr, prefix);
			int now_var = var_count;
			var_count++;
			outstr << prefix << "%" << now_var << " = ";
			(*unary_op)->output(outstr, "");
			outstr << "%" << now_var-1 << "\n";
		} else {
			unary_exp->output(outstr, prefix);
		}
	}
};

class PrimaryExpAST: public BaseAST{
public:
	std::variant<std::unique_ptr<BaseAST>, int> inside_exp;
	void output(Ost &outstr, std::string prefix) const override{
		switch(inside_exp.index()){
			case 0:
				std::get<0>(inside_exp)->output(outstr, prefix);
			break;
			case 1:
				outstr << prefix << "%" << var_count << " = "
					<< std::get<1>(inside_exp) << "\n";
				var_count++;
			break;
			default:;
		}
	}
};

class UnaryOpAST: public BaseAST{
public:
	Op_type op;
	UnaryOpAST(Op_type typ){
		op = typ;
	}
	void output(Ost &outstr, std::string prefix) const override{
		outstr << prefix;
		switch(op){
			case OP_ADD:
				// do nothing
			break;
			case OP_SUB:
				outstr << "sub 0, ";
			break;
			case OP_NOT:
				outstr << "ne 0, ";
			break;
			default:
				throw "UnaryOp type error";
			break;
		}
	}
};

} // namespace Ast_Defs

} // namespace Ast_Base

using namespace Ast_Base::Ast_Defs;

