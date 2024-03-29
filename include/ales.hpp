#ifndef ALES_HPP
#define ALES_HPP

#include <variant>
#include <vector>
#include <string>
#include <optional>
#include <iosfwd>
#include <string_view>
#include <limits>

namespace ales
{
	using Float_t = float;
	using Int_t = int;
	using String_t = std::string;
	using Bool_t = bool;

	using ArgCount_t = uint8_t;
	auto constexpr max_statement_elements = std::numeric_limits<ArgCount_t>::max();

	struct Symbol
	{
		std::string name;
	};

	using CellList_t = std::vector<struct ASTCell>;
	
	struct FunctionDecl
	{
		std::string funcName;
		std::vector<std::string> argNames;
		CellList_t body;
	};
	
	struct ASTCell
	{
		using Value_t = std::variant<Int_t, Float_t, String_t, Bool_t, Symbol, FunctionDecl, CellList_t>;
		Value_t value;
		int line;
	};

	// https://en.cppreference.com/w/cpp/utility/variant/visit
	template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
	template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;
	
	std::ostream& operator<<(std::ostream& out, ASTCell const& c);

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
		[[nodiscard]] std::vector<ASTCell> parse();
		[[nodiscard]] std::optional<ASTCell> parse_list();
		[[nodiscard]] std::optional<ASTCell> parse_atom(Token);
		
		bool expect(Token::Type type);
		void error(std::string_view msg, Token const& tk);

		size_t index = 0;
		Lexer* lexer;
	};


}

#endif