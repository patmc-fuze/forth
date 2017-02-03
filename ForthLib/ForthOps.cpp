//////////////////////////////////////////////////////////////////////
//
// ForthOps.cpp: forth builtin operator definitions
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"


#ifdef ARM9
#include <nds.h>
#endif

#include <ctype.h>
#include <time.h>
#if defined(WIN32)
#include <sys/timeb.h>
#endif
#include "Forth.h"
#include "ForthEngine.h"
#include "ForthThread.h"
#include "ForthVocabulary.h"
#include "ForthShell.h"
#include "ForthInput.h"
#include "ForthStructs.h"
#include "ForthServer.h"
#include "ForthClient.h"
#include "ForthMessages.h"
#include "ForthPortability.h"
#include "ForthObject.h"
#include "ForthBlockFileManager.h"

#ifdef LINUX
#include <strings.h>
#include <errno.h>

extern int kbhit(void);
extern int getch(void);
#endif

long gCompiledOps[NUM_COMPILED_OPS];

extern "C"
{


#include <math.h>
extern float sinf(float a);
extern float cosf(float a);
extern float tanf(float a);
extern float asinf(float a);
extern float acosf(float a);
extern float atanf(float a);
extern float ceilf(float a);
extern float floorf(float a);
extern float sqrtf(float a);
extern float expf(float a);
extern float logf(float a);
extern float log10f(float a);

// compiled token is 32-bits,
// top 8 bits are opcode type (opType)
// bottom 24 bits are immediate operand

extern void intVarAction( ForthCoreState* pCore, int* pVar );
extern GFORTHOP( byteVarActionBop );
extern GFORTHOP( ubyteVarActionBop );
extern GFORTHOP( shortVarActionBop );
extern GFORTHOP( ushortVarActionBop );
extern GFORTHOP( intVarActionBop );
extern GFORTHOP( longVarActionBop );
extern GFORTHOP( floatVarActionBop );
extern GFORTHOP( doubleVarActionBop );
extern GFORTHOP( stringVarActionBop );
extern GFORTHOP( opVarActionBop );
extern GFORTHOP( objectVarActionBop );

#if defined(WIN32) && !defined(MINGW)
// this madness is here to fix an unexplained link error - single precision float math functions are undefined
//  even though they are declared extern in InnerInterp.asm
float maximumBogosity()
{
	float aa;
	int bb;
	return asinf(sinf(0.5f)) + acosf(cosf(0.5f)) + atanf(tanf(0.5f))
		+ atan2f(0.5f, 0.7f) + powf(2.0f, 4.0f) + ldexpf(2.0f, 5) + frexpf(2.0f, &bb)
		+ ceilf(modff(0.7f, &aa)) + floorf(fmodf(1.0f, 2.0f)) + log10f(sqrtf(expf(logf(2.0f))));
}
#endif

//############################################################################
//
//   normal forth ops
//
//############################################################################

// bye is a user command which causes the entire forth program to exit
FORTHOP( byeOp )
{
    SET_STATE( kResultExitShell );
}

// badOp is used to detect executing uninitialized op variables
FORTHOP( badOpOp )
{
    SET_ERROR( kForthErrorBadOpcode );
}

// unimplementedMethodOp is used to detect executing unimplemented methods
FORTHOP( unimplementedMethodOp )
{
    SET_ERROR( kForthErrorUnimplementedMethod );
}

// unimplementedMethodOp is used to detect executing unimplemented methods
FORTHOP( illegalMethodOp )
{
    SET_ERROR( kForthErrorUnimplementedMethod );
}

FORTHOP( argvOp )
{
    NEEDS( 1 );
    SPUSH( (long) (GET_ENGINE->GetShell()->GetArg( SPOP )) );
}

FORTHOP( argcOp )
{
    SPUSH( GET_ENGINE->GetShell()->GetArgCount() );
}

//##############################
//
// math ops
//

FORTHOP( ldivideOp )
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    stackInt64 quotient;
    LPOP( b );
    LPOP( a );
    quotient.s64 = a.s64 / b.s64;
    LPUSH( quotient );
}

FORTHOP( lmodOp )
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    stackInt64 remainder;
    LPOP( b );
    LPOP( a );
    remainder.s64 = a.s64 % b.s64;
    LPUSH( remainder );
}

FORTHOP( ldivmodOp )
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    stackInt64 quotient;
    stackInt64 remainder;
    LPOP( b );
    LPOP( a );
    remainder.s64 = a.s64 % b.s64;
    LPUSH( remainder );
    quotient.s64 = a.s64 / b.s64;
    LPUSH( quotient );
}

FORTHOP( udivmodOp )
{
    NEEDS(3);
    stackInt64 a;
    stackInt64 quotient;
    unsigned long b = (unsigned long) SPOP;
    LPOP( a );
    quotient.s64 = a.u64 / b;
	unsigned long remainder = a.u64 % b;
    SPUSH( ((long) remainder) );
    LPUSH( quotient );
}

//##############################
//
// 64-bit integer comparison ops
//

FORTHOP(lEqualsOp)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    SPUSH( ( a.s64 == b.s64 ) ? -1L : 0 );
}

FORTHOP(lNotEqualsOp)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    SPUSH( ( a.s64 != b.s64 ) ? -1L : 0 );
}

FORTHOP(lGreaterThanOp)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    SPUSH( ( a.s64 > b.s64 ) ? -1L : 0 );
}

FORTHOP(lGreaterEqualsOp)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    SPUSH( ( a.s64 >= b.s64 ) ? -1L : 0 );
}

FORTHOP(lLessThanOp)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    SPUSH( ( a.s64 < b.s64 ) ? -1L : 0 );
}

FORTHOP(lLessEqualsOp)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    SPUSH( ( a.s64 <= b.s64 ) ? -1L : 0 );
}

FORTHOP(lEquals0Op)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 == 0L ) ? -1L : 0 );
}

FORTHOP(lNotEquals0Op)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 != 0L ) ? -1L : 0 );
}

FORTHOP(lGreaterThan0Op)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 > 0L ) ? -1L : 0 );
}

FORTHOP(lGreaterEquals0Op)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 >= 0L ) ? -1L : 0 );
}

FORTHOP(lLessThan0Op)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 < 0L ) ? -1L : 0 );
}

FORTHOP(lLessEquals0Op)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 <= 0L ) ? -1L : 0 );
}

FORTHOP(lWithinOp)
{
    NEEDS(3);
    stackInt64 hiLimit;
    stackInt64 loLimit;
    stackInt64 val;
    LPOP( hiLimit );
    LPOP( loLimit );
    LPOP( val );
    SPUSH( ( (loLimit.s64 <= val.s64) && (val.s64 < hiLimit.s64) ) ? -1L : 0 );
}

FORTHOP(lMinOp)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    LPUSH( (( a.s64 < b.s64 ) ? a : b) );
}

FORTHOP(lMaxOp)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    LPUSH( (( a.s64 > b.s64 ) ? a : b) );
}



FORTHOP( i2lOp )
{
    NEEDS(1);
    int a = SPOP;
    stackInt64 b;
    b.s64 = (long long) a;
    LPUSH( b );
}

FORTHOP( l2fOp )
{
    NEEDS(1);
    stackInt64 a;
    LPOP( a );
    float b = (float) a.s64;
    FPUSH( b );
}

FORTHOP( l2dOp )
{
    NEEDS(1);
    stackInt64 a;
    LPOP( a );
    double b = (double) a.s64;
    DPUSH( b );
}

FORTHOP( f2lOp )
{
    NEEDS(1);
    float a = FPOP;
    stackInt64 b;
    b.s64 = (long long) a;
    LPUSH( b );
}

FORTHOP( d2lOp )
{
    NEEDS(2);
    double a = DPOP;
    stackInt64 b;
    b.s64 = (long long) a;
    LPUSH( b );
}

//##############################
//
// control ops
//
// TODO: replace branch, tbranch, fbranch with immediate ops

// has precedence!
FORTHOP(doOp)
{
    NEEDS(2);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShellStack *pShellStack = pEngine->GetShell()->GetShellStack();
    pShellStack->Push( gCompiledOps[ OP_DO_DO ] );
    // save address for loop/+loop
    pShellStack->Push( (long)GET_DP );
    pShellStack->Push( kShellTagDo );
    // this will be fixed by loop/+loop
    pEngine->CompileBuiltinOpcode( OP_ABORT );
    pEngine->CompileLong( 0 );
}

// has precedence!
FORTHOP(checkDoOp)
{
    NEEDS(2);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShellStack *pShellStack = pEngine->GetShell()->GetShellStack();
    pShellStack->Push( gCompiledOps[ OP_DO_CHECKDO ] );
    // save address for loop/+loop
    pShellStack->Push( (long)GET_DP );
    pShellStack->Push( kShellTagDo );
    // this will be fixed by loop/+loop
    pEngine->CompileBuiltinOpcode( OP_ABORT );
    pEngine->CompileLong( 0 );
}

// has precedence!
FORTHOP(loopOp)
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "loop", pShellStack->Pop(), kShellTagDo ) )
    {
        return;
    }
    long *pDoOp = (long *) pShellStack->Pop();
	long doOpcode = pShellStack->Pop();
    *pDoOp++ = doOpcode;
    // compile the "_loop" opcode
    pEngine->CompileBuiltinOpcode( OP_DO_LOOP );
    // fill in the branch to after loop opcode
    *pDoOp = COMPILED_OP( kOpBranch, (GET_DP - pDoOp) - 1 );
}

// has precedence!
FORTHOP(loopNOp)
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "loop+", pShellStack->Pop(), kShellTagDo ) )
    {
        return;
    }
    long *pDoOp = (long *) pShellStack->Pop();
	long doOpcode = pShellStack->Pop();
    *pDoOp++ = doOpcode;
    // compile the "_loop" opcode
    pEngine->CompileBuiltinOpcode( OP_DO_LOOPN );
    // fill in the branch to after loop opcode
    *pDoOp = COMPILED_OP( kOpBranch, (GET_DP - pDoOp) - 1 );
}

// if - has precedence
FORTHOP( ifOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    // save address for else/endif
    pShellStack->Push( (long)GET_DP );
    // flag that this is the "if" branch
    pShellStack->Push( kShellTagBranchZ );
    // this will be fixed by else/endif
    pEngine->CompileBuiltinOpcode( OP_ABORT );
}


// else - has precedence
FORTHOP( elseOp )
{
    NEEDS(2);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "else", pShellStack->Pop(), kShellTagBranchZ ) )
    {
        return;
    }
    long *pBranch = (long *) pShellStack->Pop();
    // save address for endif
    pShellStack->Push( (long) GET_DP );
    // flag that this is the "else" branch
    pShellStack->Push( kShellTagBranch );
    // this will be fixed by endif
    pEngine->CompileBuiltinOpcode( OP_ABORT );
    // fill in the branch taken when "if" arg is false
    *pBranch = COMPILED_OP( kOpBranchZ, (GET_DP - pBranch) - 1 );
}


// endif - has precedence
FORTHOP( endifOp )
{
    NEEDS(2);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    long branchTag = pShellStack->Pop();
    if ( pShell->CheckSyntaxError( "endif", branchTag, kShellTagBranchZ ) )
    {
        // if branch
        // fill in the branch taken when "if" arg is false
        long *pBranch = (long *) pShellStack->Pop();
        *pBranch = COMPILED_OP( (branchTag == kShellTagBranchZ) ? kOpBranchZ : kOpBranch, (GET_DP - pBranch) - 1 );
    }
}



// begin - has precedence
FORTHOP( beginOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    // save address for repeat/until/again
    pShellStack->Push( (long)GET_DP );
    pShellStack->Push( kShellTagBegin );
}


// until - has precedence
FORTHOP( untilOp )
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( pShell->CheckSyntaxError( "until", pShellStack->Pop(), kShellTagBegin ) )
    {
        long *pBeginOp = (long *) pShellStack->Pop();
        pEngine->CompileOpcode( kOpBranchZ, (pBeginOp - GET_DP) - 1 );
    }
}



// while - has precedence
FORTHOP( whileOp )
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "while", pShellStack->Pop(), kShellTagBegin ) )
    {
        return;
    }
    // stick while tag/address under the begin tag/address
    long oldAddress = pShellStack->Pop();
    pShellStack->Push( (long) GET_DP );
    pShellStack->Push( kShellTagBranchZ );
    pShellStack->Push( oldAddress );
    pShellStack->Push( kShellTagBegin );
    // repeat will fill this in
    pEngine->CompileBuiltinOpcode( OP_ABORT );
}


// repeat - has precedence
FORTHOP( repeatOp )
{
    NEEDS(2);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "repeat", pShellStack->Pop(), kShellTagBegin ) )
    {
        return;
    }
    long *pBeginAddress =  (long *) pShellStack->Pop();
    long branchTag = pShellStack->Pop();
    if ( !pShell->CheckSyntaxError( "repeat", branchTag, kShellTagBranchZ ) )
    {
        return;
    }
    // fill in the branch taken when "while" fails
    long *pBranch =  (long *) pShellStack->Pop();
    *pBranch = COMPILED_OP( (branchTag == kShellTagBranchZ) ? kOpBranchZ : kOpBranch, (GET_DP - pBranch) );
    pEngine->CompileOpcode( kOpBranch, (pBeginAddress - GET_DP) - 1 );
}

// again - has precedence
FORTHOP( againOp )
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "again", pShellStack->Pop(), kShellTagBegin ) )
    {
        return;
    }
    long *pLoopTop =  (long *) pShellStack->Pop();
    pEngine->CompileOpcode( kOpBranch, (pLoopTop - GET_DP) - 1 );
}

// case - has precedence
FORTHOP( caseOp )
{
    // leave marker for end of list of case-exit branches for endcase
   ForthEngine *pEngine = GET_ENGINE;
   ForthShell *pShell = pEngine->GetShell();
   ForthShellStack *pShellStack = pShell->GetShellStack();
   pShellStack->Push( 0 );
   pShellStack->Push( kShellTagCase );
}


// of - has precedence
FORTHOP( ofOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "of", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }

    // save address for endof
    pShellStack->Push( (long)GET_DP );
    pShellStack->Push( kShellTagOf );
    pShellStack->Push( kShellTagCase );
    // this will be set to a caseBranch by endof
    pEngine->CompileBuiltinOpcode( OP_ABORT );
}


// ofif - has precedence
FORTHOP( ofifOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "ofif", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }

    // save address for endof
    pShellStack->Push( (long)GET_DP );
    pShellStack->Push( kShellTagOfIf );
    pShellStack->Push( kShellTagCase );
    // this will be set to a zBranch by endof
    pEngine->CompileBuiltinOpcode( OP_ABORT );
	// if the ofif test succeeded, we need to dispose of the switch input value
    pEngine->CompileBuiltinOpcode( OP_DROP );
}


// endof - has precedence
FORTHOP( endofOp )
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    long *pDP = GET_DP;

    // this will be fixed by endcase
    pEngine->CompileBuiltinOpcode( OP_ABORT );

    if ( !pShell->CheckSyntaxError( "endof", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }
    long tag = pShellStack->Pop();
    long *pOp = (long *) pShellStack->Pop();
    // fill in the branch taken when case doesn't match
	if ( tag == kShellTagOfIf )
	{
        *pOp = COMPILED_OP( kOpBranchZ, (GET_DP - pOp) - 1 );
	}
	else if ( pShell->CheckSyntaxError( "endof", tag, kShellTagOf ) )
	{
	    *pOp = COMPILED_OP( kOpCaseBranch, (GET_DP - pOp) - 1 );
	}
	else
	{
		return;
	}

    // save address for endcase
    pShellStack->Push( (long) pDP );
    pShellStack->Push( kShellTagCase );
}


// endcase - has precedence
FORTHOP( endcaseOp )
{
    long *pSP = GET_SP;
    long *pEndofOp;

    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "endcase", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }

    if ( ((GET_DP) - (long *)(pShellStack->Peek())) == 1 )
    {
        // there is no default case, we must compile a "drop" to
        //   dispose of the case selector on TOS
        pEngine->CompileBuiltinOpcode( OP_DROP );
    }
    // patch branches from end-of-case to common exit point
    while ( (pEndofOp = (long *) (pShellStack->Pop())) != NULL )
    {
        *pEndofOp = COMPILED_OP( kOpBranch, (GET_DP - pEndofOp) - 1 );
    }
    SET_SP( pSP );
}

// align (upwards) DP to longword boundary
FORTHOP( alignOp )
{
    GET_ENGINE->AlignDP();
}

FORTHOP( allotOp )
{
    GET_ENGINE->AllotBytes( SPOP );
}

FORTHOP( commaOp )
{
    GET_ENGINE->CompileLong( SPOP );
}

FORTHOP( cCommaOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pChar = (char *)GET_DP;
    *pChar++ = (char) SPOP;
    pEngine->SetDP( (long *) pChar);
}

FORTHOP( unusedOp )
{
    ForthMemorySection* pDictionarySection = pCore->pDictionary;
    SPUSH( (4 * (pDictionarySection->len - (pDictionarySection->pCurrent - pDictionarySection->pBase))) );
}

FORTHOP( here0Op )
{
    SPUSH( (long)(pCore->pDictionary->pBase) );
}

FORTHOP( mallocOp )
{
    NEEDS(1);
    long numBytes = SPOP;
	void* pMemory = __MALLOC(numBytes);
    SPUSH( (long) pMemory  );
}

FORTHOP( reallocOp )
{
    NEEDS(2);
    size_t newSize = (size_t)(SPOP);
    void* allocPtr = (void *)(SPOP);
    SPUSH(  (long) __REALLOC( allocPtr, newSize )  );
}

FORTHOP( freeOp )
{
    NEEDS(1);
    __FREE( (void *) SPOP );
}

FORTHOP(initStringArrayOp)
{
	// TOS: maximum length, number of elements, ptr to first char of first element
	long len, nLongs;
	long* pStr;
	int i, numElements;

	len = SPOP;
	numElements = SPOP;
	pStr = ((long *)(SPOP)) - 2;
	nLongs = (len >> 2) + 3;

	for (i = 0; i < numElements; i++)
	{
		*pStr = len;
		pStr[1] = 0;
		*((char *)(pStr + 2)) = 0;
		pStr += nLongs;
	}
}

FORTHOP(initStructArrayOp)
{
	// TOS: struct index, number of elements, ptr to first struct
	ForthEngine *pEngine = GET_ENGINE;
	ForthTypesManager* pManager = ForthTypesManager::GetInstance();

	long typeIndex = SPOP;
	long numElements = SPOP;
	char* pStruct = (char *)(SPOP);
	ForthTypeInfo* typeInfo = pManager->GetTypeInfo(typeIndex);
	if (typeInfo != nullptr)
	{
		ForthStructVocabulary* pVocab = typeInfo->pVocab;
		if (pVocab != nullptr)
		{
			if (pVocab->IsStruct())
			{
				long initStructOp = pVocab->GetInitOpcode();
				if (initStructOp != 0)
				{
					long len = pVocab->GetSize();

					for (int i = 0; i < numElements; i++)
					{
						SPUSH((long)pStruct);
						pEngine->ExecuteOneOp(initStructOp);
						pStruct += len;
					}
				}
			}
			else
			{
				// TODO report that vocab isn't a struct
			}
		}
		else
		{
			// TODO report struct has no vocabulary
		}
	}
	else
	{
		// TODO report unknown type for typeIndex
	}
}

//##############################
//
//  forth defining ops
//

static long    *gpSavedDP;

// builds
// - does not have precedence
// - is executed while executing the defining word
// - begins definition of new symbol
//
FORTHOP(buildsOp)
{
    ForthEngine *pEngine = GET_ENGINE;
	long *pEntry;

    // get next symbol, add it to vocabulary with type "builds/does"
    pEngine->AlignDP();
    pEngine->AddUserOp( pEngine->GetNextSimpleToken(), &pEntry, true );
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    // remember current DP (for does)
    gpSavedDP = GET_DP;
    // compile dummy word at DP, will be filled in by does
    pEngine->CompileLong( 0 );
}

// does
// - has precedence
// - is executed while compiling the defining word
// - it completes the definition of the builds part of the word, and starts
//   the definition of the does part of the word
//
FORTHOP( doesOp )
{
    long newUserOp;
    ForthEngine *pEngine = GET_ENGINE;

    if ( pEngine->IsCompiling() )
    {
        // compile dodoes opcode & dummy word
        pEngine->CompileBuiltinOpcode( OP_END_BUILDS );
        pEngine->CompileLong( 0 );
        // create a nameless vocabulary entry for does-body opcode
        newUserOp = pEngine->AddOp( GET_DP );
        newUserOp = COMPILED_OP( kOpUserDef, newUserOp );
        pEngine->CompileBuiltinOpcode( OP_DO_DOES );
        // stuff does-body opcode in dummy word
        GET_DP[-2] = newUserOp;
        // compile local vars allocation op (if needed)
        pEngine->EndOpDefinition();
    }
    else
    {
        // this is a one-off does body which will only be used by
        //  the most recently created op
        newUserOp = pEngine->AddOp( GET_DP );
        *gpSavedDP = COMPILED_OP( kOpUserDef, newUserOp );
        pEngine->CompileBuiltinOpcode( OP_DO_DOES );
        // switch to compile mode
        pEngine->SetCompileState( 1 );
    }
}

// endBuilds
// - does not have precedence
// - is executed while executing the defining word
// - it completes the definition of the new symbol created by builds by compiling
//   the opcode (defined by does) into the newly defined word
//
FORTHOP( endBuildsOp )
{
    // finish current symbol definition (of op defined by builds)
    GET_ENGINE->GetDefinitionVocabulary()->UnSmudgeNewestSymbol();
    
    // fetch opcode at pIP, compile it into dummy word remembered by builds
    *gpSavedDP = *GET_IP;
    // we are done defining, bail out
    SET_IP( (long *) (RPOP) );
}

// exit has precedence
FORTHOP( exitOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // compile exitOp
    long flags = pEngine->GetFlags();

	bool isMethodDef = ((flags & kEngineFlagIsMethod) != 0);
	if ( pEngine->HasLocalVariables() )
	{
		pEngine->CompileBuiltinOpcode( isMethodDef ? OP_DO_EXIT_ML : OP_DO_EXIT_L );
	}
	else
	{
		pEngine->CompileBuiltinOpcode( isMethodDef ? OP_DO_EXIT_M : OP_DO_EXIT );
	}
}

// semi has precedence
FORTHOP( semiOp )
{
    ForthEngine *pEngine = GET_ENGINE;

    exitOp( pCore );
    // switch back from compile mode to execute mode
    pEngine->SetCompileState( 0 );
    // finish current symbol definition
    // compile local vars allocation op (if needed)
	pEngine->EndOpDefinition( !pEngine->CheckFlag( kEngineFlagNoNameDefinition ) );
    pEngine->ClearFlag( kEngineFlagNoNameDefinition );

	pEngine->GetShell()->CheckDefinitionEnd(":", "coln");
}

FORTHOP( colonOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
	const char* pName = pEngine->GetNextSimpleToken();

	long* pEntry = pEngine->StartOpDefinition(pName, true);
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    // switch to compile mode
    pEngine->SetCompileState( 1 );
    pEngine->ClearFlag( kEngineFlagNoNameDefinition);

	pEngine->GetShell()->StartDefinition(pName, "coln");
}


FORTHOP( colonNoNameOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
	long newDef = pEngine->AddOp( GET_DP );
	newDef = COMPILED_OP(kOpUserDef, newDef);
	SPUSH(newDef);
    // switch to compile mode
    pEngine->SetCompileState( 1 );	
    pEngine->SetFlag( kEngineFlagNoNameDefinition );

	pEngine->GetShell()->StartDefinition("", "coln");
}


FORTHOP(pushTokenOp)
{
	char* pName = (char*)(SPOP);
	GET_ENGINE->GetTokenStack()->Push(pName);
}


FORTHOP(popTokenOp)
{
	SPUSH((long)(GET_ENGINE->GetTokenStack()->Pop()));
}


// func: has precedence
FORTHOP( funcOp )
{
    ForthEngine *pEngine = GET_ENGINE;

	// if compiling, push DP and compile kOpPushBranch
	// if interpreting, push DP and enter compile mode

	pEngine->GetLocalVocabulary()->Push();

    // switch to compile mode
    pEngine->SetCompileState( 1 );
	// TODO: push hasLocalVars flag?
    //pEngine->ClearFlag( kEngineFlagNoNameDefinition);

	pEngine->GetShell()->StartDefinition("", "func");
}

// ;func has precedence
FORTHOP( endfuncOp )
{
    ForthEngine *pEngine = GET_ENGINE;

    exitOp( pCore );
    // switch back from compile mode to execute mode
    pEngine->SetCompileState( 0 );
    // finish current symbol definition
    // compile local vars allocation op (if needed)
	pEngine->EndOpDefinition( !pEngine->CheckFlag( kEngineFlagNoNameDefinition ) );
    pEngine->ClearFlag( kEngineFlagNoNameDefinition );

	pEngine->GetShell()->CheckDefinitionEnd("func", "func");
}

FORTHOP( codeOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    long* pEntry = pEngine->StartOpDefinition( NULL, false );
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    long newestOp = *pEntry;
    *pEntry = COMPILED_OP( kOpNative, FORTH_OP_VALUE( newestOp ) );
}

FORTHOP( createOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    long* pEntry = pEngine->StartOpDefinition( NULL, false );
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    // remember current DP (for does)
    gpSavedDP = GET_DP;
    pEngine->CompileBuiltinOpcode( OP_DO_VAR );
}

FORTHOP( forthVocabOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->GetForthVocabulary()->DoOp( pCore );
}

FORTHOP( definitionsOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
	if ( GET_VAR_OPERATION != kVarDefaultOp )
	{
		pEngine->GetDefinitionVocabulary()->DoOp( pCore );
	}
	else
	{
		pEngine->SetDefinitionVocabulary( pVocabStack->GetTop() );
	}
	CLEAR_VAR_OPERATION;
}

FORTHOP( alsoOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    pVocabStack->DupTop();
}

FORTHOP( previousOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    if ( !pVocabStack->DropTop() )
    {
        CONSOLE_STRING_OUT( "Attempt to drop last item on vocabulary stack ignored.\n" );
    }
}

FORTHOP( onlyOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    pVocabStack->Clear();
}

FORTHOP( vocabularyOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabulary *pDefinitionsVocab = pEngine->GetDefinitionVocabulary();
    // get next symbol, add it to vocabulary with type "user op"
    char* pVocabName = pEngine->GetNextSimpleToken();
    long* pEntry = pEngine->StartOpDefinition( pVocabName );
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileBuiltinOpcode( OP_DO_VOCAB );
    ForthVocabulary* pVocab = new ForthVocabulary( pVocabName,
                                                   NUM_FORTH_VOCAB_VALUE_LONGS,
                                                   512,
                                                   GET_DP,
                                                   ForthVocabulary::GetEntryValue( pDefinitionsVocab->GetNewestEntry() ) );
    pEngine->CompileLong( (long) pVocab );
}

FORTHOP( strForgetOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    const char* pSym = (const char *)(SPOP);
	bool verbose = (GET_VAR_OPERATION != kVarDefaultOp);
    bool forgotIt = pEngine->ForgetSymbol( pSym, !verbose );
    // reset search & definitions vocabs in case we deleted a vocab we were using
    pEngine->SetDefinitionVocabulary( pEngine->GetForthVocabulary() );
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    pVocabStack->Clear();
    SPUSH( forgotIt ? -1 : 0 );
}

#define SCREEN_COLUMNS 120

// return 'q' IFF user quit out
static char
ShowVocab( ForthCoreState   *pCore,
           ForthVocabulary  *pVocab,
           bool             quick )
{
    int i;
    char retVal = 0;
    ForthShell *pShell = GET_ENGINE->GetShell();
    int nEntries = pVocab->GetNumEntries();
    long *pEntry = pVocab->GetFirstEntry();
#define ENTRY_COLUMNS 16
    char spaces[ENTRY_COLUMNS + 1];
    int column = 0;
#define BUFF_SIZE 256
    char buff[BUFF_SIZE];

    memset( spaces, ' ', ENTRY_COLUMNS );
    spaces[ENTRY_COLUMNS] = '\0';

    for ( i = 0; i < nEntries; i++ )
    {
        bool lastEntry = (i == (nEntries-1));
        if ( quick )
        {
            int nameLength = pVocab->GetEntryNameLength( pEntry );
            int numSpaces = ENTRY_COLUMNS - (nameLength % ENTRY_COLUMNS);
            if ( (column + nameLength) > SCREEN_COLUMNS )
            {
                CONSOLE_CHAR_OUT( '\n' );
                column = 0;
            }
            pVocab->GetEntryName( pEntry, buff, sizeof(buff) );
            CONSOLE_STRING_OUT( buff );
            column += (nameLength + numSpaces);
            if ( column >= SCREEN_COLUMNS )
            {
                CONSOLE_CHAR_OUT( '\n' );
                column = 0;
            }
            else
            {
                CONSOLE_STRING_OUT( spaces + (ENTRY_COLUMNS - numSpaces) );
            }
            if ( lastEntry )
            {
                CONSOLE_CHAR_OUT( '\n' );
            }
            else
            {
                pEntry = pVocab->NextEntry( pEntry );
            }
        }
        else
        {
            pVocab->PrintEntry( pEntry );
            CONSOLE_CHAR_OUT( '\n' );
            pEntry = pVocab->NextEntry( pEntry );
            if ( ((i % 22) == 21) || lastEntry )
            {
                if ( (pShell != NULL) && pShell->GetInput()->InputStream()->IsInteractive() )
                {
                    CONSOLE_STRING_OUT( "\nHit ENTER to continue, 'q' & ENTER to quit, 'n' & ENTER to do next vocabulary\n" );
                    retVal = tolower( pShell->GetChar() );
                    if ( retVal == 'q' )
                    {
                        pShell->GetChar();
                        break;
                    }
                    else if ( retVal == 'n' )
                    {
                        pShell->GetChar();
                        break;
                    }
                }
            }
        }
    }

    return retVal;
}


FORTHOP( vlistOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    bool quit = false;
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    ForthVocabulary* pVocab;
	bool verbose = (GET_VAR_OPERATION != kVarDefaultOp);
	pEngine->ShowSearchInfo();
	int depth = 0;
	while (!quit)
    {
        pVocab = pVocabStack->GetElement( depth );
        if ( pVocab == NULL )
        {
            return;
        }
        CONSOLE_STRING_OUT( pVocab->GetName() );
        CONSOLE_STRING_OUT( " vocabulary:\n" );
        quit = ( ShowVocab( pCore, pVocab, !verbose ) == 'q' );
        depth++;
    }
	CLEAR_VAR_OPERATION;
}

FORTHOP( verboseBop )
{
    SET_VAR_OPERATION( kNumVarops );
}

FORTHOP( strFindOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabulary* pFoundVocab;

    char *pSymbol = (char *) (SPOP);
    long* pEntry = pEngine->GetVocabularyStack()->FindSymbol( pSymbol, &pFoundVocab );
    SPUSH( ((long) pEntry) );
}


FORTHOP( variableOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    long* pEntry = pEngine->StartOpDefinition();
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileBuiltinOpcode( OP_DO_VAR );
    pEngine->CompileLong( 0 );
}

FORTHOP( constantOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    long* pEntry = pEngine->StartOpDefinition();
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileBuiltinOpcode( OP_DO_CONSTANT );
    pEngine->CompileLong( SPOP );
}

FORTHOP( dconstantOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    long* pEntry = pEngine->StartOpDefinition();
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileBuiltinOpcode( OP_DO_DCONSTANT );
    double d = DPOP;
    pEngine->CompileDouble( d );
}

FORTHOP( byteOp )
{
    char val = 0;
	gNativeTypeByte.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( ubyteOp )
{
    char val = 0;
	gNativeTypeUByte.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( shortOp )
{
    short val = 0;
	gNativeTypeShort.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( ushortOp )
{
    short val = 0;
	gNativeTypeUShort.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( intOp )
{
    int val = 0;
	gNativeTypeInt.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( uintOp )
{
    int val = 0;
	gNativeTypeUInt.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( longOp )
{
    long long val = 0;
	gNativeTypeLong.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( ulongOp )
{
    long long val = 0;
	gNativeTypeULong.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( floatOp )
{
    float val = 0.0;
	gNativeTypeFloat.DefineInstance( GET_ENGINE, &val );
}


FORTHOP( doubleOp )
{
    double val = 0.0;
	gNativeTypeDouble.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( stringOp )
{
    gNativeTypeString.DefineInstance( GET_ENGINE, NULL );
}

FORTHOP( opOp )
{
    int val = gCompiledOps[ OP_BAD_OP] ;
	gNativeTypeOp.DefineInstance( GET_ENGINE, &val );
}

//FORTHOP( objectOp )
//{
//    long val[2] = { 0, 0  };
//	gNativeTypeObject.DefineInstance( GET_ENGINE, val );
//}

FORTHOP( voidOp )
{
}

FORTHOP( arrayOfOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    long numElements;

    if ( pEngine->IsCompiling() )
    {
        // the symbol just before "arrayOf" should have been an integer constant
        if ( pEngine->GetLastConstant( numElements ) )
        {
            // uncompile the integer contant opcode
            pEngine->UncompileLastOpcode();
            // save #elements for var declaration ops
            pEngine->SetArraySize( numElements );
        }
        else
        {
            SET_ERROR( kForthErrorMissingSize );
        }
    }
    else
    {
        // save #elements for var declaration ops
        numElements = SPOP;
        pEngine->SetArraySize( numElements );
    }
}

FORTHOP( ptrToOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->SetFlag( kEngineFlagIsPointer );
}

FORTHOP( structOp )
{
    ForthEngine* pEngine = GET_ENGINE;
    pEngine->SetFlag( kEngineFlagInStructDefinition );
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
	const char* pName = pEngine->GetNextSimpleToken();
    ForthStructVocabulary* pVocab = pManager->StartStructDefinition(pName);
    pEngine->CompileBuiltinOpcode( OP_DO_STRUCT_TYPE );
    pEngine->CompileLong( (long) pVocab );

	pEngine->GetShell()->StartDefinition(pName, "stru");
}

FORTHOP( endstructOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->ClearFlag( kEngineFlagInStructDefinition );
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    pManager->EndStructDefinition();
	pEngine->GetShell()->CheckDefinitionEnd("struct", "stru");
}

FORTHOP( classOp )
{
    ForthEngine* pEngine = GET_ENGINE;
	const char* pName = pEngine->GetNextSimpleToken();
	pEngine->StartClassDefinition(pName);

	pEngine->GetShell()->StartDefinition(pName, "clas");
}

FORTHOP( endclassOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->EndClassDefinition();
	pEngine->GetShell()->CheckDefinitionEnd("class", "clas");
}

FORTHOP( methodOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    const char* pMethodName = pEngine->GetNextSimpleToken();
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthClassVocabulary* pVocab = pManager->GetNewestClass();

    long methodIndex = pVocab->FindMethod( pMethodName );
    pEngine->StartOpDefinition( pMethodName, true );
    // switch to compile mode
    pEngine->SetCompileState( 1 );
    pEngine->SetFlag( kEngineFlagIsMethod );
    if ( pVocab )
    {
        long* pEntry = pVocab->GetNewestEntry();
        if ( pEntry )
        {
            methodIndex = pVocab->AddMethod( pMethodName, methodIndex, pEntry[0] );
            pEntry[0] = methodIndex;
            pEntry[1] |= kDTIsMethod;
        }
        else
        {
            // TODO: error
        }
    }
    else
    {
        // TODO: report adding a method outside a class definition
    }

	pEngine->GetShell()->StartDefinition(pMethodName, "meth");
}

FORTHOP( endmethodOp )
{
    // TODO
    ForthEngine *pEngine = GET_ENGINE;

    exitOp( pCore );
    // switch back from compile mode to execute mode
    pEngine->SetCompileState( 0 );
    // finish current symbol definition
    // compile local vars allocation op (if needed)
    pEngine->EndOpDefinition( true );
    pEngine->ClearFlag( kEngineFlagIsMethod );

	pEngine->GetShell()->CheckDefinitionEnd("method", "meth");
}

FORTHOP( returnsOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    if ( pEngine->CheckFlag( kEngineFlagIsMethod ) )
    {
        char *pToken = pEngine->GetNextSimpleToken();
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        ForthClassVocabulary* pVocab = pManager->GetNewestClass();
        long* pEntry = pVocab->GetNewestEntry();
        if ( strcmp( pToken, "ptrTo" ) == 0 )
        {
            pToken = pEngine->GetNextSimpleToken();
            pEntry[1] |= kDTIsPtr;
        }
        forthBaseType baseType = pManager->GetBaseTypeFromName( pToken );
        if ( baseType != kBaseTypeUnknown )
        {
            pEntry[1] |= NATIVE_TYPE_TO_CODE( 0, baseType );
        }
        else
        {
            // see if it is a struct type
            ForthStructVocabulary* pTypeVocab = pManager->GetStructVocabulary( pToken );
            if ( pTypeVocab )
            {
                if ( pTypeVocab->IsClass() )
                {
                    pEntry[1] |= OBJECT_TYPE_TO_CODE( 0, pTypeVocab->GetTypeIndex() );
                }
                else
                {
                    pEntry[1] |= STRUCT_TYPE_TO_CODE( 0, pTypeVocab->GetTypeIndex() );
                }
            }
            else
            {
                pEngine->AddErrorText( pToken );
                pEngine->SetError( kForthErrorUnknownSymbol, " is not a known return type" );
            }
        }
    }
    else
    {
        // report error - "returns" outside of method
        pEngine->SetError( kForthErrorBadSyntax, "returns can only be used inside a method" );
    }
}

FORTHOP( doMethodOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    long methodIndex = SPOP;
    RPUSH( ((long) GET_TPD) );
    RPUSH( ((long) GET_TPM) );
    long* pMethods = ((long *) (SPOP));
    SET_TPM( pMethods );
    SET_TPD( ((long *) (SPOP)) );
    pEngine->ExecuteOneOp( pMethods[ methodIndex ] );
}

FORTHOP( implementsOp )
{
    ForthEngine *pEngine = GET_ENGINE;

    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthClassVocabulary* pVocab = pManager->GetNewestClass();
    if ( pVocab && pEngine->CheckFlag( kEngineFlagInClassDefinition ) )
    {
        pVocab->Implements( pEngine->GetNextSimpleToken() );
    }
    else
    {
        pEngine->SetError( kForthErrorBadSyntax, "implements: outside of class definition" );
    }
}

FORTHOP( endImplementsOp )
{
    ForthEngine *pEngine = GET_ENGINE;

    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthClassVocabulary* pVocab = pManager->GetNewestClass();
    if ( pVocab && pEngine->CheckFlag( kEngineFlagInClassDefinition ) )
    {
        pVocab->EndImplements();
    }
    else
    {
        pEngine->SetError( kForthErrorBadSyntax, ";implements outside of class definition" );
    }
}

FORTHOP( unionOp )
{
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    pManager->GetNewestStruct()->StartUnion();
}

FORTHOP( extendsOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pSym = pEngine->GetNextSimpleToken();
    ForthVocabulary* pFoundVocab;
    long *pEntry = pEngine->GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );
    if ( pEntry )
    {
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        ForthStructVocabulary* pParentVocab = pManager->GetStructVocabulary( pEntry[0] );
        if ( pParentVocab )
        {
			if ( pEngine->CheckFlag( kEngineFlagInClassDefinition ) )
			{
				if ( pParentVocab->IsClass() )
				{
					ForthClassVocabulary* pParentClassVocab = reinterpret_cast<ForthClassVocabulary *>(pParentVocab);
					pManager->GetNewestClass()->Extends( pParentClassVocab );
				}
				else
				{
					pEngine->SetError( kForthErrorBadSyntax, "extend class from struct is illegal" );
				}
			}
			else
			{
				if ( pEngine->CheckFlag( kEngineFlagInStructDefinition ) )
				{
					if ( pParentVocab->IsClass() )
					{
						pEngine->SetError( kForthErrorBadSyntax, "extend struct from class is illegal" );
					}
					else
					{
						pManager->GetNewestStruct()->Extends( pParentVocab );
					}
				}
				else
				{
					pEngine->SetError( kForthErrorBadSyntax, "extends must be used in a class or struct definition" );
				}
			}
        }
        else
        {
            pEngine->SetError( kForthErrorUnknownSymbol, pSym );
            pEngine->AddErrorText( " is not a structure" );
        }
    }
    else
    {
        pEngine->SetError( kForthErrorUnknownSymbol, pSym );
    }
}

FORTHOP( strSizeOfOp )
{
    // TODO: allow sizeOf to be applied to variables
    // TODO: allow sizeOf to apply to native types, including strings
    ForthEngine *pEngine = GET_ENGINE;
	char *pSym = (char *)(SPOP);
    ForthVocabulary* pFoundVocab;
    long *pEntry = pEngine->GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );
	long size = 0;

    if ( pEntry )
    {
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        ForthStructVocabulary* pStructVocab = pManager->GetStructVocabulary( pEntry[0] );
        if ( pStructVocab == NULL)
        {
			size = pStructVocab->GetSize();
        }
        else
        {
            size = pManager->GetBaseTypeSizeFromName( pSym );
            if ( size <= 0 )
            {
                pEngine->AddErrorText( pSym );
                pEngine->SetError( kForthErrorUnknownSymbol, " is not a structure or base type" );
            }
        }
    }
    else
    {
        pEngine->SetError( kForthErrorUnknownSymbol, pSym );
    }
	SPUSH(size);
}

FORTHOP( strOffsetOfOp )
{
    // TODO: allow offsetOf to be variable.field instead of just type.field
    ForthEngine *pEngine = GET_ENGINE;
	long size = 0;

	char *pType = (char *)(SPOP);
    char *pField = strchr( pType, '.' );
    if ( pField == NULL )
    {
        pEngine->SetError( kForthErrorBadSyntax, "argument must contain a period" );
    }
	else
	{
		char oldChar = *pField;
		*pField++ = '\0';

		ForthVocabulary* pFoundVocab;
		long *pEntry = pEngine->GetVocabularyStack()->FindSymbol(pType, &pFoundVocab);
		if (pEntry)
		{
			ForthTypesManager* pManager = ForthTypesManager::GetInstance();
			ForthStructVocabulary* pStructVocab = pManager->GetStructVocabulary(pEntry[0]);
			if (pStructVocab)
			{
				pEntry = pStructVocab->FindSymbol(pField);
				if (pEntry)
				{
					size = pEntry[0];
				}
				else
				{
					pEngine->AddErrorText(pField);
					pEngine->AddErrorText(" is not a field in ");
					pEngine->SetError(kForthErrorUnknownSymbol, pType);
				}
			}
			else
			{
				pEngine->AddErrorText(pType);
				pEngine->SetError(kForthErrorUnknownSymbol, " is not a structure");
			}
		}
		else
		{
			pEngine->SetError(kForthErrorUnknownSymbol, pType);
		}
		pField[-1] = oldChar;
	}

	SPUSH(size);
}

FORTHOP( processConstantOp )
{
	// if interpreting, do nothing
	// if compiling, get value from TOS and compile what is needed to push that value at runtime
    ForthEngine *pEngine = GET_ENGINE;
	if (pEngine->IsCompiling())
	{
		int constantValue = (SPOP);
		pEngine->ProcessConstant(constantValue);
	}
}

FORTHOP( processLongConstantOp )
{
	// if interpreting, do nothing
	// if compiling, get value from TOS and compile what is needed to push that value at runtime
    ForthEngine *pEngine = GET_ENGINE;
	if (pEngine->IsCompiling())
	{
		stackInt64 val;
		LPOP(val);
		pEngine->ProcessLongConstant(val.s64);
	}
}

FORTHOP(showStructOp)
{
	// TOS: structVocabPtr ptrToStructData
	ForthStructVocabulary *pStructVocab = (ForthStructVocabulary *)(SPOP);
	void* pStruct = (void *)(SPOP);
	pStructVocab->ShowData(pStruct, pCore);
}

FORTHOP( superOp )
{
	// push version of this which has current objects data pointer and super class method pointer
    SPUSH( ((long) GET_TPD) );
	long* pMethods = GET_TPM;
	// the long before method 0 holds the class object pointer
	ForthClassObject* pClassObject = (ForthClassObject*) pMethods[-1];
	// TODO: some error checking here might be nice
	pMethods = pClassObject->pVocab->ParentClass()->GetInterface(0)->GetMethods();
    SPUSH( ((long) pMethods) );
}

void __newOp(ForthCoreState* pCore, const char* pSym)
{
    // TODO: allow sizeOf to be applied to variables
    // TODO: allow sizeOf to apply to native types, including strings
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabulary* pFoundVocab;
    long *pEntry = pEngine->GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );

    if ( pEntry )
    {
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        ForthClassVocabulary* pClassVocab = (ForthClassVocabulary *) (pManager->GetStructVocabulary( pEntry[0] ));

        if ( pClassVocab && pClassVocab->IsClass() )
        {
			long initOpcode = pClassVocab->GetInitOpcode();
			if (pEngine->IsCompiling())
            {
				pEngine->ProcessConstant(pClassVocab->GetTypeIndex(), false);
                pEngine->CompileBuiltinOpcode( OP_DO_NEW );
				if (initOpcode != 0)
				{
					pEngine->CompileBuiltinOpcode(OP_OVER);
					pEngine->CompileOpcode(initOpcode);
				}
			}
            else
            {
                SPUSH( (long) pClassVocab );
                pEngine->ExecuteOneOp( pClassVocab->GetClassObject()->newOp );
				if (initOpcode != 0)
				{
					// copy object data pointer to TOS to be used by init 
					long a = (GET_SP)[1];
					SPUSH(a);
					pEngine->ExecuteOneOp(initOpcode);
				}
			}
        }
        else
        {
            pEngine->AddErrorText( pSym );
            pEngine->SetError( kForthErrorUnknownSymbol, " is not a class" );
        }
    }
    else
    {
        pEngine->SetError( kForthErrorUnknownSymbol, pSym );
    }
}

FORTHOP(newOp)
{
	ForthEngine *pEngine = GET_ENGINE;

	char *pSym = pEngine->GetNextSimpleToken();
	__newOp(pCore, pSym);
}

FORTHOP(strNewOp)
{
	ForthEngine *pEngine = GET_ENGINE;
	ForthVocabulary* pFoundVocab;

	char *pSym = (char*)(SPOP);
	long *pEntry = pEngine->GetVocabularyStack()->FindSymbol(pSym, &pFoundVocab);

	if (pEntry)
	{
		ForthTypesManager* pManager = ForthTypesManager::GetInstance();
		ForthClassVocabulary* pClassVocab = (ForthClassVocabulary *)(pManager->GetStructVocabulary(pEntry[0]));

		if (pClassVocab && pClassVocab->IsClass())
		{
			long initOpcode = pClassVocab->GetInitOpcode();
			SPUSH((long)pClassVocab);
			pEngine->ExecuteOneOp(pClassVocab->GetClassObject()->newOp);
			if (initOpcode != 0)
			{
				// copy object data pointer to TOS to be used by init 
				long a = (GET_SP)[1];
				SPUSH(a);
				pEngine->ExecuteOneOp(initOpcode);
			}
		}
		else
		{
			pEngine->AddErrorText(pSym);
			pEngine->SetError(kForthErrorUnknownSymbol, " is not a class");
		}
	}
	else
	{
		pEngine->SetError(kForthErrorUnknownSymbol, pSym);
	}
}

FORTHOP(makeObjectOp)
{
    ForthEngine *pEngine = GET_ENGINE;
	const char *pClassName = pEngine->GetNextSimpleToken();

    __newOp(pCore, pClassName);

    ForthVocabulary* pFoundVocab;
    long *pEntry = pEngine->GetVocabularyStack()->FindSymbol(pClassName, &pFoundVocab);

    if (pEntry)
    {
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        ForthClassVocabulary* pClassVocab = (ForthClassVocabulary *)(pManager->GetStructVocabulary(pEntry[0]));

        if (pClassVocab && pClassVocab->IsClass())
        {
            if (pEngine->IsCompiling())
            {
                pEngine->CompileBuiltinOpcode(OP_INTO);
            }
            else
            {
                SET_VAR_OPERATION(kVarStore);
            }
			const char *pInstanceName = pEngine->GetNextSimpleToken();
			pClassVocab->DefineInstance(pInstanceName);
        }
        else
        {
            pEngine->AddErrorText(pClassName);
            pEngine->SetError(kForthErrorUnknownSymbol, " is not a class");
        }
    }
    else
    {
        pEngine->SetError(kForthErrorUnknownSymbol, pClassName);
    }

}

FORTHOP(doNewOp)
{
	// this op is compiled for 'new foo', the class typeIndex is on TOS
	ForthTypesManager* pManager = ForthTypesManager::GetInstance();
	ForthEngine *pEngine = GET_ENGINE;

	int typeIndex = SPOP;
	ForthTypeInfo* pTypeInfo = pManager->GetTypeInfo(typeIndex);
	if (pTypeInfo != nullptr)
	{
		if (pTypeInfo->pVocab->IsClass())
		{
			ForthClassVocabulary *pClassVocab = static_cast<ForthClassVocabulary *>(pTypeInfo->pVocab);
			SPUSH((long)pClassVocab);
			pEngine->ExecuteOneOp(pClassVocab->GetClassObject()->newOp);
		}
		else
		{
			// TODO: report that vocab is not a class vocab
		}
	}
	else
	{
		// TODO report unknown class with index N
	}
}

FORTHOP( allocObjectOp )
{
	// allocObject is the default new op, it is used only for classes which don't define their
	//   own 'new' op, or extend a class that does
    ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
    ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
    if ( pPrimaryInterface )
    {
        long nBytes = pClassVocab->GetSize();
        void* pData = __MALLOC( nBytes );
		memset( pData, 0, nBytes );
        SPUSH( (long) pData );
        SPUSH( (long) (pPrimaryInterface->GetMethods()) );
    }
    else
    {
        ForthEngine *pEngine = GET_ENGINE;
        pEngine->AddErrorText( pClassVocab->GetName() );
        pEngine->SetError( kForthErrorBadParameter, " failure in new - has no primary interface" );
    }
}

FORTHOP( initMemberStringOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pString = pEngine->GetNextSimpleToken();
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthClassVocabulary* pVocab = pManager->GetNewestClass();
    long* pEntry;

    if ( !pEngine->CheckFlag( kEngineFlagIsMethod ) || (pVocab == NULL) )
    {
        pEngine->SetError( kForthErrorBadSyntax, "initMemberString can only be used inside a method" );
        return;
    }
    pEntry = pVocab->FindSymbol( pString );
    if ( !pEntry )
    {
        pEngine->AddErrorText( pString );
        pEngine->SetError( kForthErrorUnknownSymbol, " is not a field in this class" );
        return;
    }

    long typeCode = pEntry[1];
    if ( !CODE_IS_SIMPLE(typeCode) || !CODE_IS_NATIVE(typeCode) || (CODE_TO_BASE_TYPE(typeCode) != kBaseTypeString) )
    {
        pEngine->SetError( kForthErrorBadSyntax, "initMemberString can only be used on a simple string" );
        return;
    }
	// pEntry[2] is the total string struct length in bytes, subtract 9 for maxLen, curLen and terminating null
    long len = pEntry[2] - 9;
	if ( len > 4095 )
    {
        pEngine->SetError( kForthErrorBadParameter, "initMemberString can not be used on string with size > 4095" );
        return;
    }
	if ( pEntry[0] > 4095 )
    {
        pEngine->SetError( kForthErrorBadParameter, "initMemberString can not be used on string with member offset > 4095" );
        return;
    }

	// only shift up by 10 bits since pEntry[0] is member byte offset, opcode holds member offset in longs
    long varOffset = (pEntry[0] << 10) | len;

    pEngine->CompileOpcode( kOpMemberStringInit, varOffset );
}

FORTHOP( enumOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    //if ( pEngine->CheckFlag( kEngineFlagInStructDefinition ) )
    //{
    //    pEngine->SetError( kForthErrorBadSyntax, "enum definition not allowed inside struct definition" );
    //    return;
    //}
    pEngine->StartEnumDefinition();
	const char* pName = pEngine->GetNextSimpleToken();
	long* pEntry = pEngine->StartOpDefinition(pName);
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileBuiltinOpcode( OP_DO_ENUM );
	pEngine->CompileLong(4);	// default enum size is 4 bytes

	pEngine->GetShell()->StartDefinition(pName, "enum");
}

FORTHOP( endenumOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->EndEnumDefinition();
	pEngine->GetShell()->CheckDefinitionEnd("enum", "enum");
}

FORTHOP( doVocabOp )
{
    // IP points to data field
    ForthVocabulary* pVocab = (ForthVocabulary *) (*GET_IP);
    pVocab->DoOp( pCore );
    SET_IP( (long *) (RPOP) );
}

FORTHOP(getClassByIndexOp)
{
	// IP points to data field
	int typeIndex = SPOP;
	ForthObject classObject;
	classObject.pMethodOps = nullptr;
	classObject.pData = nullptr;
	ForthTypesManager* pManager = ForthTypesManager::GetInstance();
	ForthClassVocabulary* pClassVocabulary = pManager->GetClassVocabulary(typeIndex);
	if (pClassVocabulary != nullptr)
	{
		ForthClassObject* pClassObject = pClassVocabulary->GetClassObject();
		if (pClassObject != nullptr)
		{
			classObject.pMethodOps = pManager->GetClassVocabulary(kBCIClass)->GetInterface(0)->GetMethods();
			classObject.pData = reinterpret_cast<long *>(pClassObject);
		}
	}
	PUSH_OBJECT(classObject);
}


// doStructTypeOp is compiled at the start of each user-defined structure defining word 
FORTHOP( doStructTypeOp )
{
    // IP points to data field
    ForthStructVocabulary *pVocab = (ForthStructVocabulary *) (*GET_IP);

	ForthEngine *pEngine = GET_ENGINE;
	if (pEngine->IsCompiling())
	{
		// handle the case 'ref STRUCT_TYPE'
		long* pLastOp = pEngine->GetLastCompiledOpcodePtr();
		if ((pLastOp != NULL) && (*pLastOp == gCompiledOps[OP_REF]))
		{
			// compile this opcode so at runtime (ref STRUCT_OP) will push struct vocab address
			ForthTypesManager* pManager = ForthTypesManager::GetInstance();
			pEngine->CompileOpcode(pManager->GetTypeInfo(pVocab->GetTypeIndex())->op);
			SET_IP((long *)(RPOP));
			return;
		}
	}
	if (GET_VAR_OPERATION == kVarDefaultOp)
	{
		pVocab->DefineInstance();
	}
	else
	{
		pVocab->DoOp(pCore);
	}
	SET_IP((long *)(RPOP));
}

// doClassTypeOp is compiled at the start of each user-defined class defining word 
FORTHOP( doClassTypeOp )
{
    // IP points to data field
	// TODO: change this to take class typeIndex on TOS like doNewOp
    ForthClassVocabulary *pVocab = (ForthClassVocabulary *) (*GET_IP);
	pVocab->DefineInstance();
    SET_IP( (long *) (RPOP) );
}

// has precedence!
// this is compiled as the only op for each enum set defining word
// an enum set defining word acts just like the "int" op - it defines
// a variable or field that is 4 bytes in size
FORTHOP( doEnumOp )
{
	switch (*GET_IP)
	{
	case 1:
		byteOp(pCore);
		break;

	case 2:
		shortOp(pCore);
		break;

	case 4:
		intOp(pCore);
		break;

	case 8:
		longOp(pCore);
		break;

	default:
		{

			SET_ERROR(kForthErrorBadParameter);
			ForthEngine *pEngine = GET_ENGINE;
			pEngine->SetError(kForthErrorBadParameter, "enum size must be 1, 2, 4 or 8");
		}
	}
    // we need to pop the IP, since there is no instruction after this one
    SET_IP( (long *) (RPOP) );
}

// in traditional FORTH, the vocabulary entry for the operation currently
// being defined is "smudged" so that if a symbol appears in its own definition
// it will normally only match any previously existing symbol of the same name.
// This makes it easy to redefine existing symbols in terms of their old
// definitions.
// The "recursive" op just unsmudges the vocabulary entry currently being
// defined so that a definition can refer to itself.

// has precedence!
FORTHOP( recursiveOp )
{
    GET_ENGINE->GetDefinitionVocabulary()->UnSmudgeNewestSymbol();
}

FORTHOP( precedenceOp )
{
	ForthEngine *pEngine = GET_ENGINE;
	char *pSym = pEngine->GetNextSimpleToken();
	long *pEntry = pEngine->GetDefinitionVocabulary()->FindSymbol(pSym);
    
    if ( pEntry )
    {
        long op = *pEntry;
        ulong opVal = FORTH_OP_VALUE( op );
        switch ( FORTH_OP_TYPE( op ) )
        {
            case kOpNative:
                *pEntry = COMPILED_OP( kOpNativeImmediate, opVal );
                break;

            case kOpUserDef:
                *pEntry = COMPILED_OP( kOpUserDefImmediate, opVal );
                break;

            case kOpCCode:
                *pEntry = COMPILED_OP( kOpCCodeImmediate, opVal );
                break;

            default:
                CONSOLE_STRING_OUT( "!!!! Can\'t set precedence for " );
                CONSOLE_STRING_OUT( pSym );
                CONSOLE_STRING_OUT( "s - wrong type !!!!\n" );
                SPEW_ENGINE( "!!!! Can\'t set precedence for %s - wrong type !!!!\n", pSym );
                break;
        }
    }
    else
    {
        CONSOLE_STRING_OUT( "!!!! Failure finding symbol " );
        CONSOLE_STRING_OUT( pSym );
        SPEW_ENGINE( "!!!! Failure finding symbol %s !!!!\n", pSym );
    }
}

FORTHOP(strLoadOp)
{
	char *pFileName = ((char *)(SPOP));

	if (pFileName != NULL)
	{
		ForthEngine *pEngine = GET_ENGINE;
		if (pEngine->PushInputFile(pFileName) == false)
		{
			CONSOLE_STRING_OUT("!!!! Failure opening source file ");
			CONSOLE_STRING_OUT(pFileName);
			CONSOLE_STRING_OUT(" !!!!\n");
			SPEW_ENGINE("!!!! Failure opening source file %s !!!!\n", pFileName);
		}
	}
}

FORTHOP(strRunFileOp)
{
	char *pFileName = ((char *)(SPOP));

	if (pFileName != NULL)
	{
		ForthEngine *pEngine = GET_ENGINE;
		ForthShell* pShell = pEngine->GetShell();
		FILE *pInFile = pShell->OpenForthFile(pFileName);
		if (pInFile != NULL)
		{
			ForthFileInputStream* pInputStream = new ForthFileInputStream(pInFile, pFileName);
			pShell->RunOneStream(pInputStream);
		}
		else
		{
			CONSOLE_STRING_OUT("!!!! Failure opening source file ");
			CONSOLE_STRING_OUT(pFileName);
			CONSOLE_STRING_OUT(" !!!!\n");
			SPEW_ENGINE("!!!! Failure opening source file %s !!!!\n", pFileName);
		}
	}
}

FORTHOP(loadDoneOp)
{
    GET_ENGINE->PopInputStream();
}

FORTHOP( requiresOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pSymbolName = pEngine->GetNextSimpleToken();
	ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();

	if ( pVocabStack->FindSymbol( pSymbolName ) == NULL )
    {
        // symbol not found - load symbol.txt
		int buffLen = strlen( pSymbolName ) + 8;
        char *pFileName = (char *) __MALLOC( buffLen );
		SNPRINTF( pFileName, buffLen, "%s.txt", pSymbolName );
        if ( pEngine->PushInputFile( pFileName ) == false )
        {
            CONSOLE_STRING_OUT( "!!!! Failure opening source file " );
            CONSOLE_STRING_OUT( pFileName );
            CONSOLE_STRING_OUT( " !!!!\n" );
            SPEW_ENGINE( "!!!! Failure opening source file %s !!!!\n", pFileName );
        }
        __FREE( pFileName );
    }
}

FORTHOP( evaluateOp )
{
    int len = SPOP;
    const char* pStr = (const char *) SPOP;
    if ( (pStr != NULL) && (len > 0) )
    {
        ForthEngine *pEngine = GET_ENGINE;
        pEngine->PushInputBuffer( pStr, len );
		ForthShell* pShell = pEngine->GetShell();
		pShell->ProcessLine( pStr );
		pEngine->PopInputStream();
    }
}

FORTHOP( strEvaluateOp )
{
    const char* pStr = (const char *) SPOP;
    if ( pStr != NULL )
    {
        int len = strlen( pStr );
        ForthEngine *pEngine = GET_ENGINE;
		ForthShell* pShell = pEngine->GetShell();
		ForthBufferInputStream* pInputStream = new ForthBufferInputStream(pStr, len, false, len + 4);
		pShell->RunOneStream(pInputStream);
    }
}

FORTHOP( stateInterpretOp )
{
    GET_ENGINE->SetCompileState( 0 );
}

FORTHOP( stateCompileOp )
{
    GET_ENGINE->SetCompileState( 1 );
}

FORTHOP( stateOp )
{
    SPUSH( (long) GET_ENGINE->GetCompileStatePtr() );
}

FORTHOP( strTickOp )
{
    ForthEngine *pEngine = GET_ENGINE;
	char* pToken = (char *) SPOP;
    ForthVocabulary* pFoundVocab;
    long* pSymbol = pEngine->GetVocabularyStack()->FindSymbol( pToken, &pFoundVocab );
    if ( pSymbol != NULL )
    {
        SPUSH( *pSymbol );
    }
    else
    {
        SET_ERROR( kForthErrorUnknownSymbol );
        pEngine->AddErrorText( pToken );
    }
}

FORTHOP( compileOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    long* pIP = GET_IP;
    long op = *pIP++;
    SET_IP( pIP );
    pEngine->CompileOpcode( op );
}

// has precedence!
FORTHOP( postponeOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabulary* pFoundVocab;

    char *pToken = pEngine->GetNextSimpleToken();
    long* pEntry = pEngine->GetVocabularyStack()->FindSymbol( pToken, &pFoundVocab );
    if ( pEntry != NULL )
    {
        long op = *pEntry;
        int opType = FORTH_OP_TYPE( op );
        switch ( opType )
        {
        case kOpNativeImmediate:
        case kOpUserDefImmediate:
        case kOpCCodeImmediate:
            pEngine->CompileOpcode( op );
            break;

        default:
            // op without precedence
            pEngine->CompileBuiltinOpcode( OP_COMPILE );
            pEngine->CompileOpcode( op );
            break;
        }
    }
    else
    {
        SET_ERROR( kForthErrorUnknownSymbol );
    }
}

// has precedence!
FORTHOP( bracketTickOp )
{
    // TODO: what should this do if state is interpret? an error? or act the same as tick?
    ForthEngine *pEngine = GET_ENGINE;
    char *pToken = pEngine->GetNextSimpleToken();
    ForthVocabulary* pFoundVocab;
    long* pSymbol = pEngine->GetVocabularyStack()->FindSymbol( pToken, &pFoundVocab );
    if ( pSymbol != NULL )
    {
        pEngine->CompileBuiltinOpcode( OP_INT_VAL );
        pEngine->CompileLong( *pSymbol );
    }
    else
    {
        SET_ERROR( kForthErrorUnknownSymbol );
    }
}

FORTHOP( bodyOp )
{
    ForthEngine *pEngine = GET_ENGINE;

    long op = SPOP;
    unsigned long opIndex = FORTH_OP_VALUE( op );
    long opType = FORTH_OP_TYPE( op );
    switch ( opType )
    {
    case kOpNativeImmediate:
    case kOpUserDefImmediate:
    case kOpNative:
    case kOpUserDef:
        if ( opIndex < GET_NUM_OPS )
        {
            SPUSH( ((long)(OP_TABLE[ opIndex ])) + 4 );
        }
        else
        {
            SET_ERROR( kForthErrorBadOpcode );
        }
        break;

    default:
        SET_ERROR( kForthErrorBadOpcodeType );
        break;
    }
}

//##############################
//
//  output ops
//
void
consoleOutToFile( ForthCoreState   *pCore,
                  const char       *pMessage )
{    
    FILE *pOutFile = stdout;//static_cast<FILE*>(GET_CON_OUT_DATA);
    if ( pOutFile != NULL )
    {
        fprintf(pOutFile, "%s", pMessage );
    }
    else
    {
        // TODO: report error
    }
}

/*
void
consoleOutToString( ForthCoreState   *pCore,
                    const char       *pMessage )
{    
    char *pOutStr = static_cast<char*>(GET_CON_OUT_DATA);
    if ( pOutStr != NULL )
    {
        strcat( pOutStr, pMessage );
    }
    else
    {
        // TODO: report error
    }
}

void
consoleOutToOp( ForthCoreState   *pCore,
                const char       *pMessage )
{
    SPUSH( (long) pMessage );
    long op = GET_CON_OUT_OP;
    ForthEngine* pEngine = GET_ENGINE;
    pEngine->ExecuteOneOp( op );
}
*/
static void
printNumInCurrentBase( ForthCoreState   *pCore,
                       long             val )
{
    NEEDS(1);
#define PRINT_NUM_BUFF_CHARS 68
    char buff[ PRINT_NUM_BUFF_CHARS ];
    char *pNext = &buff[ PRINT_NUM_BUFF_CHARS ];
    ldiv_t v;
    long base;
    bool bIsNegative, bPrintUnsigned;
    ulong urem;
    ePrintSignedMode signMode;

    if ( val == 0 )
    {
        strcpy( buff, "0 " );
        pNext = buff;
    }
    else
    {
        base = *(GET_BASE_REF);

        signMode = (ePrintSignedMode) GET_PRINT_SIGNED_NUM_MODE;
        if ( (base == 10) && (signMode == kPrintSignedDecimal) )
        {

            // most common case - print signed decimal
            SNPRINTF( buff, sizeof(buff), "%d ", val );
            pNext = buff;

        }
        else
        {

            // unsigned or any base other than 10

            *--pNext = 0;
            *--pNext = ' ';
            bPrintUnsigned = !(signMode == kPrintAllSigned);
            if ( bPrintUnsigned )
            {
                // since div is defined as signed divide/mod, make sure
                //   that the number is not negative by generating the bottom digit
                bIsNegative = false;
                urem = ((ulong) val) % ((ulong) base);
                *--pNext = (char) ( (urem < 10) ? (urem + '0') : ((urem - 10) + 'a') );
                val = ((ulong) val) / ((ulong) base);
            }
            else
            {
                bIsNegative = ( val < 0 );
                if ( bIsNegative )
                {
                    val = (-val);
                }
            }
            while ( val != 0 )
            {
#if defined(WIN32)
#if defined(MSDEV)
                v = div( val, base );
#else
                v = ldiv( val, base );
#endif
#elif defined(LINUX)
                v = ldiv( val, base );
#endif
                *--pNext = (char) ( (v.rem < 10) ? (v.rem + '0') : ((v.rem - 10) + 'a') );
                val = v.quot;
            }
            if ( bIsNegative )
            {
                *--pNext = '-';
            }
        }
    }
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", pNext );
#endif

    CONSOLE_STRING_OUT( pNext );
}


static void
printLongNumInCurrentBase( ForthCoreState   *pCore,
						   long long        val )
{
    NEEDS(1);
#define PRINT_NUM_BUFF_CHARS 68
    char buff[ PRINT_NUM_BUFF_CHARS ];
    char *pNext = &buff[ PRINT_NUM_BUFF_CHARS ];
    ulong base;
    bool bIsNegative, bPrintUnsigned;
    ulong urem;
	unsigned long long uval;
    ePrintSignedMode signMode;

    if ( val == 0L )
    {
        strcpy( buff, "0 " );
        pNext = buff;
    }
    else
    {
        base = (ulong) *(GET_BASE_REF);

        signMode = (ePrintSignedMode) GET_PRINT_SIGNED_NUM_MODE;
        if ( base == 10 ) 
        {

            // most common case - print signed decimal
			switch ( signMode )
			{
			case kPrintAllUnsigned:
	            SNPRINTF( buff, sizeof(buff), "%ulld ", val );
				break;
			default:
	            SNPRINTF( buff, sizeof(buff), "%lld ", val );
				break;
			}
            pNext = buff;

        }
        else
        {

            // unsigned or any base other than 10

            *--pNext = 0;
            *--pNext = ' ';
            bPrintUnsigned = !(signMode == kPrintAllSigned);
            if ( bPrintUnsigned )
            {
                // since div is defined as signed divide/mod, make sure
                //   that the number is not negative by generating the bottom digit
                bIsNegative = false;
				uval = (unsigned long long) val;
                urem = uval % base;
                *--pNext = (char) ( (urem < 10) ? (urem + '0') : ((urem - 10) + 'a') );
                uval = uval / base;
            }
            else
            {
                bIsNegative = ( val < 0 );
                if ( bIsNegative )
                {
                    uval = (-val);
                }
				else
				{
					uval = val;
				}
            }
            while ( uval != 0 )
            {
                urem = uval % base;
                *--pNext = (char) ( (urem < 10) ? (urem + '0') : ((urem - 10) + 'a') );
                uval = uval / base;
            }
            if ( bIsNegative )
            {
                *--pNext = '-';
            }
        }
    }
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", pNext );
#endif

    CONSOLE_STRING_OUT( pNext );
}


FORTHOP( printNumOp )
{
    printNumInCurrentBase( pCore, SPOP );
}

FORTHOP( printLongNumOp )
{
    stackInt64 val;
    LPOP( val );
    printLongNumInCurrentBase( pCore, val.s64 );
}

FORTHOP( printNumDecimalOp )
{
    NEEDS(1);
    char buff[36];

    long val = SPOP;
    SNPRINTF( buff, sizeof(buff), "%d", val );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printNumHexOp )
{
    NEEDS(1);
    char buff[12];

    long val = SPOP;
    SNPRINTF( buff, sizeof(buff), "%x", val );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printLongDecimalOp )
{
    NEEDS(2);
    char buff[40];

    stackInt64 val;
    LPOP( val );
    SNPRINTF( buff, sizeof(buff), "%I64d", val.s64 );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printLongHexOp )
{
    NEEDS(1);
    char buff[20];

    stackInt64 val;
    LPOP( val );
    SNPRINTF( buff, sizeof(buff), "%I64x", val.s64 );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printFloatOp )
{
    NEEDS(1);
    char buff[36];

    float fval = FPOP;
    SNPRINTF( buff, sizeof(buff), "%f", fval );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printDoubleOp )
{
    NEEDS(2);
    char buff[36];
    double dval = DPOP;

    SNPRINTF( buff, sizeof(buff), "%f", dval );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( format32Op )
{
    NEEDS(2);

    ForthEngine *pEngine = GET_ENGINE;
    char* pFmt = (char *) (SPOP);
	char *pDst = pEngine->GetTmpStringBuffer();
	int buffLen = pEngine->GetTmpStringBufferSize();
	int len = strlen( pFmt );

	if ( len < 2 ) 
	{
        pEngine->SetError( kForthErrorBadParameter, " failure in format - format string too short" );
	}
	else
	{
		char lastChar = pFmt[ len - 1 ];
		switch ( tolower( lastChar ) )
		{
			case 'a': case 'e': case 'f': case 'g':
			{
				float fval = FPOP;
				double dval = (double) fval;
				SNPRINTF( pDst, buffLen, pFmt, dval );
				break;
			}
			default:
			{
				long val = SPOP;
				SNPRINTF( pDst, buffLen, pFmt, val );
			}
		}

#ifdef TRACE_PRINTS
		SPEW_PRINTS( "printed %s\n", pDst );
#endif
		SPUSH( (long) pDst );
	}
}

FORTHOP( format64Op )
{
    NEEDS(2);

    ForthEngine *pEngine = GET_ENGINE;
    char* pFmt = (char *) (SPOP);
	char *pDst = GET_ENGINE->GetTmpStringBuffer();
	int buffLen = pEngine->GetTmpStringBufferSize();
	int len = strlen( pFmt );

	if ( len < 2 ) 
	{
        pEngine->SetError( kForthErrorBadParameter, " failure in 2format - format string too short" );
	}
	else
	{
		char lastChar = pFmt[ len - 1 ];
		switch ( tolower( lastChar ) )
		{
			case 'a': case 'e': case 'f': case 'g':
			{
				double dval = DPOP;
				SNPRINTF( pDst, buffLen, pFmt, dval );
				break;
			}
			default:
			{
                stackInt64 val;
                LPOP( val );
				SNPRINTF( pDst, buffLen, pFmt, val.s64 );
			}
		}

#ifdef TRACE_PRINTS
		SPEW_PRINTS( "printed %s\n", pDst );
#endif
		SPUSH( (long) pDst );
	}
}

#ifdef ASM_INNER_INTERPRETER
#define PRINTF_SUBS_IN_ASM
#endif

#ifdef PRINTF_SUBS_IN_ASM
extern long fprintfSub( ForthCoreState* pCore );
extern long snprintfSub(ForthCoreState* pCore);
extern long fscanfSub(ForthCoreState* pCore);
extern long sscanfSub( ForthCoreState* pCore );
#else

long fprintfSub( ForthCoreState* pCore )
{
    int a[8];
    // TODO: assert if numArgs > 8
    long numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = SPOP;
    }
    const char* fmt = (const char *) SPOP;
    FILE* outfile = (FILE *) SPOP;
    return fprintf( outfile, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7] );
}

long snprintfSub( ForthCoreState* pCore )
{
    int a[8];
    // TODO: assert if numArgs > 8
    long numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = SPOP;
    }
    const char* fmt = (const char *) SPOP;
    size_t maxLen = (size_t) SPOP;
    char* outbuff = (char *) SPOP;
    return snprintf( outbuff, maxLen, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7] );
}

long fscanfSub( ForthCoreState* pCore )
{
    void* a[8];
    // TODO: assert if numArgs > 8
    long numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = (void *) SPOP;
    }
    const char* fmt = (const char *) SPOP;
    FILE* infile = (FILE *) SPOP;
    return fscanf( infile, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7] );
}

long sscanfSub( ForthCoreState* pCore )
{
    void* a[8];
    // TODO: assert if numArgs > 8
    long numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = (void *) SPOP;
    }
    const char* fmt = (const char *) SPOP;
    char* inbuff = (char *) SPOP;
    return sscanf( inbuff, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7] );
}

#endif

FORTHOP( fprintfOp )
{
    // TOS: N argN ... arg1 formatStr filePtr       (arg1 to argN are optional)
    fprintfSub( pCore );
}

FORTHOP(snprintfOp)
{
    // TOS: N argN ... arg1 formatStr bufferLen bufferPtr       (arg1 to argN are optional)
    snprintfSub(pCore);
}

FORTHOP(fscanfOp)
{
    // TOS: N argN ... arg1 formatStr filePtr       (arg1 to argN are optional)
    fscanfSub( pCore );
}

FORTHOP( sscanfOp )
{
    // TOS: N argN ... arg1 formatStr bufferPtr       (arg1 to argN are optional)
    sscanfSub( pCore );
}

FORTHOP( printStrOp )
{
    NEEDS(1);
    char *buff = (char *) SPOP;
	if ( buff == NULL )
	{
		buff = "<<<NULL>>>";
	}
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printBytesOp )
{
    NEEDS(2);
    long count = SPOP;
    const char* pChars = (const char*)(SPOP);
	if ( pChars == NULL )
	{
		pChars = "<<<NULL>>>";
		count = strlen( pChars );
	}
    CONSOLE_BYTES_OUT( pChars, count );
}

FORTHOP( printCharOp )
{
    NEEDS(1);
    char ch = static_cast<char>(SPOP);

#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %d\n", ch );
#endif

    CONSOLE_CHAR_OUT( ch );
}

FORTHOP(print4Op)
{
	NEEDS(1);
	int chars = SPOP;
	const char* pChars = (const char*)&chars;
	CONSOLE_BYTES_OUT(pChars, 4);
}

FORTHOP(print8Op)
{
	NEEDS(2);
	stackInt64 chars;
	LPOP(chars);
	const char* pChars = (const char*)&chars;
	CONSOLE_BYTES_OUT(pChars, 8);
}

FORTHOP(printSpaceOp)
{
    CONSOLE_CHAR_OUT( ' ' );
}

FORTHOP( printNewlineOp )
{
    CONSOLE_CHAR_OUT( '\n' );
}

FORTHOP( baseOp )
{
    SPUSH( (long) GET_BASE_REF );
}

FORTHOP( octalOp )
{
    *GET_BASE_REF = 8;
}

FORTHOP( decimalOp )
{
    *GET_BASE_REF = 10;
}

FORTHOP( hexOp )
{
    *GET_BASE_REF = 16;
}

FORTHOP( printDecimalSignedOp )
{
    SET_PRINT_SIGNED_NUM_MODE( kPrintSignedDecimal );
}

FORTHOP( printAllSignedOp )
{
    SET_PRINT_SIGNED_NUM_MODE( kPrintAllSigned );
}

FORTHOP( printAllUnsignedOp )
{
    SET_PRINT_SIGNED_NUM_MODE( kPrintAllUnsigned );
}

FORTHOP( getConsoleOutOp )
{
    ForthEngine *pEngine = GET_ENGINE;

	pEngine->PushConsoleOut( pCore );
}

FORTHOP( getDefaultConsoleOutOp )
{
    ForthEngine *pEngine = GET_ENGINE;

	pEngine->PushDefaultConsoleOut( pCore );
}

FORTHOP( setConsoleOutOp )
{
    ForthEngine *pEngine = GET_ENGINE;

	ForthObject conoutObject;
	POP_OBJECT( conoutObject );
	pEngine->SetConsoleOut( pCore, conoutObject );
}

FORTHOP( resetConsoleOutOp )
{
    ForthEngine *pEngine = GET_ENGINE;

	pEngine->ResetConsoleOut( pCore );
}


//##############################
//
//  File ops
//
FORTHOP( opendirOp )
{
    NEEDS(1);
	const char* pPath = (const char*) SPOP;
    int result = (int) (pCore->pFileFuncs->openDir( pPath ));
	// result is actually a DIR*
    SPUSH( result );
}

FORTHOP( readdirOp )
{
    NEEDS(1);
	void* pDir = (void *) SPOP;
    int result = (int)(pCore->pFileFuncs->readDir( pDir ));
	// result is actually a struct dirent*
    SPUSH( result );
}

FORTHOP( closedirOp )
{
    NEEDS(1);
	void* pDir = (void *) SPOP;
    int result = pCore->pFileFuncs->closeDir( pDir );
    SPUSH( result );
}

FORTHOP( rewinddirOp )
{
    NEEDS(1);
	void* pDir = (void *) SPOP;
    pCore->pFileFuncs->rewindDir( pDir );
}


FORTHOP( removeOp )
{
	const char* pFilename = (const char *) (SPOP);
    int result = pCore->pFileFuncs->fileRemove( pFilename );
    SPUSH( result );
}

FORTHOP( _dupOp )
{
	int fileHandle = SPOP;
    int result = pCore->pFileFuncs->fileDup( fileHandle );
    SPUSH( result );
}

FORTHOP( _dup2Op )
{
	int dstFileHandle = SPOP;
	int srcFileHandle = SPOP;
    int result = pCore->pFileFuncs->fileDup2( srcFileHandle, dstFileHandle );
    SPUSH( result );
}

FORTHOP( _filenoOp )
{
	FILE* pFile = (FILE *) SPOP;
	int result = pCore->pFileFuncs->fileNo( pFile );
    SPUSH( result );
}

FORTHOP( tmpnamOp )
{
	char* pOutname = (char *)__MALLOC(L_tmpnam + 1);
	pOutname[0] = '.';
    ForthEngine *pEngine = GET_ENGINE;
	if ( pCore->pFileFuncs->getTmpnam( pOutname + 1 ) == NULL )
	{
        SET_ERROR( kForthErrorFileOpen );
        pEngine->AddErrorText( "system: failure creating standard out tempfile name" );
		__FREE(pOutname);
		pOutname = NULL;
	}
    SPUSH( (long) pOutname );
}

FORTHOP( fflushOp )
{
	FILE* pFile = (FILE *) SPOP;
	int result = pCore->pFileFuncs->fileFlush( pFile );
    SPUSH( result );
}

FORTHOP( errnoOp)
{
    SPUSH( errno );
}

FORTHOP( strerrorOp)
{
	long errVal = SPOP;
    SPUSH( reinterpret_cast<long>(strerror(errVal)) );
}

FORTHOP( shellRunOp )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->runSystem( (char *) SPOP );
    SPUSH( result );
}

FORTHOP( chdirOp )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->changeDir( (const char *) SPOP );
    SPUSH( result );
}

FORTHOP( mkdirOp )
{
    NEEDS(2);
	int mode = SPOP;
	const char* pPath = (const char*) SPOP;
    int result = pCore->pFileFuncs->makeDir( pPath, mode );
    SPUSH( result );
}

FORTHOP( rmdirOp )
{
    NEEDS(1);
	const char* pPath = (const char*) SPOP;
    int result = pCore->pFileFuncs->removeDir( pPath );
    SPUSH( result );
}

FORTHOP( renameOp )
{
    NEEDS(2);
	const char* pDstPath = (const char*) SPOP;
	const char* pSrcPath = (const char*) SPOP;
    int result = pCore->pFileFuncs->renameFile( pSrcPath, pDstPath );
    SPUSH( result );
}

FORTHOP( sourceIdOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    SPUSH( pShell->GetInput()->InputStream()->GetSourceID() );
}

FORTHOP( saveInputOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    long* pData = pShell->GetInput()->InputStream()->GetInputState();
    long nItems = *pData;
    for ( int i = 0; i < nItems; i++ )
    {
        SPUSH( pData[nItems - i] );
    }
    SPUSH( nItems );
}

FORTHOP(restoreInputOp)
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    long* pSP = GET_SP;
    long nItems = *pSP;
    pShell->GetInput()->InputStream()->SetInputState(pSP);
    SET_SP(pSP + (nItems + 1));
}

FORTHOP(getEnvironmentVarOp)
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    const char* pVarName = (const char *)(SPOP);
    SPUSH((long)(pShell->GetEnvironmentVar(pVarName)));
}

FORTHOP(getEnvironmentOp)
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    const char* pVarName = (const char *)(SPOP);
    SPUSH((long)(pShell->GetEnvironmentVarNames()));
    SPUSH((long)(pShell->GetEnvironmentVarValues()));
    SPUSH(pShell->GetEnvironmentVarCount());
}

FORTHOP(getCwdOp)
{
	int maxChars = (SPOP) - 1;
	char* pDst = (char *)(SPOP);
	pDst[maxChars] = '\0';
#if defined( WIN32 )
	DWORD result = GetCurrentDirectory(maxChars, pDst);
	if (result == 0)
	{
		pDst[0] = '\0';
	}
#elif defined( LINUX )
	if (getcwd(pDst, maxChars) == NULL)
	{
		// failed to get current directory
		strcpy(pDst, ".");
	}
#endif
}

//##############################
//
// Default input/output file ops
//

FORTHOP( stdinOp )
{
    int result = (int) (pCore->pFileFuncs->getStdIn());
    SPUSH( result );
}

FORTHOP( stdoutOp )
{
    int result = (int) (pCore->pFileFuncs->getStdOut());
    SPUSH( result );
}

FORTHOP( stderrOp )
{
    int result = (int) (pCore->pFileFuncs->getStdErr());
    SPUSH( result );
}

FORTHOP( dstackOp )
{
    long *pSP = GET_SP;
    int nItems = GET_SDEPTH;
    int i;
    char buff[64];

    SNPRINTF( buff, sizeof(buff), "stack[%d]:", nItems );
    CONSOLE_STRING_OUT( buff );
    for ( i = 0; i < nItems; i++ )
    {
        CONSOLE_CHAR_OUT( ' ' );
        printNumInCurrentBase( pCore, *pSP++ );
    }
    CONSOLE_CHAR_OUT( '\n' );
}


FORTHOP( drstackOp )
{
    long *pRP = GET_RP;
    int nItems = GET_RDEPTH;
    int i;
    char buff[64];

    SNPRINTF( buff, sizeof(buff), "rstack[%d]:", nItems );
    CONSOLE_STRING_OUT( buff );
    for ( i = 0; i < nItems; i++ )
    {
        CONSOLE_STRING_OUT( " " );
        printNumInCurrentBase( pCore, *pRP++ );
    }
    CONSOLE_CHAR_OUT( '\n' );
}


FORTHOP( statsOp )
{
    char buff[512];

    SNPRINTF( buff, sizeof(buff), "pCore %p pEngine %p     DP %p DBase %p    IP %p\n",
             pCore, pCore->pEngine, pCore->pDictionary, pCore->pDictionary->pBase, pCore->IP );
    CONSOLE_STRING_OUT( buff );
    SNPRINTF( buff, sizeof(buff), "SP %p ST %p SLen %d    RP %p RT %p RLen %d\n",
             pCore->SP, pCore->ST, pCore->SLen,
             pCore->RP, pCore->RT, pCore->RLen );
    CONSOLE_STRING_OUT( buff );
    SNPRINTF( buff, sizeof(buff), "%d builtins    %d userops\n", pCore->numBuiltinOps, pCore->numOps );
    CONSOLE_STRING_OUT( buff );
}

FORTHOP( describeOp )
{
    char buff[512];


    ForthEngine *pEngine = GET_ENGINE;
    char* pSym = pEngine->GetNextSimpleToken();
	strcpy( buff, pSym );

	// allow stuff like CLASS.METHOD or VOCAB:OP
	char* pMethod = strchr( buff, '.' );
	if (pMethod == NULL)
	{
		pMethod = strchr(buff, ':');
	}
	if ( pMethod != NULL )
	{
		if ((pMethod == &(buff[0])) || (pMethod[1] == '\0'))
		{
			// don't split if ./: is at beginning or end of word
			pMethod = NULL;
		}
		else
		{
			// replace ./: with a null to split into 2 strings
			*pMethod++ = '\0';
		}
	}
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthStructVocabulary* pVocab = pManager->GetStructVocabulary( buff );
	bool verbose = (GET_VAR_OPERATION != kVarDefaultOp);

    if ( pVocab )
    {
		if ( pMethod != NULL )
		{
			ForthVocabulary* pFoundVocab = NULL;
			long* pEntry = pVocab->FindSymbol( pMethod );
			if (pEntry != NULL) 
			{
				long typeCode = pEntry[1];
				if (pVocab->IsClass())
				{
					ForthClassVocabulary* pClassVocab = (ForthClassVocabulary*)pVocab;
					if (CODE_IS_METHOD(typeCode))
					{
						// TODO: support secondary interfaces
						pEngine->DescribeOp(pSym, pClassVocab->GetInterface(0)->GetMethod(pEntry[0]), pEntry[1]);
					}
					else if (CODE_TO_BASE_TYPE(typeCode) == kBaseTypeUserDefinition)
					{
						pEngine->DescribeOp(pSym, pEntry[0], pEntry[1]);
					}
					else
					{
						pVocab->PrintEntry(pEntry);
					}
				}
				else
				{
					if (CODE_TO_BASE_TYPE(typeCode) == kBaseTypeUserDefinition)
					{
						pEngine->DescribeOp(pSym, pEntry[0], pEntry[1]);
					}
					else
					{
						pVocab->PrintEntry(pEntry);
					}
				}
			}
			else
			{
				SNPRINTF( buff, sizeof(buff), "Failed to find method %s\n", pSym );
				CONSOLE_STRING_OUT( buff );
			}
		}
		else
		{
			// show structure vocabulary entries
			while ( pVocab )
			{
				SNPRINTF( buff, sizeof(buff), "%s vocabulary %s:\n", ((pVocab->IsClass() ? "class" : "struct")), pVocab->GetName() );
				CONSOLE_STRING_OUT( buff );
				char quit = ShowVocab( pEngine->GetCoreState(), pVocab, !verbose );
				if ( quit == 'q' )
				{
					break;
				}
				else
				{
					pVocab = pVocab->BaseVocabulary();
				}
			}
		}
    }
    else
    {
        pEngine->DescribeSymbol( pSym );
    }
	CLEAR_VAR_OPERATION;
}

FORTHOP( describeAtOp )
{
	NEEDS( 2 );
	char* pString = reinterpret_cast<char *>(SPOP);
	long* pOp = reinterpret_cast<long *>(SPOP);
    long* pLengths = reinterpret_cast<long *>(pString) - 2;
	// pLengths[0] is max length, pLength[1] is current length
    pLengths[1] = 0;
	if ( pLengths[0] < 1 )
	{
		return;
	}
    *pString = '\0';

    ForthEngine *pEngine = GET_ENGINE;

	pEngine->DescribeOp( pOp, pString, pLengths[0], true );
	pLengths[1] = strlen( pString );
	CLEAR_VAR_OPERATION;
}

FORTHOP( DLLVocabularyOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabulary *pDefinitionsVocab = pEngine->GetDefinitionVocabulary();
    // get next symbol, add it to vocabulary with type "user op"
    char* pDLLOpName = pEngine->GetNextSimpleToken();
    pEngine->StartOpDefinition( pDLLOpName );
    char* pDLLName = pEngine->GetNextSimpleToken();
    ForthDLLVocabulary* pVocab = new ForthDLLVocabulary( pDLLOpName,
                                                         pDLLName,
                                                         NUM_FORTH_VOCAB_VALUE_LONGS,
                                                         512,
                                                         GET_DP,
                                                         ForthVocabulary::GetEntryValue( pDefinitionsVocab->GetNewestEntry() ) );
    pEngine->CompileBuiltinOpcode( OP_DO_VOCAB );
    pVocab->LoadDLL();
    pEngine->CompileLong( (long) pVocab );
}

FORTHOP( addDLLEntryOp )
{
    NEEDS( 2 );

    ForthEngine *pEngine = GET_ENGINE;
    char* pProcName = (char *) SPOP;
    ForthDLLVocabulary* pVocab = (ForthDLLVocabulary *) (pEngine->GetDefinitionVocabulary());
    if ( strcmp( pVocab->GetType(), "dllOp" ) )
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( kForthErrorBadParameter, " is not a DLL vocabulary - addDllEntry" );
    }
    ulong numArgs = SPOP;
    pVocab->AddEntry( pProcName, numArgs );
}

FORTHOP( DLLVoidOp )
{
	NEEDS( 0 );

    ForthEngine *pEngine = GET_ENGINE;
    ForthDLLVocabulary* pVocab = (ForthDLLVocabulary *) (pEngine->GetDefinitionVocabulary());
    if ( strcmp( pVocab->GetType(), "dllOp" ) )
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( kForthErrorBadParameter, " is not a DLL vocabulary - DLLVoidOp" );
    }
	else
	{
		pVocab->SetFlag( DLL_ENTRY_FLAG_RETURN_VOID );
	}
}

FORTHOP( DLLLongOp )
{
	NEEDS( 0 );

    ForthEngine *pEngine = GET_ENGINE;
    ForthDLLVocabulary* pVocab = (ForthDLLVocabulary *) (pEngine->GetDefinitionVocabulary());
    if ( strcmp( pVocab->GetType(), "dllOp" ) )
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( kForthErrorBadParameter, " is not a DLL vocabulary - DLLLongOp" );
    }
	else
	{
		pVocab->SetFlag( DLL_ENTRY_FLAG_RETURN_64BIT );
	}
}

FORTHOP( DLLStdCallOp )
{
	NEEDS( 0 );

    ForthEngine *pEngine = GET_ENGINE;
    ForthDLLVocabulary* pVocab = (ForthDLLVocabulary *) (pEngine->GetDefinitionVocabulary());
    if ( strcmp( pVocab->GetType(), "dllOp" ) )
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( kForthErrorBadParameter, " is not a DLL vocabulary - DLLStdCallOp" );
    }
	else
	{
		pVocab->SetFlag( DLL_ENTRY_FLAG_STDCALL );
	}
}


FORTHOP( blwordOp )
{
    NEEDS( 0 );
    ForthShell *pShell = GET_ENGINE->GetShell();
	char *pSrc = pShell->GetNextSimpleToken();
    // leave an unused byte below string so string len can be stuck there in ANSI compatability mode
	char *pDst = GET_ENGINE->GetTmpStringBuffer() + 1;
	strncpy( pDst, pSrc, (TMP_STRING_BUFFER_LEN - 2) );
    SPUSH( (long) pDst );
}

FORTHOP( strWordOp )
{
    NEEDS( 1 );
    ForthShell *pShell = GET_ENGINE->GetShell();
    char delim = (char) (SPOP);
    // leave an unused byte below string so string len can be stuck there in ANSI compatability mode
	char *pSrc = pShell->GetToken( delim, false );
	char *pDst = GET_ENGINE->GetTmpStringBuffer() + 1;
	strncpy( pDst, pSrc, (TMP_STRING_BUFFER_LEN - 2) );
    SPUSH( (long) pDst );
}

// has precedence!
// inline comment using "/*"
FORTHOP( commentOp )
{
    NEEDS( 0 );
    ForthShell *pShell = GET_ENGINE->GetShell();
    ForthInputStack* pInput = pShell->GetInput();
    const char *pBuffer = pInput->GetBufferPointer();
    const char *pEnd = strstr( pBuffer, "*/" );
    if ( pInput->InputStream()->IsInteractive() && (pEnd == NULL) )
    {
        // in interactive mode, comment can't span lines
        pShell->GetToken( 0 );
    }
	while ( pEnd == NULL )
	{
		const char *pLine = pShell->GetInput()->GetLine("");
		if ( pLine == NULL )
		{
			break;
		}
		pBuffer = pShell->GetInput()->GetBufferPointer();
	    pEnd = strstr( pBuffer, "*/" );
	}
    if ( pEnd != NULL )
    {
        pInput->SetBufferPointer( pEnd + 2 );
    }
}

// has precedence!
// comment to end of line using "\"
FORTHOP( slashCommentOp )
{
    NEEDS( 0 );
    ForthShell *pShell = GET_ENGINE->GetShell();
    pShell->GetInput()->InputStream()->SeekToLineEnd();
}

// has precedence!
// inline comment using parens
// strictly for ease of porting old style forth code
FORTHOP( parenCommentOp )
{
    NEEDS( 0 );
    ForthShell *pShell = GET_ENGINE->GetShell();
	const char* pBuffer = pShell->GetInput()->GetBufferPointer();
	while ( strchr( pBuffer, ')' ) == NULL )
	{
        if ( pShell->GetInput()->InputStream()->IsInteractive() )
        {
            // in interactive mode, comment can't span lines
            pShell->GetToken( 0 );
            return;
        }
		const char *pLine = pShell->GetInput()->GetLine("");
		if ( pLine == NULL )
		{
			break;
		}
		pBuffer = pShell->GetInput()->GetBufferPointer();
	}

	char *pToken = pShell->GetToken( ')' );
}

// fake variable used to turn on/off features
FORTHOP( featuresOp )
{
    NEEDS( 1 );
    ForthEngine* pEngine = GET_ENGINE;

    switch ( GET_VAR_OPERATION )
    {
    case kVarStore:
        pEngine->SetFeatures( SPOP );
        break;

    case kVarDefaultOp:
    case kVarFetch:
        SPUSH( pEngine->GetFeatures() );
        break;

    case kVarRef:
        SPUSH( (long)(&(pEngine->GetFeatures())) );
        break;

    case kVarPlusStore:
        pEngine->SetFeature( SPOP );
        break;

    case kVarMinusStore:
        pEngine->ClearFeature( SPOP );
        break;

    default:
        pEngine->SetError( kForthErrorBadVarOperation, "features: unkown var operation" );
        break;
    }
    CLEAR_VAR_OPERATION;
}

FORTHOP( dumpCrashStateOp )
{
	ForthEngine* pEngine = GET_ENGINE;
	pEngine->DumpCrashState();
}

FORTHOP( sourceOp )
{
    ForthInputStack* pInput = GET_ENGINE->GetShell()->GetInput();
    SPUSH( (long) (pInput->InputStream()->GetReportedBufferBasePointer()) );
    SPUSH( pInput->GetWriteOffset() );
}

FORTHOP( getInOffsetPointerOp )
{
    ForthInputStack* pInput = GET_ENGINE->GetShell()->GetInput();
    SPUSH( (long) (pInput->GetReadOffsetPointer()) );
}

FORTHOP( fillInBufferOp )
{
    ForthInputStack* pInput = GET_ENGINE->GetShell()->GetInput();
    SPUSH( (long) (pInput->GetLine( (char *) (SPOP) )) );
}

FORTHOP( keyOp )
{
	int key = getch();
    SPUSH( key );
}

FORTHOP( keyHitOp )
{
	int keyHit = kbhit();
    SPUSH( keyHit );
}

FORTHOP(vocChainHeadOp)
{
	SPUSH((long)(ForthVocabulary::GetVocabularyChainHead()));
}

FORTHOP(vocChainNextOp)
{
	ForthVocabulary* pVocab = (ForthVocabulary *)(SPOP);
	SPUSH((long)(pVocab->GetNextChainVocabulary()));
}

#if 0
FORTHOP(vocNewestEntryOp)
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	SPUSH( (long) (pVocab->GetNewestEntry()) );
}

FORTHOP( vocNextEntryOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	long *pEntry = (long *) (SPOP);
	SPUSH( (long) (pVocab->NextEntry( pEntry )) );
}

FORTHOP( vocNumEntriesOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	SPUSH( (long) (pVocab->GetNumEntries()) );
}

FORTHOP( vocNameOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	SPUSH( (long) (pVocab->GetName()) );
}

FORTHOP( vocFindEntryOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	const char* pName = (const char *) (SPOP);
	SPUSH( (long) (pVocab->FindSymbol( pName )) );
}

FORTHOP( vocFindNextEntryOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	const char* pName = (const char *) (SPOP);
	long *pEntry = (long *) (SPOP);
	SPUSH( (long) (pVocab->FindNextSymbol( pName, pEntry )) );
}

FORTHOP( vocFindEntryByValueOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	long val = SPOP;
	SPUSH( (long) (pVocab->FindSymbolByValue( val )) );
}

FORTHOP( vocFindNextEntryByValueOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	long val = SPOP;
	long *pEntry = (long *) (SPOP);
	SPUSH( (long) (pVocab->FindNextSymbolByValue( val, pEntry )) );
}

FORTHOP( vocAddEntryOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
    long opVal = SPOP;
    long opType = SPOP;
    char* pSymbol = (char *) (SPOP);
	bool addToEngineOps = (opType <= kOpDLLEntryPoint);
    pVocab->AddSymbol( pSymbol, opType, opVal, addToEngineOps );
}

FORTHOP( vocRemoveEntryOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	long *pEntry = (long *) (SPOP);
	pVocab->DeleteEntry( pEntry );
}

FORTHOP( vocValueLengthOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	SPUSH( (long) (pVocab->GetValueLength()) );
}
#endif

FORTHOP( turboOp )
{
    GET_ENGINE->ToggleFastMode();
    SET_STATE( kResultDone );
}

FORTHOP( errorOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->SetError( kForthErrorUserDefined, (char *) (SPOP) );
}

FORTHOP( addErrorTextOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->AddErrorText( (char *) (SPOP) );
}

FORTHOP( timeOp )
{
    time_t rawtime;
    time( &rawtime );
    DPUSH( *((double *) &rawtime) );
}

FORTHOP( strftimeOp )
{
    double dtime = DPOP;
    time_t rawtime = *((time_t*) &dtime);
    struct tm * timeinfo;
    const char* fmt = (const char *)(SPOP);
    size_t bufferSize = (size_t)(SPOP);
    char* buffer = (char *)(SPOP);

    timeinfo = localtime ( &rawtime );
    // Www Mmm dd yyyy (weekday, month, day, year)
	errno = 0;
	strftime(buffer, bufferSize, fmt, timeinfo);
	if (errno)
	{
		GET_ENGINE->SetError(kForthErrorBadVarOperation);
	}
}

FORTHOP( timeAndDateOp )
{
    time_t rawtime;
    time( &rawtime );
    struct tm* brokenDownTime = gmtime( &rawtime );
    SPUSH( brokenDownTime->tm_year );
    SPUSH( brokenDownTime->tm_mon );
    SPUSH( brokenDownTime->tm_mday );
    SPUSH( brokenDownTime->tm_hour );
    SPUSH( brokenDownTime->tm_min );
    SPUSH( brokenDownTime->tm_sec );
}

FORTHOP( millitimeOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    SPUSH( (long) pEngine->GetElapsedTime() );
}

FORTHOP( millisleepOp )
{
#ifdef WIN32
	DWORD dwMilliseconds = (DWORD)(SPOP);
    ::Sleep( dwMilliseconds );
#else
    int milliseconds = SPOP;
    usleep( milliseconds * 1000 );
#endif
}


//##############################
//
//  computation utilities - randoms, hashing, sorting and searcing
//

FORTHOP( randOp )
{
    SPUSH( rand() );
}

FORTHOP( srandOp )
{
    srand( (unsigned int) (SPOP) );
}

#define get16bits(d) ((((unsigned long)(((const unsigned char *)(d))[1])) << 8)\
						+(unsigned long)(((const unsigned char *)(d))[0]) )

unsigned long
SuperFastHash (const char * data, int len, unsigned long hash)
{

	unsigned long tmp;
	int rem;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--) {
		hash  += get16bits (data);
		tmp    = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (unsigned short);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
		case 3: hash += get16bits (data);
			hash ^= hash << 16;
			hash ^= data[sizeof (unsigned short)] << 18;
			hash += hash >> 11;
			break;
		case 2: hash += get16bits (data);
			hash ^= hash << 11;
			hash += hash >> 17;
			break;
		case 1: hash += *data;
			hash ^= hash << 10;
			hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

// data len hashIn ... hashOut
FORTHOP( hashOp )
{
    unsigned int hashVal = SPOP;
    int len = SPOP;
    const char* pData = (const char*) (SPOP);
    hashVal = SuperFastHash( pData, len, hashVal );
    SPUSH( hashVal );
}

namespace
{
    struct qsInfo
    {
        void*   temp;
        int     offset;
        int     elementSize;
        int     compareSize;
        int     (*compare)( const void* a, const void* b, int );
    };

    int u8Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((unsigned char *) pA);
        int b = (int) *((unsigned char *) pB);
        return a - b;
    }

    int s8Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((char *) pA);
        int b = (int) *((char *) pB);
        return a - b;
    }

    int u16Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((unsigned short *) pA);
        int b = (int) *((unsigned short *) pB);
        return a - b;
    }

    int s16Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((short *) pA);
        int b = (int) *((short *) pB);
        return a - b;
    }

    int u32Compare( const void* pA, const void* pB, int )
    {
        unsigned int a = *((unsigned int *) pA);
        unsigned int b = *((unsigned int *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int s32Compare( const void* pA, const void* pB, int )
    {
        int a = *((int *) pA);
        int b = *((int *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int f32Compare( const void* pA, const void* pB, int )
    {
        float a = *((float *) pA);
        float b = *((float *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int f64Compare( const void* pA, const void* pB, int )
    {
        double a = *((double *) pA);
        double b = *((double *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int u64Compare( const void* pA, const void* pB, int )
    {
        unsigned long long a = *((unsigned long long *) pA);
        unsigned long long b = *((unsigned long long *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int s64Compare( const void* pA, const void* pB, int )
    {
        long long a = *((long long *) pA);
        long long b = *((long long *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int strCompare( const void* pA, const void* pB, int numChars )
    {
        char* a = (char *) pA;
        char* b = (char *) pB;
        return strncmp( a, b, numChars );
    }

    int qsPartition( char* pData, qsInfo* pQS, int left, int right )
    {
        int elementSize = pQS->elementSize;
        char* pLeft = pData + (pQS->elementSize * left) + pQS->offset;
        char* pRight = pData + (pQS->elementSize * right) + pQS->offset;
        // pivot = pData[left]
        char* pivot = (char *)(pQS->temp);
        int compareSize = pQS->compareSize;
        memcpy( pivot, pLeft, compareSize );

	    while ( true )
	    {
		    // while (a[left] < pivot) left++;
            while ( pQS->compare( pLeft, pivot, compareSize ) < 0 )
            {
                left++;
                pLeft += elementSize;
            }

		    // while (a[right] > pivot) right--;
            while ( pQS->compare( pRight, pivot, compareSize ) > 0 )
            {
                right--;
                pRight -= elementSize;
            }

            if ( pQS->compare( pLeft, pRight, compareSize ) == 0 )
            {
                left++;
                pLeft += elementSize;
            }
		    else if ( left < right )
		    {
			    // swap(a[left], a[right]);
                memcpy( pivot, pLeft, pQS->elementSize );
                memcpy( pLeft, pRight, pQS->elementSize );
                memcpy( pRight, pivot, pQS->elementSize );
		    }
		    else
		    {
			    return right;
		    }
	    }
    }

    void qsStep( void* pData, qsInfo* pQS, int left, int right )
    {
	    if ( left < right )
	    {
		    int pivot = qsPartition( (char *) pData, pQS, left, right );
		    qsStep( pData, pQS, left, (pivot-1) );
		    qsStep( pData, pQS, (pivot+1), right );
	    }
    }

}

FORTHOP( qsortOp )
{
    // qsort( ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET )
    NEEDS(6);
    qsInfo qs;

    qs.offset = SPOP;
    long compareType = SPOP;
    qs.elementSize = SPOP;
    long numElements = SPOP;
    void* pArrayBase = (void *)(SPOP);

	qs.temp = __MALLOC(qs.elementSize);

    switch ( CODE_TO_BASE_TYPE(compareType) )
    {
    case kBaseTypeByte:
        qs.compareSize = 1;
        qs.compare = s8Compare;
        break;

    case kBaseTypeUByte:
        qs.compareSize = 1;
        qs.compare = u8Compare;
        break;

    case kBaseTypeShort:
        qs.compareSize = 2;
        qs.compare = s16Compare;
        break;

    case kBaseTypeUShort:
        qs.compareSize = 2;
        qs.compare = u16Compare;
        break;

    case kBaseTypeInt:
        qs.compareSize = 4;
        qs.compare = s32Compare;
        break;

    case kBaseTypeUInt:
        qs.compareSize = 4;
        qs.compare = u32Compare;
        break;

    case kBaseTypeLong:
        qs.compareSize = 8;
        qs.compare = s64Compare;
        break;

    case kBaseTypeULong:
        qs.compareSize = 8;
        qs.compare = u64Compare;
        break;

    case kBaseTypeFloat:
        qs.compareSize = 4;
        qs.compare = f32Compare;
        break;

    case kBaseTypeDouble:
        qs.compareSize = 8;
        qs.compare = f64Compare;
        break;

    case kBaseTypeString:
        qs.compareSize = CODE_TO_STRING_BYTES(compareType);
        qs.compare = strCompare;
        break;

    default:
        break;
    }

    qsStep( pArrayBase, &qs, 0, (numElements - 1) );

    __FREE( qs.temp );
}

FORTHOP(bsearchOp)
{
    // bsearch( KEY_ADDR ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET )
    NEEDS(7);

    long offset = SPOP;
    long compareType = SPOP;
    long elementSize = SPOP;
    long numElements = SPOP;
    char* pData = (char *)(SPOP);
    char* pKey = (char *)(SPOP);
    long compareSize;
    int (*compare)( const void* a, const void* b, int );

    switch ( CODE_TO_BASE_TYPE(compareType) )
    {
    case kBaseTypeByte:
        compareSize = 1;
        compare = s8Compare;
        break;

    case kBaseTypeUByte:
        compareSize = 1;
        compare = u8Compare;
        break;

    case kBaseTypeShort:
        compareSize = 2;
        compare = s16Compare;
        break;

    case kBaseTypeUShort:
        compareSize = 2;
        compare = u16Compare;
        break;

    case kBaseTypeInt:
        compareSize = 4;
        compare = s32Compare;
        break;

    case kBaseTypeUInt:
        compareSize = 4;
        compare = u32Compare;
        break;

    case kBaseTypeLong:
        compareSize = 8;
        compare = s64Compare;
        break;

    case kBaseTypeULong:
        compareSize = 8;
        compare = u64Compare;
        break;

    case kBaseTypeFloat:
        compareSize = 4;
        compare = f32Compare;
        break;

    case kBaseTypeDouble:
        compareSize = 8;
        compare = f64Compare;
        break;

    case kBaseTypeString:
        compareSize = CODE_TO_STRING_BYTES(compareType);
        compare = strCompare;
        break;

    default:
        break;
    }

    int first = 0;
    int last = numElements - 1;
    while ( first <= last )
    {
        int mid = (first + last) / 2;  // compute mid point.
        char* pMid = pData + (elementSize * mid) + offset;
        int result = compare( pKey, pMid, compareSize );
        if ( result > 0 )           //if (key > sortedArray[mid]) 
        {
            first = mid + 1;  // repeat search in top half.
        }
        else if ( result < 0 )                 // if (key < sortedArray[mid]) 
        {
            last = mid - 1; // repeat search in bottom half.
        }
        else
        {
            SPUSH( mid );     // found it. return position
            return;
        }
    }
    // key is not in array, return negative of insertion point+1
    //  the one is added so we can distinguish between an exact match with element 0 (return 0)
    //  and the key value being less than element 0 (return -1)
    SPUSH( -(first + 1) );    // failed to find key
}

///////////////////////////////////////////
//  conditional compilation
///////////////////////////////////////////
FORTHOP( poundIfOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    pShell->PoundIf();
}

FORTHOP( poundIfdefOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    pShell->PoundIfdef( true );
}

FORTHOP( poundIfndefOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    pShell->PoundIfdef( false );
}

FORTHOP( poundElseOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    pShell->PoundElse();
}

FORTHOP( poundEndifOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    pShell->PoundEndif();
}

bool checkBracketToken( const char*& pStart, const char* pToken )
{
    int len = strlen( pToken );
#if defined(WIN32)
    if ( !_strnicmp( pStart, pToken, len ) )
#elif defined(LINUX)
    if ( !strncasecmp( pStart, pToken, len ) )
#endif
    {
        char delim = pStart[len];
        if ((delim == ' ') || (delim == '\t') || (delim == '\0'))
        {
            pStart += len;
            return true;
        }
    }
    return false;
}

FORTHOP( bracketIfOp )
{
    NEEDS( 1 );
    ForthShell *pShell = GET_ENGINE->GetShell();
    ForthInputStack* pInput = pShell->GetInput();
	const char* pBuffer = pShell->GetInput()->GetBufferPointer();
    int depth = 0;
    int takeIfBranch = SPOP;
    //SPEW_ENGINE("bracketIf");

    if ( !takeIfBranch )
    {
        // skip until you hit [else] or [endif], ignore nested [if][endif] pairs
	    while ( true )
	    {
            const char* pBracket = pBuffer;
            while ( (pBracket = strchr( pBuffer, '[' )) != NULL )
            {
                //SPEW_ENGINE("bracketIf {%s}", pBracket);
                if ( checkBracketToken( pBracket, "[else]" ) )
                {
                    if ( depth == 0 )
                    {
                        pInput->SetBufferPointer( pBracket );
                        return;
                    }
                }
                else if ( checkBracketToken( pBracket, "[endif]" )  || checkBracketToken( pBracket, "[then]" ))
                {
                    if ( depth == 0 )
                    {
                        pInput->SetBufferPointer( pBracket );
                        return;
                    }
                    else
                    {
                        --depth;
                        //SPEW_ENGINE("bracketIf depth %d", depth);
                    }
                }
                else if ( checkBracketToken( pBracket, "[if]" ) || checkBracketToken( pBracket, "[ifdef]" ) || checkBracketToken( pBracket, "[ifundef]" ) )
                {
                    ++depth;
                    //SPEW_ENGINE("bracketIf depth %d", depth);
                }
                else
                {
                    pBracket++;
                }
                pBuffer = pBracket;
            }
		    const char *pLine = pShell->GetInput()->GetLine("<in [if]>");
		    if ( pLine == NULL )
		    {
			    break;
		    }
		    pBuffer = pShell->GetInput()->GetBufferPointer();
	    }
    }
}

FORTHOP( bracketIfDefOp )
{
    NEEDS( 0 );
    ForthEngine *pEngine = GET_ENGINE;
    char* pToken = pEngine->GetNextSimpleToken();
    ForthVocabulary* pFoundVocab;
    long* pSymbol = pEngine->GetVocabularyStack()->FindSymbol( pToken, &pFoundVocab );
    SPUSH ( (pSymbol != NULL) ? ~0 : 0 );
    bracketIfOp( pCore );
}

FORTHOP( bracketIfUndefOp )
{
    NEEDS( 0 );
    ForthEngine *pEngine = GET_ENGINE;
    char* pToken = pEngine->GetNextSimpleToken();
    ForthVocabulary* pFoundVocab;
    long* pSymbol = pEngine->GetVocabularyStack()->FindSymbol( pToken, &pFoundVocab );
    SPUSH ( (pSymbol != NULL) ? 0 : ~0 );
    bracketIfOp( pCore );
}

FORTHOP( bracketElseOp )
{
    NEEDS( 0 );
    ForthShell *pShell = GET_ENGINE->GetShell();
    ForthInputStack* pInput = pShell->GetInput();
	const char* pBuffer = pShell->GetInput()->GetBufferPointer();
    int depth = 0;

    // skip until you hit [endif], ignore nested [if][endif] pairs
    while ( true )
    {
        const char* pBracket = pBuffer;
        while ( (pBracket = strchr( pBuffer, '[' )) != NULL )
        {
            if ( checkBracketToken( pBracket, "[endif]" )  || checkBracketToken( pBracket, "[then]" ))
            {
                if ( depth == 0 )
                {
                    pInput->SetBufferPointer( pBracket );
                    return;
                }
                else
                {
                    --depth;
                }
            }
            else if ( checkBracketToken( pBracket, "[if]" ) || checkBracketToken( pBracket, "[ifdef]" ) || checkBracketToken( pBracket, "[ifundef]" ) )
            {
                ++depth;
            }
            else
            {
                ++pBracket;
            }
            pBuffer = pBracket;
        }
	    const char *pLine = pShell->GetInput()->GetLine("<in [else]>");
	    if ( pLine == NULL )
	    {
		    break;
	    }
	    pBuffer = pShell->GetInput()->GetBufferPointer();
    }
}

FORTHOP( bracketEndifOp )
{
}

FORTHOP(setTraceOp)
{
	int traceFlags = SPOP;
	GET_ENGINE->SetTraceFlags(traceFlags);
	SET_STATE(kResultTrace);
}

FORTHOP(getTraceOp)
{
	int traceFlags = GET_ENGINE->GetTraceFlags();
	SPUSH(traceFlags);
}

FORTHOP(ssPushBop)
{
	NEEDS(1);
	long v = SPOP;
	GET_ENGINE->GetShell()->GetShellStack()->Push(v);
}

FORTHOP(ssPopBop)
{
	SPUSH(GET_ENGINE->GetShell()->GetShellStack()->Pop());
}

FORTHOP(ssPeekBop)
{
	SPUSH(GET_ENGINE->GetShell()->GetShellStack()->Peek());
}

FORTHOP(ssDepthBop)
{
	SPUSH(GET_ENGINE->GetShell()->GetShellStack()->GetDepth());
}

///////////////////////////////////////////
//  block i/o
///////////////////////////////////////////

FORTHOP( blkOp )
{
    SPUSH( (long)(GET_ENGINE->GetBlockPtr()) );
}

FORTHOP( blockOp )
{
    ForthBlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    SPUSH( (long)(pBlockManager->GetBlock(SPOP, true)) );
}

FORTHOP( bufferOp )
{
    ForthBlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    SPUSH( (long)(pBlockManager->GetBlock(SPOP, false)) );
}

FORTHOP( emptyBuffersOp )
{
    ForthBlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    pBlockManager->EmptyBuffers();
}

FORTHOP( flushOp )
{
    ForthBlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    pBlockManager->SaveBuffers( true );
}

FORTHOP( saveBuffersOp )
{
    ForthBlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    pBlockManager->SaveBuffers( false );
}

FORTHOP( updateOp )
{
    ForthBlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    pBlockManager->UpdateCurrentBuffer();
}

FORTHOP( thruOp )
{
    unsigned int lastBlock = (unsigned int) SPOP;
    unsigned int firstBlock = (unsigned int) SPOP;
    ForthEngine* pEngine = GET_ENGINE;
    if ( lastBlock < firstBlock )
    {
        pEngine->SetError( kForthErrorIO, "thru - last block less than first block" );
    }
    else
    {
        ForthBlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
        if ( lastBlock < pBlockManager->GetNumBlocksInFile())
        {
            GET_ENGINE->PushInputBlocks( firstBlock, lastBlock );
        }
        else
        {
            pEngine->SetError( kForthErrorIO, "thru - last block beyond end of block file" );
        }
    }
}


///////////////////////////////////////////
//  threads
///////////////////////////////////////////

FORTHOP( createThreadOp )
{
    long threadOp  = SPOP;
    int returnStackSize = (int)(SPOP);
    int paramStackSize = (int)(SPOP);
    ForthThread* pThread = GET_ENGINE->CreateThread( threadOp, paramStackSize, returnStackSize );
    SPUSH( (long) pThread );
}

FORTHOP( destroyThreadOp )
{
    ForthThread* pThread = (ForthThread*)(SPOP);
    GET_ENGINE->DestroyThread( pThread );
}

FORTHOP( threadGetStateOp )
{
    ForthThread* pThread = (ForthThread*)(SPOP);
	SPUSH( (long) (pThread->GetCore()) );
}

FORTHOP( stepThreadOp )
{
    ForthThread* pThread = (ForthThread*)(SPOP);
	ForthCoreState* pThreadCore = pThread->GetCore();
	long op = *(pThreadCore->IP)++;
    long result;
    ForthEngine *pEngine = GET_ENGINE;
#ifdef ASM_INNER_INTERPRETER
	if ( pEngine->GetFastMode() )
	{
		result = (long) InterpretOneOpFast( pCore, op );
	}
	else
#endif
	{
		result = (long) InterpretOneOp( pCore, op );
	}
    SPUSH( result );
}

FORTHOP( startThreadOp )
{
    ForthThread* pThread = (ForthThread*)(SPOP);
    long result = pThread->Start();
    SPUSH( result );
}

FORTHOP( exitThreadOp )
{
    ForthThread* pThread = (ForthThread*)(pCore->pThread);
    pThread->Exit();
}

#ifdef WIN32
///////////////////////////////////////////
//  Windows support
///////////////////////////////////////////

FORTHOP( createEventOp )
{
    LPCTSTR lpName = (LPCTSTR)(SPOP);
    BOOL bInitialState = (BOOL)(SPOP);
    BOOL bManualReset = (BOOL)(SPOP);
    LPSECURITY_ATTRIBUTES lpEventAttributes = (LPSECURITY_ATTRIBUTES) (SPOP);
    HANDLE result = ::CreateEvent( lpEventAttributes, bManualReset, bInitialState, lpName );
    SPUSH( (long) result );
}

FORTHOP( closeHandleOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::CloseHandle( hObject );
    SPUSH( (long) result );
}

FORTHOP( setEventOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::SetEvent( hObject );
    SPUSH( (long) result );
}

FORTHOP( resetEventOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::ResetEvent( hObject );
    SPUSH( (long) result );
}

FORTHOP( pulseEventOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::PulseEvent( hObject );
    SPUSH( (long) result );
}

FORTHOP( getLastErrorOp )
{
    DWORD result = ::GetLastError();
    SPUSH( (long) result );
}

FORTHOP( waitForSingleObjectOp )
{
    DWORD dwMilliseconds = (DWORD)(SPOP);
    HANDLE hHandle = (HANDLE)(SPOP);

    DWORD result = ::WaitForSingleObject( hHandle, dwMilliseconds );
    SPUSH( (long) result );
}

FORTHOP( waitForMultipleObjectsOp )
{
    DWORD dwMilliseconds = (DWORD)(SPOP);
    BOOL bWaitAll = (BOOL)(SPOP);
    const HANDLE* lpHandles = (const HANDLE*)(SPOP);
    DWORD nCount = (DWORD)(SPOP);

    DWORD result = ::WaitForMultipleObjects( nCount, lpHandles, bWaitAll, dwMilliseconds );
    SPUSH( (long) result );
}

FORTHOP( initializeCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::InitializeCriticalSection( pCriticalSection );
}

FORTHOP( deleteCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::DeleteCriticalSection( pCriticalSection );
}

FORTHOP( enterCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::EnterCriticalSection( pCriticalSection );
}

FORTHOP( leaveCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::LeaveCriticalSection( pCriticalSection );
}

FORTHOP( findFirstFileOp )
{
    LPWIN32_FIND_DATA pOutData = (LPWIN32_FIND_DATA)(SPOP);
    LPCTSTR pPath = (LPCTSTR)(SPOP);
    HANDLE result = ::FindFirstFile( pPath, pOutData );
    SPUSH( (long) result );
}

FORTHOP( findNextFileOp )
{
    LPWIN32_FIND_DATA pOutData = (LPWIN32_FIND_DATA)(SPOP);
    HANDLE searchHandle = (HANDLE)(SPOP);
    BOOL result = ::FindNextFile( searchHandle, pOutData );
    SPUSH( (long) result );
}

FORTHOP( findCloseOp )
{
    HANDLE searchHandle = (HANDLE)(SPOP);
    BOOL result = ::FindClose( searchHandle );
    SPUSH( (long) result );
}

FORTHOP( showConsoleOp )
{
	NEEDS(1);
	long showIt = SPOP;
	ShowWindow( GetConsoleWindow(), (showIt != 0) ? SW_RESTORE : SW_HIDE );
}

FORTHOP( windowsConstantsOp )
{
    static int winConstants[5] =
    {
        (sizeof(winConstants)/sizeof(int)) - 1,
        sizeof(TCHAR),
        MAX_PATH,
        sizeof(WIN32_FIND_DATA),
        sizeof(CRITICAL_SECTION)
    };
    SPUSH( (long) (&(winConstants[0])) );
}

#endif

FORTHOP( setConsoleCursorOp )
{
	NEEDS( 2 );
#ifdef WIN32
	COORD screenPos;
	screenPos.Y = (SHORT) SPOP;
	screenPos.X = (SHORT) SPOP;
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	SetConsoleCursorPosition( hConsole, screenPos );
#else
	int line = SPOP;
	int column = SPOP;
	printf("%c[%d;%dH", 0x1b, line, column);
#endif
}

FORTHOP( getConsoleCursorOp )
{
	NEEDS( 0 );
#ifdef WIN32
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleScreenBufferInfo( hConsole, &consoleInfo );
	SPUSH( consoleInfo.dwCursorPosition.X );
	SPUSH( consoleInfo.dwCursorPosition.Y );
#else
	SPUSH(0);
	SPUSH(0);
#endif
}

FORTHOP( setConsoleColorOp )
{
	NEEDS( 1 );
#ifdef WIN32
	WORD attributes = (WORD) SPOP;
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	SetConsoleTextAttribute( hConsole, attributes );
#else
	int attributes = SPOP;
	printf("%c[%dm", 0x1b, attributes);
#endif
}

FORTHOP( getConsoleColorOp )
{
#ifdef WIN32
	NEEDS( 0 );
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleScreenBufferInfo( hConsole, &consoleInfo );
	SPUSH( consoleInfo.wAttributes );
#else
	SPUSH(0xF0);
#endif
}

FORTHOP(clearConsoleOp)
{
	NEEDS(0);
#ifdef WIN32
	COORD coordScreen = { 0, 0 };    // home for the cursor 
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi; 
    DWORD dwConSize;

	// Get the number of character cells in the current buffer. 
    if ( !GetConsoleScreenBufferInfo( hConsole, &csbi ) )
    {
		return;
    }

	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	// Fill the entire screen with blanks.
	if( !FillConsoleOutputCharacter( hConsole,        // Handle to console screen buffer 
                                    (TCHAR) ' ',     // Character to write to the buffer
                                    dwConSize,       // Number of cells to write 
                                    coordScreen,     // Coordinates of first cell 
                                    &cCharsWritten ))// Receive number of characters written
	{
		return;
	}

	// Get the current text attribute.
	if ( !GetConsoleScreenBufferInfo( hConsole, &csbi ) )
	{
		return;
	}

	// Set the buffer's attributes accordingly.

	if( !FillConsoleOutputAttribute( hConsole,         // Handle to console screen buffer 
                                    csbi.wAttributes, // Character attributes to use
                                    dwConSize,        // Number of cells to set attribute 
                                    coordScreen,      // Coordinates of first cell 
                                    &cCharsWritten )) // Receive number of characters written
	{
		return;
	}
	// Put the cursor at its home coordinates.

	SetConsoleCursorPosition( hConsole, coordScreen );
#else
	printf("%c[2J%c[0;0H", 0x1b, 0x1b);
#endif
}



///////////////////////////////////////////
//  Network support
///////////////////////////////////////////

FORTHOP( clientOp )
{
	const char* pServerStr = (const char*)(SPOP);
	ForthEngine *pEngine = GET_ENGINE;
	int result = ForthClientMainLoop( pEngine, pServerStr, FORTH_SERVER_PORT );
	SPUSH( result );
}

FORTHOP( serverOp )
{
	ForthEngine *pEngine = GET_ENGINE;
	int result = ForthServerMainLoop( pEngine, false, FORTH_SERVER_PORT );
	SPUSH( result );
}

FORTHOP( shutdownOp )
{
    SET_STATE( kResultShutdown );
}


FORTHOP( abortOp )
{
    SET_FATAL_ERROR( kForthErrorAbort );
}

#ifndef ASM_INNER_INTERPRETER


// done is used by the outer interpreter to make the inner interpreter
//  exit after interpreting a single opcode
FORTHOP( doneBop )
{
    SET_STATE( kResultDone );
}

// abort is a user command which causes the entire forth engine to exit,
//   and indicates that a fatal error has occured
FORTHOP( abortBop )
{
    SET_FATAL_ERROR( kForthErrorAbort );
}

FORTHOP( doVariableBop )
{
    // IP points to data field
    SPUSH( (long) GET_IP );
    SET_IP( (long *) (RPOP) );
}

//##############################
//
// math ops
//

FORTHOP( plusBop )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a + b );
}

FORTHOP( minusBop )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a - b );
}

FORTHOP( timesBop )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a * b );
}

FORTHOP( times2Bop )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a << 1 );
}

FORTHOP( times4Bop )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a << 2 );
}

FORTHOP( times8Bop )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a << 3 );
}

FORTHOP( divideBop )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a / b );
}

FORTHOP( divide2Bop )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a >> 1 );
}

FORTHOP( divide4Bop )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a >> 2 );
}

FORTHOP( divide8Bop )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a >> 3 );
}

FORTHOP( divmodBop )
{
    NEEDS(2);
    ldiv_t v;
    long b = SPOP;
    long a = SPOP;
#if defined(WIN32)
#if defined(MSDEV)
    v = div( a, b );
#else
    v = ldiv( a, b );
#endif
#elif defined(LINUX)
    v = ldiv( a, b );
#endif
    SPUSH( v.rem );
    SPUSH( v.quot );
}

FORTHOP( mtimesBop )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    stackInt64 result;
    result.s64 = ((long long) a) * b;
    LPUSH( result );
}

FORTHOP( umtimesBop )
{
    NEEDS(2);
    ulong b = (ulong)(SPOP);
    ulong a = (ulong)(SPOP);
    stackInt64 result;
    result.u64 = ((unsigned long long) a) * b;
    LPUSH( result );
}

#if 0
FORTHOP( ummodBop )
{
    NEEDS(2);
    ldiv_t v;
    unsigned long b = SPOP;
    stackInt64 a;
    LPOP( a );
    unsigned long quotient = a.64 / b;
    unsigned long remainder = a.64 % b;
    SPUSH( remainder );
    SPUSH( quotient );
}

FORTHOP( smdivremBop )
{
    NEEDS(2);
    lldiv_t v;
    long long b = (long long) SPOP;
    stackInt64 a;
    LPOP( a );

    v = lldiv(a.s64, b);

    SPUSH( (long) v.rem );
    SPUSH( (long) v.quot );
}
#endif

FORTHOP( modBop )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a % b );
}

FORTHOP( negateBop )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( -a );
}

FORTHOP( fplusBop )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( a + b );
}

FORTHOP( fminusBop )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( a - b );
}

FORTHOP( ftimesBop )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( a * b );
}

FORTHOP( fdivideBop )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( a / b );
}


FORTHOP( dplusBop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a + b );
}

FORTHOP( dminusBop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a - b );
}

FORTHOP( dtimesBop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a * b );
}

FORTHOP( ddivideBop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a / b );
}


FORTHOP( dsinBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( sin(a) );
}

FORTHOP( dasinBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( asin(a) );
}

FORTHOP( dcosBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( cos(a) );
}

FORTHOP( dacosBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( acos(a) );
}

FORTHOP( dtanBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( tan(a) );
}

FORTHOP( datanBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( atan(a) );
}

FORTHOP( datan2Bop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( atan2(a, b) );
}

FORTHOP( dexpBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( exp(a) );
}

FORTHOP( dlnBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( log(a) );
}

FORTHOP( dlog10Bop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( log10(a) );
}

FORTHOP( dpowBop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( pow(a, b) );
}

FORTHOP( dsqrtBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( sqrt(a) );
}

FORTHOP( dceilBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( ceil(a) );
}

FORTHOP( dfloorBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( floor(a) );
}

FORTHOP( dabsBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( fabs(a) );
}

FORTHOP( dldexpBop )
{
    NEEDS(3);
    int n = SPOP;
    double a = DPOP;
    DPUSH( ldexp(a, n) );
}

FORTHOP( dfrexpBop )
{
    NEEDS(2);
    int n;
    double a = DPOP;
    DPUSH( frexp( a, &n ) );
    SPUSH( n );
}

FORTHOP( dmodfBop )
{
    NEEDS(2);
    double b;
    double a = DPOP;
    a = modf( a, &b );
    DPUSH( b );
    DPUSH( a );
}

FORTHOP( dfmodBop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( fmod( a, b ) );
}

FORTHOP( fsinBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( sinf(a) );
}

FORTHOP( fasinBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( asinf(a) );
}

FORTHOP( fcosBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( cosf(a) );
}

FORTHOP( facosBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( acosf(a) );
}

FORTHOP( ftanBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( tanf(a) );
}

FORTHOP( fatanBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( atanf(a) );
}

FORTHOP( fatan2Bop )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( atan2f(a, b) );
}

FORTHOP( fexpBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( expf(a) );
}

FORTHOP( flnBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( logf(a) );
}

FORTHOP( flog10Bop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( log10f(a) );
}

FORTHOP( fpowBop )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( powf(a, b) );
}

FORTHOP( fsqrtBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( sqrtf(a) );
}

FORTHOP( fceilBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( ceilf(a) );
}

FORTHOP( ffloorBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( floorf(a) );
}

FORTHOP( fabsBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( fabs(a) );
}

FORTHOP( fldexpBop )
{
    NEEDS(3);
    int n = SPOP;
    float a = FPOP;
    FPUSH( ldexpf(a, n) );
}

FORTHOP( ffrexpBop )
{
    NEEDS(1);
    int n;
    float a = FPOP;
    FPUSH( frexpf( a, &n ) );
    SPUSH( n );
}

FORTHOP( fmodfBop )
{
    NEEDS(1);
    float b;
    float a = FPOP;
    a = modff( a, &b );
    FPUSH( b );
    FPUSH( a );
}

FORTHOP( ffmodBop )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( fmodf( a, b ) );
}

FORTHOP( lplusBop )
{
	NEEDS(4);
	stackInt64 a;
	stackInt64 b;
	stackInt64 result;
	LPOP( b );
	LPOP( a );
	result.s64 = a.s64 + b.s64;
	LPUSH( result );
}

FORTHOP( lminusBop )
{
	NEEDS(4);
	stackInt64 a;
	stackInt64 b;
	stackInt64 result;
	LPOP( b );
	LPOP( a );
	result.s64 = a.s64 - b.s64;
    LPUSH(result);
}

FORTHOP( ltimesBop )
{
	NEEDS(4);
	stackInt64 a;
	stackInt64 b;
	stackInt64 result;
	LPOP( b );
	LPOP( a );
	result.s64 = a.s64 * b.s64;
	LPUSH( result );
}

FORTHOP( i2fBop )
{
    NEEDS(1);
    float a = (float) SPOP;
    FPUSH( a );
}

FORTHOP( i2dBop )
{
    NEEDS(1);
    double a = SPOP;
    DPUSH( a );
}

FORTHOP( f2iBop )
{
    NEEDS(1);
    int a = (int) FPOP;
    SPUSH( a );
}

FORTHOP( f2dBop )
{
    NEEDS(1);
    double a = FPOP;
    DPUSH( a );
}

FORTHOP( d2iBop )
{
    NEEDS(2);
    int a = (int) DPOP;
    SPUSH( a );
}

FORTHOP( d2fBop )
{
    NEEDS(2);
    float a = (float) DPOP;
    FPUSH( a );
}

//##############################
//
// control ops
//
// TODO: replace branch, tbranch, fbranch with immediate ops
// exit normal op with no local vars
FORTHOP(doExitBop)
{
    // rstack: oldIP
    if ( GET_RDEPTH < 1 )
    {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    }
    else
    {
        SET_IP( (long *) RPOP );
    }
}

// exit normal op with local vars
FORTHOP(doExitLBop)
{
    // rstack: local_var_storage oldFP oldIP
    // FP points to oldFP
    SET_RP( GET_FP );
    SET_FP( (long *) (RPOP) );
    if ( GET_RDEPTH < 1 )
    {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    }
    else
    {
        SET_IP( (long *) RPOP );
    }
}

// exit method op with no local vars
FORTHOP(doExitMBop)
{
    // rstack: oldIP oldTP
    if ( GET_RDEPTH < 3 )
    {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    }
    else
    {
        SET_IP( (long *) RPOP );
        SET_TPM( (long *) RPOP );
        SET_TPD( (long *) RPOP );
    }
}

// exit method op with local vars
FORTHOP(doExitMLBop)
{
    // rstack: local_var_storage oldFP oldIP oldTP
    // FP points to oldFP
    SET_RP( GET_FP );
    SET_FP( (long *) (RPOP) );
    if ( GET_RDEPTH < 3 )
    {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    }
    else
    {
        SET_IP( (long *) RPOP );
        SET_TPM( (long *) RPOP );
        SET_TPD( (long *) RPOP );
    }
}

FORTHOP(callBop)
{
    RPUSH( (long) GET_IP );
    SET_IP( (long *) SPOP );
}

FORTHOP(gotoBop)
{
    SET_IP( (long *) SPOP );
}

FORTHOP(doDoBop)
{
    NEEDS(2);
    long startIndex = SPOP;
    // skip over loop exit IP right after this op
    long *newIP = (GET_IP) + 1;
    SET_IP( newIP );

    RPUSH( (long) newIP );
    RPUSH( SPOP );
    RPUSH( startIndex );
    // top of rstack is current index, next is end index,
    //  next is looptop IP
}

FORTHOP(doCheckDoBop)
{
    NEEDS(2);
    long startIndex = SPOP;
    long endIndex = SPOP;
    // skip over loop exit IP right after this op
	if ( startIndex < endIndex)
	{
	    long *newIP = (GET_IP + 1);
		RPUSH( (long) newIP );
		RPUSH( endIndex );
		RPUSH( startIndex );
		// top of rstack is current index, next is end index,
		//  next is looptop IP
	    SET_IP( newIP );
	}
}

FORTHOP(doLoopBop)
{
    RNEEDS(3);

    long *pRP = GET_RP;
    long newIndex = (*pRP) + 1;

    if ( newIndex >= pRP[1] )
    {
        // loop has ended, drop end, current indices, loopIP
        SET_RP( pRP + 3 );
    }
    else
    {
        // loop again
        *pRP = newIndex;
        SET_IP( (long *) (pRP[2]) );
    }
}

FORTHOP(doLoopNBop)
{
    RNEEDS(3);
    NEEDS(1);

    long *pRP = GET_RP;
    long increment = SPOP;
    long newIndex = (*pRP) + increment;
    bool done;

    done = (increment > 0) ? (newIndex >= pRP[1]) : (newIndex < pRP[1]);
    if ( done )
    {
        // loop has ended, drop end, current indices, loopIP
        SET_RP( pRP + 3 );
    }
    else
    {
        // loop again
        *pRP = newIndex;
        SET_IP( (long *) (pRP[2]) );
    }
}

FORTHOP(iBop)
{
    SPUSH( *(GET_RP) );
}

FORTHOP(jBop)
{
    SPUSH( GET_RP[3] );
}


// discard stuff "do" put on rstack in preparation for an "exit"
//   of the current op from inside a do loop
FORTHOP( unloopBop )
{
    RNEEDS(3);
    // loop has ended, drop end, current indices, loopIP
    SET_RP( GET_RP + 3 );
}


// exit loop immediately
FORTHOP( leaveBop )
{
    RNEEDS(3);
    long *pRP = GET_RP;
    long *newIP = (long *) (pRP[2]) - 1;

    // loop has ended, drop end, current indices, loopIP
    SET_RP( pRP + 3 );
    // point IP back to the branch op just after the _do opcode
    SET_IP( newIP );
}

///////////////////////////////////////////
//  bit-vector logic ops
///////////////////////////////////////////
FORTHOP( orBop )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a | b );
}

FORTHOP( andBop )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a & b );
}

FORTHOP( xorBop )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a ^ b );
}

FORTHOP( invertBop )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ~a );
}

FORTHOP( lshiftBop )
{
    NEEDS(2);
    unsigned long b = SPOP;
    unsigned long a = SPOP;
    SPUSH( a << b );
}

FORTHOP( rshiftBop )
{
    NEEDS(2);
    unsigned long b = SPOP;
    unsigned long a = SPOP;
    SPUSH( a >> b );
}


FORTHOP( arshiftBop )
{
    NEEDS(2);
    unsigned long b = SPOP;
    long a = SPOP;
    SPUSH( a >> b );
}


///////////////////////////////////////////
//  boolean ops
///////////////////////////////////////////
FORTHOP( notBop )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ~a );
}

FORTHOP( trueBop )
{
    SPUSH( ~0 );
}

FORTHOP( falseBop )
{
    SPUSH( 0 );
}

FORTHOP( nullBop )
{
    SPUSH( 0 );
}

FORTHOP( dnullBop )
{
    SPUSH( 0 );
    SPUSH( 0 );
}

//##############################
//
// integer comparison ops
//

FORTHOP(equalsBop)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a == b ) ? -1L : 0 );
}

FORTHOP(notEqualsBop)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a != b ) ? -1L : 0 );
}

FORTHOP(greaterThanBop)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(greaterEqualsBop)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a >= b ) ? -1L : 0 );
}

FORTHOP(lessThanBop)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(lessEqualsBop)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a <= b ) ? -1L : 0 );
}

FORTHOP(equals0Bop)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a == 0 ) ? -1L : 0 );
}

FORTHOP(notEquals0Bop)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a != 0 ) ? -1L : 0 );
}

FORTHOP(greaterThan0Bop)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a > 0 ) ? -1L : 0 );
}

FORTHOP(greaterEquals0Bop)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a >= 0 ) ? -1L : 0 );
}

FORTHOP(lessThan0Bop)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a < 0 ) ? -1L : 0 );
}

FORTHOP(lessEquals0Bop)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a <= 0 ) ? -1L : 0 );
}

FORTHOP(unsignedGreaterThanBop)
{
    NEEDS(2);
    ulong b = (ulong) SPOP;
    ulong a = (ulong) SPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(unsignedLessThanBop)
{
    NEEDS(2);
    ulong b = (ulong) SPOP;
    ulong a = (ulong) SPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(withinBop)
{
    NEEDS(3);
    long hiLimit = SPOP;
    long loLimit = SPOP;
    long val = SPOP;
    SPUSH( ( (loLimit <= val) && (val < hiLimit) ) ? -1L : 0 );
}

FORTHOP(minBop)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a < b ) ? a : b );
}

FORTHOP(maxBop)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a > b ) ? a : b );
}

FORTHOP(icmpBop)
{
	NEEDS(2);
	long b = SPOP;
	long a = SPOP;
	SPUSH( (a == b) ? 0 : (( a > b ) ? 1 : -1 ) );
}

FORTHOP(uicmpBop)
{
	NEEDS(2);
	ulong b = (ulong) SPOP;
	ulong a = (ulong) SPOP;
	SPUSH( (a == b) ? 0 : (( a > b ) ? 1 : -1 ) );
}


//##############################
//
// single-precision fp comparison ops
//

FORTHOP(fEqualsBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a == b ) ? -1L : 0 );
}

FORTHOP(fNotEqualsBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a != b ) ? -1L : 0 );
}

FORTHOP(fGreaterThanBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(fGreaterEqualsBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a >= b ) ? -1L : 0 );
}

FORTHOP(fLessThanBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(fLessEqualsBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a <= b ) ? -1L : 0 );
}

FORTHOP(fEquals0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a == 0.0f ) ? -1L : 0 );
}

FORTHOP(fNotEquals0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a != 0.0f ) ? -1L : 0 );
}

FORTHOP(fGreaterThan0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a > 0.0f ) ? -1L : 0 );
}

FORTHOP(fGreaterEquals0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a >= 0.0f ) ? -1L : 0 );
}

FORTHOP(fLessThan0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a < 0.0f ) ? -1L : 0 );
}

FORTHOP(fLessEquals0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a <= 0.0f ) ? -1L : 0 );
}

FORTHOP(fWithinBop)
{
    NEEDS(3);
    float hiLimit = FPOP;
    float loLimit = FPOP;
    float val = FPOP;
    SPUSH( ( (loLimit <= val) && (val < hiLimit) ) ? -1L : 0 );
}

FORTHOP(fMinBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( ( a < b ) ? a : b );
}

FORTHOP(fMaxBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( ( a > b ) ? a : b );
}

FORTHOP(fcmpBop)
{
	NEEDS(2);
	float b = FPOP;
	float a = FPOP;
	SPUSH( (a == b) ? 0 : (( a > b ) ? 1 : -1 ) );
}

//##############################
//
// double-precision fp comparison ops
//

FORTHOP(dEqualsBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a == b ) ? -1L : 0 );
}

FORTHOP(dNotEqualsBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a != b ) ? -1L : 0 );
}

FORTHOP(dGreaterThanBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(dGreaterEqualsBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a >= b ) ? -1L : 0 );
}

FORTHOP(dLessThanBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(dLessEqualsBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a <= b ) ? -1L : 0 );
}

FORTHOP(dEquals0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a == 0.0 ) ? -1L : 0 );
}

FORTHOP(dNotEquals0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a != 0.0 ) ? -1L : 0 );
}

FORTHOP(dGreaterThan0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a > 0.0 ) ? -1L : 0 );
}

FORTHOP(dGreaterEquals0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a >= 0.0 ) ? -1L : 0 );
}

FORTHOP(dLessThan0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a < 0.0 ) ? -1L : 0 );
}

FORTHOP(dLessEquals0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a <= 0.0 ) ? -1L : 0 );
}

FORTHOP(dWithinBop)
{
    NEEDS(3);
    double hiLimit = DPOP;
    double loLimit = DPOP;
    double val = DPOP;
    SPUSH( ( (loLimit <= val) && (val < hiLimit) ) ? -1L : 0 );
}

FORTHOP(dMinBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    DPUSH( ( a < b ) ? a : b );
}

FORTHOP(dMaxBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    DPUSH( ( a > b ) ? a : b );
}

FORTHOP(dcmpBop)
{
	NEEDS(2);
	double b = DPOP;
	double a = DPOP;
	SPUSH( (a == b) ? 0 : (( a > b ) ? 1 : -1 ) );
}

//##############################
//
// stack manipulation
//

FORTHOP(rpushBop)
{
    NEEDS(1);
    RPUSH( SPOP );
}

FORTHOP(rpopBop)
{
    RNEEDS(1);
    SPUSH( RPOP );
}

FORTHOP(rpeekBop)
{
    SPUSH( *(GET_RP) );
}

FORTHOP(rdropBop)
{
    RNEEDS(1);
    RPOP;
}

FORTHOP(rpBop)
{
	intVarAction( pCore, (int *)&(pCore->RP) );
}

FORTHOP(r0Bop)
{
    long pR0 = (long) (pCore->RT);
    SPUSH( pR0 );
}

FORTHOP(dupBop)
{
    NEEDS(1);
    long a = *(GET_SP);
    SPUSH( a );
}

FORTHOP(checkDupBop)
{
    NEEDS(1);
    long a = *(GET_SP);
    if ( a != 0 )
    {
        SPUSH( a );
    }
}

FORTHOP(swapBop)
{
    NEEDS(2);
    long a = *(GET_SP);
    *(GET_SP) = (GET_SP)[1];
    (GET_SP)[1] = a;
}

FORTHOP(dropBop)
{
    NEEDS(1);
    SPOP;
}

FORTHOP(overBop)
{
    NEEDS(1);
    long a = (GET_SP)[1];
    SPUSH( a );
}

FORTHOP(rotBop)
{
    NEEDS(3);
    int a, b, c;
    long *pSP = GET_SP;
    a = pSP[2];
    b = pSP[1];
    c = pSP[0];
    pSP[2] = b;
    pSP[1] = c;
    pSP[0] = a;
}

FORTHOP(reverseRotBop)
{
    NEEDS(3);
    int a, b, c;
    long *pSP = GET_SP;
    a = pSP[2];
    b = pSP[1];
    c = pSP[0];
    pSP[2] = c;
    pSP[1] = a;
    pSP[0] = b;
}

FORTHOP(nipBop)
{
    NEEDS(2);
    long a = SPOP;
    SPOP;
    SPUSH( a );
}

FORTHOP(tuckBop)
{
    NEEDS(2);
    long *pSP = GET_SP;
    long a = *pSP;
    long b = pSP[1];
    SPUSH( a );
    *pSP = b;
    pSP[1] = a;
}

FORTHOP(pickBop)
{
    NEEDS(1);
    long *pSP = GET_SP;
    long n = *pSP;
    long a = pSP[n + 1];
    *pSP = a;
}

FORTHOP(rollBop)
{
    // TODO: moves the Nth element to TOS
    // 1 roll is the same as swap
    // 2 roll is the same as rot
    long n = (SPOP);
    long *pSP = GET_SP;
    long a;
	if ( n != 0 )
	{
		if ( n > 0 )
		{
			a = pSP[n];
			for ( int i = n; i != 0; i-- )
			{
				pSP[i] = pSP[i - 1];
			}
			*pSP = a;
		}
		else
		{
			a = *pSP;
			n = -n;
			for ( int i = 0; i < n; i++ )
			{
				pSP[i] = pSP[i + 1];
			}
			pSP[n] = a;
		}
	}
}

FORTHOP(spBop)
{
    ulong varOp = GET_VAR_OPERATION;
    intVarAction(pCore, (int *)&(pCore->SP));
    if ((varOp == kVarDefaultOp) || (varOp == kVarFetch))
    {
        *(pCore->SP) += sizeof(long);
    }
}

FORTHOP(s0Bop)
{
    long pS0 = (long) (pCore->ST);
    SPUSH( pS0 );
}

FORTHOP(fpBop)
{
	intVarAction( pCore, (int *)&(pCore->FP) );
}

FORTHOP(ipBop)
{
	intVarAction( pCore, (int *)&(pCore->IP) );
}

FORTHOP(ddupBop)
{
    NEEDS(2);
    double *pDsp = (double *)(GET_SP);
    DPUSH( *pDsp );
}

FORTHOP(dswapBop)
{
    NEEDS(4);
    double *pDsp = (double *)(GET_SP);
    double a = *pDsp;
    *pDsp = pDsp[1];
    pDsp[1] = a;
}

FORTHOP(ddropBop)
{
    NEEDS(2);
    SET_SP( GET_SP + 2 );
}

FORTHOP(doverBop)
{
    NEEDS(2);
    double *pDsp = (double *)(GET_SP);
    DPUSH( pDsp[1] );
}

FORTHOP(drotBop)
{
    NEEDS(3);
    double *pDsp = (double *)(GET_SP);
    double a, b, c;
    a = pDsp[2];
    b = pDsp[1];
    c = pDsp[0];
    pDsp[2] = b;
    pDsp[1] = c;
    pDsp[0] = a;
}

#if 0
FORTHOP(dreverseRotBop)
{
    NEEDS(3);
    double *pDsp = (double *)(GET_SP);
    double a, b, c;
    a = pDsp[2];
    b = pDsp[1];
    c = pDsp[0];
    pDsp[2] = c;
    pDsp[1] = a;
    pDsp[0] = b;
}

FORTHOP(dnipBop)
{
    NEEDS(2);
    double a = DPOP;
    DPOP;
    DPUSH( a );
}

FORTHOP(dtuckBop)
{
    NEEDS(2);
    double *pDsp = (double *)(GET_SP);
    double a = *pDsp;
    double b = pDsp[1];
    DPUSH( a );
    *pDsp = b;
    pDsp[1] = a;
}

FORTHOP(dpickBop)
{
    NEEDS(1);
    long n = SPOP;
    double *pDsp = (double *)(GET_SP);
    double a = pDsp[n];
	DPUSH( a );
}

FORTHOP(drollBop)
{
    // TODO: moves the Nth element to TOS
    // 1 droll is the same as dswap
    // 2 droll is the same as drot
    long n = (SPOP);
    double *pDsp = (double *)(GET_SP);
    double a;
	if ( n != 0 )
	{
		if ( n > 0 )
		{
			a = pDsp[n];
			for ( int i = n; i != 0; i-- )
			{
				pDsp[i] = pDsp[i - 1];
			}
			*pDsp = a;
		}
		else
		{
			a = *pDsp;
			n = -n;
			for ( int i = 0; i < n; i++ )
			{
				pDsp[i] = pDsp[i + 1];
			}
			pDsp[n] = a;
		}
	}
}
#endif

FORTHOP(startTupleBop)
{
    long pSP = (long) (GET_SP);
    RPUSH( pSP );
}

FORTHOP(endTupleBop)
{
    long* pSP = (GET_SP);
    long* pOldSP = (long *) (RPOP);
    long count = pOldSP - pSP;
    SPUSH( count );
}

#if 0
FORTHOP(ndropBop)
{
    NEEDS(1);
	int n = SPOP;
    SET_SP( GET_SP + n );
}

FORTHOP(ndupBop)
{
    NEEDS(1);
	int n = (SPOP) - 1;
    long* pSP = (GET_SP) + n;
	for ( int i = 0; i <= n; i++ )
	{
		long v = *pSP--;
		SPUSH( v );
	}
}

FORTHOP(npickBop)
{
    NEEDS(1);
	int n = (SPOP) - 1;
	int offset = (SPOP);
    long* pSP = (GET_SP) + offset + n;
	for ( int i = 0; i <= n; i++ )
	{
		long v = *pSP--;
		SPUSH( v );
	}
	/*
	for ( int i = 0; i <= n; i++ )
	{
      long* pSP = (GET_SP);
	  SPUSH( pSP[offset] );
	}
	*/
}
#endif

//##############################
//
// loads & stores
//

FORTHOP(storeBop)
{
    NEEDS(2);
    long *pB = (long *)(SPOP); 
    long a = SPOP;
    *pB = a;
}

FORTHOP(fetchBop)
{
    NEEDS(1);
    long *pA = (long *)(SPOP); 
    SPUSH( *pA );
}

FORTHOP(storeNextBop)
{
    NEEDS(2);
    long **ppB = (long **)(SPOP);
    long *pB = *ppB;
    long a = SPOP;
    *pB++ = a;
    *ppB = pB;
}

FORTHOP(fetchNextBop)
{
    NEEDS(1);
    long **ppA = (long **)(SPOP); 
    long *pA = *ppA;
    long a = *pA++;
    SPUSH( a );
    *ppA = pA;
}

FORTHOP(cstoreBop)
{
    NEEDS(2);
    char *pB = (char *) (SPOP);
    long a = SPOP;
    *pB = (char) a;
}

FORTHOP(cfetchBop)
{
    NEEDS(1);
    unsigned char *pA = (unsigned char *) (SPOP);
    SPUSH( (*pA) );
}

FORTHOP(cstoreNextBop)
{
    NEEDS(2);
    char **ppB = (char **)(SPOP);
    char *pB = *ppB;
    long a = SPOP;
    *pB++ = (char) a;
    *ppB = pB;
}

FORTHOP(cfetchNextBop)
{
    NEEDS(1);
    unsigned char **ppA = (unsigned char **)(SPOP); 
    unsigned char *pA = *ppA;
    unsigned char a = *pA++;
    SPUSH( a );
    *ppA = pA;
}

// signed char fetch
FORTHOP(scfetchBop)
{
    NEEDS(1);
    signed char *pA = (signed char *) (SPOP);
    SPUSH( (*pA) );
}

FORTHOP(c2iBop)
{
    NEEDS(1);
    signed char a = (signed char) (SPOP);
	long b = (long) a;
    SPUSH( b );
}

FORTHOP(wstoreBop)
{
    NEEDS(2);
    short *pB = (short *) (SPOP);
    long a = SPOP;
    *pB = (short) a;
}

FORTHOP(wfetchBop)
{
    NEEDS(1);
    unsigned short *pA = (unsigned short *) (SPOP);
    SPUSH( *pA );
}

FORTHOP(wstoreNextBop)
{
    NEEDS(2);
    short **ppB = (short **)(SPOP);
    short *pB = *ppB;
    long a = SPOP;
    *pB++ = (short) a;
    *ppB = pB;
}

FORTHOP(wfetchNextBop)
{
    NEEDS(1);
    unsigned short **ppA = (unsigned short **)(SPOP); 
    unsigned short *pA = *ppA;
    unsigned short a = *pA++;
    SPUSH( a );
    *ppA = pA;
}

// signed word fetch
FORTHOP(swfetchBop)
{
    NEEDS(1);
    signed short *pA = (signed short *) (SPOP);
    SPUSH( *pA );
}

FORTHOP(w2iBop)
{
    NEEDS(1);
    short a = (short) (SPOP);
	long b = (long) a;
    SPUSH( b );
}

FORTHOP(dstoreBop)
{
	NEEDS(3);
	double *pB = (double *) (SPOP); 
	double a= DPOP;
	*pB = a;
}

FORTHOP(dfetchBop)
{
	NEEDS(1);
	double *pA = (double *) (SPOP);
	DPUSH( *pA );
}

FORTHOP(dstoreNextBop)
{
	NEEDS(2);
	double **ppB = (double **)(SPOP);
	double *pB = *ppB;
	double a = DPOP;
	*pB++ = a;
	*ppB = pB;
}

FORTHOP(dfetchNextBop)
{
	NEEDS(1);
	double **ppA = (double **)(SPOP); 
	double *pA = *ppA;
	double a = *pA++;
	DPUSH( a );
	*ppA = pA;
}

/*FORTHOP(lstoreBop)
{
    NEEDS(3);
	long long *pB = (long long *) (SPOP);
	stackInt64 a;
	LPOP(a);
    *pB = a.s64;
}

FORTHOP(lfetchBop)
{
    NEEDS(1);
	long long *pA = (long long *) (SPOP);
	stackInt64 b;
	b.s64 = *pA;
    LPUSH( b );
}

FORTHOP(lstoreNextBop)
{
    NEEDS(2);
	long long **ppB = (long long **) (SPOP);
    long long *pB = *ppB;
	stackInt64 a;
	LPOP(a);
    *pB++ = a.s64;
    *ppB = pB;
}

FORTHOP(lfetchNextBop)
{
    NEEDS(1);
	long long **ppA = (long long **) (SPOP);
	long long *pA = *ppA;
	stackInt64 a;
	a.s64 = *pA++;
	LPUSH( a );
    *ppA = pA;
}
*/

FORTHOP(ostoreBop)
{
	NEEDS(3);
	ForthObject *pB = (ForthObject *) (SPOP); 
	ForthObject a;
	POP_OBJECT(a);
	*pB = a;
}

FORTHOP(ofetchBop)
{
	NEEDS(1);
	ForthObject *pA = (ForthObject *) (SPOP); 
	PUSH_OBJECT(*pA);
}

FORTHOP(ostoreNextBop)
{
	NEEDS(2);
	ForthObject **ppB = (ForthObject **) (SPOP); 
	ForthObject *pB = *ppB;
	ForthObject a;
	POP_OBJECT(a);
	*pB++ = a;
	*ppB = pB;
}

FORTHOP(ofetchNextBop)
{
	NEEDS(1);
	ForthObject **ppA = (ForthObject **) (SPOP); 
	ForthObject *pA = *ppA;
	ForthObject a = *pA++;
	PUSH_OBJECT(a);
	*ppA = pA;
}

FORTHOP(memcpyBop)
{
    NEEDS(3);
    size_t nBytes = (size_t) (SPOP);
    const void* pSrc = (const void *) (SPOP);
    void* pDst = (void *) (SPOP);
    memcpy( pDst, pSrc, nBytes );
}

FORTHOP(moveBop)
{
    NEEDS(3);
    size_t nBytes = (size_t) (SPOP);
    void* pDst = (void *) (SPOP);
    const void* pSrc = (const void *) (SPOP);
    memmove( pDst, pSrc, nBytes );
}

FORTHOP(fillBop)
{
    NEEDS(3);
    int byteVal = (int) (SPOP);
    size_t nBytes = (size_t) (SPOP);
    void* pDst = (void *) (SPOP);
    memset( pDst, byteVal, nBytes );
}

FORTHOP(intoBop)
{
    SET_VAR_OPERATION( kVarStore );
}

FORTHOP(addToBop)
{
    SET_VAR_OPERATION( kVarPlusStore );
}

FORTHOP(subtractFromBop)
{
    SET_VAR_OPERATION( kVarMinusStore );
}

FORTHOP(refBop)
{
    SET_VAR_OPERATION( kVarRef );
}

FORTHOP(setVarActionBop)
{
    SET_VAR_OPERATION( SPOP );
}

FORTHOP(getVarActionBop)
{
    SPUSH( GET_VAR_OPERATION );
}

//##############################
//
// string ops
//

FORTHOP( strcpyBop )
{
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strcpy( pDst, pSrc );
}

FORTHOP( strncpyBop )
{
	size_t maxBytes = (size_t) SPOP;
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strncpy( pDst, pSrc, maxBytes );
}

FORTHOP( strlenBop )
{
    char *pSrc = (char *) SPOP;
    SPUSH(  strlen( pSrc ) );
}

FORTHOP( strcatBop )
{
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strcat( pDst, pSrc );
}

FORTHOP( strncatBop )
{
	size_t maxBytes = (size_t) SPOP;
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strncat( pDst, pSrc, maxBytes );
}


FORTHOP( strchrBop )
{
    int c = (int) SPOP;
    char *pStr = (char *) SPOP;
    SPUSH( (long) strchr( pStr, c ) );
}

FORTHOP( strrchrBop )
{
    int c = (int) SPOP;
    char *pStr = (char *) SPOP;
    SPUSH( (long) strrchr( pStr, c ) );
}

FORTHOP( strcmpBop )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
	int result = strcmp( pStr1, pStr2 );
	// only return 1, 0 or -1
	if ( result != 0 )
	{
		result = (result > 0) ? 1 : -1;
	}
	SPUSH( (long) result );
}

FORTHOP( stricmpBop )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
#if defined(WIN32)
	int result = stricmp( pStr1, pStr2 );
#elif defined(LINUX)
	int result = strcasecmp( pStr1, pStr2 );
#endif
	// only return 1, 0 or -1
	if ( result != 0 )
	{
		result = (result > 0) ? 1 : -1;
	}
	SPUSH( (long) result );
}


FORTHOP( strstrBop )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
    SPUSH( (long) strstr( pStr1, pStr2 ) );
}


FORTHOP( strtokBop )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
    SPUSH( (long) strtok( pStr1, pStr2 ) );
}

FORTHOP( initStringBop )
{
    long len;
    long* pStr;

    // TOS: maximum length, ptr to first char
    len = SPOP;
	pStr = (long *) (SPOP);
	pStr[-2] = len;
    pStr[-1] = 0;
    *((char *) pStr) = 0;
}

FORTHOP( strFixupBop )
{
    long *pSrcLongs = (long *) SPOP;
    char* pSrc = (char *) pSrcLongs;
    pSrc[pSrcLongs[-2]] = '\0';
    int len = strlen( pSrc );
    if (len > pSrcLongs[-2])
    {
        // characters have been written past string storage end
        SET_ERROR( kForthErrorStringOverflow );
    }
    else
    {
        pSrcLongs[-1] = len;
    }
}

// push the immediately following literal 32-bit constant
FORTHOP(litBop)
{
    long *pV = GET_IP;
    SET_IP( pV + 1 );
    SPUSH( *pV );
}

// push the immediately following literal 64-bit constant
FORTHOP(dlitBop)
{
    double *pV = (double *) GET_IP;
    DPUSH( *pV++ );
    SET_IP( (long *) pV );
}

// doDoes
// - does not have precedence
// - is executed while executing the user word
// - it gets the parameter address of the user word
// 
FORTHOP( doDoesBop )
{
    SPUSH( RPOP );
}

FORTHOP( doConstantBop )
{
    // IP points to data field
    SPUSH( *GET_IP );
    SET_IP( (long *) (RPOP) );
}

FORTHOP( doDConstantBop )
{
    // IP points to data field
    DPUSH( *((double *) (GET_IP)) );
    SET_IP( (long *) (RPOP) );
}

FORTHOP( thisBop )
{
    SPUSH( ((long) GET_TPD) );
    SPUSH( ((long) GET_TPM) );
}

FORTHOP( thisDataBop )
{
    SPUSH( ((long) GET_TPD) );
}

FORTHOP( thisMethodsBop )
{
    SPUSH( ((long) GET_TPM) );
}

// doStructOp is compiled at the start of each user-defined global structure instance
FORTHOP( doStructBop )
{
    // IP points to data field
    SPUSH( (long) GET_IP );
    SET_IP( (long *) (RPOP) );
}

// doStructArrayOp is compiled as the only op of each user-defined global structure array instance
// the word after this op is the padded element length
// after that word is storage for structure 0 of the array
FORTHOP( doStructArrayBop )
{
    // IP points to data field
    long *pA = GET_IP;
    long nBytes = (*pA++) * SPOP;

    SPUSH( ((long) pA) + nBytes );
    SET_IP( (long *) (RPOP) );
}

FORTHOP( executeBop )
{
	long op = SPOP;
    ForthEngine *pEngine = GET_ENGINE;
	pEngine->ExecuteOneOp( op );
#if 0
    long prog[2];
    long *oldIP = GET_IP;

	// execute the opcode
    prog[0] = SPOP;
    prog[1] = gCompiledOps[OP_DONE] ;
    
    SET_IP( prog );
    InnerInterpreter( pCore );
    SET_IP( oldIP );
#endif
}

//##############################
//
//  File ops
//
FORTHOP( fopenBop )
{
    NEEDS(2);
    char *pAccess = (char *) SPOP;
    char *pName = (char *) SPOP;

    FILE *pFP = pCore->pFileFuncs->fileOpen( pName, pAccess );
    SPUSH( (long) pFP );
}

FORTHOP( fcloseBop )
{
    NEEDS(1);
    
    int result = pCore->pFileFuncs->fileClose( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( fseekBop )
{
    int ctrl = SPOP;
    int offset = SPOP;
    FILE *pFP = (FILE *) SPOP;
    
    int result = pCore->pFileFuncs->fileSeek( pFP, offset, ctrl );
    SPUSH( result );
}

FORTHOP( freadBop )
{
    NEEDS(4);
    
    FILE *pFP = (FILE *) SPOP;
    int numItems = SPOP;
    int itemSize = SPOP;
    void *pDst = (void *) SPOP;
    
    int result = pCore->pFileFuncs->fileRead( pDst, itemSize, numItems, pFP );
    SPUSH( result );
}

FORTHOP( fwriteBop )
{
    
    NEEDS(4);
    FILE *pFP = (FILE *) SPOP;
    int numItems = SPOP;
    int itemSize = SPOP;
    void *pSrc = (void *) SPOP;
    
    int result = pCore->pFileFuncs->fileWrite( pSrc, itemSize, numItems, pFP );
    SPUSH( result );
}

FORTHOP( fgetcBop )
{    
    NEEDS(1);
    FILE *pFP = (FILE *) SPOP;
    
    int result = pCore->pFileFuncs->fileGetChar( pFP );
    SPUSH( result );
}

FORTHOP( fputcBop )
{    
    NEEDS(2);
    FILE *pFP = (FILE *) SPOP;
    int outChar = SPOP;
    
    int result = pCore->pFileFuncs->filePutChar( outChar, pFP );
    SPUSH( result );
}

FORTHOP( feofBop )
{
    NEEDS(1);

    int result = pCore->pFileFuncs->fileAtEnd( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( fexistsBop )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->fileExists( (const char*) SPOP );
    SPUSH( result );
}

FORTHOP( ftellBop )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->fileTell( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( flenBop )
{
    NEEDS(1);
    FILE* pFile = (FILE *) SPOP;
    int result = pCore->pFileFuncs->fileGetLength( pFile );
    SPUSH( result );
}

FORTHOP( fgetsBop )
{
    NEEDS( 3 );
    FILE* pFile = (FILE *) SPOP;
    int maxChars = SPOP;
    char* pBuffer = (char *) SPOP;
    int result = (int) pCore->pFileFuncs->fileGetString( pBuffer, maxChars, pFile );
    SPUSH( result );
}

FORTHOP( fputsBop )
{
    NEEDS( 2 );
    FILE* pFile = (FILE *) SPOP;
    char* pBuffer = (char *) SPOP;
    int result = pCore->pFileFuncs->filePutString( pBuffer, pFile );
    SPUSH( result );
}

FORTHOP( hereBop )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pChar = (char *)GET_DP;
    SPUSH( (long) pChar );
	CLEAR_VAR_OPERATION;
}

FORTHOP( archARMBop )
{
#ifdef X86_ARCHITECTURE
    SPUSH( 0 );
#else
    SPUSH( ~0 );
#endif
}

FORTHOP( archX86Bop )
{
#ifdef X86_ARCHITECTURE
    SPUSH( ~0 );
#else
    SPUSH( 0 );
#endif
}

FORTHOP( oclearBop )
{
    SET_VAR_OPERATION( kVarStore );
	DPUSH( 0L );
}

#endif


// NOTE: the order of the first few entries in this table must agree
// with the list near the top of the file!  (look for COMPILED_OP)

extern GFORTHOP( doByteBop );
extern GFORTHOP( doUByteBop );
extern GFORTHOP( doShortBop );
extern GFORTHOP( doUShortBop );
extern GFORTHOP( doIntBop );
extern GFORTHOP( doIntBop );
extern GFORTHOP( doFloatBop );
extern GFORTHOP( doDoubleBop );
extern GFORTHOP( doStringBop );
extern GFORTHOP( doOpBop );
extern GFORTHOP( doLongBop );
extern GFORTHOP( doObjectBop );
extern GFORTHOP( doByteArrayBop );
extern GFORTHOP( doUByteArrayBop );
extern GFORTHOP( doShortArrayBop );
extern GFORTHOP( doUShortArrayBop );
extern GFORTHOP( doIntArrayBop );
extern GFORTHOP( doIntArrayBop );
extern GFORTHOP( doFloatArrayBop );
extern GFORTHOP( doDoubleArrayBop );
extern GFORTHOP( doStringArrayBop );
extern GFORTHOP( doOpArrayBop );
extern GFORTHOP( doLongArrayBop );
extern GFORTHOP( doObjectArrayBop );

typedef struct {
   const char       *name;
   ulong            flags;
   ulong            value;
   int				index;
} baseDictionaryCompiledEntry;

// helper macro for built-in op entries in baseDictionary
#define OP_DEF( func, funcName )  { funcName, kOpCCode, (ulong) func }
#define OP_COMPILED_DEF( func, funcName, index )  { funcName, kOpCCode, (ulong) func, index }

// helper macro for ops which have precedence (execute at compile time)
#define PRECOP_DEF( func, funcName )  { funcName, kOpCCodeImmediate, (ulong) func }
#define PRECOP_COMPILED_DEF( func, funcName, index )  { funcName, kOpCCodeImmediate, (ulong) func, index }

#ifdef ASM_INNER_INTERPRETER

// helper macro for built-in op entries in baseDictionary
#define NATIVE_DEF( func, funcName )  { funcName, kOpNative, (ulong) func }
#define NATIVE_COMPILED_DEF( func, funcName, index ) { funcName, kOpNative, (ulong) func, index }

#define OPREF extern GFORTHOP

OPREF( abortBop );          OPREF( dropBop );           OPREF( doDoesBop );
OPREF( litBop );            OPREF( dlitBop );           OPREF( doVariableBop );
OPREF( doConstantBop );     OPREF( doDConstantBop );    OPREF( doneBop );
OPREF( doByteBop );         OPREF( doUByteBop );        OPREF( doShortBop );
OPREF( doUShortBop );       OPREF( doIntBop );          OPREF( doLongBop );
OPREF( doFloatBop );        OPREF( doDoubleBop );       OPREF( doStringBop );
OPREF( doOpBop );           OPREF( doObjectBop );       OPREF( doExitBop );
OPREF( doExitLBop );        OPREF( doExitMBop );        OPREF( doExitMLBop );
OPREF( doByteArrayBop );    OPREF( doUByteArrayBop );   OPREF( doShortArrayBop );
OPREF( doUShortArrayBop );  OPREF( doIntArrayBop );     OPREF( doIntArrayBop );
OPREF( doLongArrayBop );    OPREF( doLongArrayBop );    OPREF( doFloatArrayBop );
OPREF( doDoubleArrayBop );  OPREF( doStringArrayBop );  OPREF( doOpArrayBop );
OPREF( doObjectArrayBop );  OPREF( initStringBop );     OPREF( plusBop );
OPREF( strFixupBop );

OPREF( fetchBop );          OPREF( doStructBop );       OPREF( doStructArrayBop );
OPREF( doDoBop );           OPREF( doLoopBop );         OPREF( doLoopNBop );
//OPREF( ofetchBop );          OPREF( doCheckDoBop );
OPREF( dfetchBop );         OPREF( doCheckDoBop );
OPREF( thisBop );           OPREF( thisDataBop );       OPREF( thisMethodsBop );
OPREF( executeBop );        OPREF( callBop );           OPREF( gotoBop );
OPREF( iBop );              OPREF( jBop );              OPREF( unloopBop );
OPREF( leaveBop );          OPREF( hereBop );           OPREF( refBop );
OPREF( intoBop );           OPREF( addToBop );          OPREF( subtractFromBop );
OPREF( minusBop );          OPREF( timesBop );          OPREF( times2Bop );
OPREF( times4Bop );         OPREF( times8Bop );         OPREF( divideBop );
OPREF( divide2Bop );        OPREF( divide4Bop );        OPREF( divide8Bop );
OPREF( divmodBop );         OPREF( modBop );            OPREF( negateBop );
OPREF( fplusBop );          OPREF( fminusBop );         OPREF( ftimesBop );
OPREF( fdivideBop );        OPREF( fEqualsBop );        OPREF( fNotEqualsBop );
OPREF( fGreaterThanBop );   OPREF( fGreaterEqualsBop ); OPREF( fLessThanBop );
OPREF( fLessEqualsBop );    OPREF( fEquals0Bop );       OPREF( fNotEquals0Bop );
OPREF( fLessThan0Bop );     OPREF( fGreaterThan0Bop );  OPREF( fGreaterEquals0Bop );
OPREF( fLessEquals0Bop );   OPREF( fWithinBop );        OPREF( fMinBop );
OPREF( fMaxBop );           OPREF( dplusBop );          OPREF( dminusBop );
OPREF( dtimesBop );         OPREF( ddivideBop );        OPREF( dEqualsBop );
OPREF( dNotEqualsBop );     OPREF( dGreaterThanBop );   OPREF( dGreaterEqualsBop );
OPREF( dLessThanBop );      OPREF( dLessEqualsBop );    OPREF( dEquals0Bop );
OPREF( dNotEquals0Bop );    OPREF( dGreaterThan0Bop );  OPREF( dGreaterEquals0Bop );
OPREF( dLessThan0Bop );     OPREF( dLessEquals0Bop );   OPREF( dWithinBop );
OPREF( dMinBop );           OPREF( dMaxBop );           OPREF( dsinBop );
OPREF( dasinBop );          OPREF( dcosBop );           OPREF( dacosBop );
OPREF( dtanBop );           OPREF( datanBop );          OPREF( datan2Bop );
OPREF( dexpBop );           OPREF( dlnBop );            OPREF( dlog10Bop );
OPREF( dpowBop );           OPREF( dsqrtBop );          OPREF( dceilBop );
OPREF( dfloorBop );         OPREF( dabsBop );           OPREF( dldexpBop );
OPREF( dfrexpBop );         OPREF( dmodfBop );          OPREF( dfmodBop );
OPREF( lplusBop );          OPREF( lminusBop );         OPREF( ltimesBop );
OPREF( mtimesBop );         OPREF( umtimesBop);
OPREF( i2fBop );            OPREF( i2dBop );            OPREF( f2iBop );
OPREF( f2dBop );            OPREF( d2iBop );            OPREF( d2fBop );
OPREF( orBop );             OPREF( andBop );            OPREF( xorBop );
OPREF( invertBop );         OPREF( lshiftBop );         OPREF( rshiftBop );
OPREF( arshiftBop );        OPREF( trueBop );
OPREF( falseBop );          OPREF( nullBop );           OPREF( dnullBop );
OPREF( equalsBop );         OPREF( notEqualsBop );      OPREF( greaterThanBop );
OPREF( greaterEqualsBop );  OPREF( lessThanBop );       OPREF( lessEqualsBop );
OPREF( equals0Bop );        OPREF( notEquals0Bop );     OPREF( greaterThan0Bop );
OPREF( greaterEquals0Bop ); OPREF( lessThan0Bop );      OPREF( lessEquals0Bop );
OPREF( unsignedGreaterThanBop );                        OPREF( unsignedLessThanBop );
OPREF( withinBop );         OPREF( minBop );            OPREF( maxBop );
OPREF( icmpBop );           OPREF( uicmpBop );          OPREF( fcmpBop );
OPREF( dcmpBop );
OPREF( rpushBop );          OPREF( rpopBop );           OPREF( rpeekBop );
OPREF( rdropBop );          OPREF( rpBop );             OPREF( r0Bop );
OPREF( dupBop );            OPREF( checkDupBop );       OPREF( swapBop );
OPREF( overBop );           OPREF( rotBop );            OPREF( reverseRotBop );
OPREF( nipBop );            OPREF( tuckBop );           OPREF( pickBop );
OPREF( spBop );             OPREF( s0Bop );             OPREF( fpBop );
OPREF( ipBop );             OPREF( ddupBop );           OPREF( dswapBop );
OPREF( ddropBop );          OPREF( doverBop );          OPREF( drotBop );
OPREF( startTupleBop );     OPREF( endTupleBop );       OPREF( storeBop );
OPREF( storeNextBop );      OPREF( fetchNextBop );      OPREF( cstoreBop );
OPREF( cfetchBop );         OPREF( cstoreNextBop );     OPREF( cfetchNextBop );
OPREF( scfetchBop );        OPREF( c2iBop );            OPREF( wstoreBop );
OPREF( wfetchBop );         OPREF( wstoreNextBop );     OPREF( wfetchNextBop );
//OPREF( swfetchBop );        OPREF( w2iBop );            OPREF( ostoreBop );
//OPREF( ostoreNextBop );     OPREF( ofetchNextBop );     OPREF( lfetchBop );
OPREF( swfetchBop );        OPREF( w2iBop );            OPREF( dstoreBop );
OPREF( dstoreNextBop );     OPREF( dfetchNextBop );     OPREF( lfetchBop );
OPREF( lstoreBop );         OPREF( lstoreNextBop );     OPREF( lfetchNextBop );
OPREF( memcpyBop );
OPREF( moveBop );           OPREF( fillBop );           OPREF( setVarActionBop );
OPREF( getVarActionBop );   OPREF( byteVarActionBop );  OPREF( ubyteVarActionBop );
OPREF( shortVarActionBop ); OPREF( ushortVarActionBop ); OPREF( intVarActionBop );
OPREF( longVarActionBop );  OPREF( floatVarActionBop ); OPREF( doubleVarActionBop );
OPREF( stringVarActionBop ); OPREF( opVarActionBop );   OPREF( objectVarActionBop );
OPREF( strcpyBop );         OPREF( strncpyBop );        OPREF( strlenBop );
OPREF( strcatBop );         OPREF( strncatBop );        OPREF( strchrBop );
OPREF( strrchrBop );        OPREF( strcmpBop );         OPREF( stricmpBop );
OPREF( strstrBop );         OPREF( strtokBop );         OPREF( fopenBop );
OPREF( fcloseBop );         OPREF( fseekBop );          OPREF( freadBop );
OPREF( fwriteBop );         OPREF( fgetcBop );          OPREF( fputcBop );
OPREF( feofBop );           OPREF( fexistsBop );        OPREF( ftellBop );
OPREF( flenBop );           OPREF( fgetsBop );          OPREF( fputsBop );
OPREF( archX86Bop );        OPREF( archARMBop );        OPREF( oclearBop );
OPREF( fsinBop );           OPREF( fasinBop );          OPREF( fcosBop );
OPREF( facosBop );          OPREF( ftanBop );           OPREF( fatanBop );
OPREF( fatan2Bop );         OPREF( fexpBop );           OPREF( flnBop );
OPREF( flog10Bop );         OPREF( fpowBop );           OPREF( fsqrtBop );
OPREF( fceilBop );          OPREF( ffloorBop );         OPREF( fabsBop );
OPREF( fldexpBop );         OPREF( ffrexpBop );         OPREF( fmodfBop );
OPREF( ffmodBop );
#else

// helper macro for built-in op entries in baseDictionary
#define NATIVE_DEF( func, funcName )  { funcName, kOpCCode, (ulong) func }
#define NATIVE_COMPILED_DEF( func, funcName, index )  { funcName, kOpCCode, (ulong) func, index }

#endif

baseDictionaryCompiledEntry baseCompiledDictionary[] =
{
	//NATIVE_COMPILED_DEF(    abortBop,                "abort",			OP_ABORT ),
	OP_COMPILED_DEF(    abortOp,                "abort",			OP_ABORT ),
    NATIVE_COMPILED_DEF(    dropBop,                 "drop",			OP_DROP ),
    NATIVE_COMPILED_DEF(    doDoesBop,               "_doDoes",			OP_DO_DOES ),
    NATIVE_COMPILED_DEF(    litBop,                  "lit",				OP_INT_VAL ),
    NATIVE_COMPILED_DEF(    litBop,                  "flit",			OP_FLOAT_VAL ),
    NATIVE_COMPILED_DEF(    dlitBop,                 "dlit",			OP_DOUBLE_VAL ),
    NATIVE_COMPILED_DEF(    dlitBop,                 "2lit",			OP_LONG_VAL ),
    NATIVE_COMPILED_DEF(    doVariableBop,           "_doVariable",		OP_DO_VAR ),
    NATIVE_COMPILED_DEF(    doConstantBop,           "_doConstant",		OP_DO_CONSTANT ),
    NATIVE_COMPILED_DEF(    doDConstantBop,          "_doDConstant",	OP_DO_DCONSTANT ),
    NATIVE_COMPILED_DEF(    doneBop,                 "done",			OP_DONE ),
    NATIVE_COMPILED_DEF(    doByteBop,               "_doByte",			OP_DO_BYTE ),
    NATIVE_COMPILED_DEF(    doUByteBop,              "_doUByte",		OP_DO_UBYTE ),
    NATIVE_COMPILED_DEF(    doShortBop,              "_doShort",		OP_DO_SHORT ),
	NATIVE_COMPILED_DEF(    doUShortBop,             "_doUShort",		OP_DO_USHORT ),
    NATIVE_COMPILED_DEF(    doIntBop,                "_doInt",			OP_DO_INT ),
    NATIVE_COMPILED_DEF(    doIntBop,                "_doUInt",			OP_DO_UINT ),
    NATIVE_COMPILED_DEF(    doLongBop,               "_doLong",			OP_DO_LONG ),
    NATIVE_COMPILED_DEF(    doLongBop,               "_doULong",		OP_DO_ULONG ),
    NATIVE_COMPILED_DEF(    doFloatBop,              "_doFloat",		OP_DO_FLOAT ),
    NATIVE_COMPILED_DEF(    doDoubleBop,             "_doDouble",		OP_DO_DOUBLE ),
    NATIVE_COMPILED_DEF(    doStringBop,             "_doString",		OP_DO_STRING ),
    NATIVE_COMPILED_DEF(    doOpBop,                 "_doOp",			OP_DO_OP ),
    NATIVE_COMPILED_DEF(    doObjectBop,             "_doObject",		OP_DO_OBJECT ),
    NATIVE_COMPILED_DEF(    doExitBop,               "_exit",			OP_DO_EXIT ),      // exit normal op with no vars
    NATIVE_COMPILED_DEF(    doExitLBop,              "_exitL",			OP_DO_EXIT_L ),    // exit normal op with local vars
    NATIVE_COMPILED_DEF(    doExitMBop,              "_exitM",			OP_DO_EXIT_M ),    // exit method op with no vars
    NATIVE_COMPILED_DEF(    doExitMLBop,             "_exitML",			OP_DO_EXIT_ML ),   // exit method op with local vars
	NATIVE_COMPILED_DEF(    doByteArrayBop,          "_doByteArray",	OP_DO_BYTE_ARRAY ),
    NATIVE_COMPILED_DEF(    doUByteArrayBop,         "_doUByteArray",	OP_DO_UBYTE_ARRAY ),
    NATIVE_COMPILED_DEF(    doShortArrayBop,         "_doShortArray",	OP_DO_SHORT_ARRAY ),
    NATIVE_COMPILED_DEF(    doUShortArrayBop,        "_doUShortArray",	OP_DO_USHORT_ARRAY ),	// 32
    NATIVE_COMPILED_DEF(    doIntArrayBop,           "_doIntArray",		OP_DO_INT_ARRAY ),
    NATIVE_COMPILED_DEF(    doIntArrayBop,           "_doUIntArray",	OP_DO_UINT_ARRAY ),
    NATIVE_COMPILED_DEF(    doLongArrayBop,          "_doLongArray",	OP_DO_LONG_ARRAY ),
    NATIVE_COMPILED_DEF(    doLongArrayBop,          "_doULongArray",	OP_DO_ULONG_ARRAY ),	// 36
    NATIVE_COMPILED_DEF(    doFloatArrayBop,         "_doFloatArray",	OP_DO_FLOAT_ARRAY ),
    NATIVE_COMPILED_DEF(    doDoubleArrayBop,        "_doDoubleArray",	OP_DO_DOUBLE_ARRAY ),
    NATIVE_COMPILED_DEF(    doStringArrayBop,        "_doStringArray",	OP_DO_STRING_ARRAY ),
    NATIVE_COMPILED_DEF(    doOpArrayBop,            "_doOpArray",		OP_DO_OP_ARRAY ),		// 40
    NATIVE_COMPILED_DEF(    doObjectArrayBop,        "_doObjectArray",	OP_DO_OBJECT_ARRAY ),
	NATIVE_COMPILED_DEF(    initStringBop,           "initString",		OP_INIT_STRING ),
    NATIVE_COMPILED_DEF(    plusBop,                 "+",				OP_PLUS ),				// 44
    NATIVE_COMPILED_DEF(    fetchBop,                "@",				OP_FETCH ),
    NATIVE_COMPILED_DEF(    doStructBop,             "_doStruct",		OP_DO_STRUCT ),
    NATIVE_COMPILED_DEF(    doStructArrayBop,        "_doStructArray",	OP_DO_STRUCT_ARRAY ),	// 48
    NATIVE_COMPILED_DEF(    doDoBop,                 "_do",				OP_DO_DO ),				// 52
    NATIVE_COMPILED_DEF(    doLoopBop,               "_loop",			OP_DO_LOOP ),
    NATIVE_COMPILED_DEF(    doLoopNBop,              "_+loop",			OP_DO_LOOPN ),
    //NATIVE_COMPILED_DEF(    ofetchBop,               "o@",				OP_OFETCH ),				// 56
    NATIVE_COMPILED_DEF(    dfetchBop,               "2@",				OP_OFETCH ),				// 56
    // the order of the next four opcodes has to match the order of kVarRef...kVarMinusStore
    NATIVE_COMPILED_DEF(    refBop,            "ref",				OP_REF ),
	NATIVE_COMPILED_DEF(    intoBop,                 "->",				OP_INTO ),				// 59
    NATIVE_COMPILED_DEF(    addToBop,                "->+",				OP_INTO_PLUS ),
    NATIVE_COMPILED_DEF(    subtractFromBop,         "->-",				OP_INTO_MINUS ),
	NATIVE_COMPILED_DEF(    doCheckDoBop,            "_?do",			OP_DO_CHECKDO ),

	OP_COMPILED_DEF(		doVocabOp,              "_doVocab",			OP_DO_VOCAB ),
	OP_COMPILED_DEF(		getClassByIndexOp,      "getClassByIndex",	OP_GET_CLASS_BY_INDEX ),
	OP_COMPILED_DEF(		initStringArrayOp,      "initStringArray",	OP_INIT_STRING_ARRAY ),
    OP_COMPILED_DEF(		badOpOp,                "badOp",			OP_BAD_OP ),
	OP_COMPILED_DEF(		doStructTypeOp,         "_doStructType",	OP_DO_STRUCT_TYPE ),
    OP_COMPILED_DEF(		doClassTypeOp,          "_doClassType",		OP_DO_CLASS_TYPE ),
    PRECOP_COMPILED_DEF(	doEnumOp,               "_doEnum",			OP_DO_ENUM ),
    OP_COMPILED_DEF(		doNewOp,                "_doNew",			OP_DO_NEW ),
    OP_COMPILED_DEF(		allocObjectOp,          "_allocObject",		OP_ALLOC_OBJECT ),
	OP_COMPILED_DEF(		superOp,                "super",			OP_SUPER ),
    OP_COMPILED_DEF(		endBuildsOp,            "_endBuilds",		OP_END_BUILDS ),
    OP_COMPILED_DEF(		compileOp,              "compile",		    OP_COMPILE ),
	OP_COMPILED_DEF(		initStructArrayOp,      "initStructArray",	OP_INIT_STRUCT_ARRAY ),
	NATIVE_COMPILED_DEF(    dupBop,					"dup",				OP_DUP ),
	NATIVE_COMPILED_DEF(    overBop,				"over",				OP_OVER ),

    // following must be last in table
    OP_COMPILED_DEF(		NULL,                   NULL,					-1 )
};



baseDictionaryEntry baseDictionary[] =
{
	NATIVE_DEF(    thisBop,                 "this" ),
    NATIVE_DEF(    thisDataBop,             "thisData" ),
    NATIVE_DEF(    thisMethodsBop,          "thisMethods" ),
    NATIVE_DEF(    executeBop,              "execute" ),
    NATIVE_DEF(    callBop,                 "call" ),
    NATIVE_DEF(    gotoBop,                 "goto" ),
    NATIVE_DEF(    iBop,                    "i" ),
    NATIVE_DEF(    jBop,                    "j" ),
    NATIVE_DEF(    unloopBop,               "unloop" ),
    NATIVE_DEF(    leaveBop,                "leave" ),
    NATIVE_DEF(    hereBop,                 "here" ),

	// object varActions
    NATIVE_DEF(    subtractFromBop,         "unref" ),
    
    ///////////////////////////////////////////
    //  integer math
    ///////////////////////////////////////////
    NATIVE_DEF(    minusBop,                "-" ),
    NATIVE_DEF(    timesBop,                "*" ),
    NATIVE_DEF(    times2Bop,               "2*" ),
    NATIVE_DEF(    times4Bop,               "4*" ),
    NATIVE_DEF(    times8Bop,               "8*" ),
    NATIVE_DEF(    divideBop,               "/" ),
    NATIVE_DEF(    divide2Bop,              "2/" ),
    NATIVE_DEF(    divide4Bop,              "4/" ),
    NATIVE_DEF(    divide8Bop,              "8/" ),
    NATIVE_DEF(    divmodBop,               "/mod" ),
    NATIVE_DEF(    modBop,                  "mod" ),
    NATIVE_DEF(    negateBop,               "negate" ),
    
    ///////////////////////////////////////////
    //  single-precision fp math
    ///////////////////////////////////////////
    NATIVE_DEF(    fplusBop,                "f+" ),
    NATIVE_DEF(    fminusBop,               "f-" ),
    NATIVE_DEF(    ftimesBop,               "f*" ),
    NATIVE_DEF(    fdivideBop,              "f/" ),
    
    ///////////////////////////////////////////
    //  single-precision fp comparisons
    ///////////////////////////////////////////
    NATIVE_DEF(    fEqualsBop,              "f=" ),
    NATIVE_DEF(    fNotEqualsBop,           "f<>" ),
    NATIVE_DEF(    fGreaterThanBop,         "f>" ),
    NATIVE_DEF(    fGreaterEqualsBop,       "f>=" ),
    NATIVE_DEF(    fLessThanBop,            "f<" ),
    NATIVE_DEF(    fLessEqualsBop,          "f<=" ),
    NATIVE_DEF(    fEquals0Bop,             "f0=" ),
    NATIVE_DEF(    fNotEquals0Bop,          "f0<>" ),
    NATIVE_DEF(    fGreaterThan0Bop,        "f0>" ),
    NATIVE_DEF(    fGreaterEquals0Bop,      "f0>=" ),
    NATIVE_DEF(    fLessThan0Bop,           "f0<" ),
    NATIVE_DEF(    fLessEquals0Bop,         "f0<=" ),
    NATIVE_DEF(    fWithinBop,              "fwithin" ),
    NATIVE_DEF(    fMinBop,                 "fmin" ),
    NATIVE_DEF(    fMaxBop,                 "fmax" ),
    NATIVE_DEF(    fcmpBop,                 "fcmp" ),

    ///////////////////////////////////////////
    //  single-precision fp functions
    ///////////////////////////////////////////
    NATIVE_DEF(    fsinBop,                 "fsin" ),
    NATIVE_DEF(    fasinBop,                "fasin" ),
    NATIVE_DEF(    fcosBop,                 "fcos" ),
    NATIVE_DEF(    facosBop,                "facos" ),
    NATIVE_DEF(    ftanBop,                 "ftan" ),
    NATIVE_DEF(    fatanBop,                "fatan" ),
    NATIVE_DEF(    fatan2Bop,               "fatan2" ),
    NATIVE_DEF(    fexpBop,                 "fexp" ),
    NATIVE_DEF(    flnBop,                  "fln" ),
    NATIVE_DEF(    flog10Bop,               "flog10" ),
    NATIVE_DEF(    fpowBop,                 "fpow" ),
    NATIVE_DEF(    fsqrtBop,                "fsqrt" ),
    NATIVE_DEF(    fceilBop,                "fceil" ),
    NATIVE_DEF(    ffloorBop,               "floor" ),
    NATIVE_DEF(    fabsBop,                 "fabs" ),
    NATIVE_DEF(    fldexpBop,               "fldexp" ),
    NATIVE_DEF(    ffrexpBop,               "ffrexp" ),
    NATIVE_DEF(    fmodfBop,                "fmodf" ),
    NATIVE_DEF(    ffmodBop,                "ffmod" ),
    
    ///////////////////////////////////////////
    //  double-precision fp math
    ///////////////////////////////////////////
    NATIVE_DEF(    dplusBop,                "d+" ),
    NATIVE_DEF(    dminusBop,               "d-" ),
    NATIVE_DEF(    dtimesBop,               "d*" ),
    NATIVE_DEF(    ddivideBop,              "d/" ),


    ///////////////////////////////////////////
    //  double-precision fp comparisons
    ///////////////////////////////////////////
    NATIVE_DEF(    dEqualsBop,              "d=" ),
    NATIVE_DEF(    dNotEqualsBop,           "d<>" ),
    NATIVE_DEF(    dGreaterThanBop,         "d>" ),
    NATIVE_DEF(    dGreaterEqualsBop,       "d>=" ),
    NATIVE_DEF(    dLessThanBop,            "d<" ),
    NATIVE_DEF(    dLessEqualsBop,          "d<=" ),
    NATIVE_DEF(    dEquals0Bop,             "d0=" ),
    NATIVE_DEF(    dNotEquals0Bop,          "d0<>" ),
    NATIVE_DEF(    dGreaterThan0Bop,        "d0>" ),
    NATIVE_DEF(    dGreaterEquals0Bop,      "d0>=" ),
    NATIVE_DEF(    dLessThan0Bop,           "d0<" ),
    NATIVE_DEF(    dLessEquals0Bop,         "d0<=" ),
    NATIVE_DEF(    dWithinBop,              "dwithin" ),
    NATIVE_DEF(    dMinBop,                 "dmin" ),
    NATIVE_DEF(    dMaxBop,                 "dmax" ),
    NATIVE_DEF(    dcmpBop,                 "dcmp" ),

    ///////////////////////////////////////////
    //  double-precision fp functions
    ///////////////////////////////////////////
    NATIVE_DEF(    dsinBop,                 "dsin" ),
    NATIVE_DEF(    dasinBop,                "dasin" ),
    NATIVE_DEF(    dcosBop,                 "dcos" ),
    NATIVE_DEF(    dacosBop,                "dacos" ),
    NATIVE_DEF(    dtanBop,                 "dtan" ),
    NATIVE_DEF(    datanBop,                "datan" ),
    NATIVE_DEF(    datan2Bop,               "datan2" ),
    NATIVE_DEF(    dexpBop,                 "dexp" ),
    NATIVE_DEF(    dlnBop,                  "dln" ),
    NATIVE_DEF(    dlog10Bop,               "dlog10" ),
    NATIVE_DEF(    dpowBop,                 "dpow" ),
    NATIVE_DEF(    dsqrtBop,                "dsqrt" ),
    NATIVE_DEF(    dceilBop,                "dceil" ),
    NATIVE_DEF(    dfloorBop,               "dfloor" ),
    NATIVE_DEF(    dabsBop,                 "dabs" ),
    NATIVE_DEF(    dldexpBop,               "dldexp" ),
    NATIVE_DEF(    dfrexpBop,               "dfrexp" ),
    NATIVE_DEF(    dmodfBop,                "dmodf" ),
    NATIVE_DEF(    dfmodBop,                "dfmod" ),
    
    ///////////////////////////////////////////
    //  integer/long/float/double conversion
    ///////////////////////////////////////////
    NATIVE_DEF(    i2fBop,                  "i2f" ), 
    NATIVE_DEF(    i2dBop,                  "i2d" ),
    NATIVE_DEF(    f2iBop,                  "f2i" ),
    NATIVE_DEF(    f2dBop,                  "f2d" ),
    NATIVE_DEF(    d2iBop,                  "d2i" ),
    NATIVE_DEF(    d2fBop,                  "d2f" ),
    OP_DEF(    i2lOp,                  "i2l" ), 
    OP_DEF(    l2fOp,                  "l2f" ), 
    OP_DEF(    l2dOp,                  "l2d" ), 
    OP_DEF(    f2lOp,                  "f2l" ),
    OP_DEF(    d2lOp,                  "d2l" ),
    
    ///////////////////////////////////////////
    //  bit-vector logic
    ///////////////////////////////////////////
    NATIVE_DEF(    orBop,                   "or" ),
    NATIVE_DEF(    andBop,                  "and" ),
    NATIVE_DEF(    xorBop,                  "xor" ),
    NATIVE_DEF(    invertBop,               "invert" ),
    NATIVE_DEF(    lshiftBop,               "lshift" ),
    NATIVE_DEF(    rshiftBop,               "rshift" ),
    NATIVE_DEF(    arshiftBop,              "arshift" ),

    ///////////////////////////////////////////
    //  boolean logic
    ///////////////////////////////////////////
    NATIVE_DEF(    equals0Bop,              "not" ),
    NATIVE_DEF(    trueBop,                 "true" ),
    NATIVE_DEF(    falseBop,                "false" ),
    NATIVE_DEF(    nullBop,                 "null" ),
    NATIVE_DEF(    dnullBop,                "dnull" ),

    ///////////////////////////////////////////
    //  integer comparisons
    ///////////////////////////////////////////
    NATIVE_DEF(    equalsBop,               "=" ),
    NATIVE_DEF(    notEqualsBop,            "<>" ),
    NATIVE_DEF(    greaterThanBop,          ">" ),
    NATIVE_DEF(    greaterEqualsBop,        ">=" ),
    NATIVE_DEF(    lessThanBop,             "<" ),
	NATIVE_DEF(    lessEqualsBop,           "<=" ),
    NATIVE_DEF(    equals0Bop,              "0=" ),
    NATIVE_DEF(    notEquals0Bop,           "0<>" ),
    NATIVE_DEF(    greaterThan0Bop,         "0>" ),
    NATIVE_DEF(    greaterEquals0Bop,       "0>=" ),
    NATIVE_DEF(    lessThan0Bop,            "0<" ),
    NATIVE_DEF(    lessEquals0Bop,          "0<=" ),
    NATIVE_DEF(    unsignedGreaterThanBop,  "u>" ),
    NATIVE_DEF(    unsignedLessThanBop,     "u<" ),
    NATIVE_DEF(    withinBop,               "within" ),
    NATIVE_DEF(    minBop,                  "min" ),
    NATIVE_DEF(    maxBop,                  "max" ),
    NATIVE_DEF(    icmpBop,                 "icmp" ),
    NATIVE_DEF(    uicmpBop,                "uicmp" ),


    ///////////////////////////////////////////
    //  stack manipulation
    ///////////////////////////////////////////
    NATIVE_DEF(    rpushBop,                ">r" ),
    NATIVE_DEF(    rpopBop,                 "r>" ),
    NATIVE_DEF(    rpeekBop,                "r@" ),
    NATIVE_DEF(    rdropBop,                "rdrop" ),
    NATIVE_DEF(    rpBop,                   "rp" ),
    NATIVE_DEF(    r0Bop,                   "r0" ),
    NATIVE_DEF(    checkDupBop,             "?dup" ),
    NATIVE_DEF(    swapBop,                 "swap" ),
    NATIVE_DEF(    rotBop,                  "rot" ),
    NATIVE_DEF(    reverseRotBop,           "-rot" ),
    NATIVE_DEF(    nipBop,                  "nip" ),
    NATIVE_DEF(    nipBop,                  "l2i" ),
    NATIVE_DEF(    tuckBop,                 "tuck" ),
    NATIVE_DEF(    pickBop,                 "pick" ),
    NATIVE_DEF(    spBop,                   "sp" ),
    NATIVE_DEF(    s0Bop,                   "s0" ),
    NATIVE_DEF(    fpBop,                   "fp" ),
    NATIVE_DEF(    ipBop,                   "ip" ),
    NATIVE_DEF(    ddupBop,                 "2dup" ),
    NATIVE_DEF(    dswapBop,                "2swap" ),
    NATIVE_DEF(    ddropBop,                "2drop" ),
    NATIVE_DEF(    doverBop,                "2over" ),
    NATIVE_DEF(    drotBop,                 "2rot" ),
    NATIVE_DEF(    startTupleBop,           "r[" ),
    NATIVE_DEF(    endTupleBop,             "]r" ),
#if 0
    NATIVE_DEF(    dreverseRotBop,          "-2rot" ),
    NATIVE_DEF(    dnipBop,                 "2nip" ),
    NATIVE_DEF(    dtuckBop,                "2tuck" ),
    NATIVE_DEF(    dpickBop,                "2pick" ),
    NATIVE_DEF(    drollBop,                "2roll" ),
    NATIVE_DEF(    ndropBop,                "ndrop" ),
    NATIVE_DEF(    ndupBop,                 "ndup" ),
    NATIVE_DEF(    npickBop,                "npick" ),
#endif

    ///////////////////////////////////////////
    //  memory store/fetch
    ///////////////////////////////////////////
    NATIVE_DEF(    storeBop,                "!" ),
    NATIVE_DEF(    storeNextBop,            "@!++" ),
    NATIVE_DEF(    fetchNextBop,            "@@++" ),
    NATIVE_DEF(    cstoreBop,               "c!" ),
    NATIVE_DEF(    cfetchBop,               "c@" ),
    NATIVE_DEF(    cstoreNextBop,           "c@!++" ),
    NATIVE_DEF(    cfetchNextBop,           "c@@++" ),
    NATIVE_DEF(    scfetchBop,              "sc@" ),
    NATIVE_DEF(    c2iBop,                  "c2i" ),
    NATIVE_DEF(    wstoreBop,               "w!" ),
    NATIVE_DEF(    wfetchBop,               "w@" ),
    NATIVE_DEF(    wstoreNextBop,           "w@!++" ),
    NATIVE_DEF(    wfetchNextBop,           "w@@++" ),
    NATIVE_DEF(    swfetchBop,              "sw@" ),
    NATIVE_DEF(    w2iBop,                  "w2i" ),
    NATIVE_DEF(    dstoreBop,               "2!" ),
    NATIVE_DEF(    dstoreNextBop,           "2@!++" ),
    NATIVE_DEF(    dfetchNextBop,           "2@@++" ),
	/*
	NATIVE_DEF(    lfetchBop,               "l@" ),
    NATIVE_DEF(    lstoreBop,               "2!" ),
    NATIVE_DEF(    lstoreNextBop,           "2@!++" ),
    NATIVE_DEF(    lfetchNextBop,           "2@@++" ),
    NATIVE_DEF(    ofetchBop,               "o@" ),
    NATIVE_DEF(    ostoreBop,               "o!" ),
    NATIVE_DEF(    ostoreNextBop,           "o@!++" ),
    NATIVE_DEF(    ofetchNextBop,           "o@@++" ),
	*/
    NATIVE_DEF(    moveBop,                 "move" ),
    NATIVE_DEF(    fillBop,                 "fill" ),
    NATIVE_DEF(    setVarActionBop,         "varAction!" ),
    NATIVE_DEF(    getVarActionBop,         "varAction@" ),
    NATIVE_DEF(    byteVarActionBop,        "byteVarAction" ),
    NATIVE_DEF(    ubyteVarActionBop,       "ubyteVarAction" ),
    NATIVE_DEF(    shortVarActionBop,       "shortVarAction" ),
    NATIVE_DEF(    ushortVarActionBop,      "ushortVarAction" ),
    NATIVE_DEF(    intVarActionBop,         "intVarAction" ),
    NATIVE_DEF(    longVarActionBop,        "longVarAction" ),
    NATIVE_DEF(    floatVarActionBop,       "floatVarAction" ),
    NATIVE_DEF(    doubleVarActionBop,      "doubleVarAction" ),
    NATIVE_DEF(    stringVarActionBop,      "stringVarAction" ),
    NATIVE_DEF(    opVarActionBop,          "opVarAction" ),
    NATIVE_DEF(    objectVarActionBop,      "objectVarAction" ),
    NATIVE_DEF(    oclearBop,               "oclear" ),

    ///////////////////////////////////////////
    //  string manipulation
    ///////////////////////////////////////////
    NATIVE_DEF(    strcpyBop,               "strcpy" ),
    NATIVE_DEF(    strncpyBop,              "strncpy" ),
    NATIVE_DEF(    strlenBop,               "strlen" ),
    NATIVE_DEF(    strcatBop,               "strcat" ),
    NATIVE_DEF(    strncatBop,              "strncat" ),
    NATIVE_DEF(    strchrBop,               "strchr" ),
    NATIVE_DEF(    strrchrBop,              "strrchr" ),
    NATIVE_DEF(    strcmpBop,               "strcmp" ),
    NATIVE_DEF(    stricmpBop,              "stricmp" ),
    NATIVE_DEF(    strstrBop,               "strstr" ),
    NATIVE_DEF(    strtokBop,               "strtok" ),
    NATIVE_DEF(    strFixupBop,             "$fixup" ),

    ///////////////////////////////////////////
    //  file manipulation
    ///////////////////////////////////////////
    NATIVE_DEF(    fopenBop,                "fopen" ),
    NATIVE_DEF(    fcloseBop,               "fclose" ),
    NATIVE_DEF(    fseekBop,                "fseek" ),
    NATIVE_DEF(    freadBop,                "fread" ),
    NATIVE_DEF(    fwriteBop,               "fwrite" ),
    NATIVE_DEF(    fgetcBop,                "fgetc" ),
    NATIVE_DEF(    fputcBop,                "fputc" ),
    NATIVE_DEF(    feofBop,                 "feof" ),
    NATIVE_DEF(    fexistsBop,              "fexists" ),
    NATIVE_DEF(    ftellBop,                "ftell" ),
    NATIVE_DEF(    flenBop,                 "flen" ),
    NATIVE_DEF(    fgetsBop,                "fgets" ),
    NATIVE_DEF(    fputsBop,                "fputs" ),

    NATIVE_DEF(    lplusBop,                "l+" ),
    NATIVE_DEF(    lminusBop,               "l-" ),
    NATIVE_DEF(    ltimesBop,               "l*" ),
    NATIVE_DEF(    mtimesBop,               "m*" ),
    NATIVE_DEF(    umtimesBop,              "um*" ),

    OP_DEF(    stdinOp,                "stdin" ),
    OP_DEF(    stdoutOp,               "stdout" ),
    OP_DEF(    stderrOp,               "stderr" ),
    OP_DEF(    chdirOp,                "chdir" ),
    OP_DEF(    mkdirOp,                "mkdir" ),
    OP_DEF(    rmdirOp,                "rmdir" ),
    OP_DEF(    renameOp,               "rename" ),
    OP_DEF(    sourceIdOp,             "source-id" ),
    OP_DEF(    restoreInputOp,         "restore-input" ),
    OP_DEF(    saveInputOp,            "save-input" ),
	OP_DEF(    opendirOp,              "opendir" ),
	OP_DEF(    readdirOp,              "readdir" ),
	OP_DEF(    closedirOp,             "closedir" ),
	OP_DEF(    rewinddirOp,            "rewinddir" ),
    OP_DEF(    fflushOp,               "fflush" ),
    OP_DEF(    errnoOp,			       "errno" ),
    OP_DEF(    strerrorOp,             "strerror" ),
    OP_DEF(    getEnvironmentOp,       "getEnvironment" ),
    OP_DEF(    getEnvironmentVarOp,    "getEnvironmentVar" ),
	OP_DEF(    getCwdOp,               "getCurrentDirectory"),

    ///////////////////////////////////////////
    //  block i/o
    ///////////////////////////////////////////
    OP_DEF(    blkOp,                  "blk" ),
    OP_DEF(    blockOp,                "block" ),
    OP_DEF(    bufferOp,               "buffer" ),
    OP_DEF(    emptyBuffersOp,         "empty-buffers" ),
    OP_DEF(    flushOp,                "flush" ),
    OP_DEF(    saveBuffersOp,          "save-buffers" ),
    OP_DEF(    updateOp,               "update" ),
    OP_DEF(    thruOp,                 "thru" ),

    ///////////////////////////////////////////
    //  64-bit integer math
    ///////////////////////////////////////////
    OP_DEF(    ldivideOp,              "l/" ),
    OP_DEF(    lmodOp,                 "lmod" ),
    OP_DEF(    ldivmodOp,              "l/mod" ),
    OP_DEF(    udivmodOp,              "ud/mod" ),
    
    ///////////////////////////////////////////
    //  64-bit integer comparisons
    ///////////////////////////////////////////
    OP_DEF(    lEqualsOp,               "l=" ),
    OP_DEF(    lNotEqualsOp,            "l<>" ),
    OP_DEF(    lGreaterThanOp,          "l>" ),
    OP_DEF(    lGreaterEqualsOp,        "l>=" ),
    OP_DEF(    lLessThanOp,             "l<" ),
    OP_DEF(    lLessEqualsOp,           "l<=" ),
    OP_DEF(    lEquals0Op,              "l0=" ),
    OP_DEF(    lNotEquals0Op,           "l0<>" ),
    OP_DEF(    lGreaterThan0Op,         "l0>" ),
    OP_DEF(    lGreaterEquals0Op,       "l0>=" ),
    OP_DEF(    lLessThan0Op,            "l0<" ),
    OP_DEF(    lLessEquals0Op,          "l0<=" ),
    OP_DEF(    lWithinOp,               "lwithin" ),
    OP_DEF(    lMinOp,                  "lmin" ),
    OP_DEF(    lMaxOp,                  "lmax" ),

    ///////////////////////////////////////////
    //  control flow
    ///////////////////////////////////////////
    PRECOP_DEF(doOp,                   "do" ),
    PRECOP_DEF(checkDoOp,              "?do" ),
    PRECOP_DEF(loopOp,                 "loop" ),
    PRECOP_DEF(loopNOp,                "+loop" ),
    PRECOP_DEF(ifOp,                   "if" ),
    PRECOP_DEF(elseOp,                 "else" ),
    PRECOP_DEF(endifOp,                "endif" ),
    PRECOP_DEF(beginOp,                "begin" ),
    PRECOP_DEF(untilOp,                "until" ),
    PRECOP_DEF(whileOp,                "while" ),
    PRECOP_DEF(repeatOp,               "repeat" ),
    PRECOP_DEF(againOp,                "again" ),
    PRECOP_DEF(caseOp,                 "case" ),
    PRECOP_DEF(ofOp,                   "of" ),
    PRECOP_DEF(ofifOp,                 "ofif" ),
    PRECOP_DEF(endofOp,                "endof" ),
    PRECOP_DEF(endcaseOp,              "endcase" ),

    ///////////////////////////////////////////
    //  op definition
    ///////////////////////////////////////////
    OP_DEF(    buildsOp,               "builds" ),
    PRECOP_DEF(doesOp,                 "does" ),
    PRECOP_DEF(exitOp,                 "exit" ),
    PRECOP_DEF(semiOp,                 ";" ),
    OP_DEF(    colonOp,                ":" ),
    OP_DEF(    colonNoNameOp,          ":noname" ),
	OP_DEF(    pushTokenOp,            "pushToken"),
	OP_DEF(    popTokenOp,             "popToken"),
	PRECOP_DEF(funcOp,                 "func:" ),
	PRECOP_DEF(endfuncOp,               ";func" ),
    OP_DEF(    codeOp,                 "code" ),
    OP_DEF(    createOp,               "create" ),
    OP_DEF(    variableOp,             "variable" ),
    OP_DEF(    constantOp,             "constant" ),
    OP_DEF(    dconstantOp,            "2constant" ),
    PRECOP_DEF(byteOp,                 "byte" ),
    PRECOP_DEF(ubyteOp,                "ubyte" ),
    PRECOP_DEF(shortOp,                "short" ),
    PRECOP_DEF(ushortOp,               "ushort" ),
    PRECOP_DEF(intOp,                  "int" ),
    PRECOP_DEF(uintOp,                 "uint" ),
    PRECOP_DEF(longOp,                 "long" ),
    PRECOP_DEF(ulongOp,                "ulong" ),
    PRECOP_DEF(floatOp,                "float" ),
    PRECOP_DEF(doubleOp,               "double" ),
    PRECOP_DEF(stringOp,               "string" ),
    PRECOP_DEF(opOp,                   "op" ),
    //PRECOP_DEF(objectOp,               "object" ),
    PRECOP_DEF(voidOp,                 "void" ),
    PRECOP_DEF(arrayOfOp,              "arrayOf" ),
    PRECOP_DEF(ptrToOp,                "ptrTo" ),
    OP_DEF(    structOp,               "struct:" ),
    OP_DEF(    endstructOp,            ";struct" ),
    OP_DEF(    classOp,                "class:" ),
    OP_DEF(    endclassOp,             ";class" ),
    OP_DEF(    methodOp,               "method:" ),
    PRECOP_DEF(endmethodOp,            ";method" ),
    PRECOP_DEF(returnsOp,              "returns" ),
    OP_DEF(    doMethodOp,             "doMethod" ),
    OP_DEF(    implementsOp,           "implements:" ),
    OP_DEF(    endImplementsOp,        ";implements" ),
    OP_DEF(    unionOp,                "union" ),
    OP_DEF(    extendsOp,              "extends" ),
    OP_DEF(    strSizeOfOp,            "$sizeOf" ),
    OP_DEF(    strOffsetOfOp,          "$offsetOf" ),
    OP_DEF(    processConstantOp,      "processConstant" ),
    OP_DEF(    processLongConstantOp,  "processLongConstant" ),
    OP_DEF(    showStructOp,           "showStruct" ),
    PRECOP_DEF(newOp,                  "new" ),
    OP_DEF(    strNewOp,               "$new" ),
    PRECOP_DEF(makeObjectOp,           "makeObject" ),
    PRECOP_DEF(initMemberStringOp,     "initMemberString"),
    OP_DEF(    enumOp,                 "enum:" ),
    OP_DEF(    endenumOp,              ";enum" ),
    PRECOP_DEF(recursiveOp,            "recursive" ),
    OP_DEF(    precedenceOp,           "precedence" ),
    OP_DEF(    strRunFileOp,           "$runFile" ),
    OP_DEF(    strLoadOp,              "$load" ),
    OP_DEF(    loadDoneOp,             "loaddone" ),
    OP_DEF(    requiresOp,             "requires" ),
    OP_DEF(    evaluateOp,             "evaluate" ),
    OP_DEF(    strEvaluateOp,          "$evaluate" ),
    PRECOP_DEF(stateInterpretOp,       "[" ),
    OP_DEF(    stateCompileOp,         "]" ),
    OP_DEF(    stateOp,                "state" ),
    OP_DEF(    strTickOp,              "$\'" ),
    PRECOP_DEF(postponeOp,             "postpone" ),
    PRECOP_DEF(bracketTickOp,          "[\']" ),
    OP_DEF(    bodyOp,                 ">body" ),

    ///////////////////////////////////////////
    //  vocabulary/symbol
    ///////////////////////////////////////////
    OP_DEF(    forthVocabOp,           "forth" ),
    OP_DEF(    definitionsOp,          "definitions" ),
    OP_DEF(    vocabularyOp,           "vocabulary" ),
    OP_DEF(    alsoOp,                 "also" ),
    OP_DEF(    previousOp,             "previous" ),
    OP_DEF(    onlyOp,                 "only" ),
    OP_DEF(    strForgetOp,            "$forget" ),
    OP_DEF(    vlistOp,                "vlist" ),
    OP_DEF(    strFindOp,              "$find" ),

    ///////////////////////////////////////////
    //  data compilation/allocation
    ///////////////////////////////////////////
    OP_DEF(    alignOp,                "align" ),
    OP_DEF(    allotOp,                "allot" ),
    OP_DEF(    commaOp,                "," ),
    OP_DEF(    cCommaOp,               "c," ),
    OP_DEF(    here0Op,                "here0" ),
    OP_DEF(    unusedOp,               "unused" ),
    OP_DEF(    mallocOp,               "malloc" ),
    OP_DEF(    reallocOp,              "realloc" ),
    OP_DEF(    freeOp,                 "free" ),

    ///////////////////////////////////////////
    //  text display
    ///////////////////////////////////////////
    OP_DEF(    printNumOp,             "." ),
    OP_DEF(    printLongNumOp,         "2." ),
    OP_DEF(    printNumDecimalOp,      "%d" ),
    OP_DEF(    printNumHexOp,          "%x" ),
    OP_DEF(    printLongDecimalOp,     "%2d" ),
    OP_DEF(    printLongHexOp,         "%2x" ),
    OP_DEF(    printStrOp,             "%s" ),
    OP_DEF(    printCharOp,            "%c" ),
    OP_DEF(    printBytesOp,           "type" ),
    OP_DEF(    print4Op,               "%4c" ),
    OP_DEF(    print8Op,               "%8c" ),
    OP_DEF(    printSpaceOp,           "%bl" ),
    OP_DEF(    printNewlineOp,         "%nl" ),
    OP_DEF(    printFloatOp,           "%f" ),
    OP_DEF(    printDoubleOp,          "%2f" ),
    OP_DEF(    format32Op,             "format" ),
    OP_DEF(    format64Op,             "2format" ),
    OP_DEF(    fprintfOp,              "fprintf" ),
    OP_DEF(    snprintfOp,             "snprintf" ),
    OP_DEF(    fscanfOp,               "fscanf" ),
    OP_DEF(    sscanfOp,               "sscanf" ),
    OP_DEF(    baseOp,                 "base" ),
    OP_DEF(    octalOp,                "octal" ),
    OP_DEF(    decimalOp,              "decimal" ),
    OP_DEF(    hexOp,                  "hex" ),
    OP_DEF(    printDecimalSignedOp,   "printDecimalSigned" ),
    OP_DEF(    printAllSignedOp,       "printAllSigned" ),
    OP_DEF(    printAllUnsignedOp,     "printAllUnsigned" ),
 
	OP_DEF(    getConsoleOutOp,        "getConsoleOut" ),
	OP_DEF(    getDefaultConsoleOutOp, "getDefaultConsoleOut" ),
	OP_DEF(    setConsoleOutOp,        "setConsoleOut" ),
	OP_DEF(    resetConsoleOutOp,      "resetConsoleOut" ),

    ///////////////////////////////////////////
    //  input buffer
    ///////////////////////////////////////////
    OP_DEF(    blwordOp,               "blword" ),
    OP_DEF(    strWordOp,              "$word" ),
    PRECOP_DEF(commentOp,              "/*" ),
    PRECOP_DEF(parenCommentOp,         "(" ),
    PRECOP_DEF(slashCommentOp,         "\\" ),
    OP_DEF(    sourceOp,               "source" ),
    OP_DEF(    getInOffsetPointerOp,   ">in" ),
    OP_DEF(    fillInBufferOp,         "fillInBuffer" ),
    OP_DEF(    keyOp,                  "key" ),
    OP_DEF(    keyHitOp,               "key?" ),

    ///////////////////////////////////////////
    //  vocabulary ops
    ///////////////////////////////////////////
    OP_DEF(    vocChainHeadOp,         "vocChainHead" ),
    OP_DEF(    vocChainNextOp,         "vocChainNext" ),
#if 0
	OP_DEF(    vocNewestEntryOp,       "vocNewestEntry" ),
    OP_DEF(    vocNextEntryOp,         "vocNextEntry" ),
    OP_DEF(    vocNumEntriesOp,        "vocNumEntries" ),
    OP_DEF(    vocNameOp,              "vocName" ),
    OP_DEF(    vocFindEntryOp,         "vocFindEntry" ),
    OP_DEF(    vocFindNextEntryOp,     "vocFindNextEntry" ),
    OP_DEF(    vocFindEntryByValueOp,  "vocFindEntryByValue" ),
    OP_DEF(    vocFindNextEntryByValueOp,  "vocFindNextEntryByValue" ),
    OP_DEF(    vocAddEntryOp,          "vocAddEntry" ),
    OP_DEF(    vocRemoveEntryOp,       "vocRemoveEntry" ),
    OP_DEF(    vocValueLengthOp,       "vocValueLength" ),
#endif

    ///////////////////////////////////////////
    //  DLL support
    ///////////////////////////////////////////
    OP_DEF(    DLLVocabularyOp,        "DLLVocabulary" ),
    OP_DEF(    addDLLEntryOp,          "addDLLEntry" ),
    OP_DEF(    DLLVoidOp,              "DLLVoid" ),
    OP_DEF(    DLLLongOp,              "DLLLong" ),
    OP_DEF(    DLLStdCallOp,           "DLLStdCall" ),

    ///////////////////////////////////////////
    //  time and date
    ///////////////////////////////////////////
    OP_DEF(    timeOp,                 "time" ),
    OP_DEF(    strftimeOp,             "strftime" ),
    OP_DEF(    timeAndDateOp,          "time&date" ),
    OP_DEF(    millitimeOp,            "ms@" ),
    OP_DEF(    millisleepOp,           "ms" ),

    ///////////////////////////////////////////
    //  computation utilities - randoms, hashing, sorting and searcing
    ///////////////////////////////////////////
    OP_DEF(    randOp,                 "rand" ),
    OP_DEF(    srandOp,                "srand" ),
    OP_DEF(    hashOp,                 "hash" ),
    OP_DEF(    qsortOp,                "qsort" ),
    OP_DEF(    bsearchOp,              "bsearch" ),

    ///////////////////////////////////////////
    //  admin/debug/system
    ///////////////////////////////////////////
    OP_DEF(    dstackOp,               "dstack" ),
    OP_DEF(    drstackOp,              "drstack" ),
    OP_DEF(    removeOp,               "remove" ),
    OP_DEF(    _dupOp,                 "_dup" ),
    OP_DEF(    _dup2Op,                "_dup2" ),
    OP_DEF(    _filenoOp,              "_fileno" ),
    OP_DEF(    tmpnamOp,               "tmpnam" ),
    OP_DEF(    shellRunOp,             "_shellRun" ),
    OP_DEF(    byeOp,                  "bye" ),
    OP_DEF(    argvOp,                 "argv" ),
    OP_DEF(    argcOp,                 "argc" ),
    OP_DEF(    turboOp,                "turbo" ),
    OP_DEF(    statsOp,                "stats" ),
    OP_DEF(    describeOp,             "describe" ),
    OP_DEF(    describeAtOp,           "describe@" ),
    OP_DEF(    errorOp,                "error" ),
    OP_DEF(    addErrorTextOp,         "addErrorText" ),
    OP_DEF(    unimplementedMethodOp,  "unimplementedMethod" ),
    OP_DEF(    illegalMethodOp,        "illegalMethod" ),
	OP_DEF(    setTraceOp,             "setTrace" ),
	OP_DEF(    getTraceOp,             "getTrace" ),
    OP_DEF(    verboseBop,             "verbose" ),
    OP_DEF(    featuresOp,             "features" ),
    OP_DEF(    dumpCrashStateOp,       "dumpCrashState" ),
	OP_DEF(		ssPushBop,				">ss"),
	OP_DEF(		ssPopBop,				"ss>"),
	OP_DEF(		ssPeekBop,				"ss@"),
	OP_DEF(		ssDepthBop,				"ssdepth"),

    ///////////////////////////////////////////
    //  conditional compilation
    ///////////////////////////////////////////
    PRECOP_DEF( poundIfOp,              "#if" ),
    PRECOP_DEF( poundIfdefOp,           "#ifdef" ),
    PRECOP_DEF( poundIfndefOp,          "#ifndef" ),
    PRECOP_DEF( poundElseOp,            "#else" ),
    PRECOP_DEF( poundEndifOp,           "#endif" ),
    PRECOP_DEF( bracketIfOp,            "[if]" ),
    PRECOP_DEF( bracketIfDefOp,         "[ifdef]" ),
    PRECOP_DEF( bracketIfUndefOp,       "[ifundef]" ),
    PRECOP_DEF( bracketElseOp,          "[else]" ),
    PRECOP_DEF( bracketEndifOp,         "[endif]" ),
    PRECOP_DEF( bracketEndifOp,         "[then]" ),

    ///////////////////////////////////////////
    //  network/client/server
    ///////////////////////////////////////////
	OP_DEF(		serverOp,				"server" ),
	OP_DEF(		clientOp,				"client" ),
	OP_DEF(		shutdownOp,				"shutdown" ),

    ///////////////////////////////////////////
    //  threads
    ///////////////////////////////////////////
    OP_DEF( createThreadOp,             "createThread" ),
    OP_DEF( destroyThreadOp,            "destroyThread" ),
    OP_DEF( threadGetStateOp,           "threadGetState" ),
    OP_DEF( stepThreadOp,               "stepThread" ),
    OP_DEF( startThreadOp,              "startThread" ),
    OP_DEF( exitThreadOp,               "exitThread" ),

#ifdef WIN32
    ///////////////////////////////////////////
    //  Windows support
    ///////////////////////////////////////////
    OP_DEF( createEventOp,              "CreateEvent" ),
    OP_DEF( closeHandleOp,              "CloseHandle" ),
    OP_DEF( setEventOp,                 "SetEvent" ),
    OP_DEF( resetEventOp,               "ResetEvent" ),
    OP_DEF( pulseEventOp,               "PulseEvent" ),
    OP_DEF( getLastErrorOp,             "GetLastError" ),
    OP_DEF( waitForSingleObjectOp,      "WaitForSingleObject" ),
    OP_DEF( waitForMultipleObjectsOp,   "WaitForMultipleObjects" ),
    OP_DEF( initializeCriticalSectionOp, "InitializeCriticalSection" ),
    OP_DEF( deleteCriticalSectionOp,    "DeleteCriticalSection" ),
    OP_DEF( enterCriticalSectionOp,     "EnterCriticalSection" ),
    OP_DEF( leaveCriticalSectionOp,     "LeaveCriticalSection" ),
    OP_DEF( findFirstFileOp,            "FindFirstFile" ),
    OP_DEF( findNextFileOp,             "FindNextFile" ),
    OP_DEF( findCloseOp,                "FindClose" ),
    OP_DEF( showConsoleOp,              "showConsole" ),

    OP_DEF( windowsConstantsOp,         "windowsConstants" ),

	NATIVE_DEF( trueBop,				"WINDOWS" ),
#elif defined(LINUX)
	NATIVE_DEF( trueBop,				"LINUX" ),
#endif
    OP_DEF( setConsoleCursorOp,         "setConsoleCursor" ),
    OP_DEF( getConsoleCursorOp,         "getConsoleCursor" ),
    OP_DEF( getConsoleColorOp,          "getConsoleColor" ),
    OP_DEF( setConsoleColorOp,          "setConsoleColor" ),
    OP_DEF( clearConsoleOp,             "clearConsole" ),

	NATIVE_DEF( archARMBop,				"ARCH_ARM" ),
	NATIVE_DEF( archX86Bop,				"ARCH_X86" ),

    // following must be last in table
    OP_DEF(    NULL,                   NULL )
};


//############################################################################
//
//  The end of all things...
//
//############################################################################

void audioCallback(void *userdata, unsigned char *stream, int len)
{
    for ( int i = 0; i < len; i++ )
    {
        *stream++ = (i & 1) ? 127 : -128;
    }
}

void AddForthOps( ForthEngine* pEngine )
{
	baseDictionaryCompiledEntry* pCompEntry = baseCompiledDictionary;

	while ( pCompEntry->name != NULL )
	{
		long* pEntry = pEngine->AddBuiltinOp( pCompEntry->name, pCompEntry->flags, pCompEntry->value );
		gCompiledOps[ pCompEntry->index ] = *pEntry;
		pCompEntry++;
	}

	baseDictionaryEntry* pDictEntry = baseDictionary;

	while ( pDictEntry->name != NULL )
	{
		long* pEntry = pEngine->AddBuiltinOp( pDictEntry->name, pDictEntry->flags, pDictEntry->value );
		pDictEntry++;
	}
}

};  // end extern "C"


