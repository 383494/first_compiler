#pragma once
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_set>
#include <variant>

namespace Ast_Base {

constexpr const char *INDENT = "  ";
using Ost = std::ostringstream;
constexpr int BINARY_EXP_MAX_LEVEL = 5;

template<typename... Types>
using VariantAstPtr = std::variant<std::unique_ptr<Types>...>;

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

class BTypeAST;
class BinaryOpAST;
class BlockAST;
class BlockItemAST;
class CompUnitAST;
class ConstDeclAST;
class ConstDefAST;
class ConstExpAST;
class ConstInitValAST;
class DeclAST;
class ExpAST;
class FuncDefAST;
class InitValAST;
class LValAST;
class LValAssignAST;
class PrimaryExpAST;
class ReturnAST;
class StmtAST;
class TypAST;
class UnaryExpAST;
class UnaryOpAST;
class VarDeclAST;
class VarDefAST;

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

// Type of function.
class TypAST : public BaseAST {
public:
	std::string typ;
	void output(Ost &outstr, std::string) const override;
};

class BlockAST : public BaseAST {
public:
	std::list<std::unique_ptr<BlockItemAST>> items;
	void output(Ost &outstr, std::string prefix) const override;
};

class BlockItemAST : public BaseAST {
public:
	VariantAstPtr<DeclAST, StmtAST> item;
	void output(Ost &outstr, std::string prefix) const override;
};

class StmtAST : public BaseAST {
public:
	VariantAstPtr<ReturnAST, LValAssignAST> val;
	void output(Ost &outstr, std::string prefix) const override;
};

class ExpAST : public BaseAST {
public:
	std::unique_ptr<BinaryExpAST<BINARY_EXP_MAX_LEVEL>> binary_exp;
	int calc();
	void output(Ost &outstr, std::string prefix) const override;
};

class UnaryExpAST : public BaseAST {
public:
	std::optional<std::unique_ptr<UnaryOpAST>> unary_op;
	VariantAstPtr<UnaryExpAST, PrimaryExpAST> unary_exp;
	void output(Ost &outstr, std::string prefix) const override;
	int calc();
};

class PrimaryExpAST : public BaseAST {
public:
	std::variant<std::unique_ptr<ExpAST>, std::unique_ptr<LValAST>, int> inside_exp;
	void output(Ost &outstr, std::string prefix) const override;
	int calc();
};

class UnaryOpAST : public OpAST {
public:
	using OpAST::OpAST;
	void output(Ost &outstr, std::string prefix) const override;
	int calc(int x);
};

class BinaryOpAST : public OpAST {
public:
	using OpAST::OpAST;
	bool is_logic_op();
	void output(Ost &outstr, std::string prefix) const override;
	int calc(int lhs, int rhs);
};

template<typename Now_Level_Type, typename Nxt_Level_Type>
class BinaryExpAST_Base : public BaseAST {
public:
	std::optional<std::unique_ptr<Now_Level_Type>> now_level;
	std::optional<std::unique_ptr<BinaryOpAST>> binary_op;
	std::unique_ptr<Nxt_Level_Type> nxt_level;
	void output(Ost &outstr, std::string prefix) const override;
	int calc();
};

template<int level>
class BinaryExpAST : public BinaryExpAST_Base<BinaryExpAST<level>, BinaryExpAST<level - 1>> {};

template<>
class BinaryExpAST<0> : public BinaryExpAST_Base<BinaryExpAST<0>, UnaryExpAST> {};

// clang-format off
using MulExpAST = BinaryExpAST<0>;
using AddExpAST = BinaryExpAST<1>; template class BinaryExpAST<1>;
using RelExpAST = BinaryExpAST<2>; template class BinaryExpAST<2>;
using EqExpAST = BinaryExpAST<3>; template class BinaryExpAST<3>;
using LAndExpAST = BinaryExpAST<4>; template class BinaryExpAST<4>;
using LOrExpAST = BinaryExpAST<5>; template class BinaryExpAST<5>;
// clang-format on

class DeclAST : public BaseAST {
public:
	VariantAstPtr<ConstDeclAST, VarDeclAST> decl;
	void output(Ost &outstr, std::string prefix) const override;
};

class ConstDeclAST : public BaseAST {
public:
	std::unique_ptr<BTypeAST> typ;
	std::list<std::unique_ptr<ConstDefAST>> defs;
	void output(Ost &outstr, std::string prefix) const override;
};

class BTypeAST : public BaseAST {
public:
	BTypeAST() = default;
	BTypeAST(BTypeAST const &) = default;
	void output(Ost &outstr, std::string prefix) const override;
};

class ConstDefAST : public BaseAST {
public:
	std::string ident;
	std::unique_ptr<ConstInitValAST> val;
	void output(Ost &outstr, std::string prefix) const override;
};

class ConstInitValAST : public BaseAST {
public:
	std::unique_ptr<ConstExpAST> exp;
	std::optional<std::variant<int>> val;   // cached
	void calc();
	void output(Ost &outstr, std::string prefix) const override;
};

class ConstExpAST : public BaseAST {
public:
	std::unique_ptr<ExpAST> exp;
	int calc();
	void output(Ost &outstr, std::string prefix) const override;
};

class LValAST : public BaseAST {
public:
	std::string ident;
	void output(Ost &outstr, std::string prefix) const override;
};

class VarDeclAST : public BaseAST {
public:
	std::unique_ptr<BTypeAST> typ;
	std::list<std::unique_ptr<VarDefAST>> defs;
	void output(Ost &outstr, std::string prefix) const override;
};

class VarDefAST : public BaseAST {
public:
	std::unique_ptr<BTypeAST> typ;
	std::string ident;
	std::optional<std::unique_ptr<InitValAST>> val;
	void output(Ost &outstr, std::string prefix) const override;
};

class InitValAST : public BaseAST {
public:
	std::unique_ptr<ExpAST> exp;
	void output(Ost &outstr, std::string prefix) const override;
};

class LValAssignAST : public BaseAST {
public:
	std::unique_ptr<LValAST> lval;
	std::unique_ptr<ExpAST> exp;
	void output(Ost &outstr, std::string prefix) const override;
};

class ReturnAST : public BaseAST {
public:
	std::unique_ptr<ExpAST> exp;
	void output(Ost &outstr, std::string prefix) const override;
};

}   // namespace Ast_Defs

}   // namespace Ast_Base

using namespace Ast_Base::Ast_Defs;
