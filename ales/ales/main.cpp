#include <iostream>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <variant>
#include "ales.hpp"
#include "vm.hpp"

using namespace ales;

static std::string get_symbol_name(Expression const& e)
{
	return std::get<Symbol>(std::get<Atom>(e.value).value).name;
}

template<typename T>
static T get_atom_value(Expression const& e)
{
	return std::get<T>(std::get<Atom>(e.value).value);
}

struct Interpreter
{
	struct Env;
	using NativeFunc_t = Expression(*)(Interpreter&, Env& env, List const& list);
	using MacroFn_t = Expression(*)(Interpreter&, Env& env, List const& list);

	struct NativeMacro
	{
		MacroFn_t macro_fn;
	};

	struct ScriptMacro
	{
		std::vector<Expression> macro_body;
		List arg_list;
	};

	struct ScriptFunction
	{
		std::vector<Expression> func_body;
		List arg_list;
	};

	struct Variable
	{
		Expression exp;
		bool is_macro_parameter = false;
	};

	struct Symbol
	{
		using Value_t = std::variant<NativeFunc_t, ScriptFunction, Variable, NativeMacro, ScriptMacro>;
		Value_t value;
	};

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

	void add_macro(std::string const& name, MacroFn_t fn)
	{
		global_env.add(name, Symbol{ NativeMacro{fn} });
	}

	void add_func(std::string const& name, NativeFunc_t fn)
	{
		global_env.add(name, Symbol{ NativeFunc_t{fn} });
	}

	bool is_macro(List const& exp, Env const& env) const
	{
		return is_macro(exp.elements[0], env);
	}

	bool is_macro(Atom const& atom, Env const& env) const
	{
		if (std::holds_alternative<ales::Symbol>(atom.value))
		{
			auto const& sym = env[std::get<ales::Symbol>(atom.value).name];
			return std::holds_alternative<ScriptMacro>(sym.value) || std::holds_alternative<NativeMacro>(sym.value);
		}
		return false;
	}

	bool is_macro(Expression const& exp, Env const& env) const
	{
		if (std::holds_alternative<List>(exp.value))
		{
			return is_macro(std::get<List>(exp.value), env);
		}
		return is_macro(std::get<Atom>(exp.value), env);
	}

	Expression macro_expand(Env& env, Expression const& exp)
	{
		return  std::visit(overloaded{
			[&env, this](Atom const& arg) -> Expression
			{
				if (std::holds_alternative<ales::Symbol>(arg.value))
				{
					auto const& sym = env[std::get<ales::Symbol>(arg.value).name].value;
					if (std::holds_alternative<Variable>(sym))
					{
						auto const& var = std::get<Variable>(sym);
						return var.exp;
					}
				}
				return Expression{arg};
			},
			[this, &env](List const& list) -> Expression
			{
				List expanded_list;
				expanded_list.elements.reserve(list.elements.size());
				for (auto const& e : list.elements)
				{
					expanded_list.elements.push_back(macro_expand(env, e));
				}
				return Expression{ expanded_list };
			}
		}, exp.value);
	}

	Expression eval(Env& env, Expression const& exp)
	{
		return std::visit(overloaded{
			[&env, this](Atom const& arg)
			{
				if (std::holds_alternative<ales::Symbol>(arg.value))
				{
					auto const& sym = env[std::get<ales::Symbol>(arg.value).name].value;
					return std::get<Variable>(sym).exp;
				}
				return Expression{arg};
			},
			[&env, this] (List const& list)
			{
				if (list.elements.empty())
				{
					return Expression{list};
				}

				std::string const op = get_symbol_name(*list.elements.begin());
				auto const sym = env[op].value;
				if (std::holds_alternative<NativeFunc_t>(sym))
				{
					List arg_list;
					arg_list.elements.reserve(list.elements.size() - 1);
					for (size_t i = 1; i < list.elements.size(); i++)
						arg_list.elements.push_back(eval(env, list.elements[i]));

					return std::get<NativeFunc_t>(sym)(*this, env, arg_list);
				}
				if (std::holds_alternative<NativeMacro>(sym))
				{
					return std::get<NativeMacro>(sym).macro_fn(*this, env, list);
				}
				else if (std::holds_alternative<ScriptFunction>(sym))
				{
					auto const& fn = std::get<ScriptFunction>(sym);
					Env inner_env;
					inner_env.outer = &env;
					int i = 1;
					for (auto const& arg : fn.arg_list.elements)
					{
						inner_env.add(get_symbol_name(arg), Symbol{ Variable{ eval(env, list.elements[i]) } });
						i++;
					}
					Expression last_result;
					for (auto const& exp : fn.func_body)
					{
						last_result = eval(inner_env, exp);
					}
					return last_result;
				}
				else if (std::holds_alternative<ScriptMacro>(sym))
				{
					auto const& fn = std::get<ScriptMacro>(sym);
					int i = 1;
					for (auto const& arg : fn.arg_list.elements)
					{
						env.add(get_symbol_name(arg), Symbol{ Variable{ list.elements[i], true } });
						i++;
					}
					Expression expanded;
					for (auto const& fn_exp : fn.macro_body)
					{
						expanded = macro_expand(env, fn_exp);
						expanded = eval(env, expanded);
					}
					return expanded;
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
	std::string const code((std::istreambuf_iterator(source)), std::istreambuf_iterator<char>());
	ales::Lexer lexer(code);
	ales::Parser parser(lexer);
	auto const exps = parser.parse();

	for (auto const& c : exps)
		std::cout << c << "\n";

	std::cout << "=============exec================" << "\n";

	Interpreter interpreter;
	interpreter.add_func("eq", [](Interpreter&, Interpreter::Env&, List const& list) -> Expression
	{
		bool eq = true;
		auto* last = &list.elements[0];
		for (size_t i = 1; i < list.elements.size(); i++)
		{
			eq = *last == list.elements[i];
			last = &list.elements[i];
		}
		return Expression{ Atom{eq} };
	});

	interpreter.add_func("*", [](Interpreter&, Interpreter::Env&, List const& list) -> Expression
	{
		Int_t sum = 1;
		for (size_t i = 0; i < list.elements.size(); i++)
		{
			sum *= get_atom_value<Int_t>(list.elements[i]);
		}
		return Expression{ Atom{sum} };
	});

	interpreter.add_func("+", [](Interpreter&, Interpreter::Env&, List const& list) -> Expression
	{
		Int_t sum = 0;
		for (size_t i = 0; i < list.elements.size(); i++)
		{
			sum += get_atom_value<Int_t>(list.elements[i]);
		}
		return Expression{ Atom{sum} };
	});

	interpreter.add_macro("quote", [](Interpreter&, Interpreter::Env&, List const& list) -> Expression
	{
		List l;
		l.elements.insert(l.elements.end(), list.elements.begin() + 1, list.elements.end());
		return Expression{ std::move(l) };
	});

	interpreter.add_func("print", [](Interpreter& inter, Interpreter::Env&, List const& list) -> Expression
	{
		for (size_t i = 0; i < list.elements.size(); i++)
		{
			std::cout << list.elements[i];
		}
		return Expression{ };
	});

	interpreter.add_macro("set", [](Interpreter& inter, Interpreter::Env& env, List const& list) -> Expression
	{
		if (list.elements.size() != 3)
			throw std::runtime_error("set must have 2 args");

		auto const& name = list.elements[1];
		auto const& value = inter.eval(env, list.elements[2]);
		env.add(get_symbol_name(name), Interpreter::Symbol{ Interpreter::Variable{value} });

		return Expression{ };
	});

	interpreter.add_macro("if", [](Interpreter& inter, Interpreter::Env& env, List const& list) -> Expression
	{
		if (list.elements.size() < 3)
			throw std::runtime_error("not enough expressions in if");

		Expression const& cond = list.elements[1];

		if (get_atom_value<Bool_t>(inter.eval(env, cond)))
		{
			Expression const& true_body = list.elements[2];
			return inter.eval(env, true_body);
		}
		else if (list.elements.size() == 4)
		{
			Expression const& else_body = list.elements[3];
			return inter.eval(env, else_body);
		}
		return Expression{ };
	});

	interpreter.add_macro("macroexpand", [](Interpreter& inter, Interpreter::Env& env, List const& list)->Expression
	{
		Expression expanded;
		for (size_t i = 1; i < list.elements.size(); i++)
		{
			expanded = inter.eval(env, list.elements[i]);
		}
		return expanded;
	});

	interpreter.add_macro("defun", [](Interpreter& inter, Interpreter::Env& env, List const& list) -> Expression
	{
		if (list.elements.size() < 4)
			throw std::runtime_error("not enough expressions in defun");

		std::string const fnName = get_symbol_name(list.elements[1]);
		Interpreter::ScriptFunction script_function;
		script_function.arg_list = std::get<List>(list.elements[2].value);
		script_function.func_body.insert(script_function.func_body.end(), list.elements.begin() + 3, list.elements.end());
		env.add(fnName, Interpreter::Symbol{ std::move(script_function) });
		return Expression{ };
	});

	interpreter.add_macro("defmacro", [](Interpreter& inter, Interpreter::Env& env, List const& list) -> Expression
	{
		if (list.elements.size() < 4)
			throw std::runtime_error("not enough expressions in defmacro");

		std::string const fnName = get_symbol_name(list.elements[1]);
		Interpreter::ScriptMacro script_macro;
		script_macro.arg_list = std::get<List>(list.elements[2].value);
		script_macro.macro_body.insert(script_macro.macro_body.end(), list.elements.begin() + 3, list.elements.end());
		env.add(fnName, Interpreter::Symbol{ std::move(script_macro) });
		return Expression{ };
	});

	for (auto const& e : exps)
	{
		interpreter.eval(interpreter.global_env, e);
	}
	return 0;
}
