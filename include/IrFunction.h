#pragma once

#include "Types.h"
#include "Jitter_Statement.h"
#ifdef _WIN32
#include <xmmintrin.h>
#endif

typedef uint32 IR_OPERAND;
#ifdef _WIN32
typedef __m128 IR_UINT128;
#endif

struct IR_INSTRUCTION
{
	uint32 op;
	IR_OPERAND dst;
	IR_OPERAND src1;
	IR_OPERAND src2;
};

struct IR_HEADER
{
	uint32 stackSize;
};

class CIrFunction
{
public:
	CIrFunction(void*, uint32);

	void Execute(void*);

private:
	static Jitter::SYM_TYPE GetOperandType(uint32);
	static Jitter::CONDITION GetInstructionCondition(const IR_INSTRUCTION&);

	uint32 GetOperand(void*, IR_OPERAND);
	uint64 GetOperand64(void*, IR_OPERAND);
	IR_UINT128 GetOperand128(void*, IR_OPERAND);
	uintptr_t GetOperandPtr(void*, IR_OPERAND);

	void SetOperand(void*, IR_OPERAND, uint32);
	void SetOperand64(void*, IR_OPERAND, uint64);
	void SetOperandPtr(void*, IR_OPERAND, uintptr_t);

	void Add(void*, const IR_INSTRUCTION&);
	void AddRef(void*, const IR_INSTRUCTION&);
	void And(void*, const IR_INSTRUCTION&);
	void Call(void*, const IR_INSTRUCTION&);
	void Cmp(void*, const IR_INSTRUCTION&);
	void Cmp64(void*, const IR_INSTRUCTION&);
	void CondJmp(void*, uint32&, const IR_INSTRUCTION&);
	void Div(void*, const IR_INSTRUCTION&);
	void DivS(void*, const IR_INSTRUCTION&);
	void ExtLow64(void*, const IR_INSTRUCTION&);
	void ExtHigh64(void*, const IR_INSTRUCTION&);
	void Jmp(void*, uint32&, const IR_INSTRUCTION&);
	void LoadFromRef(void*, const IR_INSTRUCTION&);
	void Mov(void*, const IR_INSTRUCTION&);
	void Mul(void*, const IR_INSTRUCTION&);
	void MulS(void*, const IR_INSTRUCTION&);
	void Not(void*, const IR_INSTRUCTION&);
	void Or(void*, const IR_INSTRUCTION&);
	void Param(void*, const IR_INSTRUCTION&);
	void Sll(void*, const IR_INSTRUCTION&);
	void Srl(void*, const IR_INSTRUCTION&);
	void Sra(void*, const IR_INSTRUCTION&);
	void StoreAtRef(void*, const IR_INSTRUCTION&);
	void Sub(void*, const IR_INSTRUCTION&);
	void Xor(void*, const IR_INSTRUCTION&);

	IR_HEADER m_header;

	std::vector<IR_INSTRUCTION> m_instructions;
	std::vector<uint32> m_stack;
	std::vector<uint32> m_constants;
	std::vector<uint32> m_params;
};
