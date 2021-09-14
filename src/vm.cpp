#include "vm.hpp"

#include <cassert>
#include <numeric>

#include "ostream"

using namespace ales;

const char* ales::to_string(OpCode e)
{
	switch (e)
	{
	case OpCode::Add: return "Add";
	case OpCode::PushConst: return "PushString";
	case OpCode::PushVar: return "PushVar";
	case OpCode::Store: return "Store";
	case OpCode::Call: return "Call";
	default: return "unknown";
	}
}

ConstantID_t CodeChunk::add_constant(Constant_t cons)
{
	auto const it = std::find(constants.begin(), constants.end(), cons);
	// constant already exist so reuse it
	if (it != constants.end())
		return it - constants.begin();

	constants.push_back(cons);
	return constants.size() - 1;
}

Constant_t CodeChunk::get_constant(ConstantID_t id)
{
	return constants[id];
}

void CodeChunk::write(OpCode op)
{
	code_data.push_back(static_cast<uint8_t>(op));
}

std::ostream& ales::operator<<(std::ostream& out, CodeChunk const& chunk)
{
	size_t next_op_offset = 0;
	while (next_op_offset < chunk.code_data.size())
	{
		OpCode const op = static_cast<OpCode>(chunk.code_data[next_op_offset]);
		next_op_offset += sizeof(OpCode);
		out << to_string(op) << " ";
		switch (op)
		{
		case OpCode::Add: break;
		case OpCode::Call: break;
		case OpCode::PushConst:

			break;
			
		case OpCode::PushVar:
		case OpCode::Store:
			out << chunk.readStr(next_op_offset);
			break;
		}
		out << "\n";
	}
	return out;
}

RetType Compiler::compile(ASTNode const& cell)
{
	return compileTo(cell, chunk);
}

RetType Compiler::compileTo(ASTNode const& cell, CodeChunk& codechunk)
{
	RetType type = RetType::Void;
	//std::visit(overloaded{
	//	[&](FunctionCall const& sym)
	//	{
	//		type = func_compiler[sym.name](*this);
	//		// TODO: recompile function only if inline
	//		// jump to function else
	//	},
	//	[&](FuncDecl const& sym)
	//	{

	//	},
	//	[&](Variable const& value)
	//	{
	//		// TODO:
	//		codechunk.write(OpCode::PushVar);
	//		codechunk.writeStr(value.name);
	//	},
	//	[&](Int_t value)
	//	{
	//		codechunk.write(OpCode::PushConst);
	//		codechunk.write(codechunk.add_constant(Constant_t{ value }));
	//	},
	//	[&](Float_t value)
	//	{
	//		codechunk.write(OpCode::PushConst);
	//		codechunk.write(codechunk.add_constant(Constant_t{ value }));
	//	},
	//	[&](Bool_t value)
	//	{
	//		codechunk.write(OpCode::PushConst);
	//		codechunk.write(codechunk.add_constant(Constant_t{ value }));
	//	},
	//	[&](String_t const& value)
	//	{
	//		codechunk.write(OpCode::PushConst);
	//		codechunk.write(codechunk.add_constant(Constant_t{ value }));
	//	},
	//}, cell.value);
	
	return type;
}

size_t Compiler::compileFunction(std::vector<ASTNode> const& cells)
{
	CodeChunk functionChunk;
	for (auto const& cell : cells)
		compileTo(cell, functionChunk);
	functions.push_back(functionChunk);
	return functions.size() - 1;
}

void Compiler::addFunction(FuncDecl const& decl)
{
	//functionMap[name] = compileFunction(cells, enclosing);
	//func_compiler[name] = [](ales::Statement const& enclosing, ales::Compiler& compiler)
	//{
	//	compiler.chunk.writeStr(std::get<String_t>(enclosing.cells[0].value));
	//	compiler.chunk.write(OpCode::Call);
	//	return RetType::Void;
	//};
}

static Int_t cellCastInt(ASTNode const& c)
{
	if (std::holds_alternative<Float_t>(c.value))
		return static_cast<Int_t>(std::get<Float_t>(c.value));
	else if (std::holds_alternative<Int_t>(c.value))
		return std::get<Int_t>(c.value);
	// TODO: handle variables
}

static Float_t cellCastFloat(ASTNode const& c)
{
	if (std::holds_alternative<Int_t>(c.value))
		return static_cast<Float_t>(std::get<Int_t>(c.value));

	return std::get<Float_t>(c.value);
}

void VirtualMachine::run(CodeChunk code_chunk)
{
	size_t pc = 0;
	while (pc < code_chunk.code_data.size())
	{
		OpCode const op = static_cast<OpCode>(code_chunk.code_data[pc]);
		pc += sizeof(OpCode);
		
		switch (op)
		{
			case OpCode::Add:
			{
				auto const n1 = stack_memory.top();
				stack_memory.pop();
				auto const n2 = stack_memory.top();
				stack_memory.pop();

					
				//stack_memory.push();
				break;
			}
			case OpCode::PushConst:
			{
				int16_t const constantIndex = code_chunk.read<int16_t>(pc);
				std::visit(overloaded{
					[&](auto value)
					{
						stack_memory.push(ASTNode{value});
					}
				}, code_chunk.get_constant(constantIndex));
				break;
			}
			case OpCode::PushVar:
			{
				//stack_memory.push(mainEnv.symbols[code_chunk.readStr(pc)]);
				break;
			}
			case OpCode::Store:
			{
				auto const varName = code_chunk.readStr(pc);
				//mainEnv.symbols[varName] = stack_memory.top();
				stack_memory.pop();
				break;		
			}
			case OpCode::Call:
			{
				auto const fnName = code_chunk.readStr(pc);
				
				break;
			}
			
		}
	}
}

