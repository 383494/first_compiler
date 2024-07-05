#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <stack>
#include <unordered_map>

#include "ir.hpp"
#include "koopa.h"

namespace Asm_Val_Defs {

enum Asm_val_type {
	ASM_VAL_TYPE_IMMEDIATE,
	ASM_VAL_TYPE_STACK,
	ASM_VAL_TYPE_REGISTER,
};

class Asm_val {
	std::string data;
	Asm_val_type typ;

public:
	bool assigned;

	Asm_val(int data, Asm_val_type typ) {
		this->data = std::to_string(data);   // input_offset or immediate_number
		this->typ = typ;
		assigned = (typ != ASM_VAL_TYPE_STACK);
	}
	Asm_val(std::string data, Asm_val_type typ) {
		this->data = data;   // input_offset or immediate_number
		this->typ = typ;
		assigned = (typ != ASM_VAL_TYPE_STACK);
	}
	void assign_from_reg(auto &outstr, std::string reg) {
		assert(typ == ASM_VAL_TYPE_STACK);
		outstr << "sw " << reg << ", " << data << "(sp)\n";
		assigned = true;
	}
	void assign_from_t0(auto &outstr) {
		assign_from_reg(outstr, "t0");
	}
	void load_to_reg(auto &outstr, std::string reg) const {
		switch(typ) {
		case ASM_VAL_TYPE_IMMEDIATE:
			outstr << "li " << reg << ", " << data << "\n";
			break;
		case ASM_VAL_TYPE_STACK:
			outstr << "lw " << reg << ", " << data << "(sp)\n";
			break;
		case ASM_VAL_TYPE_REGISTER:
			outstr << "mv " << reg << ", " << data << "\n";
			break;
		default: assert(0);
		}
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

// return mem(byte).

int get_function_stack_mem(const koopa_raw_value_t &val) {
	if(val->ty->tag == KOOPA_RTT_UNIT) {
		return 0;
	}
	valmp[(void *)val] = std::make_shared<Asm_val>(Global_State::offset_cnt, ASM_VAL_TYPE_STACK);
	Global_State::offset_cnt += 4;
	return 4;
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
	for(size_t i = 0; i < prog.funcs.len; i++) {
		assert(prog.funcs.kind == KOOPA_RSIK_FUNCTION);
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

void dfs_ir(const koopa_raw_value_t &val, Outp &outstr) {
	if(val->ty->tag != KOOPA_RTT_UNIT && val->kind.tag != KOOPA_RVT_INTEGER) {
		if(val->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
			if(valmp.contains((void *)val)) {
				return;
			}
		} else {
			assert(valmp.contains((void *)val));
			if(valmp[(void *)val]->assigned) {
				return;
			}
		}
	}
	const auto &kind = val->kind;
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
		valmp[(void *)val] = std::make_shared<Asm_val>(kind.data.integer.value, ASM_VAL_TYPE_IMMEDIATE);
		break;
	case KOOPA_RVT_ALLOC:
		break;
	case KOOPA_RVT_STORE:
		dfs_ir(kind.data.store.value, outstr);
		valmp[(void *)kind.data.store.value]->load_to_reg(outstr, "t0");
		valmp[(void *)kind.data.store.dest]->assign_from_t0(outstr);
		break;
	case KOOPA_RVT_LOAD:
		valmp[(void *)kind.data.load.src]->load_to_reg(outstr, "t0");
		valmp[(void *)val]->assign_from_t0(outstr);
		break;
	case KOOPA_RVT_BRANCH:
		dfs_ir(kind.data.branch.cond, outstr);
		valmp[(void *)kind.data.branch.cond]->load_to_reg(outstr, "t0");
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
		int index = val->kind.data.func_arg_ref.index;
		std::shared_ptr<Asm_val> asm_val;
		if(index < 8) {
			asm_val.reset(new Asm_val(std::string("a") + std::to_string(index), ASM_VAL_TYPE_REGISTER));
		} else {
			int mem = Global_State::function_stack_mem.top() + index - 8;
			asm_val.reset(new Asm_val(mem, ASM_VAL_TYPE_STACK));
			asm_val->assigned = true;
		}
		valmp[(void *)val] = asm_val;
		break;
	}
	case KOOPA_RVT_CALL: {
		auto &args = val->kind.data.call.args;
		for(int i = 0; i < args.len; i++) {
			dfs_ir((koopa_raw_value_t)args.buffer[i], outstr);
			auto asm_val = valmp[(void *)args.buffer[i]];
			asm_val->load_to_reg(outstr, "t0");
			if(i < 8) {
				outstr << "mv a" << i << ", t0\n";
			} else {
				outstr << "sw t0, " << Global_State::function_stack_mem.top() + i - 8 << "(sp)\n";
			}
		}
		outstr << "call " << val->kind.data.call.callee->name + 1 << "\n";
		valmp[(void *)val]->assign_from_reg(outstr, "a0");
		break;
	}
	default:
		std::cerr << "koopa_raw_value_t not handled: " << kind.tag << '\n';
		assert(0);
	}
}

void dfs_ir(const koopa_raw_return_t &ret, Outp &outstr) {
	if(ret.value != nullptr) {
		dfs_ir(ret.value, outstr);
		valmp[(void *)ret.value]->load_to_reg(outstr, "a0");
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
	if(valmp[(void *)&bin]->assigned) return;
	dfs_ir(bin.lhs, outstr);
	dfs_ir(bin.rhs, outstr);
	valmp[(void *)bin.lhs]->load_to_reg(outstr, "t0");
	valmp[(void *)bin.rhs]->load_to_reg(outstr, "t1");
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
	valmp[(void *)&bin]->assign_from_t0(outstr);
}
