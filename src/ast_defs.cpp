#include "ast_defs.hpp"

namespace Ast_Base {

int var_count = 0;

class Koopa_val {
public:
	int val;
	bool is_im;

	Koopa_val() = default;
	Koopa_val(bool typ, int x) {
		is_im = typ;
		val = x;
	}
	Koopa_val& operator=(Koopa_val const &) = default;
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

std::map<std::string, Koopa_val> symbol_table;

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
	for(auto& i : items) {
		i->output(outstr, prefix + INDENT);
	}
}

void BlockItemAST::output(Ost& outstr, std::string prefix) const {
	std::visit([&](auto& i) {
		i->output(outstr, prefix);
	},
			   item);
}

void StmtAST::output(Ost& outstr, std::string prefix) const {
	ret_val->output(outstr, prefix);
	outstr << prefix << "ret " << stmt_val.top();
	stmt_val.pop();
}

void ExpAST::output(Ost& outstr, std::string prefix) const {
	binary_exp->output(outstr, prefix);
}

int ExpAST::calc() {
	return binary_exp->calc();
}

void UnaryExpAST::output(Ost& outstr, std::string prefix) const {
	if(unary_op.has_value()) {
		// unary_exp
		std::get<0>(unary_exp)->output(outstr, prefix);
		int now_var = var_count;
		var_count++;
		outstr << prefix << "%" << now_var << " = ";
		(*unary_op)->output(outstr, "");
		outstr << stmt_val.top() << "\n";
		stmt_val.pop();
		stmt_val.push(Koopa_val(false, now_var));
	} else {
		// primary_exp
		std::get<1>(unary_exp)->output(outstr, prefix);
	}
}

int UnaryExpAST::calc() {
	if(unary_op.has_value()) {
		return unary_op.value()->calc(std::get<0>(unary_exp)->calc());
	} else {
		return std::get<1>(unary_exp)->calc();
	}
}

void PrimaryExpAST::output(Ost& outstr, std::string prefix) const {
	switch(inside_exp.index()) {
	case 0:
		std::get<0>(inside_exp)->output(outstr, prefix);
		break;
	case 1:
		std::get<1>(inside_exp)->output(outstr, prefix);
		break;
	case 2:
		stmt_val.push(Koopa_val(true, std::get<2>(inside_exp)));
		break;
	default:;
	}
}

int PrimaryExpAST::calc() {
	switch(inside_exp.index()) {
	case 0:
		return std::get<0>(inside_exp)->calc();
		break;
	case 1: {
		std::string& ident = std::get<1>(inside_exp)->ident;
		if(!symbol_table.contains(ident)) {
			throw 114514;
		}
		return symbol_table[ident].val;
		break;
	}
	case 2:
		return std::get<2>(inside_exp);
		break;
	default: assert(0);
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

int UnaryOpAST::calc(int x) {
	switch(op) {
	case OP_ADD:
		return x;
		break;
	case OP_SUB:
		return -x;
		break;
	case OP_LNOT:
		return !x;
		break;
	default:
		assert(0);
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

int BinaryOpAST::calc(int lhs, int rhs) {
	switch(op) {
	case OP_ADD:
		return lhs + rhs;
		break;
	case OP_SUB:
		return lhs - rhs;
		break;
	case OP_MUL:
		return lhs * rhs;
		break;
	case OP_DIV:
		return lhs / rhs;
		break;
	case OP_MOD:
		return lhs % rhs;
		break;
	case OP_GT:
		return lhs > rhs;
		break;
	case OP_GE:
		return lhs >= rhs;
		break;
	case OP_LT:
		return lhs < rhs;
		break;
	case OP_LE:
		return lhs <= rhs;
		break;
	case OP_EQ:
		return lhs == rhs;
		break;
	case OP_NEQ:
		return lhs != rhs;
		break;
	case OP_LAND:
		return lhs && rhs;
		break;
	case OP_LOR:
		return lhs || rhs;
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
	if(binary_op.value()->is_logic_op()) {
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

template<typename T, typename U>
int BinaryExpAST_Base<T, U>::calc() {
	if(binary_op.has_value()) {
		return binary_op.value()->calc(now_level.value()->calc(), nxt_level->calc());
	} else {
		return nxt_level->calc();
	}
}

void DeclAST::output(Ost& outstr, std::string prefix) const {
	const_decl_ast->output(outstr, prefix);
}

void ConstDeclAST::output(Ost& outstr, std::string prefix) const {
	for(auto& i : defs) {
		i->output(outstr, prefix);
	}
}

void BTypeAST::output(Ost& outstr, std::string prefix) const {}

void ConstDefAST::output(Ost& outstr, std::string prefix) const {
	if(symbol_table.contains(ident)) {
		std::cerr << "It's been a long day without you my friend\n"
				  << "And I'll tell you all about it when I see you again\n"
				  << ident << "\n";
		throw 114514;
	}
	val->calc();
	Koopa_val kval(true, std::get<int>(val->val.value()));
	symbol_table[ident] = kval;
}

void ConstInitValAST::output(Ost& outstr, std::string prefix) const {}

void ConstInitValAST::calc() {
	val = exp->calc();
}

void LValAST::output(Ost& outstr, std::string prefix) const {
	if(!symbol_table.contains(ident)) {
		std::cerr << "What is " << ident << "???\n";
		throw 114514;
	}
	stmt_val.push(symbol_table[ident]);
}

void ConstExpAST::output(Ost& outstr, std::string prefix) const {}

int ConstExpAST::calc() {
	return exp->calc();
}

}   // namespace Ast_Defs
}   // namespace Ast_Base
