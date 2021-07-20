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
		ales::CodeChunk chunk;
		if (enclosing.cells.size() < 2)
		{
			printf("error not enough arguments for + line %d", enclosing.line);
			return std::vector<uint8_t>{};
		}
		
		// TODO: review how to handle compilation order
		ales::OpCode addOp = ales::OpCode::AddInt;
		if (std::holds_alternative<ales::Int_t>(enclosing.cells[1].value))
		{
			chunk.write(ales::OpCode::PushInt);
			chunk.write(std::get<ales::Int_t>(enclosing.cells[1].value));
			addOp = ales::OpCode::AddInt;
		}
		else if (std::holds_alternative<ales::Float_t>(enclosing.cells[1].value))
		{
			chunk.write(ales::OpCode::PushFloat);
			chunk.write(std::get<ales::Float_t>(enclosing.cells[1].value));
			addOp = ales::OpCode::AddFloat;
		}
		else if (std::holds_alternative<ales::Variable>(enclosing.cells[1].value))
		{
			chunk.write(ales::OpCode::PushVar);
			chunk.writeStr(std::get<ales::Variable>(enclosing.cells[1].value).name);
		}
		else
		{
			printf("args of + must be either int literal, float literal or a variable !\n");
			return std::vector<uint8_t>{};
		}
		
		for (auto it = enclosing.cells.begin() + 2; it != enclosing.cells.end(); ++it)
		{
			if (std::holds_alternative<ales::Int_t>(it->value))
			{
				chunk.write(ales::OpCode::PushInt);
				chunk.write(std::get<ales::Int_t>(it->value));
			}
			else if (std::holds_alternative<ales::Float_t>(it->value))
			{
				chunk.write(ales::OpCode::PushFloat);
				chunk.write(std::get<ales::Float_t>(it->value));
			}
			else if (std::holds_alternative<ales::Variable>(it->value))
			{
				chunk.write(ales::OpCode::PushVar);
				chunk.writeStr(std::get<ales::Variable>(it->value).name);
			}
			else
			{
				printf("args of + must be either int literal, float literal or a variable !\n");
				return std::vector<uint8_t>{};
			}
			
			chunk.write(addOp);
		}
		
		return chunk.code_data;
	};

	compiler.func_compiler["set"] = [](ales::Statement const& enclosing, ales::Compiler& compiler)
	{
		ales::CodeChunk chunk;
		if (enclosing.cells.size() < 2)
		{
			printf("error not enough arguments for + line %d", enclosing.line);
			return std::vector<uint8_t>{};
		}

		chunk.write(ales::OpCode::Store);
		chunk.writeStr(std::get<ales::Variable>(enclosing.cells[1].value).name);

		return chunk.code_data;
	};

	auto chunk = compiler.compile(c.value());
	ales::VirtualMachine vm;
	std::cout << chunk;
	vm.run(chunk);
	return 0;
}
