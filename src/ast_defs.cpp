#include "ast_defs.hpp"
#include <cassert>
#include <unordered_map>
#include <unordered_set>

namespace Ast_Base {

int var_count = 0;

namespace Koopa_Val_Def {

enum Koopa_value_type {
	KOOPA_VALUE_TYPE_IMMEDIATE,
	KOOPA_VALUE_TYPE_TEMP,
	KOOPA_VALUE_TYPE_NAMED
};

class Koopa_val_base {
public:
	virtual std::string get_str() const = 0;
	virtual void prepare(Ost& outstr, std::string prefix) { return; }
	virtual Koopa_value_type val_type() const = 0;
	virtual ~Koopa_val_base() { return; }
};

class Koopa_val_im : public Koopa_val_base {
private:
	int val;

public:
	Koopa_val_im(int x) { val = x; }
	int get_im_val() const { return val; }
	std::string get_str() const override {
		return std::to_string(val);
	}
	Koopa_value_type val_type() const override { return KOOPA_VALUE_TYPE_IMMEDIATE; }
};

class Koopa_val_temp_symbol : public Koopa_val_base {
private:
	int id;

public:
	Koopa_val_temp_symbol(int x) { id = x; }
	std::string get_str() const override {
		return std::string("%") + std::to_string(id);
	}
	Koopa_value_type val_type() const override { return KOOPA_VALUE_TYPE_TEMP; }
};

class Koopa_val_named_symbol : public Koopa_val_base {
private:
	std::string id;
	int cache_id;

public:
	void set_id(std::string str) { id = str; }
	std::string get_id() const { return id; }
	std::string get_str() const override {
		return std::string("%") + std::to_string(cache_id);
	}
	void prepare(Ost& outstr, std::string prefix) override {
		outstr << prefix << "%" << var_count << " = load @" << id << '\n';
		cache_id = var_count;
		var_count++;
	}
	Koopa_value_type val_type() const override { return KOOPA_VALUE_TYPE_NAMED; }
};

class Koopa_val {
private:
	std::shared_ptr<Koopa_val_base> val;

public:
	Koopa_val() {}
	Koopa_val(Koopa_val const &) = default;
	Koopa_val(Koopa_val_base* obj) {
		val = std::shared_ptr<Koopa_val_base>(obj);
	}
	int get_im_val() const {
		if(val_type() == KOOPA_VALUE_TYPE_IMMEDIATE) {
			return std::static_pointer_cast<Koopa_val_im>(val)->get_im_val();
		} else {
			assert(0);
		}
	}
	std::string get_named_name() const {
		if(val_type() == KOOPA_VALUE_TYPE_NAMED) {
			return std::static_pointer_cast<Koopa_val_named_symbol>(val)->get_id();
		} else {
			assert(0);
		}
	}
	Koopa_value_type val_type() const { return val->val_type(); }
	std::string get_str() const {
		return val->get_str();
	}
	void prepare(Ost& outstr, std::string prefix) {
		return val->prepare(outstr, prefix);
	}
	template<typename T>
	friend T& operator<<(T& outstr, Koopa_val const & me) {
		outstr << me.get_str();
		return outstr;
	}
};

}   // namespace Koopa_Val_Def

using namespace Koopa_Val_Def;

std::stack<Koopa_val> stmt_val;

std::unordered_map<std::string, Koopa_val> symbol_table;

// name -> value
std::unordered_map<std::string, int> symbol_const;

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
	outstr << prefix << "}\n";
}

void TypAST::output(Ost& outstr, std::string prefix) const {
	outstr << prefix << typ;
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
	std::visit([&](auto& i) {
		i->output(outstr, prefix);
	},
			   val);
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
		stmt_val.push(new Koopa_val_temp_symbol(now_var));
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
		stmt_val.push(new Koopa_val_im(std::get<2>(inside_exp)));
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
		return symbol_table[ident].get_im_val();
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
	lhs.prepare(outstr, prefix);
	rhs.prepare(outstr, prefix);
	if(binary_op.value()->is_logic_op()) {
		int now_var = var_count;
		var_count++;
		outstr << prefix << "%" << now_var << " = ne 0, " << lhs;
		lhs = new Koopa_val_temp_symbol(now_var);
		now_var = var_count;
		var_count++;
		outstr << prefix << "%" << now_var << " = ne 0, " << rhs;
		rhs = new Koopa_val_temp_symbol(now_var);
	}
	int now_var = var_count;
	var_count++;
	outstr << prefix << "%" << now_var << " = ";
	binary_op.value()->output(outstr, "");
	outstr << lhs << ", " << rhs << '\n';
	stmt_val.push(new Koopa_val_temp_symbol(now_var));
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
	std::visit([&](auto&& x) {
		x->output(outstr, prefix);
	},
			   decl);
}

void ConstDeclAST::output(Ost& outstr, std::string prefix) const {
	for(auto& i : defs) {
		i->output(outstr, prefix);
	}
}

void BTypeAST::output(Ost& outstr, std::string prefix) const {
	outstr << prefix << "i32";
}

void ConstDefAST::output(Ost& outstr, std::string prefix) const {
	if(symbol_table.contains(ident)) {
		std::cerr << "It's been a long day without you my friend\n"
				  << "And I'll tell you all about it when I see you again\n"
				  << ident << "\n";
		throw 114514;
	}
	val->calc();
	symbol_table[ident] = new Koopa_val_im(std::get<int>(val->val.value()));
	symbol_const[ident] = std::get<int>(val->val.value());
}

void ConstInitValAST::output(Ost& outstr, std::string prefix) const {
	return;
}

void ConstInitValAST::calc() {
	val = exp->calc();
}

void InitValAST::output(Ost& outstr, std::string prefix) const {
	exp->output(outstr, prefix);
}

void LValAST::output(Ost& outstr, std::string prefix) const {
	if(!symbol_table.contains(ident)) {
		std::cerr << "What is " << ident << "???\n";
		throw 114514;
	}
	stmt_val.push(symbol_table[ident]);
}

void ConstExpAST::output(Ost& outstr, std::string prefix) const {
	return;
}

int ConstExpAST::calc() {
	return exp->calc();
}

void VarDeclAST::output(Ost& outstr, std::string prefix) const {
	for(auto& i : defs) {
		i->typ = std::make_unique<BTypeAST>(*typ);
		i->output(outstr, prefix);
	}
}

void VarDefAST::output(Ost& outstr, std::string prefix) const {
	outstr << prefix << "@" << ident << " = alloc ";
	typ->output(outstr, "");
	outstr << "\n";
	auto reg_var = new Koopa_val_named_symbol;
	reg_var->set_id(ident);
	symbol_table[ident] = Koopa_val(reg_var);
	if(val.has_value()) {
		val.value()->output(outstr, prefix);
		Koopa_val last_val = stmt_val.top();
		stmt_val.pop();
		last_val.prepare(outstr, prefix);
		outstr << prefix << "store " << last_val << ", @" << ident << "\n";
	}
}

void LValAssignAST::output(Ost& outstr, std::string prefix) const {
	Koopa_val lhs, rhs;
	lval->output(outstr, prefix);
	lhs = stmt_val.top();
	stmt_val.pop();
	exp->output(outstr, prefix);
	rhs = stmt_val.top();
	stmt_val.pop();
	rhs.prepare(outstr, prefix);
	outstr << prefix << "store " << rhs << ", @" << lhs.get_named_name() << '\n';
}

void ReturnAST::output(Ost& outstr, std::string prefix) const {
	exp->output(outstr, prefix);
	Koopa_val val = stmt_val.top();
	stmt_val.pop();
	val.prepare(outstr, prefix);
	outstr << prefix << "ret " << val << '\n';
}

}   // namespace Ast_Defs
}   // namespace Ast_Base
