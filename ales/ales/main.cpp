#include <iostream>
#include <fstream>
#include "ales.hpp"
#include "vm.hpp"

int main()
{
	//std::ifstream source("../../src/test.als");
	//std::string const code((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
	//ales::Lexer lexer(code);
	//ales::Parser parser(lexer);
	//auto c = parser.parse();
	//std::cout << c.value();
	
	ales::CodeChunk code{};
	code.code_data = {
		(uint8_t)ales::OpCode::PushInt,
		(uint8_t)2,
		(uint8_t)0,
		(uint8_t)0,
		(uint8_t)0,
		(uint8_t)ales::OpCode::PushInt,
		(uint8_t)7,
		(uint8_t)0,
		(uint8_t)0,
		(uint8_t)0,
		(uint8_t)ales::OpCode::AddInt, // 2 + 7
	};
	ales::VirtualMachine vm;
	vm.run(code);
	return 0;
}
