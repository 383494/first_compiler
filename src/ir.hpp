#pragma once

#include <sstream>
#include "koopa.h"

using Outp = std::ostringstream;

void dfs_ir(const koopa_raw_program_t& prog, Outp &outstr);
void dfs_ir(const koopa_raw_function_t& func, Outp &outstr);
void dfs_ir(const koopa_raw_basic_block_t& blk, Outp &outstr);
void dfs_ir(const koopa_raw_value_t& val, Outp &outstr);
void dfs_ir(const koopa_raw_return_t& ret, Outp &outstr);

