#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "ir.hpp"
#include "koopa.h"

namespace Asm_Val_Defs {

enum Asm_val_type {
	ASM_VAL_TYPE_IMMEDIATE,
	ASM_VAL_TYPE_STACK,
	ASM_VAL_TYPE_REGISTER,
	ASM_VAL_TYPE_GLOBAL_VAR,
};

class Asm_val {
protected:
	void access_ptr_via_reg(std::string reg, bool is_load, Outp &outstr) const {
		std::string tmp_reg = (reg == "t0" ? "t1" : "t0");
		load_to_reg(tmp_reg, outstr);
		outstr << (is_load ? "lw" : "sw") << " " << reg << ", 0(" << tmp_reg << ")\n";
	}

public:
	Asm_val() {}
	virtual ~Asm_val() = default;
	virtual void load_to_reg(std::string reg, Outp &outstr) const = 0;
	virtual void load_addr_to_reg(std::string reg, Outp &outstr) const { assert(0); }
	virtual void load_ptr_to_reg(std::string reg, Outp &outstr) const { access_ptr_via_reg(reg, true, outstr); }
	virtual void load_real_to_reg(std::string reg, Outp &outstr) const { load_to_reg(reg, outstr); }
	virtual void assign_ptr_from_reg(std::string reg, Outp &outstr) const { access_ptr_via_reg(reg, false, outstr); }
	virtual void assign_from_reg(std::string reg, Outp &outstr) const = 0;
	virtual void assign_addr_from_reg(std::string reg, Outp &outstr) const { assert(0); }
};

class Asm_val_im : public Asm_val {
private:
	int data;

public:
	Asm_val_im(int x) { data = x; }
	void assign_from_reg(std::string reg, Outp &outstr) const override { assert(0); }
	void load_to_reg(std::string reg, Outp &outstr) const override {
		outstr << "li " << reg << ", " << data << "\n";
	}
};

class Asm_val_localvar : public Asm_val {
private:
	int offset;

public:
	Asm_val_localvar(int input_offset) { offset = input_offset; }
	void assign_from_reg(std::string reg, Outp &outstr) const override {
		outstr << "sw " << reg << ", " << offset << "(sp)\n";
	}
	void load_to_reg(std::string reg, Outp &outstr) const override {
		outstr << "lw " << reg << ", " << offset << "(sp)\n";
	}
	void load_addr_to_reg(std::string reg, Outp &outstr) const override {
		outstr << "li t0, " << offset << "\n"
			   << "add " << reg << ", sp, t0\n";
	}
};

class Asm_val_reg : public Asm_val {
private:
	std::string id;

public:
	Asm_val_reg(std::string name) { id = std::move(name); }
	void assign_from_reg(std::string reg, Outp &outstr) const override {
		outstr << "mv " << id << ", " << reg << "\n";
	}
	void load_to_reg(std::string reg, Outp &outstr) const override {
		outstr << "mv " << reg << ", " << id << "\n";
	}
};

class Asm_val_globalvar : public Asm_val {
private:
	std::string id;

public:
	Asm_val_globalvar(std::string name) { id = std::move(name); }
	void assign_from_reg(std::string reg, Outp &outstr) const override {
		std::string tmp_reg = (reg == "t0" ? "t1" : "t0");
		outstr << "la " << tmp_reg << ", " << id << "\n"
			   << "sw " << reg << ", 0(" << tmp_reg << ")\n";
	}
	void load_to_reg(std::string reg, Outp &outstr) const override {
		outstr << "la t0, " << id << "\n"
			   << "lw " << reg << ", 0(t0)\n";
	}
	void load_addr_to_reg(std::string reg, Outp &outstr) const override {
		outstr << "la " << reg << ", " << id << "\n";
	}
};

class Asm_val_localptr : public Asm_val {
private:
	int offset;

public:
	Asm_val_localptr(int input_offset) { offset = input_offset; }
	void load_to_reg(std::string reg, Outp &outstr) const override {
		std::string tmp_reg = (reg == "t0" ? "t1" : "t0");
		load_addr_to_reg(tmp_reg, outstr);
		outstr << "lw " << reg << ", 0(" << tmp_reg << ")\n";
	}
	void load_addr_to_reg(std::string reg, Outp &outstr) const override {
		outstr << "lw " << reg << ", " << offset << "(sp)\n";
	}
	void load_real_to_reg(std::string reg, Outp &outstr) const override {
		load_addr_to_reg(reg, outstr);
	}
	void assign_from_reg(std::string reg, Outp &outstr) const override {
		std::string tmp_reg = (reg == "t0" ? "t1" : "t0");
		load_addr_to_reg(tmp_reg, outstr);
		outstr << "sw " << reg << ", 0(" << tmp_reg << ")\n";
	}
	void assign_addr_from_reg(std::string reg, Outp &outstr) const override {
		outstr << "sw " << reg << ", " << offset << "(sp)\n";
	}
};
}   // namespace Asm_Val_Defs

using namespace Asm_Val_Defs;

namespace Global_State {

int offset_cnt;
int basic_blk_cnt;
std::stack<int> function_stack_mem;
std::stack<bool> save_ra;

}   // namespace Global_State

std::unordered_map<void *, std::shared_ptr<Asm_val>> valmp;
std::unordered_map<koopa_raw_basic_block_t, std::string> blk_id_mp;
std::unordered_set<void *> visited;

// return mem(byte).

int get_array_size(const koopa_raw_type_t arr) {
	if(arr->tag == KOOPA_RTT_ARRAY) {
		return arr->data.array.len * get_array_size(arr->data.array.base);
	} else if(arr->tag == KOOPA_RTT_POINTER) {
		return get_array_size(arr->data.pointer.base);
	} else {
		return 4;
	}
}

int get_array_size(const koopa_raw_value_t arr) {
	// assert(arr->kind.tag == KOOPA_RVT_ALLOC);
	return get_array_size(arr->ty);
}

int get_array_len(const koopa_raw_type_t arr) {
	if(arr->tag == KOOPA_RTT_ARRAY) {
		return arr->data.array.len;
	} else if(arr->tag == KOOPA_RTT_POINTER) {
		return get_array_len(arr->data.pointer.base);
	} else {
		return 1;
	}
}

int get_array_len(const koopa_raw_value_t arr) {
	return get_array_len(arr->ty);
}

int get_function_stack_mem(const koopa_raw_value_t &val) {
	if(val->ty->tag == KOOPA_RTT_UNIT) {
		return 0;
	}
	if(val->kind.tag == KOOPA_RVT_GET_ELEM_PTR || val->kind.tag == KOOPA_RVT_GET_PTR) {
		valmp[(void *)val] = std::make_shared<Asm_val_localptr>(Global_State::offset_cnt);
	} else {
		valmp[(void *)val] = std::make_shared<Asm_val_localvar>(Global_State::offset_cnt);
	}
	int siz = 4;
	if(val->kind.tag == KOOPA_RVT_ALLOC) {
		siz = get_array_size(val);
	}
	Global_State::offset_cnt += siz;
	return siz;
}

int get_function_stack_mem(const koopa_raw_basic_block_t &blk) {
	int sum_size = 0;
	for(size_t i = 0; i < blk->insts.len; i++) {
		assert(blk->insts.kind == KOOPA_RSIK_VALUE);
		koopa_raw_value_t val = (koopa_raw_value_t)blk->insts.buffer[i];
		sum_size += get_function_stack_mem(val);
	}
	return sum_size;
}

int get_function_stack_mem(const koopa_raw_function_t &func) {
	int sum_size = 0;
	for(size_t i = 0; i < func->bbs.len; i++) {
		assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
		koopa_raw_basic_block_t blk = (koopa_raw_basic_block_t)func->bbs.buffer[i];
		sum_size += get_function_stack_mem(blk);
	}
	return sum_size;
}

int get_function_max_call_param(const koopa_raw_value_t &val) {
	if(val->kind.tag != KOOPA_RVT_CALL) {
		return -1;
	}
	return val->kind.data.call.args.len;
}

int get_function_max_call_param(const koopa_raw_basic_block_t &blk) {
	int ret = -1;
	for(size_t i = 0; i < blk->insts.len; i++) {
		assert(blk->insts.kind == KOOPA_RSIK_VALUE);
		koopa_raw_value_t val = (koopa_raw_value_t)blk->insts.buffer[i];
		ret = std::max(ret, get_function_max_call_param(val));
	}
	return ret;
}

int get_function_max_call_param(const koopa_raw_function_t &func) {
	int ret = -1;
	for(size_t i = 0; i < func->bbs.len; i++) {
		assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
		koopa_raw_basic_block_t blk = (koopa_raw_basic_block_t)func->bbs.buffer[i];
		ret = std::max(ret, get_function_max_call_param(blk));
	}
	return ret;
}

int get_function_mem(const koopa_raw_function_t &func) {
	int max_call_param = get_function_max_call_param(func);
	int param_mem = std::max(max_call_param - 8, 0) * 4;
	Global_State::offset_cnt = param_mem;
	int stack_mem = get_function_stack_mem(func);
	Global_State::save_ra.push(max_call_param != -1);
	int sum_mem = param_mem + stack_mem + (max_call_param == -1 ? 0 : 4);
	return int(std::ceil(sum_mem / 16.0)) * 16;
}

void dfs_ir(const koopa_raw_program_t &prog, Outp &outstr) {
	for(size_t i = 0; i < prog.values.len; i++) {
		koopa_raw_value_t val = (koopa_raw_value_t)prog.values.buffer[i];
		dfs_ir(val, outstr);
	}
	for(size_t i = 0; i < prog.funcs.len; i++) {
		koopa_raw_function_t func = (koopa_raw_function_t)prog.funcs.buffer[i];
		dfs_ir(func, outstr);
	}
}

void dfs_ir(const koopa_raw_function_t &func, Outp &outstr) {
	if(func->bbs.len == 0) return;
	outstr << ".text\n";
	outstr << ".global " << (func->name + 1) << "\n";
	outstr << (func->name + 1) << ":\n";
	int mem = get_function_mem(func);
	Global_State::function_stack_mem.push(mem);
	if(mem > 0) {
		outstr << "li t0, " << -mem << "\nadd sp, sp, t0\n";
	}
	if(Global_State::save_ra.top()) {
		outstr << "sw ra, " << mem - 4 << "(sp)\n";
	}
	for(size_t i = 0; i < func->bbs.len; i++) {
		assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
		koopa_raw_basic_block_t blk = (koopa_raw_basic_block_t)func->bbs.buffer[i];
		blk_id_mp[blk] = "block_" + std::to_string(Global_State::basic_blk_cnt);
		Global_State::basic_blk_cnt++;
	}
	for(size_t i = 0; i < func->bbs.len; i++) {
		assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
		koopa_raw_basic_block_t blk = (koopa_raw_basic_block_t)func->bbs.buffer[i];
		dfs_ir(blk, outstr);
	}
	Global_State::function_stack_mem.pop();
	Global_State::save_ra.pop();
	// ret: sp -= mem;
}

void dfs_ir(const koopa_raw_basic_block_t &blk, Outp &outstr) {
	outstr << blk_id_mp[blk] << ":\n";
	for(size_t i = 0; i < blk->insts.len; i++) {
		assert(blk->insts.kind == KOOPA_RSIK_VALUE);
		koopa_raw_value_t val = (koopa_raw_value_t)blk->insts.buffer[i];
		dfs_ir(val, outstr);
	}
}

void aggregate_global_init(const koopa_raw_value_t &aggr, Outp &outstr) {
	switch(aggr->kind.tag) {
	case KOOPA_RVT_ZERO_INIT:
		outstr << ".zero " << get_array_size(aggr->ty) << "\n";
		break;
	case KOOPA_RVT_INTEGER:
		outstr << ".word " << aggr->kind.data.integer.value << "\n";
		break;
	case KOOPA_RVT_AGGREGATE: {
		const koopa_raw_slice_t &elems = aggr->kind.data.aggregate.elems;
		for(int i = 0; i < elems.len; i++) {
			aggregate_global_init(((koopa_raw_value_t *)elems.buffer)[i], outstr);
		}
		break;
	}
	default: assert(0);
	}
}

void dfs_ir(const koopa_raw_value_t &val, Outp &outstr) {
	const auto &kind = val->kind;
	if(val->ty->tag != KOOPA_RTT_UNIT && kind.tag != KOOPA_RVT_INTEGER) {
		if(kind.tag == KOOPA_RVT_FUNC_ARG_REF || kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
			if(valmp.contains((void *)val)) {
				return;
			}
		} else {
			assert(valmp.contains((void *)val));
			if(visited.contains((void *)val)) {
				return;
			} else {
				visited.insert((void *)val);
			}
		}
	}
	switch(kind.tag) {
	case KOOPA_RVT_BINARY:
		if(!valmp.contains((void *)&kind.data.binary)) {
			valmp[(void *)&kind.data.binary] = valmp[(void *)val];
		}
		dfs_ir(kind.data.binary, outstr);
		break;
	case KOOPA_RVT_RETURN:
		dfs_ir(kind.data.ret, outstr);
		break;
	case KOOPA_RVT_INTEGER:
		if(valmp.contains((void *)val)) break;
		valmp[(void *)val] = std::make_shared<Asm_val_im>(kind.data.integer.value);
		break;
	case KOOPA_RVT_ALLOC:
		break;
	case KOOPA_RVT_STORE:
		dfs_ir(kind.data.store.value, outstr);
		valmp[(void *)kind.data.store.value]->load_to_reg("t0", outstr);
		valmp[(void *)kind.data.store.dest]->assign_from_reg("t0", outstr);
		break;
	case KOOPA_RVT_LOAD:
		valmp[(void *)kind.data.load.src]->load_to_reg("t0", outstr);
		valmp[(void *)val]->assign_from_reg("t0", outstr);
		break;
	case KOOPA_RVT_BRANCH:
		dfs_ir(kind.data.branch.cond, outstr);
		valmp[(void *)kind.data.branch.cond]->load_to_reg("t0", outstr);
		assert(blk_id_mp.contains(kind.data.branch.true_bb));
		assert(blk_id_mp.contains(kind.data.branch.false_bb));
		outstr << "bnez t0, " << blk_id_mp[kind.data.branch.true_bb] << "\n";
		outstr << "j " << blk_id_mp[kind.data.branch.false_bb] << "\n";
		break;
	case KOOPA_RVT_JUMP:
		assert(blk_id_mp.contains(kind.data.jump.target));
		outstr << "j " << blk_id_mp[kind.data.jump.target] << "\n";
		break;
	case KOOPA_RVT_FUNC_ARG_REF: {
		int index = kind.data.func_arg_ref.index;
		std::shared_ptr<Asm_val> asm_val;
		if(index < 8) {
			asm_val.reset(new Asm_val_reg(std::string("a") + std::to_string(index)));
		} else {
			int mem = Global_State::function_stack_mem.top() + (index - 8) * 4;
			asm_val.reset(new Asm_val_localvar(mem));
		}
		valmp[(void *)val] = asm_val;
		break;
	}
	case KOOPA_RVT_CALL: {
		auto &args = kind.data.call.args;
		for(int i = 0; i < args.len; i++) {
			dfs_ir((koopa_raw_value_t)args.buffer[i], outstr);
			auto asm_val = valmp[(void *)args.buffer[i]];
			asm_val->load_real_to_reg("t0", outstr);
			if(i < 8) {
				outstr << "mv a" << i << ", t0\n";
			} else {
				outstr << "sw t0, " << (i - 8) * 4 << "(sp)\n";
			}
		}
		outstr << "call " << kind.data.call.callee->name + 1 << "\n";
		if(kind.data.call.callee->ty->data.function.ret->tag != KOOPA_RTT_UNIT) {
			valmp[(void *)val]->assign_from_reg("a0", outstr);
		}
		break;
	}
	case KOOPA_RVT_GLOBAL_ALLOC:
		outstr << ".data\n"
			   << ".global " << (val->name + 1) << "\n"
			   << (val->name + 1) << ":\n";
		aggregate_global_init(kind.data.global_alloc.init, outstr);
		// if(kind.data.global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
		// 	outstr << ".zero " << get_array_size(kind.data.global_alloc.init->ty) << "\n";
		// } else {
		// 	outstr << ".word " << kind.data.global_alloc.init->kind.data.integer.value << "\n";
		// }
		valmp[(void *)val] = std::make_shared<Asm_val_globalvar>(val->name + 1);
		break;
	case KOOPA_RVT_GET_ELEM_PTR:
		dfs_ir(kind.data.get_elem_ptr.src, outstr);
		dfs_ir(kind.data.get_elem_ptr.index, outstr);
		valmp[(void *)kind.data.get_elem_ptr.src]->load_addr_to_reg("t0", outstr);
		valmp[(void *)kind.data.get_elem_ptr.index]->load_to_reg("t1", outstr);
		outstr << "li t2, "
			   << get_array_size(kind.data.get_elem_ptr.src) /
					  get_array_len(kind.data.get_elem_ptr.src)
			   << "\n";
		outstr << "mul t1, t1, t2\n"
			   << "add t0, t0, t1\n";
		valmp[(void *)val]->assign_addr_from_reg("t0", outstr);
		break;
	case KOOPA_RVT_GET_PTR:
		dfs_ir(kind.data.get_elem_ptr.src, outstr);
		dfs_ir(kind.data.get_elem_ptr.index, outstr);
		valmp[(void *)kind.data.get_elem_ptr.src]->load_to_reg("t0", outstr);
		valmp[(void *)kind.data.get_elem_ptr.index]->load_to_reg("t1", outstr);
		outstr << "li t2, "
			   << get_array_size(kind.data.get_elem_ptr.src) /
					  get_array_len(kind.data.get_elem_ptr.src)
			   << "\n";
		outstr << "mul t1, t1, t2\n"
			   << "add t0, t0, t1\n";
		valmp[(void *)val]->assign_addr_from_reg("t0", outstr);
		break;
	default:
		std::cerr << "koopa_raw_value_t not handled: " << kind.tag << '\n';
		assert(0);
	}
}

void dfs_ir(const koopa_raw_return_t &ret, Outp &outstr) {
	if(ret.value != nullptr) {
		dfs_ir(ret.value, outstr);
		valmp[(void *)ret.value]->load_to_reg("a0", outstr);
	}
	int mem = Global_State::function_stack_mem.top();
	if(Global_State::save_ra.top()) {
		outstr << "lw ra, " << mem - 4 << "(sp)\n";
	}
	if(mem > 0) {
		outstr << "li t0, " << mem << "\n";
	}
	outstr << "add sp, sp, t0\nret\n";
	return;
}

void dfs_ir(const koopa_raw_binary_t &bin, Outp &outstr) {
	assert(valmp.contains((void *)&bin));
	if(visited.contains((void *)&bin)) {
		return;
	} else {
		visited.insert((void *)&bin);
	}
	dfs_ir(bin.lhs, outstr);
	dfs_ir(bin.rhs, outstr);
	valmp[(void *)bin.lhs]->load_to_reg("t0", outstr);
	valmp[(void *)bin.rhs]->load_to_reg("t1", outstr);
	switch(bin.op) {
	case KOOPA_RBO_ADD:
		outstr << "add";
		break;
	case KOOPA_RBO_SUB:
		outstr << "sub";
		break;
	case KOOPA_RBO_MUL:
		outstr << "mul";
		break;
	case KOOPA_RBO_DIV:
		outstr << "div";
		break;
	case KOOPA_RBO_MOD:
		outstr << "rem";
		break;
	case KOOPA_RBO_EQ:
	case KOOPA_RBO_NOT_EQ:
		outstr << "xor";
		break;
	case KOOPA_RBO_LE:
	case KOOPA_RBO_GT:
		outstr << "sgt";
		break;
	case KOOPA_RBO_GE:
	case KOOPA_RBO_LT:
		outstr << "slt";
		break;
	case KOOPA_RBO_AND:
		outstr << "and";
		break;
	case KOOPA_RBO_OR:
		outstr << "or";
		break;
	default:
		std::cerr << "Unsupported binary operator: " << bin.op << '\n';
		assert(0);
	}
	outstr << " t0, t0, t1\n";
	switch(bin.op) {
	case KOOPA_RBO_EQ:
	case KOOPA_RBO_LE:
	case KOOPA_RBO_GE:
		outstr << "seqz t0, t0\n";
		break;
	case KOOPA_RBO_NOT_EQ:
		outstr << "snez t0, t0\n";
		break;
	default:
		break;
	}
	valmp[(void *)&bin]->assign_from_reg("t0", outstr);
}
