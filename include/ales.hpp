#ifndef ALES_HPP
#define ALES_HPP

#include <variant>
#include <vector>
#include <string>
#include <optional>
#include <iosfwd>
#include <string_view>
#include <unordered_map>

namespace ales
{
	using Float_t = float;
	using Int_t = int;
	using String_t = std::string;

	struct Statement
	{
		std::vector<struct Cell> cells;
	};

	struct Symbol
	{
		std::string name;
	};

	struct Cell
	{
		using CellValue_t = std::variant<Int_t, Float_t, String_t, Symbol, Statement>;
		CellValue_t value;
	};

	// https://en.cppreference.com/w/cpp/utility/variant/visit
	template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
	template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;
	
	std::ostream& operator<<(std::ostream& out, Cell const& c);

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
			StringLiteral,
		};

		using TokenValue_t = std::variant<Int_t, Float_t, String_t>;
		
		Type type;
		int line = -1;
		TokenValue_t value;
	};

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
		[[nodiscard]] std::optional<Cell> parse();
		[[nodiscard]] std::optional<Cell> parse_statement();
		void error(std::string_view msg, Token tk);
		
		size_t index = 0;
		Lexer* lexer;
	};
	
	class Environement
	{
		std::unordered_map<std::string, Cell> symbols;
	};

}

#endif