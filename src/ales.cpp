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

std::vector<ASTNode> Parser::parse()
{
	std::vector<ASTNode> statements;
	while (expect(Token::Type::NodeOpen))
	{
		std::optional<ASTNode> c = parse_list();
		if (c)
			statements.push_back(*c);
		else
			break;
	}
	return statements;
}

std::optional<ASTNode> Parser::parse_statement()
{
	auto const tk = lexer->read_next_token();
	
	if (!tk)
	{
		return {};
	}
	
	Token const& current = tk.value();
	
	if (current.type == Token::Type::NodeOpen)
	{
		auto const firstTk = lexer->read_next_token();

		if (!firstTk)
		{
			error("Node should be closed before end of file", current);
			return {};
		}
		if (firstTk->type == Token::Type::NodeClose)
		{
			error("Empty nodes are illegal", *firstTk);
			return {};
		}
		if (firstTk->type != Token::Type::Identifier)
		{
			error("First node element should be either a function a keyword or an operator", *firstTk);
			return {};
		}
		if (std::get<std::string>(firstTk->value) == "defun")
			return parse_function_decl();
		return {};
	}
	if (current.type == Token::Type::IntLiteral)
	{
		return ASTNode{ std::get<Int_t>(current.value) };
	}
	else if (current.type == Token::Type::FloatLiteral)
	{
		return ASTNode{ std::get<Float_t>(current.value) };
	}
	else if (current.type == Token::Type::StringLiteral)
	{
		return ASTNode{ std::get<String_t>(current.value) };
	}
	else if (current.type == Token::Type::BoolLiteral)
	{
		return ASTNode{ std::get<Bool_t>(current.value) };
	}
	
	return {};
}

std::optional<ASTNode> Parser::parse_list()
{
	ASTNode node;
	ASTNode::List_t list;
	for (auto tk = lexer->read_next_token(); tk && tk->type != Token::Type::NodeClose; tk = lexer->read_next_token())
	{
		std::optional<ASTNode> r;
		if (tk->type == Token::Type::NodeOpen)
		{
			r = parse_list();
		}
		else
		{
			r = parse_atom(*tk);
		}

		if (r)
			list.push_back(*r);
		else
		{
			error("invalid atom or list", *tk);
			return {};
		}
	}
	node.value = std::move(list);
	return node;
}

std::optional<ASTNode> Parser::parse_atom(Token current)
{
	if (current.type == Token::Type::IntLiteral)
	{
		return ASTNode{ std::get<Int_t>(current.value) };
	}
	else if (current.type == Token::Type::FloatLiteral)
	{
		return ASTNode{ std::get<Float_t>(current.value) };
	}
	else if (current.type == Token::Type::StringLiteral)
	{
		return ASTNode{ std::get<String_t>(current.value) };
	}
	else if (current.type == Token::Type::BoolLiteral)
	{
		return ASTNode{ std::get<Bool_t>(current.value) };
	}
	else if (current.type == Token::Type::Identifier)
	{
		return ASTNode{ std::get<String_t>(current.value) };
	}
	
	return {};
}

std::optional<ASTNode> Parser::parse_function_decl()
{
	auto const nameTk = lexer->read_next_token();
	if (!nameTk)
	{
		error("function declaration must specify a name !", Token{});
		return {};
	}
	
	FuncDecl fnDecl{ std::get<std::string>(nameTk->value) };

	if (!expect(Token::Type::NodeOpen))
		return {};

	for (auto arg = lexer->read_next_token(); arg.has_value(); arg = lexer->read_next_token())
	{
		if (arg->type != Token::Type::Identifier)
		{
			error("function arguments must be identifiers", *arg);
			break;
		}
		fnDecl.argNames.push_back(std::get<std::string>(arg->value));
	}

	if (!expect(Token::Type::NodeClose))
		return {};
	return {};

	//return ASTNode{ fnDecl, nameTk->line };
}

bool Parser::expect(Token::Type type)
{
	auto const current = lexer->read_next_token();
	if (!current)
	{
		error("eof", Token{});
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

std::ostream& ales::operator<<(std::ostream& out, ASTNode const& c)
{
	std::visit(overloaded {
		[&out](Int_t arg) { out << arg << " "; },
		[&out](Bool_t arg) { out << std::boolalpha << arg << " "; },
		[&out](Float_t arg) { out << std::fixed << arg << " "; },
		[&out](String_t const& arg) { out << std::quoted(arg) << " "; },
		//[&out](FunctionCall const& arg) { out << "[FuncCall : " << arg.name << "] "; },
		//[&out](FuncDecl const& arg) { out << "[FuncDecl : " << arg.name << "] "; },
		//[&out](Variable const& arg) { out << "[Var : " << arg.name << "] "; },
		[&out](ASTNode::List_t const& list)
		{
			out << "(";
			for (auto const& e : list)
				out << " " << e;
			out << ") ";
		},
	}, c.value);
	return out;
}
