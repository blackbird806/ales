#ifndef ALES_HPP
#define ALES_HPP

#include <variant>
#include <vector>
#include <string>
#include <optional>
#include <iosfwd>
#include <string_view>
#include <limits>
#include <functional>

/*
	list -> '(' expression* ')'
	expression -> atom | list
	atom -> number | symbol | string | bool
	symbol -> function | variable
	number -> int | float
 */
namespace ales
{
	using Float_t = float;
	using Int_t = int;
	using String_t = std::string;
	using Bool_t = bool;
	struct Expression;
	using Function_t = std::function<Expression(std::vector<Expression> const&)>;
	using Var_t = std::string;

	using ArgCount_t = uint8_t;
	auto constexpr max_statement_elements = std::numeric_limits<ArgCount_t>::max();


	struct Symbol
	{
		using Value_t = std::variant<Var_t, Function_t>;
		std::string name;
		Value_t value;
	};

	struct Atom
	{
		using Value_t = std::variant<Int_t, Float_t, String_t, Bool_t, Symbol>;
		Value_t value;
	};

	struct List
	{
		std::vector<Expression> elements;
	};

	struct Expression
	{
		using Value_t = std::variant<Atom, List>;
		Value_t value;
	};

	// https://en.cppreference.com/w/cpp/utility/variant/visit
	template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
	template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;
	
	std::ostream& operator<<(std::ostream& out, Expression const& c);
	std::ostream& operator<<(std::ostream& out, List const& c);
	std::ostream& operator<<(std::ostream& out, Atom const& c);

	constexpr char node_open_chars[] = { '(' };
	constexpr char node_close_chars[] = { ')' };
	
	struct Token
	{
		enum class Type
		{
			NodeOpen,
			NodeClose,
			Identifier,
			IntLiteral,
			FloatLiteral,
			BoolLiteral,
			StringLiteral,
		};

		using TokenValue_t = std::variant<Int_t, Float_t, String_t, Bool_t>;
		
		Type type;
		int line = -1;
		TokenValue_t value;
	};

	const char* to_string(Token::Type e);
	
	struct Lexer
	{
		explicit Lexer(std::string_view src);
		std::optional<Token> read_next_token();
		void error(std::string_view msg);

		std::string_view source;
		size_t index = 0;
		int current_line = 1;
	};

	struct Parser
	{
		explicit Parser(Lexer& lexer);
		[[nodiscard]] std::vector<Expression> parse();
		[[nodiscard]] List parse_list();
		[[nodiscard]] Atom parse_atom(Token);
		
		bool expect(Token::Type type);
		void error(std::string_view msg, Token const& tk);

		size_t index = 0;
		Lexer* lexer;
	};


}

#endif