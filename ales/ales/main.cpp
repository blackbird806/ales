#include <iostream>
#include <fstream>
#include <algorithm>
#include <variant>
#include "ales.hpp"
#include "vm.hpp"

int main()
{
	std::ifstream source("../../src/test.als");
	std::string const code((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
	ales::Lexer lexer(code);
	ales::Parser parser(lexer);
	auto c = parser.parse();
	std::cout << c.value() << "\n";
	ales::Compiler compiler;

	compiler.func_compiler["+"] = [](ales::Statement const& enclosing, ales::Compiler& compiler)
	{
		if (enclosing.cells.size() < 2)
		{
			printf("error not enough arguments for + line %d\n", enclosing.line);
			return ales::RetType::Err;
		}
		
		// TODO: review how to handle compilation order
		ales::OpCode addOp = ales::OpCode::AddInt;
		ales::RetType const t1 = compiler.compile(enclosing.cells[1], &enclosing);
		if (t1 == ales::RetType::Int)
		{
			addOp = ales::OpCode::AddInt;
		}
		else if (t1 == ales::RetType::Float)
		{
			addOp = ales::OpCode::AddFloat;
		}
		else
		{
			printf("error only Int or float can be added\n");
			return ales::RetType::Err;
		}
		
		for (auto it = enclosing.cells.begin() + 2; it != enclosing.cells.end(); ++it)
		{
			ales::RetType const t = compiler.compile(*it, &enclosing);
			if (t != ales::RetType::Int && t != ales::RetType::Float)
			{
				printf("args of + must be either int literal, float literal or a variable !\n");
				return ales::RetType::Err;
			}
			
			compiler.chunk.write(addOp);
		}
		
		if (addOp == ales::OpCode::AddInt)
			return ales::RetType::Int;
		return ales::RetType::Float;
	};

	compiler.func_compiler["set"] = [](ales::Statement const& enclosing, ales::Compiler& compiler)
	{
		if (enclosing.cells.size() < 2)
		{
			printf("error not enough arguments for + line %d", enclosing.line);
			return ales::RetType::Err;
		}

		for (auto it = enclosing.cells.cbegin() + 2; it != enclosing.cells.cend(); ++it)
		{
			compiler.compile(*it, &enclosing);
		}
		
		compiler.chunk.write(ales::OpCode::Store);
		compiler.chunk.writeStr(std::get<ales::Variable>(enclosing.cells[1].value).name);
		return ales::RetType::Void;
	};

	compiler.compile(c.value());
	ales::VirtualMachine vm;
	std::cout << compiler.chunk;
	vm.run(compiler.chunk);
	std::cout << "a = " << vm.mainEnv.symbols["a"];
	return 0;
}
