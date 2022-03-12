//#ifndef ALES_VM_HPP
//#define ALES_VM_HPP
//
//#include <cstdint>
//#include <vector>
//#include <stack>
//#include <variant>
//#include <unordered_map>
//
//// TODO common include
//#include "ales.hpp"
//
//namespace ales
//{
//	enum class OpCode : uint8_t
//	{
//		Add,
//		PushConst,
//		PushVar,
//		Store,
//		Call,
//	};
//
//	const char* to_string(OpCode e);
//
//	using Constant_t = std::variant<Int_t, Float_t, String_t>;
//	using ConstantID_t = int32_t;
//	
//	struct CodeChunk
//	{
//		// TODO: refactor this, these functions are probably UB and error prone
//		
//		ConstantID_t add_constant(Constant_t);
//		Constant_t get_constant(ConstantID_t) const;
//
//		void write(OpCode op);
//		
//		template<typename T>
//		void write(T value)
//		{
//			static_assert(std::is_trivially_copyable_v<T> == true);
//			size_t const offset = code_data.size();
//			code_data.resize(offset + sizeof(T));
//			memcpy(&code_data[offset], &value, sizeof(T));
//		}
//
//		void writeStr(std::string const& value)
//		{
//			size_t const offset = code_data.size();
//			code_data.resize(offset + value.size() + 1);
//			memcpy(&code_data[offset], value.data(), value.size() + 1);
//		}
//		
//		template<typename T>
//		T read(size_t& offset) const
//		{
//			static_assert(std::is_trivially_copyable_v<T> == true);
//			T const val = *reinterpret_cast<T const*>(&code_data[offset]);
//			offset += sizeof(T);
//			return val;
//		}
//
//		std::string readStr(size_t& offset) const
//		{
//			size_t const len = strlen((char*)&code_data[offset]);
//			std::string const str((char*)&code_data[offset], (char*)&code_data[offset + len]);
//			offset += len + 1;
//			return str;
//		}
//		
//		ConstantID_t nextId = 0;
//
//		std::vector<Constant_t> constants;
//		std::vector<uint8_t> code_data;
//	};
//
//	std::ostream& operator<<(std::ostream& out, CodeChunk const& c);
//	
//	class Compiler
//	{
//	public:
//		using CompileFuncFn_t = CodeChunk(*)();
//
//		void compile(ASTCell const& root);
//		void compileTo(ASTCell const& root, CodeChunk& codechunk);
//		
//		std::vector<CodeChunk> functions;
//
//		std::vector<ASTCell> constants;
//		std::unordered_map<std::string, size_t> functionMap;
//		std::unordered_map<std::string, CompileFuncFn_t> func_compiler;
//	};
//	
//	class VirtualMachine
//	{
//	public:
//
//		void run(CodeChunk code_chunk);
//		
//		std::stack<ASTCell> stack_memory;
//	};
//}
//
//#endif