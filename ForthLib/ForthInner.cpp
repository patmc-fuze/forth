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

#ifdef TRACE_INNER_INTERPRETER

// provide trace ability for builtin ops
#define NUM_TRACEABLE_OPS 500
static char *gOpNames[ NUM_TRACEABLE_OPS ];

#endif




//////////////////////////////////////////////////////////////////////
////
///
//                     int
// 

// doInt{Fetch,Ref,Store,PlusStore,MinusStore} are parts of doIntOp
VAR_ACTION( doIntFetch ) 
{
    // IP points to data field
    SPUSH( *GET_IP );
}

VAR_ACTION( doIntRef )
{
    // IP points to data field
    SPUSH( (long) GET_IP );
}

VAR_ACTION( doIntStore ) 
{
    // IP points to data field
    long *pA = (long *) (GET_IP);
    *pA = SPOP;
}

VAR_ACTION( doIntPlusStore ) 
{
    // IP points to data field
    long *pA = (long *) (GET_IP);
    *pA += SPOP;
}

VAR_ACTION( doIntMinusStore ) 
{
    // IP points to data field
    long *pA = (long *) (GET_IP);
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
    if ( GET_VAR_OPERATION == kVarFetch ) {

        SPUSH( *GET_IP );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        intOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
    SET_IP( (long *) (RPOP) );
}

VAR_ACTION( doLocalIntFetch ) 
{
    // IP points to data field
    SPUSH( * (long *) SPOP );
}

VAR_ACTION( doLocalIntRef )
{
    // address is already on stack
}

VAR_ACTION( doLocalIntStore ) 
{
    // IP points to data field
    long *pA = (long *) SPOP;
    *pA = SPOP;
}

VAR_ACTION( doLocalIntPlusStore ) 
{
    // IP points to data field
    long *pA = (long *) SPOP;
    *pA += SPOP;
}

VAR_ACTION( doLocalIntMinusStore ) 
{
    // IP points to data field
    long *pA = (long *) SPOP;
    *pA -= SPOP;
}

VarAction localIntOps[] = {
    doLocalIntFetch,
    doLocalIntRef,
    doLocalIntStore,
    doLocalIntPlusStore,
    doLocalIntMinusStore
};

OPTYPE_ACTION( LocalIntAction )
{
    if ( GET_VAR_OPERATION == kVarFetch ) {

        SPUSH( *(GET_FP - opVal) );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        SPUSH( (long)(GET_FP - opVal) );
        // TOS points to data field
        localIntOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


OPTYPE_ACTION( FieldIntAction )
{
    long *pVar = ((long *)*(GET_SP)) + opVal;
    if ( GET_VAR_OPERATION == kVarFetch ) {

        *(GET_SP) = *pVar;

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        *(GET_SP) = (long) pVar;
        // TOS points to data field
        localIntOps[ GET_VAR_OPERATION ] ( pCore );
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
        localIntOps[ GET_VAR_OPERATION ] ( pCore );
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
    float *pA = (float *) (GET_IP);
    *pA += FPOP;
}

VAR_ACTION( doFloatMinusStore ) 
{
    // IP points to data field
    float *pA = (float *) (GET_IP);
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
    if ( GET_VAR_OPERATION == kVarFetch ) {

        FPUSH( *(float *) GET_IP );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        floatOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
    SET_IP( (long *) (RPOP) );
}

VAR_ACTION( doLocalFloatPlusStore ) 
{
    // IP points to data field
    float *pA = (float *) SPOP;
    *pA += FPOP;
}

VAR_ACTION( doLocalFloatMinusStore ) 
{
    // IP points to data field
    float *pA = (float *) SPOP;
    *pA -= FPOP;
}

VarAction localFloatOps[] = {
    doLocalIntFetch,
    doLocalIntRef,
    doLocalIntStore,
    doLocalFloatPlusStore,
    doLocalFloatMinusStore
};

OPTYPE_ACTION( LocalFloatAction )
{
    if ( GET_VAR_OPERATION == kVarFetch ) {

        FPUSH( *(float *)(GET_FP - opVal) );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        SPUSH( (long)(GET_FP - opVal) );
        localFloatOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


OPTYPE_ACTION( FieldFloatAction )
{
    float *pVar = (float *) ( ((long *)*(GET_SP)) + opVal );

    if ( GET_VAR_OPERATION == kVarFetch ) {

        *(GET_SP) = *((long *) pVar);

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        *(GET_SP) = (long) pVar;
        // TOS points to data field
        localFloatOps[ GET_VAR_OPERATION ] ( pCore );
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
        localFloatOps[ GET_VAR_OPERATION ] ( pCore );
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
    double *pA = (double *) (GET_IP);
    DPUSH( *pA );
}

VAR_ACTION( doDoubleStore ) 
{
    // IP points to data field
    double *pA = (double *) (GET_IP);
    *pA = DPOP;
}

VAR_ACTION( doDoublePlusStore ) 
{
    // IP points to data field
    double *pA = (double *) (GET_IP);
    *pA += DPOP;
}

VAR_ACTION( doDoubleMinusStore ) 
{
    // IP points to data field
    double *pA = (double *) (GET_IP);
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
    if ( GET_VAR_OPERATION == kVarFetch ) {

        DPUSH( *(double *) GET_IP );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        doubleOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
    SET_IP( (long *) (RPOP) );
}

VAR_ACTION( doLocalDoubleFetch ) 
{
    // IP points to data field
    double *pA = (double *) SPOP;
    DPUSH( *pA );
}

VAR_ACTION( doLocalDoubleStore ) 
{
    // IP points to data field
    double *pA = (double *) SPOP;
    *pA = DPOP;
}

VAR_ACTION( doLocalDoublePlusStore ) 
{
    // IP points to data field
    double *pA = (double *) SPOP;
    *pA += DPOP;
}

VAR_ACTION( doLocalDoubleMinusStore ) 
{
    // IP points to data field
    double *pA = (double *) SPOP;
    *pA -= DPOP;
}

VarAction localDoubleOps[] = {
    doLocalDoubleFetch,
    doLocalIntRef,
    doLocalDoubleStore,
    doLocalDoublePlusStore,
    doLocalDoubleMinusStore
};

OPTYPE_ACTION( LocalDoubleAction )
{
    if ( GET_VAR_OPERATION == kVarFetch ) {

        DPUSH( *(double *)(GET_FP - opVal) );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        SPUSH( (long)(GET_FP - opVal) );
        localDoubleOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


OPTYPE_ACTION( FieldDoubleAction )
{
    double *pVar = (double *) ( (long *)(SPOP) + opVal );

    if ( GET_VAR_OPERATION == kVarFetch ) {

        DPUSH( *pVar );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarMinusStore) ) {

        SPUSH( (long) pVar );
        localDoubleOps[ GET_VAR_OPERATION ] ( pCore );
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
        localDoubleOps[ GET_VAR_OPERATION ] ( pCore );
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

VAR_ACTION( doStringStore ) 
{
    // IP points to data field
    char *pA = (char *) (GET_IP);
    strcpy( pA, (char *) SPOP );
}

VAR_ACTION( doStringAppend ) 
{
    // IP points to data field
    char *pA = (char *) (GET_IP);
    strcat( pA, (char *) SPOP );
}

VarAction stringOps[] = {
    doIntRef,
    doIntRef,
    doStringStore,
    doStringAppend
};

GFORTHOP( doStringOp )
{
    
    // IP points to data field
    if ( GET_VAR_OPERATION == kVarFetch ) {

        SPUSH( (long) GET_IP );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarPlusStore) ) {

        stringOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
    SET_IP( (long *) (RPOP) );
}

VAR_ACTION( doLocalStringStore ) 
{
    // IP points to data field
    char *pA = (char *) SPOP;
    strcpy( pA, (char *) SPOP );
}

VAR_ACTION( doLocalStringAppend ) 
{
    // IP points to data field
    char *pA = (char *) SPOP;
    strcat( pA, (char *) SPOP );
}

VarAction localStringOps[] = {
    doLocalIntRef,
    doLocalIntRef,
    doLocalStringStore,
    doLocalStringAppend
};

OPTYPE_ACTION( LocalStringAction )
{
    if ( GET_VAR_OPERATION == kVarFetch ) {

        SPUSH( (long)(GET_FP - opVal) );

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarPlusStore) ) {

        SPUSH( (long)(GET_FP - opVal) );
        // TOS points to data field
        localStringOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


OPTYPE_ACTION( FieldStringAction )
{
    long *pVar = ((long *)*(GET_SP)) + opVal;
    if ( GET_VAR_OPERATION == kVarFetch ) {

        *(GET_SP) = (long) pVar;

    } else if ( (GET_VAR_OPERATION > kVarFetch) && (GET_VAR_OPERATION <= kVarPlusStore) ) {

        *(GET_SP) = (long) pVar;
        // TOS points to data field
        localStringOps[ GET_VAR_OPERATION ] ( pCore );
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
        localStringOps[ GET_VAR_OPERATION ] ( pCore );
        CLEAR_VAR_OPERATION;

    //} else {
        // TBD: report GET_VAR_OPERATION out of range
    }
}


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

OPTYPE_ACTION( StringAction )
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
    StringAction,

    AllocLocalsAction,

    LocalIntAction,
    LocalFloatAction,
    LocalDoubleAction,
    LocalStringAction,

    FieldIntAction,
    FieldFloatAction,
    FieldDoubleAction,
    FieldStringAction,

    MethodWithThisAction,

    MemberIntAction,
    MemberFloatAction,
    MemberDoubleAction,
    MemberStringAction,

    NULL            // this must be last to end the list
};

void InitDispatchTables( ForthCoreState& core )
{
    int i;

    for ( i = 0; i < 256; i++ ) {
        core.optypeAction[i] = IllegalOptypeAction;
    }

    for ( i = 0; i < MAX_BUILTIN_OPS; i++ ) {
        core.builtinOps[i] = BadOpcodeOp;
    }
    for ( i = 0; builtinOptypeAction[i] != NULL; i++ )
    {
        core.optypeAction[i] = builtinOptypeAction[i];
    }
}

void InitCore( ForthCoreState& core )
{
    core.builtinOps = NULL;
    core.numBuiltinOps = 0;
    core.userOps = NULL;
    core.numUserOps = 0;
    core.maxUserOps = 0;
    core.IP = NULL;
    core.SP = NULL;
    core.RP = NULL;
    core.FP = NULL;
    core.TP = NULL;
    core.varMode = kVarFetch;
    core.state = kResultOk;
    core.pThread = NULL;
    core.pEngine = NULL;
    core.DP = NULL;
    core.DBase = NULL;
    core.DLen = 0;
}

void CoreSetError( ForthCoreState *pCore, eForthError error, bool isFatal )
{
    pCore->pThread->error =  error;
    pCore->state = (isFatal) ? kResultFatalError : kResultError;
}

//############################################################################
//
//          I N N E R    I N T E R P R E T E R
//
//############################################################################

eForthResult
InnerInterpreterFunc( ForthCoreState *pCore )
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


