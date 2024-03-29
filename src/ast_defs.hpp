#pragma once
#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>

namespace Ast_Base {

constexpr const char *INDENT = "  ";
using Ost = std::ostringstream;
constexpr int BINARY_EXP_MAX_LEVEL = 5;

namespace Ast_Defs {

enum Op_type {
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_GT,
	OP_GE,
	OP_LT,
	OP_LE,
	OP_EQ,
	OP_NEQ,
	OP_LAND,
	OP_LOR,
	OP_LNOT
};

class BaseAST {
public:
	virtual ~BaseAST() = default;
	virtual void output(Ost &, std::string) const = 0;
};

class OpAST : public BaseAST {
public:
	Op_type op;
	OpAST(Op_type typ) { op = typ; }
};

class CompUnitAST;
class FuncDefAST;
class TypAST;
class BlockAST;
class StmtAST;
class ExpAST;
class UnaryExpAST;
class UnaryOpAST;
class PrimaryExpAST;
class BinaryOpAST;
template<int>
class BinaryExpAST;

class CompUnitAST : public BaseAST {
public:
	std::unique_ptr<FuncDefAST> func_def;
	void output(Ost &outstr, std::string prefix) const override;
};

class FuncDefAST : public BaseAST {
public:
	std::unique_ptr<TypAST> func_typ;
	std::string ident;
	std::unique_ptr<BlockAST> block;
	void output(Ost &outstr, std::string prefix) const override;
};

class TypAST : public BaseAST {
public:
	std::string typ;
	void output(Ost &outstr, std::string) const override;
};

class BlockAST : public BaseAST {
public:
	std::unique_ptr<StmtAST> stmt;
	void output(Ost &outstr, std::string prefix) const override;
};

class StmtAST : public BaseAST {
public:
	std::unique_ptr<ExpAST> ret_val;
	void output(Ost &outstr, std::string prefix) const override;
};

class ExpAST : public BaseAST {
public:
	std::unique_ptr<BinaryExpAST<BINARY_EXP_MAX_LEVEL>> binary_exp;
	void output(Ost &outstr, std::string prefix) const override;
};

class UnaryExpAST : public BaseAST {
public:
	std::optional<std::unique_ptr<UnaryOpAST>> unary_op;
	std::unique_ptr<BaseAST> unary_exp;
	// UnartExpAST or PrimaryExpAST
	void output(Ost &outstr, std::string prefix) const override;
};

class PrimaryExpAST : public BaseAST {
public:
	std::variant<std::unique_ptr<ExpAST>, int> inside_exp;
	void output(Ost &outstr, std::string prefix) const override;
};

class UnaryOpAST : public OpAST {
public:
	using OpAST::OpAST;
	void output(Ost &outstr, std::string prefix) const override;
};

class BinaryOpAST : public OpAST {
public:
	using OpAST::OpAST;
	bool is_logic_op();
	void output(Ost &outstr, std::string prefix) const override;
};

template<typename Now_Level_Type, typename Nxt_Level_Type>
class BinaryExpAST_Base : public BaseAST {
public:
	std::optional<std::unique_ptr<Now_Level_Type>> now_level;
	std::optional<std::unique_ptr<BinaryOpAST>> binary_op;
	std::unique_ptr<Nxt_Level_Type> nxt_level;
	void output(Ost &outstr, std::string prefix) const override;
};

template<int level>
class BinaryExpAST : public BinaryExpAST_Base<BinaryExpAST<level>, BinaryExpAST<level - 1>> {
public:
	static void instantiate() {
		BinaryExpAST<level> initer;
		BinaryExpAST<level - 1>::instantiate();
	}
};

template<>
class BinaryExpAST<0> : public BinaryExpAST_Base<BinaryExpAST<0>, UnaryExpAST> {
public:
	static void instantiate() {}
};

using MulExpAST = BinaryExpAST<0>;
using AddExpAST = BinaryExpAST<1>;
using RelExpAST = BinaryExpAST<2>;
using EqExpAST  = BinaryExpAST<3>;
using LAndExpAST = BinaryExpAST<4>;
using LOrExpAST = BinaryExpAST<5>;

}   // namespace Ast_Defs

}   // namespace Ast_Base

using namespace Ast_Base::Ast_Defs;
