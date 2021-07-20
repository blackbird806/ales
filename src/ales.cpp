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

static bool is_identifier_special_char(char c)
{
	return c == '_' || c == '-' || c == '+'
		|| c == '*' || c == '/' || c == '%'
		|| c == '^' || c == '&' || c == '$'
		|| c == '!' || c == '?' || c == '~'
		|| c == '#' || c == '<' || c == '>'
		|| c == '=';
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

	// comments
	if (source[index] == ';')
	{
		while (index < source.size() && source[index++] != '\n') {}
		goto l_start;
	}

	// node open
	if (contains(node_open_chars, source[index]))
	{
		index++;
		return Token{ Token::Type::NodeOpen, current_line };
	}
	
	// node close
	if (contains(node_close_chars, source[index]))
	{
		index++;
		return Token{ Token::Type::NodeClose, current_line };
	}
	
	// identifiers
	if (isalpha(source[index]) || is_identifier_special_char(source[index]))
	{
		auto const first = index;
		while (isalnum(source[index]) || is_identifier_special_char(source[index]))
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
					error("float should have only one dot '.' !");
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
				static_cast<Float_t>(std::stod(tok_str)) };

		return Token{	Token::Type::IntLiteral,
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

	error("token not recognized");
	return {};
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
	return parse_statement(nullptr);
}

std::optional<Cell> Parser::parse_statement(Statement* parent)
{
	auto const tk = lexer->read_next_token();
	
	if (!tk)
	{
		error("Statement must end with ')' token", {});
		return {};
	}
	
	Token const& current = tk.value();
	
	if (current.type == Token::Type::NodeOpen)
	{
		Statement st;
		st.parent = parent;
		st.line = current.line;
		while (true)
		{
			auto const res = parse_statement(&st);
			if (res.has_value())
				st.cells.push_back(res.value());
			else 
				break;
		}
		if (st.cells.size() >= max_statement_elements)
			error("statement can have max " + std::to_string(max_statement_elements) + " elements", current);
		
		return Cell{ st };
	}
	if (current.type == Token::Type::IntLiteral)
	{
		return Cell{ std::get<Int_t>(current.value) };
	}
	else if (current.type == Token::Type::FloatLiteral)
	{
		return Cell{ std::get<Float_t>(current.value) };
	}
	else if (current.type == Token::Type::StringLiteral)
	{
		return Cell{ std::get<String_t>(current.value) };
	}
	else if (current.type == Token::Type::BoolLiteral)
	{
		return Cell{ std::get<Bool_t>(current.value) };
	}
	else if (current.type == Token::Type::Identifier)
	{
		if (parent == nullptr || parent->cells.empty())
		{
			return Cell{ Function{std::get<String_t>(current.value)} };
		}
		return Cell{ Variable{ std::get<String_t>(current.value)} };
	}
	
	return {};
}

void Parser::error(std::string_view msg, Token const& tk)
{
	std::cerr << "parser error line " << tk.line << " : " << msg << "\n";
}

std::ostream& ales::operator<<(std::ostream& out, Cell const& c)
{
	std::visit(overloaded {
		[&out](Int_t arg) { out << arg << " "; },
		[&out](Bool_t arg) { out << std::boolalpha << arg << " "; },
		[&out](Float_t arg) { out << std::fixed << arg << " "; },
		[&out](String_t const& arg){ out << std::quoted(arg) << " "; },
		[&out](Function const& arg) { out << "[Sym : " << arg.name << "] "; },
		[&out](Variable const& arg) { out << "[Var : " << arg.name << "] "; },
		[&out](Statement const& arg)
		{
			out << "(";
			for (auto const& e : arg.cells)
				out << " " << e;
			out << ") ";
		},
	}, c.value);
	return out;
}
