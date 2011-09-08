//////////////////////////////////////////////////////////////////////
//
// ForthInner.cpp: implementation of the inner interpreter
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "ForthEngine.h"
#include "ForthThread.h"
#include "ForthShell.h"
#include "ForthVocabulary.h"


extern "C"
{

#ifdef _ASM_INNER_INTERPRETER
// UserCodeAction is used to execute user ops which are defined in assembler
extern void UserCodeAction( ForthCoreState *pCore, ulong opVal );
#endif

//////////////////////////////////////////////////////////////////////
////
///
//                     byte
// 

// doByte{Fetch,Ref,Store,PlusStore,MinusStore} are parts of doByteOp
VAR_ACTION( doByteFetch ) 
{
    // IP points to data field
    signed char c = *(signed char *)(SPOP);
    SPUSH( (long) c );
}

VAR_ACTION( doByteRef )
{
}

VAR_ACTION( doByteStore ) 
{
    // IP points to data field
    unsigned char *pA = (unsigned char *)(SPOP);
    *pA = (unsigned char) (SPOP);
}

VAR_ACTION( doBytePlusStore ) 
{
    // IP points to data field
    unsigned char *pA = (unsigned char *)(SPOP);
    *pA = (unsigned char) ((*pA) + SPOP);
}

VAR_ACTION( doByteMinusStore ) 
{
    // IP points to data field
    unsigned char *pA = (unsigned char *)(SPOP);
    *pA = (unsigned char) ((*pA) - SPOP);
}

VarAction byteOps[] =
{
    doByteFetch,
    doByteFetch,
    doByteRef,
    doByteStore,
    doBytePlusStore,
    doByteMinusStore
};

// this is an internal op that is compiled before the data field of each byte variable
GFORTHOP( doByteOp )
{
    // IP points to data field
    signed char* pVar = (signed char *)(GET_IP);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

VAR_ACTION( doUByteFetch ) 
{
    // IP points to data field
    unsigned char c = *(unsigned char *)(SPOP);
    SPUSH( (long) c );
}

VarAction ubyteOps[] =
{
    doUByteFetch,
    doUByteFetch,
    doByteRef,
    doByteStore,
    doBytePlusStore,
    doByteMinusStore
};

// this is an internal op that is compiled before the data field of each unsigned byte variable
GFORTHOP( doUByteOp )
{
    // IP points to data field
    unsigned char* pVar = (unsigned char *)(GET_IP);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            ubyteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalByteAction )
{
    signed char* pVar = (signed char *)(GET_FP - opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( LocalUByteAction )
{
    unsigned char* pVar = (unsigned char *)(GET_FP - opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( FieldByteAction )
{
    signed char* pVar = (signed char *)(SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( FieldUByteAction )
{
    unsigned char* pVar = (unsigned char *)(SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( MemberByteAction )
{
    signed char* pVar = (signed char *)(((long)(GET_TPD)) + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( MemberUByteAction )
{
    unsigned char* pVar = (unsigned char *)(((long)(GET_TPD)) + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

// this is an internal op that is compiled before the data field of each byte array
GFORTHOP( doByteArrayOp )
{
    signed char* pVar = (signed char *)(SPOP + (long)(GET_IP));
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

// this is an internal op that is compiled before the data field of each unsigned byte array
GFORTHOP( doUByteArrayOp )
{
    unsigned char* pVar = (unsigned char *)(SPOP + (long)(GET_IP));
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalByteArrayAction )
{
    signed char* pVar = (signed char *)(SPOP + ((long) (GET_FP - opVal)));
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( LocalUByteArrayAction )
{
    unsigned char* pVar = (unsigned char *)(SPOP + ((long) (GET_FP - opVal)));
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( FieldByteArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of byte[0]
    signed char* pVar = (signed char *)(SPOP + opVal);
    pVar += SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( FieldUByteArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of byte[0]
    unsigned char* pVar = (unsigned char *)(SPOP + opVal);
    pVar += SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( MemberByteArrayAction )
{
    // TOS is index
    // opVal is byte offset of byte[0]
    signed char* pVar = (signed char *)(((long)(GET_TPD)) + SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( MemberUByteArrayAction )
{
    // TOS is index
    // opVal is byte offset of byte[0]
    unsigned char* pVar = (unsigned char *)(((long)(GET_TPD)) + SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            byteOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}


//////////////////////////////////////////////////////////////////////
////
///
//                     short
// 

// doShort{Fetch,Ref,Store,PlusStore,MinusStore} are parts of doShortOp
VAR_ACTION( doShortFetch )
{
    // IP points to data field
    short s = *(short *)(SPOP);
    SPUSH( (long) s );
}

VAR_ACTION( doShortRef )
{
}

VAR_ACTION( doShortStore ) 
{
    // IP points to data field
    short *pA = (short *)(SPOP);
    *pA = (short) (SPOP);
}

VAR_ACTION( doShortPlusStore ) 
{
    // IP points to data field
    short *pA = (short *)(SPOP);
    *pA = (short)((*pA) + SPOP);
}

VAR_ACTION( doShortMinusStore ) 
{
    // IP points to data field
    short *pA = (short *)(SPOP);
    *pA = (short)((*pA) - SPOP);
}

VarAction shortOps[] =
{
    doShortFetch,
    doShortFetch,
    doShortRef,
    doShortStore,
    doShortPlusStore,
    doShortMinusStore
};

// this is an internal op that is compiled before the data field of each short variable
GFORTHOP( doShortOp )
{
    // IP points to data field
    short* pVar = (short *)(GET_IP);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

VAR_ACTION( doUShortFetch )
{
    // IP points to data field
    unsigned short s = *(unsigned short *)(SPOP);
    SPUSH( (long) s );
}

VarAction ushortOps[] =
{
    doUShortFetch,
    doUShortFetch,
    doShortRef,
    doShortStore,
    doShortPlusStore,
    doShortMinusStore
};

// this is an internal op that is compiled before the data field of each unsigned short variable
GFORTHOP( doUShortOp )
{
    // IP points to data field
    unsigned short* pVar = (unsigned short *)(GET_IP);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            ushortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalShortAction )
{
    short* pVar = (short *)(GET_FP - opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( LocalUShortAction )
{
    unsigned short* pVar = (unsigned short *)(GET_FP - opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            ushortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( FieldShortAction )
{
    short* pVar = (short *)(SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( FieldUShortAction )
{
    unsigned short* pVar = (unsigned short *)(SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            ushortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( MemberShortAction )
{
    short* pVar = (short *)(((long)(GET_TPD)) + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( MemberUShortAction )
{
    unsigned short* pVar = (unsigned short *)(((long)(GET_TPD)) + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            ushortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

// this is an internal op that is compiled before the data field of each short array
GFORTHOP( doShortArrayOp )
{
    // IP points to data field
    short* pVar = ((short *) (GET_IP)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

GFORTHOP( doUShortArrayOp )
{
    // IP points to data field
    unsigned short* pVar = ((unsigned short *) (GET_IP)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalShortArrayAction )
{
    short* pVar = ((short *) (GET_FP - opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( LocalUShortArrayAction )
{
    unsigned short* pVar = ((unsigned short *) (GET_FP - opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( FieldShortArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of short[0]
    short* pVar = (short *)(SPOP + opVal);
    pVar += SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( FieldUShortArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of short[0]
    unsigned short* pVar = (unsigned short *)(SPOP + opVal);
    pVar += SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( MemberShortArrayAction )
{
    // TOS is index
    // opVal is byte offset of byte[0]
    short* pVar = ((short *) (((long)(GET_TPD)) + opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}

OPTYPE_ACTION( MemberUShortArrayAction )
{
    // TOS is index
    // opVal is byte offset of byte[0]
    unsigned short* pVar = ((unsigned short *) (((long)(GET_TPD)) + opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            shortOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (long) *pVar );
    }
}



//////////////////////////////////////////////////////////////////////
////
///
//                     int
// 

// doInt{Fetch,Ref,Store,PlusStore,MinusStore} are parts of doIntOp
VAR_ACTION( doIntFetch ) 
{
    // IP points to data field
    long *pA = (long *) (SPOP);
    SPUSH( *pA );
}

VAR_ACTION( doIntRef )
{
}

VAR_ACTION( doIntStore ) 
{
    // IP points to data field
    long *pA = (long *) (SPOP);
    *pA = SPOP;
}

VAR_ACTION( doIntPlusStore ) 
{
    // IP points to data field
    long *pA = (long *) (SPOP);
    *pA += SPOP;
}

VAR_ACTION( doIntMinusStore ) 
{
    // IP points to data field
    long *pA = (long *) (SPOP);
    *pA -= SPOP;
}

VarAction intOps[] =
{
    doIntFetch,
    doIntFetch,
    doIntRef,
    doIntStore,
    doIntPlusStore,
    doIntMinusStore
};

// this is an internal op that is compiled before the data field of each int variable
GFORTHOP( doIntOp )
{
    // IP points to data field
    long* pVar = (long *)(GET_IP);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            intOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalIntAction )
{
    long* pVar = (long *)(GET_FP - opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            intOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *pVar );
    }
}


OPTYPE_ACTION( FieldIntAction )
{
    long* pVar = (long *)(SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            intOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *pVar );
    }
}

OPTYPE_ACTION( MemberIntAction )
{
    long *pVar = (long *) (((long)(GET_TPD)) + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            intOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *pVar );
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doIntArrayOp )
{
    // IP points to data field
    long* pVar = ((long *) (GET_IP)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            intOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalIntArrayAction )
{
    long* pVar = ((long *) (GET_FP - opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            intOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *pVar );
    }
}

OPTYPE_ACTION( FieldIntArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of int[0]
    long* pVar = (long *)(SPOP + opVal);
    pVar += SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            intOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *pVar );
    }
}

OPTYPE_ACTION( MemberIntArrayAction )
{
    // TOS is index
    // opVal is byte offset of byte[0]
    long* pVar = ((long *) (((long)(GET_TPD)) + opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            intOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *pVar );
    }
}


//////////////////////////////////////////////////////////////////////
////
///
//                     float
// 

VAR_ACTION( doFloatPlusStore ) 
{
    // IP points to data field
    float *pA = (float *) (SPOP);
    *pA += FPOP;
}

VAR_ACTION( doFloatMinusStore ) 
{
    // IP points to data field
    float *pA = (float *) (SPOP);
    *pA -= FPOP;
}

VarAction floatOps[] =
{
    doIntFetch,
    doIntFetch,
    doIntRef,
    doIntStore,
    doFloatPlusStore,
    doFloatMinusStore
};

GFORTHOP( doFloatOp )
{    
    // IP points to data field
    float* pVar = (float *)(GET_IP);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            floatOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *((long *) pVar) );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalFloatAction )
{
    float* pVar = (float *)(GET_FP - opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            floatOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *((long *) pVar) );
    }
}

OPTYPE_ACTION( FieldFloatAction )
{
    float* pVar = (float *)(SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            floatOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *((long *) pVar) );
    }
}

OPTYPE_ACTION( MemberFloatAction )
{
    float *pVar = (float *) (((long)(GET_TPD)) + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            floatOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *((long *) pVar) );
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doFloatArrayOp )
{
    // IP points to data field
    float* pVar = ((float *) (GET_IP)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            floatOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *((long *) pVar) );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalFloatArrayAction )
{
    float* pVar = ((float *) (GET_FP - opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            floatOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *((long *) pVar) );
    }
}

OPTYPE_ACTION( FieldFloatArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of float[0]
    float* pVar = (float *)(SPOP + opVal);
    pVar += SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            floatOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *((long *) pVar) );
    }
}

OPTYPE_ACTION( MemberFloatArrayAction )
{
    // TOS is index
    // opVal is byte offset of byte[0]
    float* pVar = ((float *) (((long)(GET_TPD)) + opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            floatOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *((long *) pVar) );
    }
}


//////////////////////////////////////////////////////////////////////
////
///
//                     double
// 


VAR_ACTION( doDoubleFetch ) 
{
    // IP points to data field
    double *pA = (double *) (SPOP);
    DPUSH( *pA );
}

VAR_ACTION( doDoubleStore ) 
{
    // IP points to data field
    double *pA = (double *) (SPOP);
    *pA = DPOP;
}

VAR_ACTION( doDoublePlusStore ) 
{
    // IP points to data field
    double *pA = (double *) (SPOP);
    *pA += DPOP;
}

VAR_ACTION( doDoubleMinusStore ) 
{
    // IP points to data field
    double *pA = (double *) (SPOP);
    *pA -= DPOP;
}

VarAction doubleOps[] =
{
    doDoubleFetch,
    doDoubleFetch,
    doIntRef,
    doDoubleStore,
    doDoublePlusStore,
    doDoubleMinusStore
};

GFORTHOP( doDoubleOp )
{
    // IP points to data field
    double* pVar = (double *)(GET_IP);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            doubleOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalDoubleAction )
{
    double* pVar = (double *)(GET_FP - opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            doubleOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}


OPTYPE_ACTION( FieldDoubleAction )
{
    double* pVar = (double *)(SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            doubleOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}


OPTYPE_ACTION( MemberDoubleAction )
{
    double *pVar = (double *) (((long)(GET_TPD)) + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            doubleOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doDoubleArrayOp )
{
    // IP points to data field
    double* pVar = ((double *) (GET_IP)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            doubleOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalDoubleArrayAction )
{
    double* pVar = ((double *) (GET_FP - opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            doubleOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}

OPTYPE_ACTION( FieldDoubleArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of double[0]
    double* pVar = (double *)(SPOP + opVal);
    pVar += SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            doubleOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}

OPTYPE_ACTION( MemberDoubleArrayAction )
{
    // TOS is index
    // opVal is byte offset of byte[0]
    double* pVar = ((double *) (((long)(GET_TPD)) + opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            doubleOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}


//////////////////////////////////////////////////////////////////////
////
///
//                     string
// 
// a string has 2 longs at its start:
// - maximum string length
// - current string length

VAR_ACTION( doStringFetch )
{
    // TOS:  ptr to dst maxLen field
    long a = (long) (SPOP + 8);
    SPUSH( a );
}

VAR_ACTION( doStringStore ) 
{
    // TOS:  ptr to dst maxLen field, ptr to src first byte
    long *pLen = (long *) (SPOP);
    char *pSrc = (char *) (SPOP);
    long srcLen = strlen( pSrc );
    long maxLen = *pLen++;
    if ( srcLen > maxLen )
    {
        srcLen = maxLen;
    }
    // set current length
    *pLen++ = srcLen;
    char *pDst = (char *) pLen;
    strncpy( pDst, pSrc, srcLen );
    pDst[ srcLen ] = 0;
}

VAR_ACTION( doStringAppend ) 
{
    // TOS:  ptr to dst maxLen field, ptr to src first byte
    long *pLen = (long *) (SPOP);
    char *pSrc = (char *) (SPOP);
    long srcLen = strlen( pSrc );
    long maxLen = *pLen++;
    long curLen = *pLen;
    long newLen = curLen + srcLen;
    if ( newLen > maxLen )
    {
        newLen = maxLen;
        srcLen = maxLen - curLen;
    }
    // set current length
    *pLen++ = newLen;
    char *pDst = (char *) pLen;
    strncat( pDst, pSrc, srcLen );
    pDst[ newLen ] = 0;
}

VarAction stringOps[] =
{
    doStringFetch,
    doStringFetch,
    doIntRef,
    doStringStore,
    doStringAppend
};

GFORTHOP( doStringOp )
{
    char* pVar = (char *) (GET_IP);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarPlusStore )
        {
            SPUSH( (long) pVar );
            stringOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - skip the maxLen/curLen fields
        SPUSH( ((long) pVar) + 8 );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalStringAction )
{
    char* pVar = (char *) (GET_FP - opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarPlusStore )
        {
            SPUSH( (long) pVar );
            stringOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - skip the maxLen/curLen fields
        SPUSH( ((long) pVar) + 8 );
    }
}


OPTYPE_ACTION( FieldStringAction )
{
    char* pVar = (char *) (SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarPlusStore )
        {
            SPUSH( (long) pVar );
            stringOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - skip the maxLen/curLen fields
        SPUSH( ((long) pVar) + 8 );
    }
}


OPTYPE_ACTION( MemberStringAction )
{
    char *pVar = (char *) (((long)(GET_TPD)) + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarPlusStore )
        {
            SPUSH( (long) pVar );
            stringOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - skip the maxLen/curLen fields
        SPUSH( ((long) pVar) + 8 );
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doStringArrayOp )
{
    // IP points to data field
    long *pLongs = GET_IP;
    int index = SPOP;
    long len = ((*pLongs) >> 2) + 3;      // length of one string in longwords
    char *pVar = (char *) (pLongs + (index * len));
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarPlusStore )
        {
            SPUSH( (long) pVar );
            stringOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - skip the maxLen/curLen fields
        SPUSH( ((long) pVar) + 8 );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalStringArrayAction )
{
    long *pLongs = GET_FP - opVal;
    int index = SPOP;
    long len = ((*pLongs) >> 2) + 3;      // length of one string in longwords
    char *pVar = (char *) (pLongs + (index * len));
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarPlusStore )
        {
            SPUSH( (long) pVar );
            stringOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - skip the maxLen/curLen fields
        SPUSH( ((long) pVar) + 8 );
    }
}

OPTYPE_ACTION( FieldStringArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of string[0]
    long *pLongs = (long *) (SPOP + opVal);
    int index = SPOP;
    long len = ((*pLongs) >> 2) + 3;      // length of one string in longwords
    char *pVar = (char *) (pLongs + (index * len));
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarPlusStore )
        {
            SPUSH( (long) pVar );
            stringOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - skip the maxLen/curLen fields
        SPUSH( ((long) pVar) + 8 );
    }
}

OPTYPE_ACTION( MemberStringArrayAction )
{
    // TOS is index
    // opVal is byte offset of string[0]
    long *pLongs = (long *) ((long)(GET_TPD) + opVal);
    int index = SPOP;
    long len = ((*pLongs) >> 2) + 3;      // length of one string in longwords
    char *pVar = (char *) (pLongs + (index * len));
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarPlusStore )
        {
            SPUSH( (long) pVar );
            stringOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - skip the maxLen/curLen fields
        SPUSH( ((long) pVar) + 8 );
    }
}


//////////////////////////////////////////////////////////////////////
////
///
//                     op
// 

VAR_ACTION( doOpExecute ) 
{
    ((ForthEngine *)pCore->pEngine)->ExecuteOneOp( *(long *)(SPOP) );
}

VarAction opOps[] =
{
    doOpExecute,
    doIntFetch,
    doIntRef,
    doIntStore,
};

GFORTHOP( doOpOp )
{    
    // IP points to data field
    long* pVar = (long *)(GET_IP);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            opOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - execute the op
        ((ForthEngine *)pCore->pEngine)->ExecuteOneOp( *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalOpAction )
{
    long* pVar = (long *)(GET_FP - opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            opOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - execute the op
        ((ForthEngine *)pCore->pEngine)->ExecuteOneOp( *pVar );
    }
}


OPTYPE_ACTION( FieldOpAction )
{
    long* pVar = (long *)(SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            opOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - execute the op
        ((ForthEngine *)pCore->pEngine)->ExecuteOneOp( *pVar );
    }
}


OPTYPE_ACTION( MemberOpAction )
{
    long *pVar = (long *) (((long)(GET_TPD)) + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            opOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - execute the op
        ((ForthEngine *)pCore->pEngine)->ExecuteOneOp( *pVar );
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doOpArrayOp )
{
    // IP points to data field
    long* pVar = ((long *) (GET_IP)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            opOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - execute the op
        ((ForthEngine *)pCore->pEngine)->ExecuteOneOp( *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalOpArrayAction )
{
    long* pVar = ((long *) (GET_FP - opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            opOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - execute the op
        ((ForthEngine *)pCore->pEngine)->ExecuteOneOp( *pVar );
    }
}

OPTYPE_ACTION( FieldOpArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of op[0]
    long* pVar = (long *)(SPOP + opVal);
    pVar += SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            opOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - execute the op
        ((ForthEngine *)pCore->pEngine)->ExecuteOneOp( *pVar );
    }
}

OPTYPE_ACTION( MemberOpArrayAction )
{
    // TOS is index
    // opVal is byte offset of byte[0]
    long* pVar = ((long *) (((long)(GET_TPD)) + opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            opOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - execute the op
        ((ForthEngine *)pCore->pEngine)->ExecuteOneOp( *pVar );
    }
}


//////////////////////////////////////////////////////////////////////
////
///
//                     object
// 


VarAction objectOps[] =
{
    doDoubleFetch,
    doDoubleFetch,
    doIntRef,
    doDoubleStore,
};

GFORTHOP( doObjectOp )
{
    // IP points to data field
    double* pVar = (double *)(GET_IP);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            objectOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalObjectAction )
{
    double* pVar = (double *)(GET_FP - opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            objectOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}


OPTYPE_ACTION( FieldObjectAction )
{
    double* pVar = (double *)(SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            objectOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}


OPTYPE_ACTION( MemberObjectAction )
{
    double *pVar = (double *) (((long)(GET_TPD)) + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            objectOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doObjectArrayOp )
{
    // IP points to data field
    double* pVar = ((double *) (GET_IP)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            objectOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalObjectArrayAction )
{
    double* pVar = ((double *) (GET_FP - opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            objectOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}

OPTYPE_ACTION( FieldObjectArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of double[0]
    double* pVar = (double *)(SPOP + opVal);
    pVar += SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            objectOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}

OPTYPE_ACTION( MemberObjectArrayAction )
{
    // TOS is index
    // opVal is byte offset of byte[0]
    double* pVar = ((double *) (((long)(GET_TPD)) + opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            objectOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}


//////////////////////////////////////////////////////////////////////
////
///
//                     long - 64 bit integer
// 


VAR_ACTION( doLongFetch ) 
{
    // IP points to data field
    long long *pA = (long long *) (SPOP);
    LPUSH( *pA );
}

VAR_ACTION( doLongStore ) 
{
    // IP points to data field
    long long *pA = (long long *) (SPOP);
    *pA = LPOP;
}

VAR_ACTION( doLongPlusStore ) 
{
    // IP points to data field
    long long *pA = (long long *) (SPOP);
    *pA += LPOP;
}

VAR_ACTION( doLongMinusStore ) 
{
    // IP points to data field
    long long *pA = (long long *) (SPOP);
    *pA -= LPOP;
}

VarAction longOps[] =
{
    doLongFetch,
    doLongFetch,
    doIntRef,
    doLongStore,
    doLongPlusStore,
    doLongMinusStore
};

GFORTHOP( doLongOp )
{
    // IP points to data field
    long long* pVar = (long long *)(GET_IP);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarStore )
        {
            SPUSH( (long) pVar );
            longOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        LPUSH( *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalLongAction )
{
    long long* pVar = (long long *)(GET_FP - opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            longOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        LPUSH( *pVar );
    }
}


OPTYPE_ACTION( FieldLongAction )
{
    long long* pVar = (long long *)(SPOP + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            longOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        LPUSH( *pVar );
    }
}


OPTYPE_ACTION( MemberLongAction )
{
    long long* pVar = (long long *) (((long)(GET_TPD)) + opVal);
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            longOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        LPUSH( *pVar );
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doLongArrayOp )
{
    // IP points to data field
    long long* pVar = ((long long *) (GET_IP)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            longOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        LPUSH( *pVar );
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalLongArrayAction )
{
    long long* pVar = ((long long *) (GET_FP - opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            longOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        LPUSH( *pVar );
    }
}

OPTYPE_ACTION( FieldLongArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of double[0]
    long long* pVar = (long long *)(SPOP + opVal);
    pVar += SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            longOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        LPUSH( *pVar );
    }
}

OPTYPE_ACTION( MemberLongArrayAction )
{
    // TOS is index
    // opVal is byte offset of byte[0]
    long long* pVar = ((long long *) (((long)(GET_TPD)) + opVal)) + SPOP;
    ulong varOp = GET_VAR_OPERATION;
    if ( varOp )
    {
        if ( varOp <= kVarMinusStore )
        {
            SPUSH( (long) pVar );
            longOps[ varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            ((ForthEngine *)pCore->pEngine)->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        LPUSH( *pVar );
    }
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

OPTYPE_ACTION( BuiltinAction )
{
    ForthOp opRoutine;
    // op is builtin
    if ( opVal < pCore->numBuiltinOps )
    {
        opRoutine = pCore->builtinOps[opVal];
        opRoutine( pCore );
    }
    else
    {
        SET_ERROR( kForthErrorBadOpcode );
    }
}

OPTYPE_ACTION( UserDefAction )
{
    // op is normal user-defined, push IP on rstack, lookup new IP
    //  in table of user-defined ops
    if ( opVal < GET_NUM_USER_OPS )
    {
        RPUSH( (long) GET_IP );
        SET_IP( USER_OP_TABLE[opVal] );
    }
    else
    {
        SET_ERROR( kForthErrorBadOpcode );
    }
}

OPTYPE_ACTION( BranchAction )
{
    if ( (opVal & 0x00800000) != 0 )
    {
        // TBD: trap a hard loop (opVal == -1)?
        opVal |= 0xFF000000;
    }
    SET_IP( GET_IP + opVal );
}

OPTYPE_ACTION( BranchNZAction )
{
    if ( SPOP != 0 )
    {
        if ( (opVal & 0x00800000) != 0 )
        {
            // TBD: trap a hard loop (opVal == -1)?
            opVal |= 0xFF000000;
        }
        SET_IP( GET_IP + opVal );
    }
}

OPTYPE_ACTION( BranchZAction )
{
    if ( SPOP == 0 )
    {
        if ( (opVal & 0x00800000) != 0 )
        {
            // TBD: trap a hard loop (opVal == -1)?
            opVal |= 0xFF000000;
        }
        SET_IP( GET_IP + opVal );
    }
}

OPTYPE_ACTION( CaseBranchAction )
{
    // TOS: this_case_value case_selector
    long *pSP = GET_SP;
    if ( *pSP == pSP[1] )
    {
        // case matched
        pSP += 2;
    }
    else
    {
        // no match - drop this_case_value & skip to next case
        pSP++;
        // case branch is always forward
        SET_IP( GET_IP + opVal );
    }
    SET_SP( pSP );
}

OPTYPE_ACTION( ConstantAction )
{
    // push constant in opVal
    if ( (opVal & 0x00800000) != 0 )
    {
      opVal |= 0xFF000000;
    }
    SPUSH( opVal );
}

OPTYPE_ACTION( OffsetAction )
{
    // push constant in opVal
    if ( (opVal & 0x00800000) != 0 )
    {
      opVal |= 0xFF000000;
    }
    long v = SPOP + opVal;
    SPUSH( v );
}

OPTYPE_ACTION( OffsetFetchAction )
{
    // push constant in opVal
    if ( (opVal & 0x00800000) != 0 )
    {
      opVal |= 0xFF000000;
    }
    long v = *((long *)(SPOP + opVal));
    SPUSH( v );
}

OPTYPE_ACTION( ArrayOffsetAction )
{
    // opVal is array element size
    // TOS is array base, index
    char* pArray = (char *) (SPOP);
    pArray += ((SPOP) * opVal);
    SPUSH( (long) pArray );
}

OPTYPE_ACTION( LocalStructArrayAction )
{
    // bits 0..11 are padded struct length in bytes, bits 12..23 are frame offset in longs
    // init the current & max length fields of a local string
    long* pStruct = GET_FP - (opVal >> 12);
    long offset = ((opVal & 0xFFF) * SPOP) + ((long) pStruct);
    SPUSH( offset );
}

OPTYPE_ACTION( ConstantStringAction )
{
    // push address of immediate string & skip over
    // opVal is number of longwords in string
    long *pIP = GET_IP;
    SPUSH( (long) pIP );
    SET_IP( pIP + opVal );
}

OPTYPE_ACTION( AllocLocalsAction )
{
    // allocate a local var stack frame
    RPUSH( (long) GET_FP );      // rpush old FP
    SET_FP( GET_RP );                // set FP = RP, points at oldFP
    SET_RP( GET_RP - opVal );                // allocate storage for local vars
}

OPTYPE_ACTION( InitLocalStringAction )
{
    // bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
    // init the current & max length fields of a local string
    long* pFP = GET_FP;
    long* pStr = pFP - (opVal >> 12);
    *pStr++ = (opVal & 0xFFF);          // max length
    *pStr++ = 0;                        // current length
    *((char *) pStr) = 0;               // terminating null
}

OPTYPE_ACTION( LocalRefAction )
{
    // opVal is offset in longs
    SPUSH( (long)(GET_FP - opVal) );
}

OPTYPE_ACTION( MemberRefAction )
{
    // opVal is offset in bytes
    SPUSH( ((long)GET_TPD) + opVal );
}

// bits 0..18 are index into ForthCoreState userOps table, 19..23 are arg count
OPTYPE_ACTION( DLLEntryPointAction )
{
#ifdef _WINDOWS
    ulong entryIndex = CODE_TO_DLL_ENTRY_INDEX( opVal );
    ulong argCount = CODE_TO_DLL_ENTRY_NUM_ARGS( opVal );
    if ( entryIndex < GET_NUM_USER_OPS )
    {
        CallDLLRoutine( (DLLRoutine)(USER_OP_TABLE[entryIndex]), argCount, pCore );
    }
    else
    {
        SET_ERROR( kForthErrorBadOpcode );
    }
#endif
}

OPTYPE_ACTION( MethodWithThisAction )
{
    // this is called when an object method invokes another method on itself
    // opVal is the method number
    ForthEngine *pEngine = GET_ENGINE;
    long* pMethods = GET_TPM;
    RPUSH( ((long) GET_TPD) );
    RPUSH( ((long) pMethods) );
    pEngine->ExecuteOneOp( pMethods[ opVal ] );
}

OPTYPE_ACTION( MethodWithTOSAction )
{
    // TOS is object (top is vtable, next is data)
    // this is called when a method is invoked from inside another
    // method in the same class - the difference is that in this case
    // there is no explicit source for the "this" pointer, we just keep
    // on using the current "this" pointer
    ForthEngine *pEngine = GET_ENGINE;
    RPUSH( ((long) GET_TPD) );
    RPUSH( ((long) GET_TPM) );
    long* pMethods = (long *)(SPOP);
    SET_TPM( pMethods );
    SET_TPD( (long *) (SPOP) );
    pEngine->ExecuteOneOp( pMethods[ opVal ] );
}

OPTYPE_ACTION( InitMemberStringAction )
{
    // bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
    // init the current & max length fields of a local string
    long* pThis = GET_TPD;
    long* pStr = pThis + (opVal >> 12);
    *pStr++ = (opVal & 0xFFF);          // max length
    *pStr++ = 0;                        // current length
    *((char *) pStr) = 0;               // terminating null
}

OPTYPE_ACTION( IllegalOptypeAction )
{
    SET_ERROR( kForthErrorBadOpcodeType );
}

OPTYPE_ACTION( ReservedOptypeAction )
{
    SET_ERROR( kForthErrorBadOpcodeType );
}

OPTYPE_ACTION( MethodAction )
{
#if 0
    // token is object method invocation
    long op = GET_CURRENT_OP;
    forthOpType opType = FORTH_OP_TYPE( op );
    int methodNum = ((int) opType) & 0x7F;
    long* pObj = NULL;
    if ( opVal & 0x00800000 )
    {
        // object ptr is in local variable
        pObj = GET_FP - (opVal & 0x007F0000);
    }
    else
    {
        // object ptr is in global op
        if ( opVal < pCore->numUserOps )
        {
            pObj = (long *) (pCore->userOps[opVal]);
        }
        else
        {
            SET_ERROR( kForthErrorBadOpcode );
        }
    }
    if ( pObj != NULL )
    {
        // pObj is a pair of pointers, first pointer is to
        //   class descriptor for this type of object,
        //   second pointer is to storage for object (this ptr)
        long *pClass = (long *) (*pObj);
        if ( (pClass[1] == CLASS_MAGIC_NUMBER)
            && (pClass[2] > methodNum) )
        {
            RPUSH( (long) GET_IP );
            RPUSH( (long) GET_TP );
            SET_TP( pObj );
            SET_IP( (long *) (pClass[methodNum + 3]) );
        }
        else
        {
            // bad class magic number, or bad method number
            SET_ERROR( kForthErrorBadOpcode );
        }
    }
    else
    {
        SET_ERROR( kForthErrorBadOpcode );
    }
#endif
}

// NOTE: there is no opcode assigned to this op
FORTHOP( BadOpcodeOp )
{
    SET_ERROR( kForthErrorBadOpcode );
}

optypeActionRoutine builtinOptypeAction[] =
{
    // 00 - 09
    BuiltinAction,
    BuiltinAction,          // immediate
    UserDefAction,
    UserDefAction,          // immediate
#ifdef _ASM_INNER_INTERPRETER
    UserCodeAction,
    UserCodeAction,         // immediate
#else
    ReservedOptypeAction,
    ReservedOptypeAction,
#endif
    DLLEntryPointAction,
    ReservedOptypeAction,
    ReservedOptypeAction,
    ReservedOptypeAction,

    // 10 - 19
    BranchAction,
    BranchNZAction,
    BranchZAction,
    CaseBranchAction,
    ReservedOptypeAction,
    ReservedOptypeAction,
    ReservedOptypeAction,
    ReservedOptypeAction,
    ReservedOptypeAction,
    ReservedOptypeAction,

    // 20 - 29
    ConstantAction,
    ConstantStringAction,
    OffsetAction,
    ArrayOffsetAction,
    AllocLocalsAction,
    LocalRefAction,
    InitLocalStringAction,
    LocalStructArrayAction,
    OffsetFetchAction,
    MemberRefAction,

    // 30 -39
    LocalByteAction,
    LocalShortAction,
    LocalIntAction,
    LocalFloatAction,
    LocalDoubleAction,
    LocalStringAction,
    LocalOpAction,
    LocalLongAction,
    LocalObjectAction,
    LocalUByteAction,

    // 40 - 49
    LocalUShortAction,
    LocalByteArrayAction,
    LocalShortArrayAction,
    LocalIntArrayAction,
    LocalFloatArrayAction,
    LocalDoubleArrayAction,
    LocalStringArrayAction,
    LocalOpArrayAction,
    LocalLongArrayAction,
    LocalObjectArrayAction,

    // 50 - 59
    LocalUByteArrayAction,
    LocalUShortArrayAction,
    FieldByteAction,
    FieldShortAction,
    FieldIntAction,
    FieldFloatAction,
    FieldDoubleAction,
    FieldStringAction,
    FieldOpAction,
    FieldLongAction,

    // 60 - 69
    FieldObjectAction,
    FieldUByteAction,
    FieldUShortAction,
    FieldByteArrayAction,
    FieldShortArrayAction,
    FieldIntArrayAction,
    FieldFloatArrayAction,
    FieldDoubleArrayAction,
    FieldStringArrayAction,
    FieldOpArrayAction,

    // 70 - 79
    FieldLongArrayAction,
    FieldObjectArrayAction,
    FieldUByteArrayAction,
    FieldUShortArrayAction,
    MemberByteAction,
    MemberShortAction,
    MemberIntAction,
    MemberFloatAction,
    MemberDoubleAction,
    MemberStringAction,

    // 80 - 89
    MemberOpAction,
    MemberLongAction,
    MemberObjectAction,
    MemberUByteAction,
    MemberUShortAction,
    MemberByteArrayAction,
    MemberShortArrayAction,
    MemberIntArrayAction,
    MemberFloatArrayAction,
    MemberDoubleArrayAction,

    // 90 - 99
    MemberStringArrayAction,
    MemberOpArrayAction,
    MemberLongArrayAction,
    MemberObjectArrayAction,
    MemberUByteArrayAction,
    MemberUShortArrayAction,
    MethodWithThisAction,
    MethodWithTOSAction,
    InitMemberStringAction,
    ReservedOptypeAction,

    NULL            // this must be last to end the list
};

void InitDispatchTables( ForthCoreState* pCore )
{
    int i;

    for ( i = 0; i < 256; i++ )
    {
        pCore->optypeAction[i] = IllegalOptypeAction;
    }

    for ( i = 0; i < MAX_BUILTIN_OPS; i++ )
    {
        pCore->builtinOps[i] = BadOpcodeOp;
    }
    for ( i = 0; builtinOptypeAction[i] != NULL; i++ )
    {
        pCore->optypeAction[i] = builtinOptypeAction[i];
    }
}

void CoreSetError( ForthCoreState *pCore, eForthError error, bool isFatal )
{
    pCore->error =  error;
    pCore->state = (isFatal) ? kResultFatalError : kResultError;
}

//############################################################################
//
//          I N N E R    I N T E R P R E T E R
//
//############################################################################



eForthResult
InnerInterpreter( ForthCoreState *pCore )
{
    ulong opVal, numBuiltinOps;
    forthOpType opType;
    long *pIP;
    long op;
    numBuiltinOps = pCore->numBuiltinOps;

    SET_STATE( kResultOk );
    
    while ( GET_STATE == kResultOk )
    {
        // fetch op at IP, advance IP
        pIP = GET_IP;
#ifdef TRACE_INNER_INTERPRETER
        GET_ENGINE->TraceOp( pCore );
#endif
        op = *pIP++;
#if 0
        if ( op < pCore->numBuiltinOps )
        {
            pCore->builtinOps[ op ]( pCore );
        }
        else
        {
            SET_IP( pIP );
            opType = FORTH_OP_TYPE( op );
            opVal = FORTH_OP_VALUE( op );
            pCore->optypeAction[ (int) opType ]( pCore, opVal );
        }
#else
        SET_IP( pIP );
        opType = FORTH_OP_TYPE( op );
        opVal = FORTH_OP_VALUE( op );
        pCore->optypeAction[ (int) opType ]( pCore, opVal );
#endif
    }

    return GET_STATE;
}


};      // end extern "C"
