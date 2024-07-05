#include "ast_defs.hpp"
#include <cassert>
#include <unordered_map>
#include <unordered_set>

namespace Ast_Base {

int unnamed_var_cnt = 0;
int named_var_cnt = 0;
int if_cnt = 0;
int loop_cnt = 0;

constexpr const char* SHORT_TMP_VAR_NAME = "@_tmp_short";


namespace Koopa_Val_Def {

enum Koopa_value_type {
	KOOPA_VALUE_TYPE_IMMEDIATE,
	KOOPA_VALUE_TYPE_TEMP,
	KOOPA_VALUE_TYPE_NAMED,
	KOOPA_VALUE_TYPE_GLOBAL_FUNCTION,
	KOOPA_VALUE_TYPE_GLOBAL_NAMED,
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
	void set_id(std::string str) {
		id = str + "_" + std::to_string(named_var_cnt);
		named_var_cnt++;
	}
	std::string get_id() const { return id; }
	std::string get_str() const override {
		return std::string("%") + std::to_string(cache_id);
	}
	void prepare(Ost& outstr, std::string prefix) override {
		outstr << prefix << "%" << unnamed_var_cnt << " = load @" << id << '\n';
		cache_id = unnamed_var_cnt;
		unnamed_var_cnt++;
	}
	Koopa_value_type val_type() const override { return KOOPA_VALUE_TYPE_NAMED; }
};

class Koopa_val_global_func : public Koopa_val_base {
private:
	bool is_func_void;
	std::string ident;

public:
	Koopa_val_global_func(std::string const & id, bool is_void) {
		is_func_void = is_void;
		ident = "@" + id;
	}
	Koopa_val_global_func(FuncDefAST const * func) {
		is_func_void = func->func_typ->is_void;
		ident = "@" + func->ident;
	}
	bool is_void() const { return is_func_void; }
	std::string get_str() const override { return ident; }
	Koopa_value_type val_type() const override { return KOOPA_VALUE_TYPE_GLOBAL_FUNCTION; }
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
		assert(val_type() == KOOPA_VALUE_TYPE_IMMEDIATE);
		return std::static_pointer_cast<Koopa_val_im>(val)->get_im_val();
	}
	std::string get_named_name() const {
		assert(val_type() == KOOPA_VALUE_TYPE_NAMED);
		return std::static_pointer_cast<Koopa_val_named_symbol>(val)->get_id();
	}
	bool is_func_void() const {
		assert(val->val_type() == KOOPA_VALUE_TYPE_GLOBAL_FUNCTION);
		return std::static_pointer_cast<Koopa_val_global_func>(val)->is_void();
	}
	Koopa_value_type val_type() const { return val->val_type(); }
	std::string get_str() const {
		return val->get_str();
	}
	void prepare(Ost& outstr, std::string prefix) {
		return val->prepare(outstr, prefix);
	}
};

Ost& operator<<(Ost& outstr, Koopa_val const & me) {
	outstr << me.get_str();
	return outstr;
}

}   // namespace Koopa_Val_Def

using namespace Koopa_Val_Def;

namespace Sysy_Library {
std::pair<std::string, bool> sysy_lib_funcs[] = {
	{"getint", false},
	{"getch", false},
	{"getarray", false},
	{"putint", true},
	{"putch", true},
	{"putarray", true},
	{"starttime", true},
	{"stoptime", true},
};
}

std::stack<Koopa_val> stmt_val;
std::stack<int> loop_level;

template<typename T>
class Symbol_table_stack {
	std::list<std::unordered_map<std::string, T>> symbols;

public:
	auto empty_iter() const {
		return symbols.front().end();
	}
	auto find(const std::string& key) {
		for(auto i = symbols.rbegin(); i != symbols.rend(); i++) {
			auto val_iter = i->find(key);
			if(val_iter != i->end()) {
				return val_iter;
			}
		}
		return symbols.front().end();
	}
	bool contains(const std::string& key) { return find(key) != empty_iter(); }
	T& operator[](const std::string& key) {
		return find(key)->second;
	}
	void add_table() { symbols.emplace_back(); }
	void del_table() { symbols.pop_back(); }
	void insert(std::pair<std::string, T> val) {
		bool insert_ok = symbols.back().insert(val).second;
		if(!insert_ok) {
			std::cerr << "It's been a long day without you my friend\n"
					  << "And I'll tell you all about it when I see you again\n"
					  << val.first << "\n";
			throw 114514;
		}
	}
};

Symbol_table_stack<Koopa_val> symbol_table;

/*
namespace Exceptions {

class End_of_block : public std::exception {
	const char* what() const throw() {
		return "Basic block ends. But why I was not catched???\n";
	}
};

}   // namespace Exceptions
*/

void enter_sysy_block() {
	symbol_table.add_table();
}

void exit_sysy_block() {
	symbol_table.del_table();
}

void enter_koopa_block(std::string id, Ost& outstr, std::string prefix) {
	outstr << id << ":\n";
}

void exit_koopa_block(Ost& outstr, std::string prefix) {
	assert(outstr.muted);
	outstr.unmute();
}

// name -> value
// Symbol_table_stack<int> symbol_const;

namespace Ast_Defs {

template class BinaryExpAST_Base<BinaryExpAST<0>, UnaryExpAST>;

void CompUnitAST::output(Ost& outstr, std::string prefix) const {
	outstr << R"(decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

)";
	enter_sysy_block();
	for(auto [func_id, is_void] : Sysy_Library::sysy_lib_funcs) {
		symbol_table.insert({func_id, Koopa_val(new Koopa_val_global_func(func_id, is_void))});
	}
	for(auto& i : func_def) {
		i->output(outstr, prefix);
	}
	exit_sysy_block();
}

void FuncDefAST::output(Ost& outstr, std::string prefix) const {
	symbol_table.insert({ident, Koopa_val(new Koopa_val_global_func(this))});
	outstr << prefix << "fun @" << ident << "(";
	if(params.has_value()) {
		params.value()->output(outstr, "");
	}
	outstr << ")";
	if(!func_typ->is_void) {
		func_typ->output(outstr, ": ");
	}
	outstr << " {\n";
	enter_koopa_block("%entry", outstr, prefix);
	outstr << prefix + INDENT << SHORT_TMP_VAR_NAME << " = alloc i32\n";
	if(params.has_value()) {
		params.value()->output_save(outstr, prefix + INDENT);
	}
	block->output(outstr, prefix);
	exit_koopa_block(outstr, prefix);
	outstr << prefix << "}\n";
}

void TypAST::output(Ost& outstr, std::string prefix) const {
	outstr << prefix << typ;
}

void BlockAST::output(Ost& outstr, std::string prefix) const {
	if(outstr.muted) {
		return;
	}
	enter_sysy_block();
	for(auto& i : items) {
		i->output(outstr, prefix + INDENT);
	}
	exit_sysy_block();
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

bool OptionalExpAST::has_value() const {
	return exp.has_value();
}

void OptionalExpAST::output(Ost& outstr, std::string prefix) const {
	if(exp.has_value()) {
		exp.value()->output(outstr, prefix);
	}
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
		int now_var = unnamed_var_cnt;
		unnamed_var_cnt++;
		stmt_val.top().prepare(outstr, prefix);
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
	case OP_ADD: outstr << "add "; break;
	case OP_SUB: outstr << "sub "; break;
	case OP_MUL: outstr << "mul "; break;
	case OP_DIV: outstr << "div "; break;
	case OP_MOD: outstr << "mod "; break;
	case OP_GT: outstr << "gt "; break;
	case OP_GE: outstr << "ge "; break;
	case OP_LT: outstr << "lt "; break;
	case OP_LE: outstr << "le "; break;
	case OP_EQ: outstr << "eq "; break;
	case OP_NEQ: outstr << "ne "; break;
	case OP_LAND: outstr << "and "; break;
	case OP_LOR: outstr << "or "; break;
	default: assert(0);
	}
}

int BinaryOpAST::calc(int lhs, int rhs) {
	switch(op) {
	case OP_ADD: return lhs + rhs;
	case OP_SUB: return lhs - rhs;
	case OP_MUL: return lhs * rhs;
	case OP_DIV: return lhs / rhs;
	case OP_MOD: return lhs % rhs;
	case OP_GT: return lhs > rhs;
	case OP_GE: return lhs >= rhs;
	case OP_LT: return lhs < rhs;
	case OP_LE: return lhs <= rhs;
	case OP_EQ: return lhs == rhs;
	case OP_NEQ: return lhs != rhs;
	case OP_LAND: return lhs && rhs;
	case OP_LOR: return lhs || rhs;
	default: assert(0);
	}
}

template<typename T, typename U>
void BinaryExpAST_Base<T, U>::output(Ost& outstr, std::string prefix) const {
	if(!binary_op.has_value()) {
		return nxt_level->output(outstr, prefix);
	}
	if(binary_op.value()->is_logic_op()) {
		now_level.value()->output(outstr, prefix);
		Koopa_val lhs = stmt_val.top();
		stmt_val.pop();
		lhs.prepare(outstr, prefix);
		int cur_if_cnt = if_cnt;
		if_cnt++;
		outstr << prefix << "br " << lhs << ", %then_short" << cur_if_cnt
			   << ", %else_short" << cur_if_cnt << "\n";
		if(binary_op.value()->op == OP_LOR) {
			enter_koopa_block("%then_short" + std::to_string(cur_if_cnt), outstr, prefix);
			outstr << prefix << "store 1, " << SHORT_TMP_VAR_NAME << "\n";
			// outstr << prefix << "%" << now_var << " = or 0, 1\n";
			outstr << prefix << "jump %end_short" << cur_if_cnt << "\n";
			outstr.mute();
			exit_koopa_block(outstr, prefix);
			enter_koopa_block("%else_short" + std::to_string(cur_if_cnt), outstr, prefix);
		} else {
			enter_koopa_block("%else_short" + std::to_string(cur_if_cnt), outstr, prefix);
			outstr << prefix << "store 0, " << SHORT_TMP_VAR_NAME << "\n";
			outstr << prefix << "jump %end_short" << cur_if_cnt << "\n";
			outstr.mute();
			exit_koopa_block(outstr, prefix);
			enter_koopa_block("%then_short" + std::to_string(cur_if_cnt), outstr, prefix);
		}

		nxt_level->output(outstr, prefix);
		Koopa_val rhs = stmt_val.top();
		stmt_val.pop();
		rhs.prepare(outstr, prefix);
		int now_var = unnamed_var_cnt;
		unnamed_var_cnt++;
		outstr << prefix << "%" << now_var << " = ne 0, " << rhs << "\n";
		outstr << prefix << "store %" << now_var << ", " << SHORT_TMP_VAR_NAME << "\n";
		// outstr << "%" << now_var << " = or 0, " << rhs << "\n";
		outstr << prefix << "jump %end_short" << cur_if_cnt << "\n";
		outstr.mute();
		exit_koopa_block(outstr, prefix);
		enter_koopa_block("%end_short" + std::to_string(cur_if_cnt), outstr, prefix);
		now_var = unnamed_var_cnt;
		unnamed_var_cnt++;
		outstr << prefix << "%" << now_var << " = load " << SHORT_TMP_VAR_NAME << "\n";
		stmt_val.push(new Koopa_val_temp_symbol(now_var));
		return;
	}
	now_level.value()->output(outstr, prefix);
	Koopa_val lhs = stmt_val.top();
	stmt_val.pop();
	nxt_level->output(outstr, prefix);
	Koopa_val rhs = stmt_val.top();
	stmt_val.pop();
	lhs.prepare(outstr, prefix);
	rhs.prepare(outstr, prefix);
	// if(binary_op.value()->is_logic_op()) {
	// 	int now_var = unnamed_var_cnt;
	// 	unnamed_var_cnt++;
	// 	outstr << prefix << "%" << now_var << " = ne 0, " << lhs;
	// 	lhs = new Koopa_val_temp_symbol(now_var);
	// 	now_var = unnamed_var_cnt;
	// 	unnamed_var_cnt++;
	// 	outstr << prefix << "%" << now_var << " = ne 0, " << rhs;
	// 	rhs = new Koopa_val_temp_symbol(now_var);
	// }
	int now_var = unnamed_var_cnt;
	unnamed_var_cnt++;
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
	val->calc();
	symbol_table.insert({ident, new Koopa_val_im(std::get<int>(val->val.value()))});
	// symbol_table[ident] = new Koopa_val_im(std::get<int>(val->val.value()));
	// symbol_const.insert({ident, std::get<int>(val->val.value())});
	// symbol_const[ident] = std::get<int>(val->val.value());
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
	auto reg_var = new Koopa_val_named_symbol;
	reg_var->set_id(ident);
	outstr << prefix << "@" << reg_var->get_id() << " = alloc ";
	typ->output(outstr, "");
	outstr << "\n";
	symbol_table.insert({ident, Koopa_val(reg_var)});
	if(val.has_value()) {
		val.value()->output(outstr, prefix);
		Koopa_val last_val = stmt_val.top();
		stmt_val.pop();
		last_val.prepare(outstr, prefix);
		outstr << prefix << "store " << last_val << ", @" << reg_var->get_id() << "\n";
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
	if(exp->has_value()) {
		Koopa_val val = stmt_val.top();
		stmt_val.pop();
		val.prepare(outstr, prefix);
		outstr << prefix << "ret " << val << '\n';
	} else {
		outstr << prefix << "ret\n";
	}
	// throw Exceptions::End_of_block();
	outstr.mute();
}

void IfAST::set_if_cnt() {
	if_id = if_cnt;
	if_cnt++;
}

std::string IfAST::get_then_str() const {
	return "%then" + std::to_string(if_id);
}

std::string IfAST::get_else_str() const {
	if(else_stmt.has_value()) {
		return "%else" + std::to_string(if_id);
	} else {
		return "%end" + std::to_string(if_id);
	}
}

std::string IfAST::get_end_str() const {
	return "%end" + std::to_string(if_id);
}

void IfAST::output(Ost& outstr, std::string prefix) const {
	cond->output(outstr, prefix);
	auto cond_val = stmt_val.top();
	stmt_val.pop();
	cond_val.prepare(outstr, prefix);
	outstr << prefix << "br " << cond_val << ", " << get_then_str() << ", " << get_else_str() << "\n";
	enter_koopa_block(get_then_str(), outstr, prefix);
	if_stmt->output(outstr, prefix);
	if(!outstr.muted) {
		outstr << prefix << "jump " << get_end_str() << "\n";
		outstr.mute();
	}
	exit_koopa_block(outstr, prefix);
	if(else_stmt.has_value()) {
		enter_koopa_block(get_else_str(), outstr, prefix);
		else_stmt.value()->output(outstr, prefix);
		if(!outstr.muted) {
			outstr << prefix << "jump " << get_end_str() << "\n";
			outstr.mute();
		}
		exit_koopa_block(outstr, prefix);
	}
	enter_koopa_block(get_end_str(), outstr, prefix);
}

void WhileAST::output(Ost& outstr, std::string prefix) const {
	int cur_loop_cnt = loop_cnt;
	loop_cnt++;
	loop_level.push(cur_loop_cnt);
	outstr << prefix << "jump %loop_entry" << cur_loop_cnt << "\n";
	outstr.mute();
	exit_koopa_block(outstr, prefix);
	enter_koopa_block("%loop_entry" + std::to_string(cur_loop_cnt), outstr, prefix);
	cond->output(outstr, prefix);
	auto cond = stmt_val.top();
	stmt_val.pop();
	cond.prepare(outstr, prefix);
	outstr << prefix
		   << "br " << cond
		   << ", %loop_body" << cur_loop_cnt
		   << ", %loop_end" << cur_loop_cnt
		   << "\n";
	outstr.mute();
	exit_koopa_block(outstr, prefix);
	enter_koopa_block("%loop_body" + std::to_string(cur_loop_cnt), outstr, prefix);
	stmt->output(outstr, prefix);
	outstr << prefix << "jump %loop_entry" << cur_loop_cnt << "\n";
	outstr.mute();
	exit_koopa_block(outstr, prefix);
	loop_level.pop();
	enter_koopa_block("%loop_end" + std::to_string(cur_loop_cnt), outstr, prefix);
}

void BreakAST::output(Ost& outstr, std::string prefix) const {
	outstr << prefix << "jump %loop_end" << loop_level.top() << "\n";
	outstr.mute();
}

void ContinueAST::output(Ost& outstr, std::string prefix) const {
	outstr << prefix << "jump %loop_entry" << loop_level.top() << "\n";
	outstr.mute();
}

void FuncDefParamsAST::output(Ost& outstr, std::string prefix) const {
	bool is_first_param = true;
	for(auto& i : params) {
		if(is_first_param) {
			is_first_param = false;
		} else {
			outstr << ", ";
		}
		outstr << "@" << i.second << "_param: ";
		i.first->output(outstr, "");
	}
}

void FuncDefParamsAST::output_save(Ost& outstr, std::string prefix) const {
	for(auto& i : params) {
		auto val = new Koopa_val_named_symbol();
		val->set_id(i.second);
		outstr << prefix << "@" << val->get_id() << " = alloc ";
		i.first->output(outstr, "");
		outstr << "\n"
			   << prefix << "store @" << i.second << "_param, @" << val->get_id() << "\n";
		symbol_table.insert({i.second, val});
	}
}

void FuncCallParamsAST::output(Ost& outstr, std::string prefix) const {
	for(auto i = params.rbegin(); i != params.rend(); i++) {
		(*i)->output(outstr, prefix);
	}
}

int FuncCallParamsAST::get_param_cnt() const {
	return params.size();
}

void FuncCallAST::output(Ost& outstr, std::string prefix) const {
	params->output(outstr, prefix);
	int param_cnt = params->get_param_cnt();
	int now_var = -1;
	auto func_in_koopa = symbol_table[func];
	if(func_in_koopa.is_func_void()) {
		outstr << prefix;
	} else {
		now_var = unnamed_var_cnt;
		unnamed_var_cnt++;
		outstr << prefix << "%" << now_var << " = ";
	}
	outstr << "call " << func_in_koopa.get_str() << "(";
	for(int i = 0; i < param_cnt; i++) {
		if(i != 0) {
			outstr << ", ";
		}
		outstr << stmt_val.top();
		stmt_val.pop();
	}
	outstr << ")\n";
	if(!func_in_koopa.is_func_void()) {
		stmt_val.push(Koopa_val(new Koopa_val_temp_symbol(now_var)));
	}
}

}   // namespace Ast_Defs
}   // namespace Ast_Base
