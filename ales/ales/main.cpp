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
	using NativeFunc_t = Expression(*)(Interpreter&, Env& env, List const& list);
	using MacroFn_t = Expression(*)(Interpreter&, Env& env, List const& list);

	struct Macro
	{
		MacroFn_t macro_fn;
	};

	struct ScriptFunction
	{
		std::vector<Expression> func_body;
		List arg_list;
	};

	struct Variable
	{
		Expression exp;
	};

	struct Symbol
	{
		using Value_t = std::variant<NativeFunc_t, ScriptFunction, Variable, Macro>;
		Value_t value;
	};

	// bruh
	struct Env
	{
		Symbol& add(std::string const& name, Symbol const& sym)
		{
			return sym_table.insert_or_assign(name, sym).first->second;
		}

		Symbol& operator[](std::string const& name)
		{
			auto const it = sym_table.find(name);
			if (it != sym_table.end())
				return it->second;

			if (outer)
				return (*outer)[name];

			throw std::out_of_range("symbol doesn't exist");
		}

		Symbol const& operator[](std::string const& name) const
		{
			auto const it = sym_table.find(name);
			if (it != sym_table.end())
				return it->second;

			if (outer)
				return (*outer)[name];

			throw std::out_of_range("symbol doesn't exist");
		}

		std::unordered_map<std::string, Symbol> sym_table;
		Env* outer = nullptr;
	};

	Expression eval(Env& env, Expression const& exp)
	{
		return std::visit(overloaded{
			[&env](Atom const& arg)
			{
				if (std::holds_alternative<ales::Symbol>(arg.value))
				{
					return std::get<Variable>(env[std::get<ales::Symbol>(arg.value).name].value).exp;
				}
				return Expression{arg};
			},
			[&env, this] (List const& list)
			{
				if (list.elements.empty())
				{
					return Expression{list};
				}

				std::string const op = getSymbolName(*list.elements.begin());
				auto const sym = env[op].value;
				if (std::holds_alternative<NativeFunc_t>(sym))
				{
					List arg_list;
					arg_list.elements.reserve(list.elements.size() - 1);
					for (size_t i = 1; i < list.elements.size(); i++)
						arg_list.elements.push_back(eval(env, list.elements[i]));

					return std::get<NativeFunc_t>(sym)(*this, env, arg_list);
				}
				if (std::holds_alternative<Macro>(sym))
				{
					return std::get<Macro>(sym).macro_fn(*this, env, list);
				}
				else if (std::holds_alternative<ScriptFunction>(sym))
				{
					auto const& fn = std::get<ScriptFunction>(sym);
					Env inner_env;
					inner_env.outer = &env;
					int i = 1;
					for (auto const& arg : fn.arg_list.elements)
					{
						inner_env.add(getSymbolName(arg), Symbol{ Variable{ eval(env, list.elements[i]) } });
						i++;
					}
					Expression last_result;
					for (auto const& exp : fn.func_body)
					{
						last_result = eval(inner_env, exp);
					}
					return last_result;
				}
				return Expression{ };
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
	interpreter.global_env.add("eq", Interpreter::Symbol{ [](Interpreter&, Interpreter::Env&, List const& list) -> Expression
		{
			bool eq = true;
			auto* last = &list.elements[1];
			for (size_t i = 2; i < list.elements.size(); i++)
			{
				eq = *last == list.elements[i];
				last = &list.elements[i];
			}
			return Expression{ Atom{eq} };
		} });

	interpreter.global_env.add("*", Interpreter::Symbol{ [](Interpreter&, Interpreter::Env&, List const& list) -> Expression
	{
		Int_t sum = 1;
		for (size_t i = 0; i < list.elements.size(); i++)
		{
			sum *= getAtomValue<Int_t>(list.elements[i]);
		}
		return Expression{ Atom{sum} };
	} });

	interpreter.global_env.add("print", Interpreter::Symbol{ [](Interpreter& inter, Interpreter::Env&, List const& list) -> Expression
	{
		for (size_t i = 0; i < list.elements.size(); i++)
		{
			std::cout << list.elements[i];
		}
		return Expression{ };
	} });

	interpreter.global_env.add("if", Interpreter::Symbol{ Interpreter::Macro{ [](Interpreter& inter, Interpreter::Env& env, List const& list) -> Expression
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
	} } });

	interpreter.global_env.add("defun", Interpreter::Symbol{ Interpreter::Macro{ [](Interpreter& inter, Interpreter::Env& env, List const& list) -> Expression
	{
		if (list.elements.size() < 4)
			throw std::runtime_error("not enough expressions in defun");

		std::string const fnName = getSymbolName(list.elements[1]);
		Interpreter::ScriptFunction scriptFunction;
		scriptFunction.arg_list = std::get<List>(list.elements[2].value);
		scriptFunction.func_body.insert(scriptFunction.func_body.end(), list.elements.begin() + 3, list.elements.end());
		env.add(fnName, Interpreter::Symbol{ std::move(scriptFunction) });
		return Expression{ };
	} } });


	for (auto const& e : exps)
	{
		interpreter.eval(interpreter.global_env, e);
	}
	return 0;
}
