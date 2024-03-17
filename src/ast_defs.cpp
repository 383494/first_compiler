#include "ast_defs.hpp"

namespace Ast_Base{
int var_count = 0;

namespace Ast_Defs{

void CompUnitAST::output(Ost &outstr, std::string prefix) const {
	func_def->output(outstr, prefix);
}

void FuncDefAST::output(Ost &outstr, std::string prefix) const {
	outstr << prefix << "fun @" << ident << "():";
	func_typ->output(outstr, "");
	outstr << "{\n";
	block->output(outstr, prefix + INDENT);
	outstr << "\n" << prefix << "}\n";
}

void TypAST::output(Ost &outstr, std::string) const {
	outstr << typ;
}

void BlockAST::output(Ost &outstr, std::string prefix) const {
	outstr << prefix << "%entry:\n";
	stmt->output(outstr, prefix + INDENT);
}

void StmtAST::output(Ost &outstr, std::string prefix) const {
	ret_val->output(outstr, prefix);
	outstr << prefix << "ret %" << var_count-1;
}

void ExpAST::output(Ost& outstr, std::string prefix) const {
	unary_exp->output(outstr, prefix);
}

void UnaryExpAST::output(Ost &outstr, std::string prefix) const {
	if(unary_op.has_value()){
		unary_exp->output(outstr, prefix);
		int now_var = var_count;
		var_count++;
		outstr << prefix << "%" << now_var << " = ";
		(*unary_op)->output(outstr, "");
		outstr << "%" << now_var-1 << "\n";
	} else {
		unary_exp->output(outstr, prefix);
	}
}

void PrimaryExpAST::output(Ost &outstr, std::string prefix) const {
	switch(inside_exp.index()){
		case 0:
			std::get<0>(inside_exp)->output(outstr, prefix);
		break;
		case 1:
			outstr << prefix << "%" << var_count << " = "
				<< std::get<1>(inside_exp) << "\n";
			var_count++;
		break;
		default:;
	}
}

void UnaryOpAST::output(Ost &outstr, std::string prefix) const {
	outstr << prefix;
	switch(op){
		case OP_ADD:
			// do nothing
		break;
		case OP_SUB:
			outstr << "sub 0, ";
		break;
		case OP_NOT:
			outstr << "ne 0, ";
		break;
		default:
			throw "UnaryOp type error";
		break;
	}
}
}
}
