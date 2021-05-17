#include "vm.hpp"
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
	case OpCode::StoreInt: return "StoreInt";
	case OpCode::StoreFloat: return "StoreFloat";
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
		switch (op)
		{
		case OpCode::AddInt: break;
		case OpCode::AddFloat: break;
		case OpCode::PushInt: 
			next_op_offset += sizeof(Int_t);
			break;
		case OpCode::PushFloat:
			next_op_offset += sizeof(Float_t);
			break;
		case OpCode::StoreInt: break;
		case OpCode::StoreFloat: break;
		default: ;
		}
		out << to_string(op) << "\n";
	}
	return out;
}

CodeChunk Compiler::compile(Cell cell)
{
	std::visit(overloaded{
		[&](Statement const& statement)
		{
			for (auto it = statement.cells.rbegin(); it != statement.cells.rend(); ++it)
				compile(*it);
		},
		[&](Symbol const& sym)
		{
			auto compiled_symbol = symbol_compilers[sym.name]();
			chunk.code_data.insert(chunk.code_data.end(), compiled_symbol.begin(), compiled_symbol.end());
		},
		[&](Int_t value)
		{
			chunk.write(OpCode::PushInt);
			chunk.write(value);
		},
		[&](Float_t value)
		{
			chunk.write(OpCode::PushFloat);
			chunk.write(value);
		},
		[&](Bool_t value)
		{
		},
		[&](String_t value)
		{
		},
	}, cell.value);
	return chunk;
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
		case OpCode::AddFloat:
		{
			Float_t const a = pop<Float_t>();
			Float_t const b = pop<Float_t>();
			push<Float_t>(a + b);
			break;
		}
		case OpCode::PushInt:
		{
			push<Int_t>(read<Int_t>(code_chunk.code_data, next_op_offset));
			break;
		}
		case OpCode::PushFloat:
		{
			push<Float_t>(read<Float_t>(code_chunk.code_data, next_op_offset));
			break;
		}
		case OpCode::StoreInt:
		{
			break;
		}
		}
	}
}

