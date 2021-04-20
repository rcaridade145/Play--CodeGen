#include "IrFunction.h"
#include <cassert>
#include "PtrStream.h"

using namespace Jitter;

SYM_TYPE CIrFunction::GetOperandType(uint32 op)
{
	return static_cast<SYM_TYPE>(op >> 16);
}

CONDITION CIrFunction::GetInstructionCondition(const IR_INSTRUCTION& instr)
{
	return static_cast<Jitter::CONDITION>(instr.op >> 24);
}

CIrFunction::CIrFunction(void* code, uint32 size)
{
	Framework::CPtrStream inputStream(code, size);
	inputStream.Read(&m_header, sizeof(IR_HEADER));

	uint32 instrsSize = inputStream.Read32();
	m_instructions.resize(instrsSize);
	inputStream.Read(m_instructions.data(), instrsSize * sizeof(IR_INSTRUCTION));

	uint32 constantsSize = inputStream.Read32();
	m_constants.resize(constantsSize);
	inputStream.Read(m_constants.data(), constantsSize * sizeof(uint32));

	m_stack.resize(m_header.stackSize / 4);
}

void CIrFunction::Execute(void* context)
{
	for(uint32 ip = 0; ip < m_instructions.size(); ip++)
	{
		const auto& instr = m_instructions[ip];
		auto op = static_cast<Jitter::OPERATION>(instr.op & 0xFFFF);
		switch(op)
		{
		case OP_MOV:
		{
			uint32 src1Value = GetOperand(context, instr.src1);
			SetOperand(context, instr.dst, src1Value);
		}
		break;
		case OP_ADDREF:
		{
			uintptr_t src1Value = GetOperandPtr(context, instr.src1);
			uint32 src2Value = GetOperand(context, instr.src2);
			uintptr_t dstValue = src1Value + src2Value;
			SetOperandPtr(context, instr.dst, dstValue);
		}
		break;
		case OP_AND:
			And(context, instr);
			break;
		case OP_LOADFROMREF:
			LoadFromRef(context, instr);
			break;
		case OP_CMP:
			Cmp(context, instr);
			break;
		case OP_CONDJMP:
			CondJmp(context, ip, instr);
			break;
		case OP_DIV:
			Div(context, instr);
			break;
		case OP_EXTLOW64:
			ExtLow64(context, instr);
			break;
		case OP_SLL:
			Sll(context, instr);
			break;
		case OP_SRL:
			Srl(context, instr);
			break;
		case OP_SRA:
			Sra(context, instr);
			break;
		case OP_STOREATREF:
			StoreAtRef(context, instr);
			break;
		case OP_SUB:
			Sub(context, instr);
			break;
		case OP_XOR:
			Xor(context, instr);
			break;
		case OP_EXTERNJMP:
		{
			uintptr_t src1Value = GetOperandPtr(context, instr.src1);
			reinterpret_cast<void (*)(void*)>(src1Value)(context);
			return;
		}
		break;
		default:
			assert(false);
			break;
		}
	}
}

void CIrFunction::And(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint32 dstValue = src1Value & src2Value;
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::Cmp(void* context, const IR_INSTRUCTION& instr)
{
	auto cc = GetInstructionCondition(instr);
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint32 dstValue = 0;
	switch(cc)
	{
	case CONDITION_NE:
		dstValue = src1Value != src2Value;
		break;
	case CONDITION_LT:
		dstValue = (static_cast<int32>(src1Value) < static_cast<int32>(src2Value));
		break;
	default:
		assert(false);
		break;
	}
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::CondJmp(void* context, uint32& ip, const IR_INSTRUCTION& instr)
{
	auto cc = GetInstructionCondition(instr);
	auto src1Type = GetOperandType(instr.src1);
	auto src2Type = GetOperandType(instr.src2);
	if((src1Type == SYM_TMP_REFERENCE) && (src2Type == SYM_CONSTANT))
	{
		auto src1Value = GetOperandPtr(context, instr.src1);
		auto src2Value = GetOperand(context, instr.src2);
		assert(src2Value == 0);
		bool result = false;
		switch(cc)
		{
		case CONDITION_EQ:
			result = (src1Value == src2Value);
			break;
		default:
			assert(false);
			break;
		}
		if(result)
		{
			ip = instr.dst;
		}
	}
}

void CIrFunction::Div(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint32 dividend = src1Value / src2Value;
	uint32 remainder = src1Value % src2Value;
	uint64 dstValue = dividend | static_cast<uint64>(remainder) << 32;
	SetOperand64(context, instr.dst, dstValue);
}

void CIrFunction::ExtLow64(void* context, const IR_INSTRUCTION& instr)
{
	uint64 src1Value = GetOperand64(context, instr.src1);
	uint32 dstValue = static_cast<uint64>(src1Value);
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::LoadFromRef(void* context, const IR_INSTRUCTION& instr)
{
	auto dstType = GetOperandType(instr.dst);
	auto src1Value = GetOperandPtr(context, instr.src1);
	switch(dstType)
	{
	case SYM_TMP_REFERENCE:
	{
		auto dstValue = *reinterpret_cast<uintptr_t*>(src1Value);
		SetOperandPtr(context, instr.dst, dstValue);
	}
	break;
	default:
		assert(false);
		break;
	}
}

void CIrFunction::Sll(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint32 dstValue = src1Value << src2Value;
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::Srl(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint32 dstValue = src1Value >> src2Value;
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::Sra(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint32 dstValue = (static_cast<int32>(src1Value) >> static_cast<int32>(src2Value));
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::StoreAtRef(void* context, const IR_INSTRUCTION& instr)
{
	auto src1Value = GetOperandPtr(context, instr.src1);
	auto src2Type = GetOperandType(instr.src2);
	switch(src2Type)
	{
	//case SYM_RELATIVE128:
	//	break;
	default:
		assert(false);
		break;
	}
}

void CIrFunction::Sub(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint32 dstValue = src1Value - src2Value;
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::Xor(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint32 dstValue = src1Value ^ src2Value;
	SetOperand(context, instr.dst, dstValue);
}

uint32 CIrFunction::GetOperand(void* context, IR_OPERAND op)
{
	auto type = static_cast<SYM_TYPE>(op >> 16);
	uint32 offset = op & 0xFFFF;
	uint32 value = 0;
	switch(type)
	{
	case SYM_RELATIVE:
		value = reinterpret_cast<uint32*>(context)[offset / 4];
		break;
	case SYM_TEMPORARY:
		value = m_stack[offset / 4];
		break;
	case SYM_CONSTANT:
		value = m_constants[offset];
		break;
	default:
		assert(false);
		break;
	}
	return value;
}

uint64 CIrFunction::GetOperand64(void* context, IR_OPERAND op)
{
	auto type = static_cast<SYM_TYPE>(op >> 16);
	uint32 offset = op & 0xFFFF;
	uint64 value = 0;
	switch(type)
	{
	case SYM_TEMPORARY64:
		value = *reinterpret_cast<uint64*>(m_stack.data() + offset / 4);
		break;
	default:
		assert(false);
		break;
	}
	return value;
}

uintptr_t CIrFunction::GetOperandPtr(void* context, IR_OPERAND op)
{
	auto type = static_cast<SYM_TYPE>(op >> 16);
	uint32 offset = op & 0xFFFF;
	uintptr_t value = 0;
	switch(type)
	{
	case SYM_CONSTANTPTR:
		value = *reinterpret_cast<uintptr_t*>(m_constants.data() + offset);
		break;
	case SYM_REL_REFERENCE:
		value = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uint8*>(context) + offset);
		break;
	case SYM_TMP_REFERENCE:
		value = *reinterpret_cast<uintptr_t*>(m_stack.data() + offset / 4);
		break;
	default:
		assert(false);
		break;
	}
	return value;
}

void CIrFunction::SetOperand(void* context, IR_OPERAND op, uint32 value)
{
	auto type = static_cast<SYM_TYPE>(op >> 16);
	uint32 offset = op & 0xFFFF;
	switch(type)
	{
	case SYM_RELATIVE:
		reinterpret_cast<uint32*>(context)[offset / 4] = value;
		break;
	case SYM_TEMPORARY:
		m_stack[offset / 4] = value;
		break;
	default:
		assert(false);
		break;
	}
}

void CIrFunction::SetOperand64(void* context, IR_OPERAND op, uint64 value)
{
	auto type = static_cast<SYM_TYPE>(op >> 16);
	uint32 offset = op & 0xFFFF;
	switch(type)
	{
	case SYM_TEMPORARY64:
		*reinterpret_cast<uint64*>(m_stack.data() + offset / 4) = value;
		break;
	default:
		assert(false);
		break;
	}
}

void CIrFunction::SetOperandPtr(void* context, IR_OPERAND op, uintptr_t value)
{
	auto type = static_cast<SYM_TYPE>(op >> 16);
	uint32 offset = op & 0xFFFF;
	switch(type)
	{
	case SYM_TMP_REFERENCE:
		*reinterpret_cast<uintptr_t*>(m_stack.data() + offset / 4) = value;
		break;
	default:
		assert(false);
		break;
	}
}
