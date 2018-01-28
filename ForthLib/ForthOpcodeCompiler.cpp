//////////////////////////////////////////////////////////////////////
//
// ForthOpcodeCompiler.cpp: implementation of the ForthOpcodeCompiler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if defined(LINUX) || defined(MACOSX)
#include <ctype.h>
#endif
#include "ForthEngine.h"
#include "ForthOpcodeCompiler.h"

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthOpcodeCompiler
// 

ForthOpcodeCompiler::ForthOpcodeCompiler(ForthMemorySection*	pDictionarySection)
: mpDictionarySection( pDictionarySection )
, mbCompileComboOps(true)
{
	for ( unsigned int i = 0; i < MAX_PEEPHOLE_PTRS; ++i )
	{
		mPeephole[i] = NULL;
	}
	Reset();
}

ForthOpcodeCompiler::~ForthOpcodeCompiler()
{
} 

void ForthOpcodeCompiler::Reset()
{
	mPeepholeIndex = 0;
	mPeepholeValidCount = 0;
	mpLastIntoOpcode = NULL;
}

#ifdef ASM_INNER_INTERPRETER
#define NATIVE_OPTYPE kOpNative
#else
#define NATIVE_OPTYPE kOpCCode
#endif

#define FITS_IN_BITS( VAL, NBITS )  ((VAL) < (1 << NBITS))
// VAL must already have high 8 bits zeroed
#define FITS_IN_SIGNED_BITS( VAL, NBITS )  (((VAL) < (1 << (NBITS - 1))) || ((VAL) > (((1 << 24) - (1 << NBITS)) + 1)))

void ForthOpcodeCompiler::CompileOpcode( forthOpType opType, long opVal )
{
    long* pOpcode = mpDictionarySection->pCurrent;
	long op = COMPILED_OP( opType, opVal );
	forthOpType previousType;
	long previousVal;
	
	switch( opType )
	{
	case NATIVE_OPTYPE:
		{
			if ((op == gCompiledOps[OP_INTO]) || (op == gCompiledOps[OP_INTO_PLUS]))
			{
			   // we need this to support initialization of local string vars (ugh)
			   mpLastIntoOpcode = pOpcode;
			}
			else
			{
			}
		}
		break;

	case kOpBranchZ:
		{
			unsigned long uVal = opVal & OPCODE_VALUE_MASK;
			if (mbCompileComboOps && GetPreviousOpcode( previousType, previousVal )
                && (previousType == NATIVE_OPTYPE)
				&& FITS_IN_BITS(previousVal, 12) && FITS_IN_SIGNED_BITS(uVal, 12) )
			{
				// OP ZBRANCH combo - bits 0:11 are opcode, bits 12:23 are signed integer branch offset in longs
				UncompileLastOpcode();
                pOpcode--;
				op = COMPILED_OP(kOpOZBCombo, previousVal | (uVal << 12));
			}
		}
		break;

    case kOpBranchNZ:
    {
        unsigned long uVal = opVal & OPCODE_VALUE_MASK;
        if (mbCompileComboOps && GetPreviousOpcode(previousType, previousVal)
            && (previousType == NATIVE_OPTYPE)
            && FITS_IN_BITS(previousVal, 12) && FITS_IN_SIGNED_BITS(uVal, 12))
        {
            // OP NZBRANCH combo - bits 0:11 are opcode, bits 12:23 are signed integer branch offset in longs
            UncompileLastOpcode();
            pOpcode--;
            op = COMPILED_OP(kOpONZBCombo, previousVal | (uVal << 12));
        }
    }
    break;

	default:
		break;
	}

	mPeepholeIndex = (mPeepholeIndex + 1) & (MAX_PEEPHOLE_PTRS - 1);
	mPeephole[mPeepholeIndex] = pOpcode;
	*pOpcode++ = op;
	mpDictionarySection->pCurrent = pOpcode;
	mPeepholeValidCount++;
}

void ForthOpcodeCompiler::PatchOpcode(forthOpType opType, long opVal, long* pOpcode)
{
    if ((opType == kOpBranchZ) || (opType == kOpBranchNZ))
    {
        long oldOpcode = *pOpcode;
        forthOpType oldOpType = FORTH_OP_TYPE(oldOpcode);
        long oldOpVal = FORTH_OP_VALUE(oldOpcode);
        switch (oldOpType)
        {
        case kOpBranchZ:
        case kOpBranchNZ:
            *pOpcode = COMPILED_OP(opType, opVal);
            break;

        case kOpOZBCombo:
        case kOpONZBCombo:
            // preserve opcode part of old value
            oldOpVal &= 0xFFF;
            if (opType == kOpBranchZ)
            {
                *pOpcode = COMPILED_OP(kOpOZBCombo, oldOpVal | (opVal << 12));
            }
            else
            {
                *pOpcode = COMPILED_OP(kOpONZBCombo, oldOpVal | (opVal << 12));
            }
            break;

        default:
            // TODO - report error
            break;
        }
    }
    else
    {
        // TODO: report error
    }
}

void ForthOpcodeCompiler::UncompileLastOpcode()
{
	if (mPeepholeValidCount > 0)
	{
		if ( mPeephole[mPeepholeIndex] <= mpLastIntoOpcode )
		{
			mpLastIntoOpcode = NULL;

		}
		mpDictionarySection->pCurrent = mPeephole[mPeepholeIndex];
		mPeepholeIndex = (mPeepholeIndex - 1) & (MAX_PEEPHOLE_PTRS - 1);
		mPeepholeValidCount--;
	}
}

unsigned int ForthOpcodeCompiler::PeepholeValidCount()
{
	return (mPeepholeValidCount > MAX_PEEPHOLE_PTRS) ? MAX_PEEPHOLE_PTRS : mPeepholeValidCount;
}

void ForthOpcodeCompiler::ClearPeephole()
{
	Reset();
}

long* ForthOpcodeCompiler::GetLastCompiledOpcodePtr( void )
{
	return (mPeepholeValidCount > 0) ? mPeephole[mPeepholeIndex] : NULL;
}

long* ForthOpcodeCompiler::GetLastCompiledIntoPtr( void )
{
	return mpLastIntoOpcode;
}

bool ForthOpcodeCompiler::GetPreviousOpcode( forthOpType& opType, long& opVal )
{
	if ( mPeepholeValidCount > 0 )
	{
		long op = *(mPeephole[mPeepholeIndex]);

		opType = FORTH_OP_TYPE( op );
		opVal = FORTH_OP_VALUE( op );
		return true;
	}
	return false;
}


