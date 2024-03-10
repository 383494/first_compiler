#include <string>
#include <iostream>
#include <memory>

constexpr const char* INDENT = "  ";

class BaseAST{
public:
	virtual ~BaseAST() = default;
	virtual void output(std::string) const = 0;
};

class CompUnitAST: public BaseAST{
public:
	std::unique_ptr<BaseAST> func_def;
	void output(std::string prefix) const override{
		std::cout << prefix << "CompUnit{\n";
		func_def->output(prefix + INDENT);
		std::cout << '\n' << prefix << "}";
	}
};

class FuncDefAST: public BaseAST{
public:
	std::unique_ptr<BaseAST> func_typ;
	std::string ident;
	std::unique_ptr<BaseAST> block;
	void output(std::string prefix) const override{
		std::cout << prefix << "FuncDef(";
		func_typ->output("");
		std::cout << ident << "){\n";
		block->output(prefix + INDENT);
		std::cout << "\n" << prefix << "}";
	}
};


class TypAST: public BaseAST{
public:
	std::string typ;
	void output(std::string) const override{
		std::cout << typ;
	}
};

class BlockAST: public BaseAST{
public:
	std::unique_ptr<BaseAST> stmt;
	void output(std::string prefix) const override{
		std::cout << prefix << "BlkAST{\n";
		stmt->output(prefix + INDENT);
		std::cout << '\n' << prefix << "}";
	}
};

class StmtAST: public BaseAST{
public:
	int ret_val;
	void output(std::string prefix) const override{
		std::cout << "RET " << ret_val;
	}
};
