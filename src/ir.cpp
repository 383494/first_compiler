#include <cassert>
#include <iostream>
#include <memory>
#include <stack>
#include <unordered_map>

#include "ir.hpp"
#include "koopa.h"

class Asm_val {
	int data;

public:
	bool assigned;
	bool is_im;

	Asm_val(int dat, bool isim) {
		data = dat;   // input_offset or immediate_number
		is_im = isim;
		assigned = isim;
	}
	template<typename T>
	void assign_from_t0(T &outstr) {
		assert(!is_im);
		outstr << "sw t0, " << data << "(sp)\n";
		assigned = true;
	}
	template<typename T>
	void load(T &outstr, std::string reg) {
		if(is_im) {
			outstr << "li " << reg << ", " << data << '\n';
		} else {
			outstr << "lw " << reg << ", " << data << "(sp)\n";
		}
	}
};

namespace Global_State {

int offset_cnt;
int basic_blk_cnt;
std::stack<int> function_stack_mem;

}   // namespace Global_State

std::unordered_map<void *, std::shared_ptr<Asm_val>> valmp;
std::unordered_map<koopa_raw_basic_block_t, std::string> blk_id_mp;

// return mem(byte).

int get_function_stack_mem(const koopa_raw_value_t &val) {
	if(val->ty->tag == KOOPA_RTT_UNIT) {
		return 0;
	}
	valmp[(void *)val] = std::make_shared<Asm_val>(Global_State::offset_cnt, false);
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
	Global_State::offset_cnt = 0;
	for(size_t i = 0; i < func->bbs.len; i++) {
		assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
		koopa_raw_basic_block_t blk = (koopa_raw_basic_block_t)func->bbs.buffer[i];
		sum_size += get_function_stack_mem(blk);
	}
	return sum_size;
}

void dfs_ir(const koopa_raw_program_t &prog, Outp &outstr) {
	outstr << ".text\n";
	outstr << ".global main\n";
	for(size_t i = 0; i < prog.funcs.len; i++) {
		assert(prog.funcs.kind == KOOPA_RSIK_FUNCTION);
		koopa_raw_function_t func = (koopa_raw_function_t)prog.funcs.buffer[i];
		dfs_ir(func, outstr);
	}
}

void dfs_ir(const koopa_raw_function_t &func, Outp &outstr) {
	outstr << (func->name + 1) << ":\n";
	int mem = get_function_stack_mem(func);
	Global_State::function_stack_mem.push(mem);
	if(mem > 0) {
		outstr << "li t0, " << -mem << "\nadd sp, sp, t0\n";
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
		assert(valmp.contains((void *)val));
		if(valmp[(void *)val]->assigned) return;
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
		valmp[(void *)val] = std::make_shared<Asm_val>(kind.data.integer.value, true);
		break;
	case KOOPA_RVT_ALLOC:
		break;
	case KOOPA_RVT_STORE:
		dfs_ir(kind.data.store.value, outstr);
		valmp[(void *)kind.data.store.value]->load(outstr, "t0");
		valmp[(void *)kind.data.store.dest]->assign_from_t0(outstr);
		break;
	case KOOPA_RVT_LOAD:
		valmp[(void *)kind.data.load.src]->load(outstr, "t0");
		valmp[(void *)val]->assign_from_t0(outstr);
		break;
	case KOOPA_RVT_BRANCH:
		dfs_ir(kind.data.branch.cond, outstr);
		valmp[(void *)kind.data.branch.cond]->load(outstr, "t0");
		assert(blk_id_mp.contains(kind.data.branch.true_bb));
		assert(blk_id_mp.contains(kind.data.branch.false_bb));
		outstr << "bnez t0, " << blk_id_mp[kind.data.branch.true_bb] << "\n";
		outstr << "j " << blk_id_mp[kind.data.branch.false_bb] << "\n";
		break;
	case KOOPA_RVT_JUMP:
		assert(blk_id_mp.contains(kind.data.jump.target));
		outstr << "j " << blk_id_mp[kind.data.jump.target] << "\n";
		break;
	default:
		std::cerr << "koopa_raw_value_t not handled: " << kind.tag << '\n';
		assert(0);
	}
}

void dfs_ir(const koopa_raw_return_t &ret, Outp &outstr) {
	dfs_ir(ret.value, outstr);
	valmp[(void *)ret.value]->load(outstr, "a0");
	int mem = Global_State::function_stack_mem.top();
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
	valmp[(void *)bin.lhs]->load(outstr, "t0");
	valmp[(void *)bin.rhs]->load(outstr, "t1");
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
