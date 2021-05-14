#include <iostream>
#include <fstream>
#include "ales.hpp"

int main()
{
    std::cout << "Hello World!\n";
	std::ifstream source("../../src/test.als");
	std::string const code((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
	ales::Lexer lexer(code);
	ales::Parser parser(lexer);
	auto c = parser.parse();
	std::cout << c.value();

	return 0;
}
