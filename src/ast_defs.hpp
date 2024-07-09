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
// using Ost = std::ostringstream;
class Ost {
public:
	std::ostringstream *stream;
	bool muted;
	Ost(std::ostringstream &s) {
		stream = &s;
		muted = false;
	}
	Ost &operator<<(auto x) {
		if(!muted) {
			(*stream) << x;
		}
		return *this;
	}
	void mute() { muted = true; }
	void unmute() { muted = false; }
};
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

class BinaryOpAST;
class BlockAST;
class BlockItemAST;
class BreakAST;
class CompUnitAST;
class ConstDeclAST;
class ConstDefAST;
class ConstExpAST;
class ConstInitValAST;
class ContinueAST;
class DeclAST;
class ExpAST;
class FuncCallAST;
class FuncCallParamsAST;
class FuncDefAST;
class FuncDefParamsAST;
class IfAST;
class InitValAST;
class LValAST;
class LValAssignAST;
class OptionalExpAST;
class PrimaryExpAST;
class ReturnAST;
class StmtAST;
class TypeAST;
class UnaryExpAST;
class UnaryOpAST;
class VarDeclAST;
class VarDefAST;
class WhileAST;

template<int>
class BinaryExpAST;

class CompUnitAST : public BaseAST {
public:
	std::list<VariantAstPtr<FuncDefAST, DeclAST>> decls;
	void output(Ost &outstr, std::string prefix) const override;
};

class FuncDefAST : public BaseAST {
public:
	std::unique_ptr<TypeAST> func_typ;
	std::optional<std::unique_ptr<FuncDefParamsAST>> params;
	std::string ident;
	std::unique_ptr<BlockAST> block;
	void output(Ost &outstr, std::string prefix) const override;
};

// Type of function or variable.
class TypeAST : public BaseAST {
public:
	std::string typ;
	bool is_void;
	void output(Ost &outstr, std::string) const override;
};

class BlockAST : public BaseAST {
public:
	std::list<std::unique_ptr<BlockItemAST>> items;
	void output_base(Ost &outstr, std::string prefix, bool update_symbol_table) const;
	void output(Ost &outstr, std::string prefix) const override;
};

class BlockItemAST : public BaseAST {
public:
	VariantAstPtr<DeclAST, StmtAST> item;
	void output(Ost &outstr, std::string prefix) const override;
};

class StmtAST : public BaseAST {
public:
	VariantAstPtr<ReturnAST, LValAssignAST, OptionalExpAST, BlockAST, IfAST, WhileAST, BreakAST, ContinueAST> val;
	void output(Ost &outstr, std::string prefix) const override;
};

class OptionalExpAST : public BaseAST {
public:
	std::optional<std::unique_ptr<ExpAST>> exp;
	bool has_value() const;
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
using AddExpAST = BinaryExpAST<1>;	template class BinaryExpAST<1>;
using RelExpAST = BinaryExpAST<2>;	template class BinaryExpAST<2>;
using EqExpAST = BinaryExpAST<3>;	template class BinaryExpAST<3>;
using LAndExpAST = BinaryExpAST<4>;	template class BinaryExpAST<4>;
using LOrExpAST = BinaryExpAST<5>;	template class BinaryExpAST<5>;
// clang-format on

class DeclAST : public BaseAST {
public:
	VariantAstPtr<ConstDeclAST, VarDeclAST> decl;
	void output(Ost &outstr, std::string prefix) const override;
	void output_global(Ost &outstr, std::string prefix) const;
};

class ConstDeclAST : public BaseAST {
public:
	std::unique_ptr<TypeAST> typ;
	std::list<std::unique_ptr<ConstDefAST>> defs;
	void output(Ost &outstr, std::string prefix) const override;
	void output_global(Ost &outstr, std::string prefix) const;
};

class ConstDefAST : public BaseAST {
public:
	std::string ident;
	std::list<int> dimension;   // [x][y][z] -> (x, y, z)
	std::unique_ptr<ConstInitValAST> val;
	void output_base(Ost &outstr, std::string prefix, bool is_global) const;
	void output(Ost &outstr, std::string prefix) const override;
	void output_global(Ost &outstr, std::string prefix) const;
};

class ConstInitValAST : public BaseAST {
public:
	std::variant<std::unique_ptr<ConstExpAST>, std::list<std::unique_ptr<ConstInitValAST>>> exp;
	bool is_zero;   // must be list
	std::list<int> dimension;
	void calc();
	void output(Ost &outstr, std::string prefix) const override;
	void output_global(Ost &outstr, std::string prefix) const;
};

class ConstExpAST : public BaseAST {
public:
	std::unique_ptr<ExpAST> exp;
	int val;
	void calc();
	void output(Ost &outstr, std::string prefix) const override;
};

class LValAST : public BaseAST {
public:
	std::string ident;
	std::list<int> dimension;
	void output(Ost &outstr, std::string prefix) const override;
};

class VarDeclAST : public BaseAST {
public:
	std::unique_ptr<TypeAST> typ;
	std::list<std::unique_ptr<VarDefAST>> defs;
	void output(Ost &outstr, std::string prefix) const override;
	void output_global(Ost &outstr, std::string prefix) const;
};

class VarDefAST : public BaseAST {
public:
	std::unique_ptr<TypeAST> typ;
	std::string ident;
	std::list<int> dimension;
	std::optional<std::unique_ptr<InitValAST>> val;
	void output_base(Ost &outstr, std::string prefix, bool is_global) const;
	void output(Ost &outstr, std::string prefix) const override;
	void output_global(Ost &outstr, std::string prefix) const;
};

class InitValAST : public BaseAST {
public:
	std::variant<std::unique_ptr<ExpAST>, std::list<std::unique_ptr<InitValAST>>> exp;
	bool is_zero;   // must be list
	std::list<int> dimension;
	void output(Ost &outstr, std::string prefix) const override;
	void output_global(Ost &outstr, std::string prefix) const;
};

class LValAssignAST : public BaseAST {
public:
	std::unique_ptr<LValAST> lval;
	std::unique_ptr<ExpAST> exp;
	void output(Ost &outstr, std::string prefix) const override;
};

class ReturnAST : public BaseAST {
public:
	std::unique_ptr<OptionalExpAST> exp;
	void output(Ost &outstr, std::string prefix) const override;
};

// else is optional
class IfAST : public BaseAST {
public:
	std::unique_ptr<ExpAST> cond;
	std::unique_ptr<StmtAST> if_stmt;
	std::optional<std::unique_ptr<StmtAST>> else_stmt;
	int if_id;
	void set_if_cnt();
	std::string get_then_str() const;
	std::string get_else_str() const;
	std::string get_end_str() const;
	void output(Ost &outstr, std::string prefix) const override;
};

class WhileAST : public BaseAST {
public:
	std::unique_ptr<ExpAST> cond;
	std::unique_ptr<StmtAST> stmt;
	void output(Ost &outstr, std::string prefix) const override;
};

class BreakAST : public BaseAST {
public:
	void output(Ost &outstr, std::string prefix) const override;
};

class ContinueAST : public BaseAST {
public:
	void output(Ost &outstr, std::string prefix) const override;
};

class FuncDefParamsAST : public BaseAST {
public:
	std::list<std::pair<std::unique_ptr<TypeAST>, std::string>> params;
	void output(Ost &outstr, std::string prefix) const override;
	void output_save(Ost &outstr, std::string prefix) const;
};

class FuncCallAST : public BaseAST {
public:
	std::string func;
	std::unique_ptr<FuncCallParamsAST> params;
	void output(Ost &outstr, std::string prefix) const override;
};

class FuncCallParamsAST : public BaseAST {
public:
	std::list<std::unique_ptr<ExpAST>> params;
	int get_param_cnt() const;
	void output(Ost &outstr, std::string prefix) const override;
};

}   // namespace Ast_Defs

}   // namespace Ast_Base

using namespace Ast_Base::Ast_Defs;
