#include "Jitter_CodeGen_Interp.h"
#include "IrFunction.h"

using namespace Jitter;

void CCodeGen_Interp::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	assert(m_constants.empty());

	{
		IR_HEADER header;
		header.stackSize = stackSize;
		m_stream->Write(&header, sizeof(header));
	}

	std::vector<IR_INSTRUCTION> instrs;
	std::map<uint32, uint32> labelDefs;
	std::multimap<uint32, uint32> labelRefs;

	instrs.reserve(statements.size());

	for(const auto& statement : statements)
	{
		if(statement.op == OP_LABEL)
		{
			labelDefs.insert(std::make_pair(statement.jmpBlock, static_cast<uint32>(instrs.size())));
			continue;
		}

		if((statement.op == OP_CONDJMP) || (statement.op == OP_JMP))
		{
			labelRefs.insert(std::make_pair(statement.jmpBlock, static_cast<uint32>(instrs.size())));
		}

		if(statement.op == OP_RETVAL)
		{
			auto& callInstr = *instrs.rbegin();
			assert(callInstr.op == OP_CALL);
			callInstr.dst = EncodeOperand(statement.dst);
			continue;
		}

		IR_INSTRUCTION instr;
		instr.op = EncodeOp(statement);
		instr.dst = EncodeOperand(statement.dst);
		instr.src1 = EncodeOperand(statement.src1);
		instr.src2 = EncodeOperand(statement.src2);
		instrs.push_back(instr);
	}

	for(const auto& labelRefPair : labelRefs)
	{
		assert(labelDefs.find(labelRefPair.first) != std::end(labelDefs));
		const auto& labelPointer = labelDefs[labelRefPair.first];
		assert(labelPointer <= instrs.size());
		assert(labelRefPair.second < instrs.size());
		auto& instr = instrs[labelRefPair.second];
		assert(instr.dst == 0);
		instr.dst = labelPointer;
	}

	m_stream->Write32(instrs.size());
	m_stream->Write(instrs.data(), instrs.size() * sizeof(IR_INSTRUCTION));

	m_stream->Write32(m_constants.size());
	m_stream->Write(m_constants.data(), m_constants.size() * sizeof(uint32));
	m_constants.clear();
}

void CCodeGen_Interp::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
}

void CCodeGen_Interp::RegisterExternalSymbols(CObjectFile*) const
{
}

unsigned int CCodeGen_Interp::GetAvailableRegisterCount() const
{
	return 0;
}

unsigned int CCodeGen_Interp::GetAvailableMdRegisterCount() const
{
	return 0;
}

bool CCodeGen_Interp::CanHold128BitsReturnValueInRegisters() const
{
	return false;
}

uint32 CCodeGen_Interp::GetPointerSize() const
{
	return sizeof(uintptr_t);
}

uint32 CCodeGen_Interp::EncodeOp(const Jitter::STATEMENT& statement)
{
	assert(statement.op < 0x10000);
	return (statement.op & 0xFFFF) | (statement.jmpCondition << 24);
}

uint32 CCodeGen_Interp::EncodeOperand(const Jitter::SymbolRefPtr& symbolRef)
{
	if(!symbolRef) return 0;
	const auto& symbol = symbolRef->GetSymbol();
	auto type = symbol->m_type;
	uint32 offset = 0;
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		offset = symbol->m_valueLow;
		break;
	case SYM_CONTEXT:
		break;
	case SYM_RELATIVE:
	case SYM_RELATIVE64:
	case SYM_RELATIVE128:
	case SYM_REL_REFERENCE:
		offset = symbol->m_valueLow;
		break;
	case SYM_CONSTANT:
		offset = m_constants.size() * sizeof(uint32);
		m_constants.push_back(symbol->m_valueLow);
		break;
	case SYM_CONSTANT64:
		//Align on 64-bit
		if(m_constants.size() & 1)
		{
			m_constants.push_back(0);
		}
		offset = m_constants.size() * sizeof(uint32);
		m_constants.push_back(symbol->m_valueLow);
		m_constants.push_back(symbol->m_valueHigh);
		break;
	case SYM_CONSTANTPTR:
		offset = m_constants.size() * sizeof(uint32);
		m_constants.push_back(symbol->m_valueLow);
		m_constants.push_back(symbol->m_valueHigh);
		break;
	case SYM_TEMPORARY:
	case SYM_TEMPORARY64:
	case SYM_TMP_REFERENCE:
		offset = symbol->m_stackLocation;
		break;
	default:
		assert(false);
		break;
	}
	return (offset & 0xFFFF) | (type << 16);
}
