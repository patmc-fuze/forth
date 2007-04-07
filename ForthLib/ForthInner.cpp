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
#include "ForthClass.h"

extern baseDictEntry baseDict[];

extern "C" {


//////////////////////////////////////////////////////////////////////
////
///
//                     byte
// 

// doByte{Fetch,Ref,Store,PlusStore,MinusStore} are parts of doByteOp
VAR_ACTION( doByteFetch ) 
{
    // IP points to data field
    unsigned char c = *(unsigned char *)(SPOP);
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

VarAction byteOps[] = {
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
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long) (GET_IP) );
        byteOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalByteAction )
{
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long)(GET_FP - opVal) );
        // TOS points to data field
        byteOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

OPTYPE_ACTION( FieldByteAction )
{
    long pVar = SPOP + opVal;
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        // TOS points to data field
        byteOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

// this is an internal op that is compiled before the data field of each byte array
GFORTHOP( doByteArrayOp )
{
    // IP points to data field
    int index = SPOP;
    SPUSH( ((long) (GET_IP)) + index );
    byteOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalByteArrayAction )
{
    int index = SPOP;
    SPUSH( ((long) (GET_FP - opVal)) + index );
    byteOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
}

OPTYPE_ACTION( FieldByteArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of byte[0]
    long pVar = SPOP + opVal;
    pVar += SPOP;
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        // TOS points to data field
        byteOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
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
    short c = *(short *)(SPOP);
    SPUSH( (long) c );
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

VarAction shortOps[] = {
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
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long) (GET_IP) );
        shortOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalShortAction )
{
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long)(GET_FP - opVal) );
        // TOS points to data field
        shortOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

OPTYPE_ACTION( FieldShortAction )
{
    long pVar = SPOP + opVal;
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        // TOS points to data field
        shortOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

// this is an internal op that is compiled before the data field of each short array
GFORTHOP( doShortArrayOp )
{
    // IP points to data field
    int index = (SPOP) << 1;
    SPUSH( ((long) (GET_IP)) + index );
    shortOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalShortArrayAction )
{
    int index = (SPOP) << 1;
    SPUSH( ((long) (GET_FP - opVal)) + index );
    shortOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
}

OPTYPE_ACTION( FieldShortArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of short[0]
    long pVar = SPOP + opVal;
    pVar += (SPOP << 1);
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        // TOS points to data field
        shortOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
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

VarAction intOps[] = {
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
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long) (GET_IP) );
        intOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalIntAction )
{
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long)(GET_FP - opVal) );
        // TOS points to data field
        intOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


OPTYPE_ACTION( FieldIntAction )
{
    long pVar = SPOP + opVal;
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        // TOS points to data field
        intOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

OPTYPE_ACTION( MemberIntAction )
{
    long *pInt = ((long *) (GET_TP[1])) + opVal;
    if ( GET_VAR_OPERATION == kVarFetch ) {

        SPUSH( *pInt );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        SPUSH( (long)pInt );
        // TOS points to data field
        intOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doIntArrayOp )
{
    // IP points to data field
    int index = SPOP;
    SPUSH( ((long) ((GET_IP) + index)) );
    intOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalIntArrayAction )
{
    int index = SPOP;
    SPUSH( ((long) ((GET_FP - opVal) + index)) );
    intOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
}

OPTYPE_ACTION( FieldIntArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of int[0]
    long pVar = SPOP + opVal;
    pVar += (SPOP << 2);
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        // TOS points to data field
        intOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
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

VarAction floatOps[] = {
    doIntFetch,
    doIntRef,
    doIntStore,
    doFloatPlusStore,
    doFloatMinusStore
};

GFORTHOP( doFloatOp )
{    
    // IP points to data field
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long) (GET_IP) );
        floatOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalFloatAction )
{
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long)(GET_FP - opVal) );
        // TOS points to data field
        floatOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

OPTYPE_ACTION( FieldFloatAction )
{
    long pVar = SPOP + opVal;
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        // TOS points to data field
        floatOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

OPTYPE_ACTION( MemberFloatAction )
{
    float *pFloat = ((float *) (GET_TP[1])) + opVal;
    if ( GET_VAR_OPERATION == kVarFetch ) {

        FPUSH( *pFloat );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        SPUSH( (long) pFloat );
        // TOS points to data field
        floatOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doFloatArrayOp )
{
    // IP points to data field
    int index = SPOP;
    SPUSH( ((long) ((GET_IP) + index)) );
    floatOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalFloatArrayAction )
{
    int index = SPOP;
    SPUSH( ((long) ((GET_FP - opVal) + index)) );
    floatOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
}

OPTYPE_ACTION( FieldFloatArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of float[0]
    long pVar = SPOP + opVal;
    pVar += (SPOP << 2);
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        // TOS points to data field
        floatOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
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

VarAction doubleOps[] = {
    doDoubleFetch,
    doIntRef,
    doDoubleStore,
    doDoublePlusStore,
    doDoubleMinusStore
};

GFORTHOP( doDoubleOp )
{
    // IP points to data field
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long) (GET_IP) );
        doubleOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalDoubleAction )
{
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long)(GET_FP - opVal) );
        // TOS points to data field
        doubleOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


OPTYPE_ACTION( FieldDoubleAction )
{
    long pVar = SPOP + opVal;
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        doubleOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


OPTYPE_ACTION( MemberDoubleAction )
{
    double *pDouble = (double *) (((long *) (GET_TP[1])) + opVal);
    if ( GET_VAR_OPERATION == kVarFetch ) {

        DPUSH( *pDouble );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        SPUSH( (long) pDouble );
        // TOS points to data field
        doubleOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doDoubleArrayOp )
{
    // IP points to data field
    int index = SPOP << 1;
    SPUSH( ((long) ((GET_IP) + index)) );
    doubleOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalDoubleArrayAction )
{
    int index = SPOP << 1;
    SPUSH( ((long) ((GET_FP - opVal) + SPOP)) );
    doubleOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
}

OPTYPE_ACTION( FieldDoubleArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of double[0]
    long pVar = SPOP + opVal;
    pVar += (SPOP << 3);
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        // TOS points to data field
        doubleOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
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

VarAction stringOps[] = {
    doStringFetch,
    doIntRef,
    doStringStore,
    doStringAppend
};

GFORTHOP( doStringOp )
{
    // IP points to data field
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long) (GET_IP) );
        stringOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalStringAction )
{
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( (long)(GET_FP - opVal) );
        stringOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


OPTYPE_ACTION( FieldStringAction )
{
    long pVar = SPOP + opVal;
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        stringOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


OPTYPE_ACTION( MemberStringAction )
{
    char *pString = (char *) (((long *) (GET_TP[1])) + opVal);
    if ( GET_VAR_OPERATION == kVarFetch ) {

        SPUSH( (long) pString );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        SPUSH( (long) pString );
        // TOS points to data field
        stringOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doStringArrayOp )
{
    // IP points to data field
    int index = SPOP;
    long *pStr = GET_IP;
    long len = ((*pStr) >> 2) + 3;      // length of one string in longwords
    SPUSH( ((long) (pStr + (index * len))) );
    stringOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalStringArrayAction )
{
    int index = SPOP;
    long *pStr = GET_FP - opVal;
    long len = ((*pStr) >> 2) + 3;      // length of one string in longwords
    SPUSH( ((long) (pStr + (index * len))) );
    stringOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
}

OPTYPE_ACTION( FieldStringArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of string[0]
    long *pStr = (long *) (SPOP + opVal);
    long len = ((*pStr) >> 2) + 3;      // length of one string in longwords
    int index = SPOP;
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( ((long) (pStr + (index * len))) );
        // TOS points to data field
        stringOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


//////////////////////////////////////////////////////////////////////
////
///
//                     op
// 

VAR_ACTION( doOpExecute ) 
{
    // IP points to data field
    long prog[2];

    // execute the opcode
    prog[0] = *(long *)(SPOP);
    prog[1] = BUILTIN_OP( OP_DONE );

    SET_IP( prog );
    InnerInterpreter( pCore );
}

VarAction opOps[] = {
    doOpExecute,
    doIntRef,
    doIntStore,
};

GFORTHOP( doOpOp )
{    
    // IP points to data field
    if ( GET_VAR_OPERATION <= kVarStore )
    {
        SPUSH( (long) (GET_IP) );
        opOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalOpAction )
{
    if ( GET_VAR_OPERATION <= kVarStore )
    {
        SPUSH( (long)(GET_FP - opVal) );
        opOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


OPTYPE_ACTION( FieldOpAction )
{
    long pVar = SPOP + opVal;

    if ( GET_VAR_OPERATION <= kVarStore )
    {
        SPUSH( pVar );
        // TOS points to data field
        opOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


OPTYPE_ACTION( MemberOpAction )
{
    long *pOp = ((long *) (GET_TP[1])) + opVal;
    if ( GET_VAR_OPERATION <= kVarStore )
    {
        SPUSH( (long) pOp );
        // TOS points to data field
        opOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}

// this is an internal op that is compiled before the data field of each array
GFORTHOP( doOpArrayOp )
{
    // IP points to data field
    int index = SPOP;
    SPUSH( ((long) ((GET_IP) + index)) );
    opOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
    SET_IP( (long *) (RPOP) );
}

OPTYPE_ACTION( LocalOpArrayAction )
{
    int index = SPOP;
    SPUSH( ((long) ((GET_FP - opVal) + index)) );
    opOps[ GET_VAR_OPERATION ] ( pCore );
    CLEAR_VAR_OPERATION;
}

OPTYPE_ACTION( FieldOpArrayAction )
{
    // TOS is struct base, NOS is index
    // opVal is byte offset of op[0]
    long pVar = SPOP + opVal;
    pVar += (SPOP << 2);
    if ( (GET_VAR_OPERATION >= kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) )
    {
        SPUSH( pVar );
        // TOS points to data field
        opOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

OPTYPE_ACTION( BuiltinAction )
{
    // op is normal user-defined, push IP on rstack, lookup new IP
    //  in table of user-defined ops
    if ( opVal < GET_NUM_USER_OPS ) {
        RPUSH( (long) GET_IP );
        SET_IP( USER_OP_TABLE[opVal] );
    } else {
        SET_ERROR( kForthErrorBadOpcode );
    }
}

OPTYPE_ACTION( UserDefAction )
{
    // op is normal user-defined, push IP on rstack, lookup new IP
    //  in table of user-defined ops
    if ( opVal < GET_NUM_USER_OPS ) {
        RPUSH( (long) GET_IP );
        SET_IP( USER_OP_TABLE[opVal] );
    } else {
        SET_ERROR( kForthErrorBadOpcode );
    }
}

OPTYPE_ACTION( BranchAction )
{
    if ( (opVal & 0x00800000) != 0 ) {
        // TBD: trap a hard loop (opVal == -1)?
        opVal |= 0xFF000000;
    }
    SET_IP( GET_IP + opVal );
}

OPTYPE_ACTION( BranchNZAction )
{
    if ( SPOP != 0 ) {
        if ( (opVal & 0x00800000) != 0 ) {
            // TBD: trap a hard loop (opVal == -1)?
            opVal |= 0xFF000000;
        }
        SET_IP( GET_IP + opVal );
    }
}

OPTYPE_ACTION( BranchZAction )
{
    if ( SPOP == 0 ) {
        if ( (opVal & 0x00800000) != 0 ) {
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
    if ( *pSP == pSP[1] ) {
        // case matched
        pSP += 2;
    } else {
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
    if ( (opVal & 0x00800000) != 0 ) {
      opVal |= 0xFF000000;
    }
    SPUSH( opVal );
}

OPTYPE_ACTION( OffsetAction )
{
    // push constant in opVal
    if ( (opVal & 0x00800000) != 0 ) {
      opVal |= 0xFF000000;
    }
    long v = SPOP + opVal;
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
    SPUSH( (long)(GET_FP - opVal) );
}

OPTYPE_ACTION( MethodWithThisAction )
{
    // this is called when a method is invoked from inside another
    // method in the same class - the difference is that in this case
    // there is no explicit source for the "this" pointer, we just keep
    // on using the current "this" pointer
    long *pObj = GET_TP;
    if ( pObj != NULL ) {
        // pObj is a pair of pointers, first pointer is to
        //   class descriptor for this type of object,
        //   second pointer is to storage for object (this ptr)
        long *pClass = (long *) (*pObj);
        if ( pClass[2] > (long) opVal ) {
            RPUSH( (long) GET_IP );
            RPUSH( (long) GET_TP );
            SET_TP( pObj );
            SET_IP( (long *) (pClass[opVal + 3]) );
        } else {
            // bad method number
            SET_ERROR( kForthErrorBadOpcode );
        }
    } else {
        SET_ERROR( kForthErrorBadOpcode );
    }
}

OPTYPE_ACTION( IllegalBuiltinAction )
{
    SET_ERROR( kForthErrorBadOpcode );
}

OPTYPE_ACTION( IllegalOptypeAction )
{
    SET_ERROR( kForthErrorBadOpcodeType );
}

OPTYPE_ACTION( MethodAction )
{
    // token is object method invocation
    long op = GET_CURRENT_OP;
    forthOpType opType = FORTH_OP_TYPE( op );
    int methodNum = ((int) opType) & 0x7F;
    long* pObj = NULL;
    if ( opVal & 0x00800000 ) {
        // object ptr is in local variable
        pObj = GET_FP - (opVal & 0x007F0000);
    } else {
        // object ptr is in global op
        if ( opVal < pCore->numUserOps ) {
            pObj = (long *) (pCore->userOps[opVal]);
        } else {
            SET_ERROR( kForthErrorBadOpcode );
        }
    }
    if ( pObj != NULL ) {
        // pObj is a pair of pointers, first pointer is to
        //   class descriptor for this type of object,
        //   second pointer is to storage for object (this ptr)
        long *pClass = (long *) (*pObj);
        if ( (pClass[1] == CLASS_MAGIC_NUMBER)
          && (pClass[2] > methodNum) ) {
            RPUSH( (long) GET_IP );
            RPUSH( (long) GET_TP );
            SET_TP( pObj );
            SET_IP( (long *) (pClass[methodNum + 3]) );
        } else {
            // bad class magic number, or bad method number
            SET_ERROR( kForthErrorBadOpcode );
        }
    } else {
        SET_ERROR( kForthErrorBadOpcode );
    }
}

// NOTE: there is no opcode assigned to this op
FORTHOP( BadOpcodeOp )
{
    SET_ERROR( kForthErrorBadOpcode );
}

optypeActionRoutine builtinOptypeAction[] =
{
    IllegalBuiltinAction,
    UserDefAction,

    BranchAction,
    BranchNZAction,
    BranchZAction,
    CaseBranchAction,

    ConstantAction,
    OffsetAction,
    ArrayOffsetAction,
    LocalStructArrayAction,

    ConstantStringAction,

    AllocLocalsAction,
    InitLocalStringAction,
    LocalRefAction,

    LocalByteAction,
    LocalShortAction,
    LocalIntAction,
    LocalFloatAction,
    LocalDoubleAction,
    LocalStringAction,
    LocalOpAction,

    FieldByteAction,
    FieldShortAction,
    FieldIntAction,
    FieldFloatAction,
    FieldDoubleAction,
    FieldStringAction,
    FieldOpAction,

    LocalByteArrayAction,
    LocalShortArrayAction,
    LocalIntArrayAction,
    LocalFloatArrayAction,
    LocalDoubleArrayAction,
    LocalStringArrayAction,
    LocalOpArrayAction,

    FieldByteArrayAction,
    FieldShortArrayAction,
    FieldIntArrayAction,
    FieldFloatArrayAction,
    FieldDoubleArrayAction,
    FieldStringArrayAction,
    FieldOpArrayAction,

    MethodWithThisAction,

    MemberIntAction,    // TBD: byte
    MemberIntAction,    // TBD: short
    MemberIntAction,
    MemberFloatAction,
    MemberDoubleAction,
    MemberStringAction,
    MemberOpAction,

    NULL            // this must be last to end the list
};

void InitDispatchTables( ForthCoreState* pCore )
{
    int i;

    for ( i = 0; i < 256; i++ ) {
        pCore->optypeAction[i] = IllegalOptypeAction;
    }

    for ( i = 0; i < MAX_BUILTIN_OPS; i++ ) {
        pCore->builtinOps[i] = BadOpcodeOp;
    }
    for ( i = 0; builtinOptypeAction[i] != NULL; i++ )
    {
        pCore->optypeAction[i] = builtinOptypeAction[i];
    }
}

void InitCore( ForthCoreState* pCore )
{
    pCore->builtinOps = NULL;
    pCore->numBuiltinOps = 0;
    pCore->userOps = NULL;
    pCore->numUserOps = 0;
    pCore->maxUserOps = 0;
    pCore->IP = NULL;
    pCore->SP = NULL;
    pCore->ST = NULL;
    pCore->SLen = 0;
    pCore->RP = NULL;
    pCore->RT = NULL;
    pCore->RLen = 0;
    pCore->FP = NULL;
    pCore->TP = NULL;
    pCore->varMode = kVarFetch;
    pCore->state = kResultOk;
    pCore->error = kForthErrorNone;
    pCore->pThread = NULL;
    pCore->pEngine = NULL;
    pCore->DP = NULL;
    pCore->DBase = NULL;
    pCore->DLen = 0;
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
    optypeActionRoutine* opTypeAction = &(pCore->optypeAction[0]);
    ForthOp* builtinOp = &(pCore->builtinOps[0]);
    numBuiltinOps = pCore->numBuiltinOps;

    SET_STATE( kResultOk );
    
    while ( GET_STATE == kResultOk ) {
        // fetch op at IP, advance IP
        pIP = GET_IP;
        op = *pIP++;
        SET_IP( pIP );
#ifdef TRACE_INNER_INTERPRETER
        GET_ENGINE->TraceOp();
#endif
        opType = FORTH_OP_TYPE( op );
        opVal = FORTH_OP_VALUE( op );
        if ( opType == kOpBuiltIn )
        {
            if ( opVal < numBuiltinOps ) {
                builtinOp[ opVal ]( pCore );
            } else {
                SET_ERROR( kForthErrorBadOpcode );
            }
        }
        else
        {
            pCore->optypeAction[ (int) opType ]( pCore, opVal );
        }
    }

    return GET_STATE;
}


};      // end extern "C"

