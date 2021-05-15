#ifndef ALES_VM_HPP
#define ALES_VM_HPP

#include <cstdint>
#include <vector>
#include <variant>

// @TODO common include
#include "ales.hpp"

namespace ales
{
	enum class OpCode : uint8_t
	{
		AddInt,
		AddFloat,
		PushInt,
		PushFloat,
		StoreInt,
		StoreFloat,
	};

	const char* to_string(OpCode e);

	using Constant_t = std::variant<Int_t, Float_t, String_t>;
	using ConstantID_t = int32_t;
	
	struct CodeChunk
	{
		void add_constant(Constant_t);

		void write(OpCode op);
		
		template<typename T>
		void write(T value)
		{
			static_assert(std::is_trivially_copyable_v<T> == true);
			size_t const offset = code_data.size();
			code_data.resize(offset + sizeof(T));
			memcpy(&code_data[offset], &value, sizeof(T));
		}

		ConstantID_t nextId = 0;

		// @Performance use std vector here ?
		std::unordered_map<Constant_t, ConstantID_t> constants;
		std::vector<uint8_t> code_data;
	};

	std::ostream& operator<<(std::ostream& out, CodeChunk const& c);
	
	class Compiler
	{
	public:
		using CompileSymbolFn_t = std::vector<uint8_t>(*)();

		CodeChunk compile(Cell root);

		CodeChunk chunk;
		std::unordered_map<std::string, CompileSymbolFn_t> symbol_compilers;
	};

	
	class VirtualMachine
	{
	public:

		void run(CodeChunk code_chunk);

		template<typename T>
		void push(T value)
		{
			static_assert(std::is_trivially_copyable_v<T> == true);
			size_t const current_offset = stack_memory.size();
			stack_memory.resize(current_offset + sizeof(T));
			(*reinterpret_cast<T*>(&stack_memory[current_offset])) = value;
		}

		template<typename T>
		T pop()
		{
			static_assert(std::is_trivially_copyable_v<T> == true);
			size_t const start_offset = stack_memory.size() - sizeof(T);
			T const value = *reinterpret_cast<T*>(&stack_memory[start_offset]);
			stack_memory.erase(stack_memory.begin() + start_offset, stack_memory.end());
			return value;
		}
		
		template<typename T>
		T read(std::vector<uint8_t> const& code, size_t& offset)
		{
			static_assert(std::is_trivially_copyable_v<T> == true);
			T const val = *reinterpret_cast<T const*>(&code[offset]);
			offset += sizeof(T);
			return val;
		}

		std::vector<uint8_t> stack_memory;
	};
}

#endif