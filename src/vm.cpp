#include "vm.hpp"

using namespace ales;

void CodeChunk::add_constant(Constant_t cons)
{
	if (constants.find(cons) == constants.end())
		constants[cons] = nextId++;
}

void CodeChunk::write(OpCode op)
{
	code_data.push_back(static_cast<uint8_t>(op));
}

CodeChunk ales::compile(Cell root)
{
	CodeChunk chunk;
	std::visit(overloaded{
		[&](Int_t value)
		{
			chunk.write(OpCode::PushInt);
			chunk.write(value);
		},
		[&](Float_t value)
		{
			chunk.write(value);
		},
	}, root.value);
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
			Int_t const a = pop<Int_t>();
			Int_t const b = pop<Int_t>();
			push<Int_t>(a + b);
			break;
		}
		case OpCode::PushInt:
		{
			push<Int_t>(read<Int_t>(code_chunk.code_data, next_op_offset));
			break;
		}
		case OpCode::StoreInt:
		{
			break;
		}
		}
	}
}

