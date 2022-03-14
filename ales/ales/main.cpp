#include <iostream>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <variant>
#include "ales.hpp"
#include "vm.hpp"

using namespace ales;

static std::string getSymbolName(Expression const& e)
{
	return std::get<Symbol>(std::get<Atom>(e.value).value).name;
}

template<typename T>
static T getAtomValue(Expression const& e)
{
	return std::get<T>(std::get<Atom>(e.value).value);
}

struct Interpreter
{
	struct Env;
	using SymFn_t = Expression(*)(Interpreter&, Env& env, List const& list);

	// bruh
	struct Env : std::unordered_map<std::string, SymFn_t>
	{
	};

	Expression eval(Env& env, Expression const& exp)
	{
		return std::visit(overloaded{
			[](Atom const& arg) { return Expression{arg}; },
			[env, this] (List const& list) mutable
			{
				if (list.elements.empty())
				{
					return Expression{list};
				}

				std::string const op = getSymbolName(*list.elements.begin());
				return env[op](*this, env, list);
			},
		}, exp.value);
	}
	Env global_env;
};

int main()
{
	std::ifstream source("../../src/test.als");
	std::string const code((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
	ales::Lexer lexer(code);
	ales::Parser parser(lexer);
	auto const exps = parser.parse();

	for (auto const& c : exps)
		std::cout << c << "\n";

	std::cout << "=============exec================" << "\n";

	Interpreter interpreter;
	interpreter.global_env["eq"] = [](Interpreter&, Interpreter::Env&, List const& list) -> Expression
	{
		bool eq = true;
		auto* last = &list.elements[1];
		for (size_t i = 2; i < list.elements.size(); i++)
		{
			eq = *last == list.elements[i];
			last = &list.elements[i];
		}
		return Expression{ Atom{eq} };
	};

	interpreter.global_env["print"] = [](Interpreter&, Interpreter::Env&, List const& list) -> Expression
	{
		for (size_t i = 1; i < list.elements.size(); i++)
		{
			std::cout << list.elements[i];
		}
		return Expression{ };
	};

	interpreter.global_env["if"] = [](Interpreter& inter, Interpreter::Env& env, List const& list) -> Expression
	{
		if (list.elements.size() < 3)
			throw std::runtime_error("not enough expressions in if");

		Expression const cond = list.elements[1];

		if (getAtomValue<Bool_t>(inter.eval(env, cond)))
		{
			Expression const trueBody = list.elements[2];
			return inter.eval(env, trueBody);
		}
		else if (list.elements.size() == 4)
		{
			Expression const elseBody = list.elements[3];
			return inter.eval(env, elseBody);
		}
		return Expression{ };
	};

	for (auto const& e : exps)
	{
		interpreter.eval(interpreter.global_env, e);
	}
	return 0;
}
