#include "DivTest.h"
#include "MemStream.h"

#define VALUE_REL0 (0xFFFF8000)
#define VALUE_REL1 (0x8000FFFF)
#define VALUE_CST0 (0x80004040)
#define VALUE_CST1 (0x40408000)

CDivTest::CDivTest(bool isSigned)
    : m_isSigned(isSigned)
{
}

void CDivTest::Run()
{
	memset(&m_context, 0, sizeof(m_context));

	m_context.relArg0 = VALUE_REL0;
	m_context.relArg1 = VALUE_REL1;

	m_function(&m_context);

	if(m_isSigned)
	{
		TEST_VERIFY(m_context.cstCstResultLo == static_cast<int32>(VALUE_CST0) / static_cast<int32>(VALUE_CST1));
		TEST_VERIFY(m_context.cstCstResultHi == static_cast<int32>(VALUE_CST0) % static_cast<int32>(VALUE_CST1));

		TEST_VERIFY(m_context.relRelResultLo == static_cast<int32>(VALUE_REL0) / static_cast<int32>(VALUE_REL1));
		TEST_VERIFY(m_context.relRelResultHi == static_cast<int32>(VALUE_REL0) % static_cast<int32>(VALUE_REL1));

		TEST_VERIFY(m_context.relCstResultLo == static_cast<int32>(VALUE_REL0) / static_cast<int32>(VALUE_CST1));
		TEST_VERIFY(m_context.relCstResultHi == static_cast<int32>(VALUE_REL0) % static_cast<int32>(VALUE_CST1));

		TEST_VERIFY(m_context.cstRelResultLo == static_cast<int32>(VALUE_CST0) / static_cast<int32>(VALUE_REL1));
		TEST_VERIFY(m_context.cstRelResultHi == static_cast<int32>(VALUE_CST0) % static_cast<int32>(VALUE_REL1));
	}
	else
	{
		TEST_VERIFY(m_context.cstCstResultLo == static_cast<uint32>(VALUE_CST0) / static_cast<uint32>(VALUE_CST1));
		TEST_VERIFY(m_context.cstCstResultHi == static_cast<uint32>(VALUE_CST0) % static_cast<uint32>(VALUE_CST1));

		TEST_VERIFY(m_context.relRelResultLo == static_cast<uint32>(VALUE_REL0) / static_cast<uint32>(VALUE_REL1));
		TEST_VERIFY(m_context.relRelResultHi == static_cast<uint32>(VALUE_REL0) % static_cast<uint32>(VALUE_REL1));

		TEST_VERIFY(m_context.relCstResultLo == static_cast<uint32>(VALUE_REL0) / static_cast<uint32>(VALUE_CST1));
		TEST_VERIFY(m_context.relCstResultHi == static_cast<uint32>(VALUE_REL0) % static_cast<uint32>(VALUE_CST1));

		TEST_VERIFY(m_context.cstRelResultLo == static_cast<uint32>(VALUE_CST0) / static_cast<uint32>(VALUE_REL1));
		TEST_VERIFY(m_context.cstRelResultHi == static_cast<uint32>(VALUE_CST0) % static_cast<uint32>(VALUE_REL1));
	}
}

void CDivTest::Compile(Jitter::CJitter& jitter)
{
	Framework::CMemStream codeStream;
	jitter.SetStream(&codeStream);

	jitter.Begin();
	{
		//Cst / Cst
		{
			jitter.PushCst(VALUE_CST0);
			jitter.PushCst(VALUE_CST1);

			if(m_isSigned)
			{
				jitter.DivS();
			}
			else
			{
				jitter.Div();
			}

			jitter.PushTop();

			jitter.ExtLow64();
			jitter.PullRel(offsetof(CONTEXT, cstCstResultLo));

			jitter.ExtHigh64();
			jitter.PullRel(offsetof(CONTEXT, cstCstResultHi));
		}

		//Rel / Rel
		{
			jitter.PushRel(offsetof(CONTEXT, relArg0));
			jitter.PushRel(offsetof(CONTEXT, relArg1));

			if(m_isSigned)
			{
				jitter.DivS();
			}
			else
			{
				jitter.Div();
			}

			jitter.PushTop();

			jitter.ExtLow64();
			jitter.PullRel(offsetof(CONTEXT, relRelResultLo));

			jitter.ExtHigh64();
			jitter.PullRel(offsetof(CONTEXT, relRelResultHi));
		}

		//Rel / Cst
		{
			jitter.PushRel(offsetof(CONTEXT, relArg0));
			jitter.PushCst(VALUE_CST1);

			if(m_isSigned)
			{
				jitter.DivS();
			}
			else
			{
				jitter.Div();
			}

			jitter.PushTop();

			jitter.ExtLow64();
			jitter.PullRel(offsetof(CONTEXT, relCstResultLo));

			jitter.ExtHigh64();
			jitter.PullRel(offsetof(CONTEXT, relCstResultHi));
		}

		//Cst / Rel
		{
			jitter.PushCst(VALUE_CST0);
			jitter.PushRel(offsetof(CONTEXT, relArg1));

			if(m_isSigned)
			{
				jitter.DivS();
			}
			else
			{
				jitter.Div();
			}

			jitter.PushTop();

			jitter.ExtLow64();
			jitter.PullRel(offsetof(CONTEXT, cstRelResultLo));

			jitter.ExtHigh64();
			jitter.PullRel(offsetof(CONTEXT, cstRelResultHi));
		}
	}
	jitter.End();

	m_function = FunctionType(codeStream.GetBuffer(), codeStream.GetSize());
}
