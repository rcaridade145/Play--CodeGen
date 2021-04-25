#include "IrFunction.h"
#include <cassert>
#include "PtrStream.h"

using namespace Jitter;

SYM_TYPE CIrFunction::GetOperandType(IR_OPERAND op)
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
		case OP_ADD:
			Add(context, instr);
			break;
		case OP_ADDREF:
			AddRef(context, instr);
			break;
		case OP_AND:
			And(context, instr);
			break;
		case OP_AND64:
			And64(context, instr);
			break;
		case OP_CALL:
			Call(context, instr);
			break;
		case OP_CMP:
			Cmp(context, instr);
			break;
		case OP_CMP64:
			Cmp64(context, instr);
			break;
		case OP_CONDJMP:
			CondJmp(context, ip, instr);
			break;
		case OP_DIV:
			Div(context, instr);
			break;
		case OP_DIVS:
			DivS(context, instr);
			break;
		case OP_EXTERNJMP:
		{
			uintptr_t src1Value = GetOperandPtr(context, instr.src1);
			reinterpret_cast<void (*)(void*)>(src1Value)(context);
			return;
		}
		break;
		case OP_EXTLOW64:
			ExtLow64(context, instr);
			break;
		case OP_EXTHIGH64:
			ExtHigh64(context, instr);
			break;
		case OP_JMP:
			Jmp(context, ip, instr);
			break;
		case OP_LOADFROMREF:
			LoadFromRef(context, instr);
			break;
		case OP_LOAD16FROMREF:
			Load16FromRef(context, instr);
			break;
		case OP_MOV:
			Mov(context, instr);
			break;
		case OP_MUL:
			Mul(context, instr);
			break;
		case OP_MULS:
			MulS(context, instr);
			break;
		case OP_NOT:
			Not(context, instr);
			break;
		case OP_OR:
			Or(context, instr);
			break;
		case OP_PARAM:
			Param(context, instr);
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
		case OP_STORE16ATREF:
			Store16AtRef(context, instr);
			break;
		case OP_SUB:
			Sub(context, instr);
			break;
		case OP_XOR:
			Xor(context, instr);
			break;
		default:
			assert(false);
			break;
		}
	}
}

void CIrFunction::Add(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint32 dstValue = src1Value + src2Value;
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::AddRef(void* context, const IR_INSTRUCTION& instr)
{
	uintptr_t src1Value = GetOperandPtr(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uintptr_t dstValue = src1Value + src2Value;
	SetOperandPtr(context, instr.dst, dstValue);
}

void CIrFunction::And(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint32 dstValue = src1Value & src2Value;
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::And64(void* context, const IR_INSTRUCTION& instr)
{
	uint64 src1Value = GetOperand64(context, instr.src1);
	uint64 src2Value = GetOperand64(context, instr.src2);
	uint64 dstValue = src1Value & src2Value;
	SetOperand64(context, instr.dst, dstValue);
}

void CIrFunction::Call(void* context, const IR_INSTRUCTION& instr)
{
	auto src1Value = GetOperandPtr(context, instr.src1);
	auto src2Value = GetOperand(context, instr.src2);
	assert(src2Value <= 3);
	assert(src2Value <= m_params.size());
	uint32 fctSignature = 0;
	if(instr.dst != 0)
	{
		uint32 type = GetOperandType(instr.dst);
		assert(type < 0x80);
		fctSignature |= static_cast<uint8>(type | 0x80);
	}
	for(uint32 i = 0; i < src2Value; i++)
	{
		auto param = m_params[i];
		uint32 type = GetOperandType(param);
		assert(type < 0x80);
		fctSignature |= static_cast<uint8>(type | 0x80) << ((i + 1) * 8);
	}
	switch(fctSignature)
	{
	case 0x00008084:
	{
		void* param0 = context;
		auto fct = reinterpret_cast<uint32 (*)(void*)>(src1Value);
		uint32 dstValue = fct(param0);
		SetOperand(context, instr.dst, dstValue);
	}
	break;
	case 0x00008484:
	{
		uint32 param0 = GetOperand(context, m_params[0]);
		auto fct = reinterpret_cast<uint32 (*)(uint32)>(src1Value);
		uint32 dstValue = fct(param0);
		SetOperand(context, instr.dst, dstValue);
	}
	break;
	case 0x00808484:
	{
		void* param0 = context;
		uint32 param1 = GetOperand(context, m_params[0]);
		auto fct = reinterpret_cast<uint32 (*)(void*, uint32)>(src1Value);
		uint32 dstValue = fct(param0, param1);
		SetOperand(context, instr.dst, dstValue);
	}
	break;
	case 0x0080848A:
	{
		void* param0 = context;
		uint32 param1 = GetOperand(context, m_params[0]);
		auto fct = reinterpret_cast<uint64 (*)(void*, uint32)>(src1Value);
		uint64 dstValue = fct(param0, param1);
		SetOperand64(context, instr.dst, dstValue);
	}
	break;
	case 0x80818400:
	case 0x80818300:
	case 0x80838400:
	{
		void* param0 = context;
		uint32 param1 = GetOperand(context, m_params[1]);
		uint32 param2 = GetOperand(context, m_params[0]);
		auto fct = reinterpret_cast<void (*)(void*, uint32, uint32)>(src1Value);
		fct(param0, param1, param2);
	}
	break;
	case 0x80898400:
	{
		void* param0 = context;
		uint64 param1 = GetOperand64(context, m_params[1]);
		uint32 param2 = GetOperand(context, m_params[0]);
		auto fct = reinterpret_cast<void (*)(void*, uint64, uint32)>(src1Value);
		fct(param0, param1, param2);
	}
	break;
	default:
		assert(false);
		break;
	}
	m_params.erase(m_params.begin(), m_params.begin() + src2Value);
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

void CIrFunction::Cmp64(void* context, const IR_INSTRUCTION& instr)
{
	auto cc = GetInstructionCondition(instr);
	uint64 src1Value = GetOperand64(context, instr.src1);
	uint64 src2Value = GetOperand64(context, instr.src2);
	uint32 dstValue = 0;
	switch(cc)
	{
	case CONDITION_NE:
		dstValue = src1Value != src2Value;
		break;
	case CONDITION_BL:
		dstValue = src1Value < src2Value;
		break;
	case CONDITION_LT:
		dstValue = (static_cast<int64>(src1Value) < static_cast<int64>(src2Value));
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
	bool isSrc1Word = (src1Type == SYM_RELATIVE) || (src1Type == SYM_TEMPORARY);
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
			ip = instr.dst - 1;
		}
	}
	else if(isSrc1Word && (src2Type == SYM_CONSTANT))
	{
		auto src1Value = GetOperand(context, instr.src1);
		auto src2Value = GetOperand(context, instr.src2);
		bool result = false;
		switch(cc)
		{
		case CONDITION_EQ:
			result = (src1Value == src2Value);
			break;
		case CONDITION_NE:
			result = (src1Value != src2Value);
			break;
		case CONDITION_GT:
			result = (static_cast<int32>(src1Value) > static_cast<int32>(src2Value));
			break;
		default:
			assert(false);
			break;
		}
		if(result)
		{
			ip = instr.dst - 1;
		}
	}
	else
	{
		assert(false);
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

void CIrFunction::DivS(void* context, const IR_INSTRUCTION& instr)
{
	int32 src1Value = GetOperand(context, instr.src1);
	int32 src2Value = GetOperand(context, instr.src2);
	int32 dividend = src1Value / src2Value;
	int32 remainder = src1Value % src2Value;
	uint64 dstValue = dividend | static_cast<uint64>(remainder) << 32;
	SetOperand64(context, instr.dst, dstValue);
}

void CIrFunction::ExtLow64(void* context, const IR_INSTRUCTION& instr)
{
	uint64 src1Value = GetOperand64(context, instr.src1);
	uint32 dstValue = static_cast<uint32>(src1Value);
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::ExtHigh64(void* context, const IR_INSTRUCTION& instr)
{
	uint64 src1Value = GetOperand64(context, instr.src1);
	uint32 dstValue = static_cast<uint32>(src1Value >> 32);
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::Jmp(void* context, uint32& ip, const IR_INSTRUCTION& instr)
{
	ip = instr.dst - 1;
}

void CIrFunction::LoadFromRef(void* context, const IR_INSTRUCTION& instr)
{
	auto dstType = GetOperandType(instr.dst);
	auto src1Value = GetOperandPtr(context, instr.src1);
	switch(dstType)
	{
	case SYM_RELATIVE:
	case SYM_TEMPORARY:
	{
		auto dstValue = *reinterpret_cast<uint32*>(src1Value);
		SetOperand(context, instr.dst, dstValue);
	}
	break;
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

void CIrFunction::Load16FromRef(void* context, const IR_INSTRUCTION& instr)
{
	auto src1Value = GetOperandPtr(context, instr.src1);
	auto dstValue = *reinterpret_cast<uint16*>(src1Value);
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::Mov(void* context, const IR_INSTRUCTION& instr)
{
	auto src1Type = GetOperandType(instr.src1);
	switch(src1Type)
	{
	case SYM_RELATIVE:
	case SYM_TEMPORARY:
	case SYM_CONSTANT:
	{
		uint32 value = GetOperand(context, instr.src1);
		SetOperand(context, instr.dst, value);
	}
	break;
	case SYM_RELATIVE64:
	case SYM_TEMPORARY64:
	case SYM_CONSTANT64:
	{
		uint64 value = GetOperand64(context, instr.src1);
		SetOperand64(context, instr.dst, value);
	}
	break;
	default:
		assert(false);
		break;
	}
}

void CIrFunction::Mul(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint64 dstValue = static_cast<uint64>(src1Value) * static_cast<uint64>(src2Value);
	SetOperand64(context, instr.dst, dstValue);
}

void CIrFunction::MulS(void* context, const IR_INSTRUCTION& instr)
{
	int32 src1Value = GetOperand(context, instr.src1);
	int32 src2Value = GetOperand(context, instr.src2);
	int64 dstValue = static_cast<int64>(src1Value) * static_cast<int64>(src2Value);
	SetOperand64(context, instr.dst, dstValue);
}

void CIrFunction::Not(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 dstValue = ~src1Value;
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::Or(void* context, const IR_INSTRUCTION& instr)
{
	uint32 src1Value = GetOperand(context, instr.src1);
	uint32 src2Value = GetOperand(context, instr.src2);
	uint32 dstValue = src1Value | src2Value;
	SetOperand(context, instr.dst, dstValue);
}

void CIrFunction::Param(void* context, const IR_INSTRUCTION& instr)
{
	m_params.push_back(instr.src1);
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
	case SYM_RELATIVE:
	case SYM_CONSTANT:
	{
		auto value = GetOperand(context, instr.src2);
		*reinterpret_cast<uint32*>(src1Value) = value;
	}
	break;
	case SYM_RELATIVE64:
	{
		auto value = GetOperand64(context, instr.src2);
		*reinterpret_cast<uint64*>(src1Value) = value;
	}
	break;
	case SYM_RELATIVE128:
	{
		auto value = GetOperand128(context, instr.src2);
		_mm_store_ps(reinterpret_cast<float*>(src1Value), value);
	}
	break;
	default:
		assert(false);
		break;
	}
}

void CIrFunction::Store16AtRef(void* context, const IR_INSTRUCTION& instr)
{
	auto src1Value = GetOperandPtr(context, instr.src1);
	auto value = GetOperand(context, instr.src2);
	*reinterpret_cast<uint16*>(src1Value) = static_cast<uint16>(value);
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
	assert((offset & 0x03) == 0);
	uint32 elemOffset = offset / 4;
	uint32 value = 0;
	switch(type)
	{
	case SYM_RELATIVE:
		value = reinterpret_cast<uint32*>(context)[elemOffset];
		break;
	case SYM_TEMPORARY:
		value = reinterpret_cast<uint32*>(m_stack.data())[elemOffset];
		break;
	case SYM_CONSTANT:
		value = reinterpret_cast<uint32*>(m_constants.data())[elemOffset];
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
	assert((offset & 0x07) == 0);
	uint32 elemOffset = offset / 8;
	uint64 value = 0;
	switch(type)
	{
	case SYM_RELATIVE64:
		value = reinterpret_cast<uint64*>(context)[elemOffset];
		break;
	case SYM_TEMPORARY64:
		value = reinterpret_cast<uint64*>(m_stack.data())[elemOffset];
		break;
	case SYM_CONSTANT64:
		value = reinterpret_cast<uint64*>(m_constants.data())[elemOffset];
		break;
	default:
		assert(false);
		break;
	}
	return value;
}

IR_UINT128 CIrFunction::GetOperand128(void* context, IR_OPERAND op)
{
	auto type = static_cast<SYM_TYPE>(op >> 16);
	uint32 offset = op & 0xFFFF;
	assert((offset & 0x0F) == 0);
	uint32 elemOffset = offset / 4;
	IR_UINT128 value = _mm_setzero_ps();
	switch(type)
	{
	case SYM_RELATIVE128:
		value = _mm_load_ps(reinterpret_cast<float*>(context) + elemOffset);
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
	static_assert(sizeof(uintptr_t) == 8);
	assert((offset & 0x07) == 0);
	uint32 elemOffset = offset / 8;
	uintptr_t value = 0;
	switch(type)
	{
	case SYM_REL_REFERENCE:
		value = reinterpret_cast<uintptr_t*>(context)[elemOffset];
		break;
	case SYM_TMP_REFERENCE:
		value = reinterpret_cast<uintptr_t*>(m_stack.data())[elemOffset];
		break;
	case SYM_CONSTANTPTR:
		value = reinterpret_cast<uintptr_t*>(m_constants.data())[elemOffset];
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
	assert((offset & 0x03) == 0);
	uint32 elemOffset = offset / 4;
	switch(type)
	{
	case SYM_RELATIVE:
		reinterpret_cast<uint32*>(context)[elemOffset] = value;
		break;
	case SYM_TEMPORARY:
		reinterpret_cast<uint32*>(m_stack.data())[elemOffset] = value;
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
	assert((offset & 0x07) == 0);
	uint32 elemOffset = offset / 8;
	switch(type)
	{
	case SYM_RELATIVE64:
		reinterpret_cast<uint64*>(context)[elemOffset] = value;
		break;
	case SYM_TEMPORARY64:
		reinterpret_cast<uint64*>(m_stack.data())[elemOffset] = value;
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
	static_assert(sizeof(uintptr_t) == 8);
	assert((offset & 0x07) == 0);
	uint32 elemOffset = offset / 8;
	switch(type)
	{
	case SYM_TMP_REFERENCE:
		reinterpret_cast<uintptr_t*>(m_stack.data())[elemOffset] = value;
		break;
	default:
		assert(false);
		break;
	}
}
