//#include "vm.hpp"
//
//#include <cassert>
//#include <numeric>
//
//#include "ostream"
//
//using namespace ales;
//
//const char* ales::to_string(OpCode e)
//{
//	switch (e)
//	{
//	case OpCode::Add: return "Add";
//	case OpCode::PushConst: return "PushConst";
//	case OpCode::PushVar: return "PushVar";
//	case OpCode::Store: return "Store";
//	case OpCode::Call: return "Call";
//	default: return "unknown";
//	}
//}
//
//ConstantID_t CodeChunk::add_constant(Constant_t cons)
//{
//	auto const it = std::find(constants.begin(), constants.end(), cons);
//	// constant already exist so reuse it
//	if (it != constants.end())
//		return it - constants.begin();
//
//	constants.push_back(cons);
//	return constants.size() - 1;
//}
//
//Constant_t CodeChunk::get_constant(ConstantID_t id) const
//{
//	return constants[id];
//}
//
//void CodeChunk::write(OpCode op)
//{
//	code_data.push_back(static_cast<uint8_t>(op));
//}
//
//std::ostream& ales::operator<<(std::ostream& out, CodeChunk const& chunk)
//{
//	size_t next_op_offset = 0;
//	while (next_op_offset < chunk.code_data.size())
//	{
//		OpCode const op = static_cast<OpCode>(chunk.code_data[next_op_offset]);
//		next_op_offset += sizeof(OpCode);
//		out << to_string(op) << " ";
//		switch (op)
//		{
//		case OpCode::Add: break;
//		case OpCode::Call:
//			out << chunk.read<size_t>(next_op_offset);
//			break;
//		case OpCode::PushConst:
//		{
//			auto const& c = chunk.get_constant(chunk.read<ConstantID_t>(next_op_offset));
//			std::visit(overloaded{
//				[&](auto value)
//				{
//					out << value;
//				}
//				}, c);
//		}
//		break;
//		case OpCode::PushVar:
//		case OpCode::Store:
//			out << chunk.readStr(next_op_offset);
//			break;
//		}
//		out << "\n";
//	}
//	return out;
//}
//
//void Compiler::compile(ASTCell const& cell)
//{
//	for (auto const& [n, f] : func_compiler)
//	{
//		functionMap[n] = functions.size();
//		functions.push_back(f());
//	}
//	compileTo(cell, functions.emplace_back());
//}
//
//void Compiler::compileTo(ASTCell const& cell, CodeChunk& codechunk)
//{
//	std::visit(overloaded{
//		[&] (CellList_t const& value)
//		{
//			// if list is function call
//			if (!value.empty() && std::holds_alternative<Symbol>(value[0].value))
//			{
//				std::string const& fnName = std::get<Symbol>(value[0].value).name;
//				auto const fnIt = functionMap.find(fnName);
//				if (fnIt != functionMap.end())
//				{
//					for (auto arg = value.begin() + 1; arg != value.end(); ++arg)
//					{
//						compileTo(*arg, codechunk);
//					}
//					
//					codechunk.write(OpCode::Call);
//					codechunk.write(fnIt->second);
//				}
//			}
//			else
//			{
//				for (auto const& e : value)
//					compileTo(e, codechunk);
//			}
//		},
//		[&](FunctionDecl const& value)
//		{
//		},
//		[&](Symbol const& value)
//		{
//			codechunk.write(OpCode::PushVar);
//			codechunk.writeStr(value.name);
//		},
//		[&](Int_t value)
//		{
//			codechunk.write(OpCode::PushConst);
//			codechunk.write(codechunk.add_constant(Constant_t{ value }));
//		},
//		[&](Float_t value)
//		{
//			codechunk.write(OpCode::PushConst);
//			codechunk.write(codechunk.add_constant(Constant_t{ value }));
//		},
//		[&](Bool_t value)
//		{
//			codechunk.write(OpCode::PushConst);
//			codechunk.write(codechunk.add_constant(Constant_t{ value }));
//		},
//		[&](String_t const& value)
//		{
//			codechunk.write(OpCode::PushConst);
//			codechunk.write(codechunk.add_constant(Constant_t{ value }));
//		},
//	}, cell.value);
//}
//
//static Int_t cellCastInt(ASTCell const& c)
//{
//	if (std::holds_alternative<Float_t>(c.value))
//		return static_cast<Int_t>(std::get<Float_t>(c.value));
//	else if (std::holds_alternative<Int_t>(c.value))
//		return std::get<Int_t>(c.value);
//	// TODO: handle variables
//}
//
//static Float_t cellCastFloat(ASTCell const& c)
//{
//	if (std::holds_alternative<Int_t>(c.value))
//		return static_cast<Float_t>(std::get<Int_t>(c.value));
//
//	return std::get<Float_t>(c.value);
//}
//
//void VirtualMachine::run(CodeChunk code_chunk)
//{
//	size_t pc = 0;
//	while (pc < code_chunk.code_data.size())
//	{
//		OpCode const op = static_cast<OpCode>(code_chunk.code_data[pc]);
//		pc += sizeof(OpCode);
//		
//		switch (op)
//		{
//			case OpCode::Add:
//			{
//				auto const n1 = stack_memory.top();
//				stack_memory.pop();
//				auto const n2 = stack_memory.top();
//				stack_memory.pop();
//
//				stack_memory.push({std::get<Int_t>(n1.value) + std::get<Int_t>(n2.value) });
//				break;
//			}
//			case OpCode::PushConst:
//			{
//				auto const constantIndex = code_chunk.read<ConstantID_t>(pc);
//				std::visit(overloaded{
//					[&](auto value)
//					{
//						stack_memory.push(ASTCell{value});
//					}
//				}, code_chunk.get_constant(constantIndex));
//				break;
//			}
//			case OpCode::PushVar:
//			{
//				//stack_memory.push(mainEnv.symbols[code_chunk.readStr(pc)]);
//				break;
//			}
//			case OpCode::Store:
//			{
//				auto const varName = code_chunk.readStr(pc);
//				//mainEnv.symbols[varName] = stack_memory.top();
//				stack_memory.pop();
//				break;		
//			}
//			case OpCode::Call:
//			{
//				auto const fnName = code_chunk.readStr(pc);
//					
//				break;
//			}
//			
//		}
//	}
//}
//
