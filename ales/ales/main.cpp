#include <iostream>
#include "ales.hpp"

int main()
{
    std::cout << "Hello World!\n";
	ales::Lexer lexer("(var 45 65) ((54.3223 \"test\"))");
	ales::Parser parser(lexer);
	auto c = parser.parse();
	std::cout << c.value();
	c = parser.parse();
	std::cout << c.value();
	return 0;
}
