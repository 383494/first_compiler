#include <cassert>

#include "ir.hpp"
#include "koopa.h"

void dfs_ir(const koopa_raw_program_t& prog, Outp &outstr){
	outstr << ".text\n";
	outstr << ".global main\n";
	for(size_t i = 0; i < prog.funcs.len; i++){
		assert(prog.funcs.kind == KOOPA_RSIK_FUNCTION);
		koopa_raw_function_t func = (koopa_raw_function_t) prog.funcs.buffer[i];
		dfs_ir(func, outstr);
	}
}

void dfs_ir(const koopa_raw_function_t& func, Outp &outstr){
	outstr << (func->name+1) << ":\n";
	for(size_t i = 0; i < func->bbs.len; i++){
		assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
		koopa_raw_basic_block_t blk = (koopa_raw_basic_block_t)func->bbs.buffer[i];
		dfs_ir(blk, outstr);
	}
}

void dfs_ir(const koopa_raw_basic_block_t& blk, Outp &outstr){
	for(size_t i = 0; i < blk->insts.len; i++){
		assert(blk->insts.kind == KOOPA_RSIK_VALUE);
		koopa_raw_value_t val = (koopa_raw_value_t)blk->insts.buffer[i];
		dfs_ir(val, outstr);
	}
}

void dfs_ir(const koopa_raw_value_t& val, Outp &outstr){
	const auto &kind = val->kind;
	switch(kind.tag){
		case KOOPA_RVT_RETURN:
			dfs_ir(kind.data.ret, outstr);
		break;
		// case KOOPA_RVT_INTEGER:
		// 	dfs_ir(kind.data.integer, outstr);
		// break;
		default:
			assert(0);
	}
}

void dfs_ir(const koopa_raw_return_t& ret, Outp &outstr){
	outstr << "li a0, 0\n";
	outstr << "ret\n";
	return;
}

