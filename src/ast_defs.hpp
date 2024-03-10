#include <string>
#include <iostream>
#include <memory>
#include <sstream>

namespace Ast_Base{

constexpr const char* INDENT = "  ";
using Ost = std::ostream;

namespace Ast_Defs {

class BaseAST{
public:
	virtual ~BaseAST() = default;
	virtual void output(Ost&, std::string) const = 0;
};

class CompUnitAST: public BaseAST{
public:
	std::unique_ptr<BaseAST> func_def;
	void output(Ost &outstr, std::string prefix) const override{
		//outstr << prefix << "CompUnit{\n";
		func_def->output(outstr, prefix);
		//outstr << '\n' << prefix << "}";
	}
};

class FuncDefAST: public BaseAST{
public:
	std::unique_ptr<BaseAST> func_typ;
	std::string ident;
	std::unique_ptr<BaseAST> block;
	void output(Ost &outstr, std::string prefix) const override{
		outstr << prefix << "fun @" << ident << "():";
		func_typ->output(outstr, "");
		outstr << "{\n";
		block->output(outstr, prefix + INDENT);
		outstr << "\n" << prefix << "}";
	}
};

class TypAST: public BaseAST{
public:
	std::string typ;
	void output(Ost &outstr, std::string) const override{
		outstr << typ;
	}
};

class BlockAST: public BaseAST{
public:
	std::unique_ptr<BaseAST> stmt;
	void output(Ost &outstr, std::string prefix) const override{
		outstr << prefix << "%entry:\n";
		stmt->output(outstr, prefix + INDENT);
		//outstr << '\n' << prefix << "}";
	}
};

class StmtAST: public BaseAST{
public:
	int ret_val;
	void output(Ost &outstr, std::string prefix) const override{
		outstr << prefix << "ret " << ret_val;
	}
};

} // namespace Ast_Defs

} // namespace Ast_Base

using namespace Ast_Base::Ast_Defs;

