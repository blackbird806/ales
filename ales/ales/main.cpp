#include <iostream>
#include <fstream>
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
	compiler.symbol_compilers["+"] = []()
	{
		ales::CodeChunk chunk;
		chunk.write(ales::OpCode::AddFloat);
		return chunk.code_data;
	};
	auto chunk = compiler.compile(c.value());
	ales::VirtualMachine vm;
	std::cout << chunk;
	vm.run(chunk);
	return 0;
}
