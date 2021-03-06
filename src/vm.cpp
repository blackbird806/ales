#include "vm.hpp"

#include <cassert>
#include <numeric>

#include "ostream"

using namespace ales;

const char* ales::to_string(OpCode e)
{
	switch (e)
	{
	case OpCode::AddInt: return "AddInt";
	case OpCode::AddFloat: return "AddFloat";
	case OpCode::PushInt: return "PushInt";
	case OpCode::PushFloat: return "PushFloat";
	case OpCode::Store: return "Store";
	default: return "unknown";
	}
}

void CodeChunk::add_constant(Constant_t cons)
{
	if (constants.find(cons) == constants.end())
		constants[cons] = nextId++;
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
		case OpCode::AddInt: break;
		case OpCode::AddFloat: break;
		case OpCode::PushInt:
			out << chunk.read<Int_t>(next_op_offset);
			break;
		case OpCode::PushFloat:
			out << chunk.read<Float_t>(next_op_offset);
			break;
		case OpCode::Store:
			out << chunk.readStr(next_op_offset);
			break;
		default: ;
		}
		out << "\n";
	}
	return out;
}

RetType Compiler::compile(Cell const& cell, Statement const* enclosing)
{
	RetType type = RetType::Void;
	std::visit(overloaded{
		[&](Statement const& statement)
		{
			if (statement.cells.empty())
				return;
			//
			//for (auto it = statement.cells.begin(); it != statement.cells.end(); ++it)
			//	compile(*it, &statement);
			
			type = compile(*statement.cells.begin(), &statement);
		},
		[&](Function const& sym)
		{
			assert(enclosing != nullptr);
			type = func_compiler[sym.name](*enclosing, *this);
		},
		[&](Variable const& value)
		{
			chunk.write(OpCode::PushVar);
			chunk.writeStr(value.name);
		},
		[&](Int_t value)
		{
			chunk.write(OpCode::PushInt);
			chunk.write(value);
			type = RetType::Int;
		},
		[&](Float_t value)
		{
			chunk.write(OpCode::PushFloat);
			chunk.write(value);
			type = RetType::Float;
		},
		[&](Bool_t value)
		{
		},
		[&](String_t const& value)
		{
			chunk.write(OpCode::PushString);
			chunk.writeStr(value);
			type = RetType::String;
		},
	}, cell.value);
	
	return type;
}

static Int_t cellCastInt(Cell const& c)
{
	if (std::holds_alternative<Float_t>(c.value))
		return static_cast<Int_t>(std::get<Float_t>(c.value));
	
	return std::get<Int_t>(c.value);
}

static Float_t cellCastFloat(Cell const& c)
{
	if (std::holds_alternative<Int_t>(c.value))
		return static_cast<Float_t>(std::get<Int_t>(c.value));

	return std::get<Float_t>(c.value);
}

void VirtualMachine::run(CodeChunk code_chunk)
{
	size_t next_op_offset = 0;
	while (next_op_offset < code_chunk.code_data.size())
	{
		OpCode const op = static_cast<OpCode>(code_chunk.code_data[next_op_offset]);
		next_op_offset += sizeof(OpCode);
		
		switch (op)
		{
			case OpCode::AddInt:
			{
				auto const n1 = cellCastInt(stack_memory.top());
				stack_memory.pop();
				auto const n2 = cellCastInt(stack_memory.top());
				stack_memory.pop();
				stack_memory.push({n1 + n2});
				break;
			}
			case OpCode::AddFloat:
			{
				auto const n1 = cellCastFloat(stack_memory.top());
				stack_memory.pop();
				auto const n2 = cellCastFloat(stack_memory.top());
				stack_memory.pop();
				stack_memory.push({ n1 + n2 });
				break;
			}
			case OpCode::PushInt:
			{
				stack_memory.push(Cell{ code_chunk.read<Int_t>(next_op_offset) });
				break;
			}
			case OpCode::PushFloat:
			{
				stack_memory.push(Cell{ code_chunk.read<Float_t>(next_op_offset) });
				break;
			}
			case OpCode::PushVar:
			{
				stack_memory.push(mainEnv.symbols[code_chunk.readStr(next_op_offset)]);
				break;
			}
			case OpCode::Store:
			{
				auto const varName = code_chunk.readStr(next_op_offset);
				mainEnv.symbols[varName] = stack_memory.top();
				stack_memory.pop();
				break;		
			}
		}
	}
}

