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

const char* ales::to_string(Token::Type e)
{
	switch (e)
	{
	case Token::Type::NodeOpen: return "NodeOpen";
	case Token::Type::NodeClose: return "NodeClose";
	case Token::Type::Identifier: return "Identifier";
	case Token::Type::IntLiteral: return "IntLiteral";
	case Token::Type::FloatLiteral: return "FloatLiteral";
	case Token::Type::BoolLiteral: return "BoolLiteral";
	case Token::Type::StringLiteral: return "StringLiteral";
	default: return "unknown";
	}
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

std::vector<Expression> Parser::parse()
{
	std::vector<Expression> expressions;
	for (auto tk = lexer->read_next_token(); tk; tk = lexer->read_next_token())
	{
		if (tk->type == Token::Type::NodeOpen)
			expressions.push_back({ parse_list() });
		else
			expressions.push_back({ parse_atom(*tk) });
	}
	return expressions;
}

List Parser::parse_list()
{
	List list;
	for (auto tk = lexer->read_next_token(); tk && tk->type != Token::Type::NodeClose; tk = lexer->read_next_token())
	{
		Expression exp;
		if (tk->type == Token::Type::NodeOpen)
			exp.value = parse_list();
		else
			exp.value = parse_atom(*tk);

		list.elements.push_back(exp);
	}
	return list;
}

Atom Parser::parse_atom(Token current)
{
	switch(current.type)
	{
	case Token::Type::IntLiteral:
		return Atom{ std::get<Int_t>(current.value) };
	case Token::Type::FloatLiteral:
		return Atom{ std::get<Float_t>(current.value) };
	case Token::Type::BoolLiteral:
		return Atom{ std::get<Bool_t>(current.value) };
	case Token::Type::Identifier:
		return Atom{ Symbol{ std::get<String_t>(current.value) } };
	case Token::Type::StringLiteral:
		return Atom{ std::get<String_t>(current.value) };
	}
	
	return {};
}

bool Parser::expect(Token::Type type)
{
	auto const current = lexer->read_next_token();
	if (!current)
	{
		error("unexpected eof", Token{});
		return false;
	}
	if (current->type != type)
	{
		char errorBuffer[512];
		snprintf(errorBuffer, std::size(errorBuffer), "expected token %s got %s", to_string(type), to_string(current->type));
		error(errorBuffer, *current);
		return false;
	}
	return true;
}

void Parser::error(std::string_view msg, Token const& tk)
{
	std::cerr << "parser error line " << tk.line << " : " << msg << "\n";
}

std::ostream& ales::operator<<(std::ostream& out, Atom const& c)
{
	std::visit(overloaded {
		[&out](Int_t arg) { out << arg << " "; },
		[&out](Bool_t arg) { out << std::boolalpha << arg << " "; },
		[&out](Float_t arg) { out << std::fixed << arg << " "; },
		[&out](String_t const& arg) { out << std::quoted(arg) << " "; },
		[&out](Symbol const& arg) { out << arg.name << " "; },
	}, c.value);
	return out;
}

std::ostream& ales::operator<<(std::ostream& out, Expression const& c)
{
	std::visit(overloaded{
		[&out](auto const& arg) { out << arg << " "; },
		}, c.value);
	return out;
}


std::ostream& ales::operator<<(std::ostream& out, List const& c)
{
	for (auto const& e : c.elements)
		out << e;
	return out;
}
