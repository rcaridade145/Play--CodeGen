#pragma once

#include "Jitter_CodeGen.h"
#include "AArch64Assembler.h"

namespace Jitter
{
	class CCodeGen_AArch64 : public CCodeGen
	{
	public:
		           CCodeGen_AArch64();
		virtual    ~CCodeGen_AArch64();

		void            GenerateCode(const StatementList&, unsigned int) override;
		void            SetStream(Framework::CStream*) override;
		void            RegisterExternalSymbols(CObjectFile*) const override;
		unsigned int    GetAvailableRegisterCount() const override;
		unsigned int    GetAvailableMdRegisterCount() const override;
		bool            CanHold128BitsReturnValueInRegisters() const override;

	private:
		typedef std::map<uint32, CAArch64Assembler::LABEL> LabelMapType;
		typedef void (CCodeGen_AArch64::*ConstCodeEmitterType)(const STATEMENT&);

		enum MAX_TEMP_REGS
		{
			MAX_TEMP_REGS = 7,
		};

		struct CONSTMATCHER
		{
			OPERATION               op;
			MATCHTYPE               dstType;
			MATCHTYPE               src1Type;
			MATCHTYPE               src2Type;
			ConstCodeEmitterType    emitter;
		};

		void    LoadMemoryInRegister(CAArch64Assembler::REGISTER32, CSymbol*);
		void    StoreRegisterInMemory(CSymbol*, CAArch64Assembler::REGISTER32);
		
		void    LoadMemory64InRegister(CAArch64Assembler::REGISTER64, CSymbol*);
		void    StoreRegisterInMemory64(CSymbol*, CAArch64Assembler::REGISTER64);
		
		CAArch64Assembler::REGISTER32    PrepareSymbolRegisterDef(CSymbol*, CAArch64Assembler::REGISTER32);
		CAArch64Assembler::REGISTER32    PrepareSymbolRegisterUse(CSymbol*, CAArch64Assembler::REGISTER32);
		void                             CommitSymbolRegister(CSymbol*, CAArch64Assembler::REGISTER32);
		
		//SHIFTOP ----------------------------------------------------------
		struct SHIFTOP_BASE
		{
			typedef void (CAArch64Assembler::*OpImmType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, uint8);
//			typedef void (CAArch64Assembler::*OpRegType)(CAArch32Assembler::REGISTER, CAArch32Assembler::REGISTER, CAArch32Assembler::REGISTER);
		};
		
		struct SHIFTOP_ASR : public SHIFTOP_BASE
		{
			static OpImmType	OpImm()		{ return &CAArch64Assembler::Asr; }
//			static OpRegType	OpReg()		{ return &CAArch32Assembler::Asr; }
		};
		
		struct SHIFTOP_LSL : public SHIFTOP_BASE
		{
			static OpImmType	OpImm()		{ return &CAArch64Assembler::Lsl; }
//			static OpRegType	OpReg()		{ return &CAArch32Assembler::Lsl; }
		};

		struct SHIFTOP_LSR : public SHIFTOP_BASE
		{
			static OpImmType	OpImm()		{ return &CAArch64Assembler::Lsr; }
//			static OpRegType	OpReg()		{ return &CAArch32Assembler::Lsr; }
		};
		
		//SHIFT64OP ----------------------------------------------------------
		struct SHIFT64OP_BASE
		{
			typedef void (CAArch64Assembler::*OpImmType)(CAArch64Assembler::REGISTER64, CAArch64Assembler::REGISTER64, uint8);
//			typedef void (CAArch64Assembler::*OpRegType)(CAArch32Assembler::REGISTER, CAArch32Assembler::REGISTER, CAArch32Assembler::REGISTER);
		};
		
		struct SHIFT64OP_ASR : public SHIFT64OP_BASE
		{
			static OpImmType	OpImm()		{ return &CAArch64Assembler::Asr; }
//			static OpRegType	OpReg()		{ return &CAArch32Assembler::Asr; }
		};
		
		void    Emit_Prolog(uint32);
		void    Emit_Epilog(uint32);
		
		CAArch64Assembler::LABEL GetLabel(uint32);
		void                     MarkLabel(const STATEMENT&);
		
		void    Emit_Mov_VarVar(const STATEMENT&);
		
		void    Emit_Mov_Mem64Mem64(const STATEMENT&);
		
		//SHIFT
		template <typename> void    Emit_Shift_VarVarCst(const STATEMENT&);

		//SHIFT64
		template <typename> void    Emit_Shift64_MemMemCst(const STATEMENT&);
		
		static CONSTMATCHER    g_constMatchers[];
		static CAArch64Assembler::REGISTER32    g_tempRegs[MAX_TEMP_REGS];
		static CAArch64Assembler::REGISTER64    g_tempRegs64[MAX_TEMP_REGS];
		static CAArch64Assembler::REGISTER64    g_baseRegister;

		Framework::CStream*    m_stream = nullptr;
		CAArch64Assembler      m_assembler;
		LabelMapType           m_labels;
		uint32                 m_stackLevel = 0;
	};
};
