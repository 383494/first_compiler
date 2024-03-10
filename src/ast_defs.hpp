#include <string>
#include <iostream>
#include <memory>

class BaseAST{
public:
	~BaseAST() = default;
	friend std::ostream &operator<<(std::ostream &cout, const BaseAST &me){
		return cout;
	}
};

class CompUnitAST{
public:
	std::shared_ptr<BaseAST> func_def;
};
