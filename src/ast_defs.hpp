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

class CompUnitAST;
class FuncDefAST;
class TypAST;
class BlockAST;
class StmtAST;
class ExpAST;
class UnaryExpAST;
class PrimaryExpAST;
class UnaryOpAST;

class CompUnitAST: public BaseAST{
public:
	std::unique_ptr<FuncDefAST> func_def;
	void output(Ost &outstr, std::string prefix) const override;
};

class FuncDefAST: public BaseAST{
public:
	std::unique_ptr<TypAST> func_typ;
	std::string ident;
	std::unique_ptr<BlockAST> block;
	void output(Ost &outstr, std::string prefix) const override;
};

class TypAST: public BaseAST{
public:
	std::string typ;
	void output(Ost &outstr, std::string) const override;
};

class BlockAST: public BaseAST{
public:
	std::unique_ptr<StmtAST> stmt;
	void output(Ost &outstr, std::string prefix) const override;
};

class StmtAST: public BaseAST{
public:
	std::unique_ptr<ExpAST> ret_val;
	void output(Ost &outstr, std::string prefix) const override;
};

class ExpAST: public BaseAST{
public:
	std::unique_ptr<UnaryExpAST> unary_exp;
	void output(Ost& outstr, std::string prefix) const override;
};

class UnaryExpAST: public BaseAST{
public:
	std::optional<std::unique_ptr<UnaryOpAST>> unary_op;
	std::unique_ptr<BaseAST> unary_exp;
	// UnartExpAST or PrimaryExpAST
	void output(Ost &outstr, std::string prefix) const override;
};

class PrimaryExpAST: public BaseAST{
public:
	std::variant<std::unique_ptr<ExpAST>, int> inside_exp;
	void output(Ost &outstr, std::string prefix) const override;
};

class UnaryOpAST: public BaseAST{
public:
	Op_type op;
	UnaryOpAST(Op_type typ){ op = typ; }
	void output(Ost &outstr, std::string prefix) const override;
};

} // namespace Ast_Defs

} // namespace Ast_Base

using namespace Ast_Base::Ast_Defs;

