#include "stdafx.h"

#include <memory>

#include "Function.h"
#include "NativeJIT/ExecutionBuffer.h"
#include "NativeJIT/FunctionBuffer.h"
#include "SuiteCpp/UnitTest.h"
#include "Temporary/Allocator.h"


namespace NativeJIT
{
    namespace FunctionUnitTest
    {
        TestClass(FunctionTest)
        {
        public:
            FunctionTest()
                : m_allocator(5000),
                  m_executionBuffer(5000)
            {
                m_code.reset(new FunctionBuffer(m_executionBuffer, 5000, 10, 10, 3, 0, false));
            }


            //
            // TODO:
            //   Register spilling
            //   Common sub-expressions
            //   Calling out to C++
            //   Storage class
            //   Array of primitives
            //   Other boolean operators: shift, sub, mul, etc.
            //   Floating point
            //   Signed vs unsigned
            //   Richer integration examples (e.g. JitExample1)
            //


            //
            // Immediate values
            //

            // TODO: cases for (signed, unsigned) x (1, 2, 4, 8), float
            TestCase(ImmediateU64)
            {
                AutoResetAllocator reset(m_allocator);

                {
                    Function<unsigned __int64> expression(m_allocator, *m_code);

                    unsigned __int64 value = 0x1234ull;
                    auto & a = expression.Immediate(value);
                    auto function = expression.Compile(a);

                    auto expected = value;
                    auto observed = function();

                    TestAssert(observed == expected);
                }
            }


            //
            // Functions
            //

            TestCase(FunctionOneParameter)
            {
                AutoResetAllocator reset(m_allocator);

                {
                    Function<__int64, __int64> expression(m_allocator, *m_code);

                    auto & a = expression.GetP1();
                    auto function = expression.Compile(a);

                    __int64 p1 = 1234ll;

                    auto expected = p1;
                    auto observed = function(p1);

                    TestAssert(observed == expected);
                }
            }


            TestCase(FunctionTwoParameters)
            {
                AutoResetAllocator reset(m_allocator);

                {
                    Function<__int64, __int64, __int64> expression(m_allocator, *m_code);

                    auto & a = expression.Add(expression.GetP2(), expression.GetP1());
                    auto function = expression.Compile(a);

                    __int64 p1 = 12340000ll;
                    __int64 p2 = 5678ll;

                    auto expected = p1 + p2;
                    auto observed = function(p1, p2);

                    TestAssert(observed == expected);
                }
            }


            //
            // FieldPointer
            //

            class InnerClass
            {
            public:
                unsigned __int64 m_a;
                unsigned __int64 m_b;
            };


            class OuterClass
            {
            public:
                long long m_p;
                InnerClass* m_innerPointer;
                InnerClass m_innerEmbedded;
                long long m_q;
            };


            TestCase(FieldPointerPrimitive)
            {
                AutoResetAllocator reset(m_allocator);

                {
                    Function<unsigned __int64, InnerClass*> expression(m_allocator, *m_code);

                    auto & a = expression.GetP1();
                    auto & b = expression.FieldPointer(a, &InnerClass::m_b);
                    auto & c = expression.Deref(b);
                    auto function = expression.Compile(c);

                    InnerClass innerClass;
                    innerClass.m_b = 1234ull;
                    InnerClass* p1 = &innerClass;

                    auto expected = p1->m_b;
                    auto observed = function(p1);

                    TestAssert(observed == expected);
                }
            }


            TestCase(FieldPointerEmbedded)
            {
                AutoResetAllocator reset(m_allocator);

                {
                    Function<unsigned __int64, OuterClass*> expression(m_allocator, *m_code);

                    auto & a = expression.GetP1();
                    auto & b = expression.FieldPointer(a, &OuterClass::m_innerEmbedded);
                    auto & c = expression.FieldPointer(b, &InnerClass::m_b);
                    auto & d = expression.Deref(c);
                    auto function = expression.Compile(d);

                    OuterClass outerClass;
                    outerClass.m_innerEmbedded.m_b = 1234ull;
                    OuterClass* p1 = &outerClass;

                    auto expected = p1->m_innerEmbedded.m_b;
                    auto observed = function(p1);

                    TestAssert(observed == expected);
                }
            }


            //
            // Binary operations
            //

            TestCase(AddUnsignedInt32)
            {
                AutoResetAllocator reset(m_allocator);

                {
                    Function<unsigned __int32, unsigned __int32, unsigned __int32> expression(m_allocator, *m_code);

                    auto & a = expression.Add(expression.GetP2(), expression.GetP1());
                    auto function = expression.Compile(a);

                    unsigned __int32 p1 = 12340000ul;
                    unsigned __int32 p2 = 5678ul;

                    auto expected = p1 + p2;
                    auto observed = function(p1, p2);

                    TestAssert(observed == expected);
                }
            }


            //
            // Array indexing
            //

            TestCase(ArrayOfClass)
            {
                AutoResetAllocator reset(m_allocator);

                {
                    Function<__int64, OuterClass*, unsigned __int64> expression(m_allocator, *m_code);

                    auto & a = expression.Add(expression.GetP1(), expression.GetP2());
                    auto & b = expression.FieldPointer(a, &OuterClass::m_q);
                    auto & c = expression.Deref(b);
                    auto function = expression.Compile(c);

                    OuterClass array[10];
                    OuterClass* p1 = array;
                    unsigned __int64 p2 = 3ull;

                    auto expected = p1[p2].m_q;
                    auto observed = function(p1, p2);

                    TestAssert(observed == expected);
                }
            }




            //
            // Conditionals
            //

            TestCase(Conditional)
            {
                AutoResetAllocator reset(m_allocator);

                {
                    Function<unsigned __int64, unsigned __int64, unsigned __int64> expression(m_allocator, *m_code);

                    unsigned __int64 trueValue = 5;
                    unsigned __int64 falseValue = 6;

                    auto & a = expression.GreaterThan(expression.GetP1(), expression.GetP2());
                    auto & b = expression.Conditional(a, expression.Immediate(trueValue), expression.Immediate(falseValue));
                    auto function = expression.Compile(b);

                    unsigned __int64 p1 = 3;
                    unsigned __int64 p2 = 4;

                    auto expected = (p1 > p2) ? trueValue : falseValue;
                    auto observed = function(p1, p2);

                    TestAssert(observed == expected);

                    p1 = 5;
                    p2 = 4;

                    expected = (p1 > p2) ? trueValue : falseValue;
                    observed = function(p1, p2);

                    // TODO: Enable after implementing conditional jumps.
                    // TestAssert(observed == expected);
                }
            }


        private:
            Allocator m_allocator;
            ExecutionBuffer m_executionBuffer;
            std::unique_ptr<FunctionBuffer> m_code;
        };
    }
}
