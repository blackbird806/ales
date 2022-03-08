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
	auto cells = parser.parse();

	for (auto const& c : cells)
		std::cout << c << "\n";


	ales::Compiler compiler;

	compiler.func_compiler["+"] = []()
	{
		ales::CodeChunk chunk;

		chunk.write(ales::OpCode::Add);
		return chunk;
	};

	//compiler.func_compiler["set"] = [](ales::Statement const& enclosing, ales::Compiler& compiler)
	//{
	//	if (enclosing.cells.size() < 2)
	//	{
	//		printf("error not enough arguments for + line %d", enclosing.line);
	//		return ales::RetType::Err;
	//	}

	//	for (auto it = enclosing.cells.cbegin() + 2; it != enclosing.cells.cend(); ++it)
	//	{
	//		compiler.compile(*it, &enclosing);
	//	}
	//	
	//	compiler.chunk.write(ales::OpCode::Store);
	//	compiler.chunk.writeStr(std::get<ales::Symbol>(enclosing.cells[1].value).name);
	//	return ales::RetType::Void;
	//};

	//compiler.func_compiler["defun"] = [](ales::Statement const& enclosing, ales::Compiler& compiler)
	//{
	//	if (enclosing.cells.size() < 3)
	//	{
	//		printf("to few args for function declaration\n");
	//		return ales::RetType::Err;
	//	}

	//	compiler.addFunction(std::get<ales::Symbol>(enclosing.cells[1].value).name, std::vector<ales::Cell>(enclosing.cells.begin() + 3, enclosing.cells.end()), &enclosing);
	//	
	//	return ales::RetType::Void;
	//};
	//

	compiler.compile(cells[0]);

	int i = 0; 
	for (auto const& chunk : compiler.functions)
	{
		std::cout << i++ << "\n" << chunk << "\n========\n";
	}
	ales::VirtualMachine vm;
	vm.run(compiler.functions[1]);
	return 0;
}
