#include <cassert>
#include <iostream>
#include <unordered_map>

#include "ir.hpp"
#include "koopa.h"

// class Asm_val{
// private:
// 	int val;
// 	bool is_im;
// public:
// 	Asm_val(bool typ, int x){
// 		is_im = typ;
// 		val = x;
// 	}
// 	template<typename T>
// 	friend T &operator<<(T& outstr, Asm_val &me){
// 		if(me.is_im){
// 			outstr << me.val;
// 		} else {
// 			outstr << "a" << me.val;
// 		}
// 		return outstr;
// 	}
// };

int var_cnt = 0;

std::unordered_map<void *, int> valmp;

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
	for(size_t i = 0; i < func->bbs.len; i++) {
		assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
		koopa_raw_basic_block_t blk = (koopa_raw_basic_block_t)func->bbs.buffer[i];
		dfs_ir(blk, outstr);
	}
}
void dfs_ir(const koopa_raw_basic_block_t &blk, Outp &outstr) {
	for(size_t i = 0; i < blk->insts.len; i++) {
		assert(blk->insts.kind == KOOPA_RSIK_VALUE);
		koopa_raw_value_t val = (koopa_raw_value_t)blk->insts.buffer[i];
		dfs_ir(val, outstr);
	}
}

void dfs_ir(const koopa_raw_value_t &val, Outp &outstr) {
	if(valmp.contains((void *)val)) return;
	const auto &kind = val->kind;
	switch(kind.tag) {
	case KOOPA_RVT_BINARY:
		dfs_ir(kind.data.binary, outstr);
		valmp[(void *)val] = valmp[(void *)&kind.data.binary];
		break;
	case KOOPA_RVT_RETURN:
		dfs_ir(kind.data.ret, outstr);
		break;
	case KOOPA_RVT_INTEGER:
		outstr << "li t" << var_cnt << ", " << kind.data.integer.value << '\n';
		valmp[(void *)val] = var_cnt;
		var_cnt++;
		break;
	default:
		assert(0);
	}
}

void dfs_ir(const koopa_raw_return_t &ret, Outp &outstr) {
	dfs_ir(ret.value, outstr);
	outstr << "mv a0, t" << valmp[(void *)ret.value] << "\n";
	outstr << "ret\n";
	return;
}

void dfs_ir(const koopa_raw_binary_t &bin, Outp &outstr) {
	if(valmp.contains((void *)&bin)) return;
	dfs_ir(bin.lhs, outstr);
	dfs_ir(bin.rhs, outstr);
	switch(bin.op) {
	case KOOPA_RBO_ADD:
		outstr << "add";
		break;
	case KOOPA_RBO_SUB:
		outstr << "sub";
		break;
	case KOOPA_RBO_EQ:
	case KOOPA_RBO_NOT_EQ:
		outstr << "xor";
		break;
	case KOOPA_RBO_MUL:
		outstr << "mul";
		break;
	case KOOPA_RBO_DIV:
		outstr << "div";
		break;
	case KOOPA_RBO_MOD:
		outstr << "mod";
		break;
	default:
		std::cerr << "Unsupported binary operator: " << bin.op << '\n';
		assert(0);
	}
	outstr << " t" << var_cnt
		   << ", "
		   << "t" << valmp[(void *)bin.lhs]
		   << ", "
		   << "t" << valmp[(void *)bin.rhs]
		   << '\n';
	switch(bin.op) {
	case KOOPA_RBO_EQ:
		outstr << "seqz t" << var_cnt << ", t" << var_cnt << '\n';
		break;
	case KOOPA_RBO_NOT_EQ:
		outstr << "snez t" << var_cnt << ", t" << var_cnt << '\n';
		break;
	default:
		break;
	}
	valmp[(void *)&bin] = var_cnt;
	var_cnt++;
}
