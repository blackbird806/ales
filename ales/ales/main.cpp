#include <iostream>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <variant>
#include "ales.hpp"
#include "vm.hpp"

using namespace ales;

struct Interpreter
{
	static std::string getSymbolName(Expression const& e)
	{
		return std::get<Symbol>(std::get<Atom>(e.value).value).name;
	}

	using Env = std::unordered_map<std::string, Symbol>;

	Expression eval(Env& env, Expression exp)
	{
		Expression result;
		std::visit(overloaded{
			[&result](Atom const& arg) { result.value = arg; },
			[&result, &env, this](List const& arg)
			{
				if (arg.elements.empty())
				{
					result.value = arg;
					return;
				}

				try {
					auto const op = getSymbolName(*arg.elements.begin());
					if (op == "defun")
					{
						if (arg.elements.size() < 4)
							throw std::runtime_error("not enough expressions in function declaration !");

						std::string const name = getSymbolName(arg.elements[1]);
						List argList = std::get<List>(arg.elements[2].value);
						
						std::vector<Expression> body(arg.elements.begin() + 2, arg.elements.end());
						env[name].value = [argList, body](std::vector<Expression> const& fnArgs)
						{
							if (fnArgs.size() != argList.elements.size())
								throw std::runtime_error("arg count mismatch !");

							Env fnEnv;
							for (size_t i = 0; i < fnArgs.size(); i++)
							{
								fnEnv[getSymbolName(argList.elements[i])].value = Var_t(getSymbolName(argList.elements[i]));
							}
							return Expression{};
						};
					}
					else
					{
						std::vector<Expression> args;
						args.reserve(arg.elements.size() - 1);

						for (size_t i = 1; i < arg.elements.size(); i++)
							args.push_back(eval(env, arg.elements[i]));
						// regular call
						result = std::get<Function_t>(env.at(op).value)(args);
					}
				}
				catch(std::exception const& e)
				{
					std::cerr << e.what();
					throw;
				}
			},
		}, exp.value);
		return result;
	}
	Env global_env;
};

int main()
{
	std::ifstream source("../../src/test.als");
	std::string const code((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
	ales::Lexer lexer(code);
	ales::Parser parser(lexer);
	auto exps = parser.parse();

	for (auto const& c : exps)
		std::cout << c << "\n";

	std::cout << "=============exec================" << "\n";

	Interpreter interpreter;
	interpreter.global_env["print"].value = [](std::vector<Expression> const& args) -> Expression
	{
		for (auto const& arg : args)
		{
			std::cout << arg;
		}
		std::cout << "\n";

		return Expression{   };
	};

	interpreter.global_env["+"].value = [](std::vector<Expression> const& args) -> Expression
	{
		Int_t sum = 0;
		for (auto const& arg : args)
		{
			sum += std::get<Int_t>(std::get<Atom>(arg.value).value);
		}
		return Expression{ Atom{ Int_t{sum} } };
	};

	for (auto const& e : exps)
	{
		interpreter.eval(interpreter.global_env, e);
	}
	return 0;
}
