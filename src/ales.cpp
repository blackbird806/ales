#include "ales.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace ales;

template<typename R, typename T>
static bool contains(R&& range, T&& e)
{
	return std::find(std::begin(range), std::end(range), e) != std::end(range);
}

static bool is_blank(char c)
{
	return isspace(c);
}

Lexer::Lexer(std::string_view src) : source(src)
{
	
}

std::optional<Token> Lexer::read_next_token()
{
	l_start:
	for (;index < source.size() && is_blank(source[index]); index++)
	{
		if (source[index] == '\n')
			current_line++;
	}

	if (index == source.size()) 
		return {};
	
	if (source[index] == ';')
	{
		while (index < source.size() && source[index++] != '\n') {}
		goto l_start;
	}

	// node open / close
	if (contains(node_open_chars, source[index]))
	{
		index++;
		return Token{ Token::Type::NodeOpen, current_line };
	}
	
	if (contains(node_close_chars, source[index]))
	{
		index++;
		return Token{ Token::Type::NodeClose, current_line };
	}
	
	// identifiers
	if (isalpha(source[index]))
	{
		auto const first = index;
		while (isalnum(source[index]) || source[index] == '_' || source[index] == '-')
		{
			index++;
		}
		return Token{ Token::Type::Identifier, current_line, String_t(source.substr(first, index - first))};
	}

	//numbers
	if (isdigit(source[index]) || source[index] == '-')
	{
		auto const first = index;
		bool is_float = false;
		while (isdigit(source[index]) || source[index] == '_' || source[index] == '.')
		{
			if (source[index] == '.')
			{
				if (is_float)
				{
					error("float should have only one dot ('.') !");
					return {};
				}
				is_float = true;
			}
			index++;
		}
		std::string tok_str(source.substr(first, index - first));
		tok_str.erase(std::remove(tok_str.begin(), tok_str.end(), '_'), tok_str.end());
		
		if (is_float)
			return Token{ Token::Type::FloatLiteral,
				current_line,
				static_cast<Float_t>(std::stol(tok_str)) };

		return Token{ Token::Type::IntLiteral,
		current_line,
		static_cast<Int_t>(std::stoll(tok_str)) };
	}

	// string
	if (source[index] == '"')
	{
		auto const first = ++index;
		while (source[index] != '"')
		{
			index++;
		}
		auto const last = index;
		index++;
		return Token{ Token::Type::StringLiteral, current_line, String_t(source.substr(first, last - first)) };
	}
}

void Lexer::error(std::string_view msg)
{
	std::cerr << "lexer error line " << current_line << " - " << msg << "\n";
}

Parser::Parser(Lexer& lexer_) : lexer(&lexer_)
{
}

std::optional<Cell> Parser::parse()
{
	current_token = lexer->read_next_token();

	if (!current_token)
		return {};
	
	Token const& current = current_token.value();
	if (current.type == Token::Type::NodeOpen)
	{
		Statement st;
		while (current_token.has_value() && current_token->type != Token::Type::NodeClose)
		{
			auto const res = parse();
			if (res.has_value())
				st.cells.emplace_back(res.value());
		}
		lexer->read_next_token(); // ignore close token
		return Cell{st};
	}
	else if (current.type == Token::Type::IntLiteral)
	{
		return Cell{ std::get<Int_t>(current_token->value) };
	}
	else if (current.type == Token::Type::FloatLiteral)
	{
		return Cell{ std::get<Float_t>(current_token->value) };
	}
	else if (current.type == Token::Type::StringLiteral)
	{
		return Cell{ std::get<String_t>(current_token->value) };
	}
	else if (current.type == Token::Type::Identifier)
	{
		return Cell{ Symbol{std::get<String_t>(current_token->value)} };
	}

	return {};
}

void Parser::error(std::string_view msg, Token tk)
{
	std::cerr << "parser error line " << tk.line << " - " << msg << "\n";
}

// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;

std::ostream& ales::operator<<(std::ostream& out, Cell const& c)
{
	std::visit(overloaded {
		[&out](Int_t arg) { out << arg << ' '; },
		[&out](Float_t arg) { out << std::fixed << arg << ' '; },
		[&out](String_t const& arg){ out << std::quoted(arg) << ' '; },
		[&out](List_t const& arg)
		{
			out << '[';
			for (auto const& e : arg)
				out << e << ", ";
			out << "] ";
		},
		[&out](Symbol const& arg) { out << "Sym : " << arg.name << ' '; },
		[&out](Statement const& arg)
		{
			out << '(';
			for (auto const& e : arg.cells)
				out << e;
			out << ") ";
		},
	}, c.value);
	return out;
}
