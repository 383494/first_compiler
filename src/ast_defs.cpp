#include "ast_defs.hpp"
#include <stack>

namespace Ast_Base {

int var_count = 0;

class Koopa_val {
private:
	int val;
	bool is_im;

public:
	Koopa_val(bool typ, int x) {
		is_im = typ;
		val = x;
	}
	std::string get_str() {
		if(is_im) {
			return std::to_string(val);
		} else {
			return std::string("%") + std::to_string(val);
		}
	}
	template<typename T>
	friend T& operator<<(T& outstr, const Koopa_val& me) {
		if(me.is_im) {
			outstr << me.val;
		} else {
			outstr << "%" << me.val;
		}
		return outstr;
	}
};

std::stack<Koopa_val> stmt_val;

namespace Ast_Defs {

template class BinaryExpAST_Base<BinaryExpAST<0>, UnaryExpAST>;

void CompUnitAST::output(Ost& outstr, std::string prefix) const {
	func_def->output(outstr, prefix);
}

void FuncDefAST::output(Ost& outstr, std::string prefix) const {
	outstr << prefix << "fun @" << ident << "():";
	func_typ->output(outstr, "");
	outstr << "{\n";
	block->output(outstr, prefix);
	outstr << "\n"
		   << prefix << "}\n";
}

void TypAST::output(Ost& outstr, std::string) const {
	outstr << typ;
}

void BlockAST::output(Ost& outstr, std::string prefix) const {
	outstr << prefix << "%entry:\n";
	stmt->output(outstr, prefix + INDENT);
}

void StmtAST::output(Ost& outstr, std::string prefix) const {
	ret_val->output(outstr, prefix);
	outstr << prefix << "ret " << stmt_val.top();
	stmt_val.pop();
}

void ExpAST::output(Ost& outstr, std::string prefix) const {
	binary_exp->output(outstr, prefix);
}

void UnaryExpAST::output(Ost& outstr, std::string prefix) const {
	if(unary_op.has_value()) {
		unary_exp->output(outstr, prefix);
		int now_var = var_count;
		var_count++;
		outstr << prefix << "%" << now_var << " = ";
		(*unary_op)->output(outstr, "");
		outstr << stmt_val.top() << "\n";
		stmt_val.pop();
		stmt_val.push(Koopa_val(false, now_var));
	} else {
		// primary_exp
		unary_exp->output(outstr, prefix);
	}
}

void PrimaryExpAST::output(Ost& outstr, std::string prefix) const {
	switch(inside_exp.index()) {
	case 0:
		std::get<0>(inside_exp)->output(outstr, prefix);
		break;
	case 1:
		stmt_val.push(Koopa_val(true, std::get<1>(inside_exp)));
		break;
	default:;
	}
}

void UnaryOpAST::output(Ost& outstr, std::string prefix) const {
	outstr << prefix;
	switch(op) {
	case OP_ADD:
		outstr << "add 0, ";
		break;
	case OP_SUB:
		outstr << "sub 0, ";
		break;
	case OP_LNOT:
		outstr << "eq 0, ";
		break;
	default:
		assert(0);
		break;
	}
}

bool BinaryOpAST::is_logic_op() {
	return op == OP_LAND || op == OP_LOR;
}

void BinaryOpAST::output(Ost& outstr, std::string prefix) const {
	outstr << prefix;
	switch(op) {
	case OP_ADD:
		outstr << "add ";
		break;
	case OP_SUB:
		outstr << "sub ";
		break;
	case OP_MUL:
		outstr << "mul ";
		break;
	case OP_DIV:
		outstr << "div ";
		break;
	case OP_MOD:
		outstr << "mod ";
		break;
	case OP_GT:
		outstr << "gt ";
		break;
	case OP_GE:
		outstr << "ge ";
		break;
	case OP_LT:
		outstr << "lt ";
		break;
	case OP_LE:
		outstr << "le ";
		break;
	case OP_EQ:
		outstr << "eq ";
		break;
	case OP_NEQ:
		outstr << "ne ";
		break;
	case OP_LAND:
		outstr << "and ";
		break;
	case OP_LOR:
		outstr << "or ";
		break;
	default: assert(0);
	}
}

template<typename T, typename U>
void BinaryExpAST_Base<T, U>::output(Ost& outstr, std::string prefix) const {
	if(!binary_op.has_value()) {
		return nxt_level->output(outstr, prefix);
	}
	now_level.value()->output(outstr, prefix);
	Koopa_val lhs = stmt_val.top();
	stmt_val.pop();
	nxt_level->output(outstr, prefix);
	Koopa_val rhs = stmt_val.top();
	stmt_val.pop();
	if(binary_op.value()->is_logic_op()){
		int now_var = var_count;
		var_count++;
		outstr << prefix << "%" << now_var << " = ne 0, " << lhs;
		lhs = Koopa_val(false, now_var);
		now_var = var_count;
		var_count++;
		outstr << prefix << "%" << now_var << " = ne 0, " << rhs;
		rhs = Koopa_val(false, now_var);
	}
	int now_var = var_count;
	var_count++;
	outstr << prefix << "%" << now_var << " = ";
	binary_op.value()->output(outstr, "");
	outstr << lhs << ", " << rhs << '\n';
	stmt_val.push(Koopa_val(false, now_var));
}

}   // namespace Ast_Defs
}   // namespace Ast_Base

auto init_hook = []() {
	BinaryExpAST<Ast_Base::BINARY_EXP_MAX_LEVEL>::instantiate();
	return 0;
}();
