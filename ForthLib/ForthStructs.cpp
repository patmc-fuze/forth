//////////////////////////////////////////////////////////////////////
//
// ForthStructs.cpp: support for user-defined structures
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthShell.h"
#include "ForthForgettable.h"
#include "ForthBuiltinClasses.h"

// symbol entry layout for struct vocabulary (fields and method symbols
// offset   contents
//  0..3    field offset or method index
//  4..7    field data type or method return type
//  8..12   element size for arrays
//  13      1-byte symbol length (not including padding)
//  14..    symbol characters
//
// see forth.h for most current description of struct symbol entry fields

extern ForthBaseType *gpBaseTypes[];
#define STRUCTS_EXPANSION_INCREMENT     16


//////////////////////////////////////////////////////////////////////
////
///     ForthTypesManager
//
//

ForthTypesManager *ForthTypesManager::mpInstance = NULL;


ForthTypesManager::ForthTypesManager()
: ForthForgettable( NULL, 0 )
, mNumStructs( 0 )
, mMaxStructs( 0 )
, mpStructInfo( NULL )
, mpSavedDefinitionVocab( NULL )
, mpClassMethods( NULL )
{
    ASSERT( mpInstance == NULL );
    mpInstance = this;
}

ForthTypesManager::~ForthTypesManager()
{
    mpInstance = NULL;
    delete mpStructInfo;
}

void
ForthTypesManager::ForgetCleanup( void *pForgetLimit, long op )
{
    // remove struct info for forgotten struct types
    int i;

    for ( i = mNumStructs - 1; i >= 0; i-- )
    {
        if ( FORTH_OP_VALUE( mpStructInfo[i].op ) >= op )
        {
            // this struct is among forgotten ops
            SPEW_STRUCTS( "Forgetting struct vocab %s\n", mpStructInfo[i].pVocab->GetName() );
            mNumStructs--;
        }
        else
        {
            break;
        }
    }
}


ForthStructVocabulary*
ForthTypesManager::StartStructDefinition( const char *pName )
{
    ForthEngine *pEngine = ForthEngine::GetInstance();
    ForthVocabulary* pDefinitionsVocab = pEngine->GetDefinitionVocabulary();

    if ( mNumStructs == mMaxStructs )
    {
        mMaxStructs += STRUCTS_EXPANSION_INCREMENT;
        SPEW_STRUCTS( "StartStructDefinition expanding structs table to %d entries\n", mMaxStructs );
        mpStructInfo = (ForthTypeInfo *) realloc( mpStructInfo, sizeof(ForthTypeInfo) * mMaxStructs );
    }
    ForthTypeInfo *pInfo = &(mpStructInfo[mNumStructs]);

    long *pEntry = pEngine->StartOpDefinition( pName, true, kOpUserDefImmediate );
    pInfo->pVocab = new ForthStructVocabulary( pName, mNumStructs );
    pInfo->op = *pEntry;
    SPEW_STRUCTS( "StartStructDefinition %s struct index %d\n", pName, mNumStructs );
    mNumStructs++;
    return pInfo->pVocab;
}

void
ForthTypesManager::EndStructDefinition()
{
    SPEW_STRUCTS( "EndStructDefinition\n" );
    ForthEngine *pEngine = ForthEngine::GetInstance();
    pEngine->EndOpDefinition( true );
    GetNewestStruct()->EndDefinition();
}

ForthClassVocabulary*
ForthTypesManager::StartClassDefinition( const char *pName )
{
    ForthEngine *pEngine = ForthEngine::GetInstance();
    ForthVocabulary* pDefinitionsVocab = pEngine->GetDefinitionVocabulary();

    if ( mNumStructs == mMaxStructs )
    {
        mMaxStructs += STRUCTS_EXPANSION_INCREMENT;
        SPEW_STRUCTS( "StartClassDefinition expanding structs table to %d entries\n", mMaxStructs );
        mpStructInfo = (ForthTypeInfo *) realloc( mpStructInfo, sizeof(ForthTypeInfo) * mMaxStructs );
    }
    ForthTypeInfo *pInfo = &(mpStructInfo[mNumStructs]);

    // can't smudge class definition, since method definitions will be nested inside it
    long *pEntry = pEngine->StartOpDefinition( pName, false, kOpUserDefImmediate );
    pInfo->pVocab = new ForthClassVocabulary( pName, mNumStructs );
    pInfo->op = *pEntry;
    SPEW_STRUCTS( "StartClassDefinition %s struct index %d\n", pName, mNumStructs );
    mpSavedDefinitionVocab = pEngine->GetDefinitionVocabulary();
    pEngine->SetDefinitionVocabulary( pInfo->pVocab );
    mNumStructs++;
    return (ForthClassVocabulary*) (pInfo->pVocab);
}

void
ForthTypesManager::EndClassDefinition()
{
    SPEW_STRUCTS( "EndClassDefinition\n" );
    ForthEngine *pEngine = ForthEngine::GetInstance();
    pEngine->EndOpDefinition( false );
    pEngine->SetDefinitionVocabulary( mpSavedDefinitionVocab );
    mpSavedDefinitionVocab = NULL;
}

ForthStructVocabulary*
ForthTypesManager::GetStructVocabulary( long op )
{
    int i;
    ForthTypeInfo *pInfo;

    //TBD: replace this with a map
    for ( i = 0; i < mNumStructs; i++ )
    {
        pInfo = &(mpStructInfo[i]);
        if ( pInfo->op == op )
        {
            return pInfo->pVocab;
        }
    }
    return NULL;
}

ForthStructVocabulary*
ForthTypesManager::GetStructVocabulary( const char* pName )
{
    int i;
    ForthTypeInfo *pInfo;

    //TBD: replace this with a map
    for ( i = 0; i < mNumStructs; i++ )
    {
        pInfo = &(mpStructInfo[i]);
        if ( strcmp( pInfo->pVocab->GetName(), pName ) == 0 )
        {
            return pInfo->pVocab;
        }
    }
    return NULL;
}

ForthTypeInfo*
ForthTypesManager::GetStructInfo( int structIndex )
{
    if ( structIndex >= mNumStructs )
    {
        SPEW_STRUCTS( "GetStructInfo error: structIndex is %d, only %d structs exist\n",
                        structIndex, mNumStructs );
        return NULL;
    }
    return &(mpStructInfo[ structIndex ]);
}

ForthTypesManager*
ForthTypesManager::GetInstance( void )
{
    ASSERT( mpInstance != NULL );
    return mpInstance;
}

void
ForthTypesManager::GetFieldInfo( long fieldType, long& fieldBytes, long& alignment )
{
    long subType;
    if ( CODE_IS_PTR( fieldType ) )
    {
        fieldBytes = 4;
        alignment = 4;
    }
    else if ( CODE_IS_NATIVE( fieldType ) )
    {
        subType = CODE_TO_BASE_TYPE( fieldType );
        if ( subType == kBaseTypeString )
        {
            // add in for maxLen, curLen fields & terminating null byte
            fieldBytes = 9 + CODE_TO_STRING_BYTES( fieldType );
        }
        else
        {
            fieldBytes = gpBaseTypes[subType]->GetSize();
        }
        alignment = gpBaseTypes[subType]->GetAlignment();
    }
    else
    {
        ForthTypeInfo* pInfo = GetStructInfo( CODE_TO_STRUCT_INDEX( fieldType ) );
        if ( pInfo )
        {
            alignment = pInfo->pVocab->GetAlignment();
            fieldBytes = pInfo->pVocab->GetSize();
        }
    }
}

ForthStructVocabulary *
ForthTypesManager::GetNewestStruct( void )
{
    ForthTypesManager* pThis = GetInstance();
    return pThis->mpStructInfo[ pThis->mNumStructs - 1 ].pVocab;
}

ForthClassVocabulary *
ForthTypesManager::GetNewestClass( void )
{
    ForthTypesManager* pThis = GetInstance();
    ForthStructVocabulary* pVocab = pThis->mpStructInfo[ pThis->mNumStructs - 1 ].pVocab;
    if ( pVocab && !pVocab->IsClass() )
    {
        pVocab = NULL;
    }
    return (ForthClassVocabulary *) pVocab;
}


// compile/interpret symbol if is a valid structure accessor
bool
ForthTypesManager::ProcessSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus )
{
    long typeCode;
    ForthEngine *pEngine = ForthEngine::GetInstance();
    long *pDst = &(mCode[0]);
    strcpy( mToken, pInfo->GetToken() );
    // get first token
    char *pToken = &(mToken[0]);
    char *pNextToken = strchr( mToken, '.' );
    char* pLastChar = pNextToken - 1;
    ForthVocabulary *pFoundVocab = NULL;
    ForthCoreState* pCore = pEngine->GetCoreState();
    char errorMsg[256];

    // if we get here, we know the symbol had a period in it
    if ( pNextToken == NULL )
    {
        return false;
    }
    *pNextToken++ = '\0';

    ///////////////////////////////////
    //
    //  process first token
    //

    // see if first token is local or global struct
    long* pEntry = NULL;
    if ( pInfo->GetFlags() & PARSE_FLAG_HAS_COLON )
    {
        int tokenLength = pInfo->GetTokenLength();
        char* pColon = strchr( pToken, ':' );
        if ( pColon != NULL )
        {
            int colonPos = pColon - pToken;
            if ( (tokenLength > 4) && (colonPos > 0) && (colonPos < (tokenLength - 2)) )
            {
                ////////////////////////////////////
                //
                // symbol may be of form VOCABULARY:SYMBOL.STUFF
                //
                ////////////////////////////////////
                *pColon = '\0';
                ForthVocabulary* pVocab = ForthVocabulary::FindVocabulary( pToken );
                if ( pVocab != NULL )
                {
                    pEntry = pVocab->FindSymbol( pColon + 1 );
                    if ( pEntry != NULL )
                    {
                        pFoundVocab = pVocab;
                    }
                }
                *pColon = ':';
            }
        }
    }
    bool explicitTOSCast = false;
    bool isClassReference = false;
    if ( (pEntry == NULL) && (*pToken == '<') && (*pLastChar == '>') )
    {
        ////////////////////////////////////
        //
        // symbol may be of form <STRUCTTYPE>.STUFF
        //
        ////////////////////////////////////
        *pLastChar = '\0';
        ForthStructVocabulary* pCastVocab = GetStructVocabulary( pToken + 1 );
        if ( pCastVocab != NULL )
        {
            explicitTOSCast = true;
            typeCode = STRUCT_TYPE_TO_CODE( kDTIsPtr, pCastVocab->GetTypeIndex() );
        }
        *pLastChar = '>';
    }
    if ( !explicitTOSCast )
    {
        if ( pEntry == NULL )
        {
            pEntry = pEngine->GetLocalVocabulary()->FindSymbol( mToken );
        }
        if ( pEntry == NULL )
        {
            pEntry = pEngine->GetVocabularyStack()->FindSymbol( mToken, &pFoundVocab );
            if ( pEntry != NULL )
            {
                // see if mToken is a class name
                ForthStructVocabulary* pClassVocab = GetStructVocabulary( mToken );
                if ( (pClassVocab != NULL) && pClassVocab->IsClass() )
                {
                    // this is invoking a class method on a class object (IE object.new)
                    // the first compiled opcode is the varop 'vocabToClass', which will be
                    // followed by the opcode for the class vocabulary, ForthVocabulary::DoOp
                    // will do the pushing of the class object
                    isClassReference = true;
                    *pDst++ = OP_VOCAB_TO_CLASS;
                    typeCode = OBJECT_TYPE_TO_CODE( 0, kBCIClass );
                }
            }
        }
        if ( pEntry == NULL )
        {
            // token not found in search or local vocabs
            return false;
        }
        // see if token is a struct
        if ( !isClassReference )
        {
            typeCode = pEntry[1];
            if ( CODE_IS_NATIVE( typeCode ) )
            {
                SPEW_STRUCTS( "Native type cant have fields\n" );
                return false;
            }
        }
    }
    bool isPtr = CODE_IS_PTR( typeCode );
    bool isArray = CODE_IS_ARRAY( typeCode );
    long baseType = CODE_TO_BASE_TYPE( typeCode );
    bool isObject = (baseType == kBaseTypeObject);
    bool previousWasObject = false;
    long compileVarop = 0;

    // TBD: there is some wasteful fetching of full object when we just end up dropping method ptr

    if ( !explicitTOSCast )
    {
        SPEW_STRUCTS( "First field %s op 0x%x\n", pToken, pEntry[0] );
        // handle case where previous opcode was varAction setting op (one of addressOf -> ->+ ->-)
        // we need to execute the varAction setting op after the first op, since if the first op is
        // a pointer type, it will use the varAction and clear it, when the varAction is meant to be
        // used by the final field op
        if ( pEngine->IsCompiling() )
        {
            long *pLastOp = pEngine->GetLastCompiledOpcodePtr();
            if ( pLastOp && ((pLastOp + 1) == GET_DP)
                && (*pLastOp >= OP_ADDRESS_OF) && (*pLastOp <= OP_INTO_MINUS) )
            {
                // overwrite the varAction setting op with first accessor op
                compileVarop = *pLastOp;
            }
        }
        else
        {
            // we are interpreting, clear any existing varAction, but compile an op to set it after first op
            ulong varMode = GET_VAR_OPERATION;
            if ( varMode )
            {
                CLEAR_VAR_OPERATION;
                compileVarop = OP_ADDRESS_OF + (varMode - kVarRef);
            }
        }
        *pDst++ = pEntry[0];
        if ( isObject && isPtr )
        {
            *pDst++ = OP_DFETCH;
        }
    }
#if 0
    // this is broken - for now, the first field can only be a simple struct or object
    if ( isArray || isPtr )
    {
        if ( isArray )
        {
            // TBD: verify the element size for arrays of ptrs to structs is 4
            SPEW_STRUCTS( " arrayOffsetOp 0x%x", COMPILED_OP( kOpArrayOffset, pEntry[2] ) );
            *pDst++ = COMPILED_OP( kOpArrayOffset, pEntry[2] );
        }
        if ( isPtr )
        {
            SPEW_STRUCTS( " fetchOp 0x%x", BUILTIN_OP( OP_FETCH ) );
            *pDst++ = BUILTIN_OP( OP_FETCH );
        }
        if ( isObject )
        {
            SPEW_STRUCTS( " dfetchOp 0x%x", BUILTIN_OP( OP_DFETCH ) );
            *pDst++ = BUILTIN_OP( OP_DFETCH );
            previousWasObject = true;
        }
    }
#endif

    ForthTypeInfo* pStruct = GetStructInfo( CODE_TO_STRUCT_INDEX( typeCode ) );
    if ( pStruct == NULL )
    {
        SPEW_STRUCTS( "First field not found by types manager\n" );
        return false;
    }
    else
    {
        SPEW_STRUCTS( "First field of type %s\n", pStruct->pVocab->GetName() );
    }
    if ( pStruct->pVocab->IsClass() )
    {
        previousWasObject = true;
    }
    long offset = 0;

    ///////////////////////////////////
    //
    //  process tokens after first
    //

    //  process each accessor token, compiling code in mCode
    while ( pNextToken != NULL )
    {
        pToken = pNextToken;
        pNextToken = strchr( pToken, '.' );
        if ( pNextToken != NULL )
        {
           *pNextToken++ = '\0';
        }
        bool isFinal = (pNextToken == NULL);
        pEntry = pStruct->pVocab->FindSymbol( pToken );
        SPEW_STRUCTS( "field %s", pToken );
        if ( pEntry == NULL )
        {
            SPEW_STRUCTS( " not found!\n" );
            return false;
        }
        typeCode = pEntry[1];
        bool isNative = CODE_IS_NATIVE( typeCode );
        isPtr = CODE_IS_PTR( typeCode );
        isArray = CODE_IS_ARRAY( typeCode );
        baseType = CODE_TO_BASE_TYPE( typeCode );
        isObject = (baseType == kBaseTypeObject);
        bool isMethod = CODE_IS_METHOD( typeCode );
        long opType;
        offset += pEntry[0];
        SPEW_STRUCTS( " offset %d", offset );
        if ( isArray )
        {
            SPEW_STRUCTS( (isPtr) ? " array of pointers" : " array" );
        }
        else if ( isPtr )
        {
            SPEW_STRUCTS( " pointer" );
        }
        if ( previousWasObject )
        {
            if ( !isMethod )
            {
                // TOS is object pair, discard vtable ptr since this is a member field access
                *pDst++ = OP_DROP;
                previousWasObject = false;
            }
        }
        if ( isFinal )
        {
            //
            // this is final accessor field
            //
            if ( compileVarop != 0 )
            {
                // compile variable-mode setting op just before final field
                *pDst++ = compileVarop;
            }
            SPEW_STRUCTS( " FINAL" );
            if ( isMethod )
            {
                opType = kOpMethodWithTOS;
                offset = pEntry[0];
            }
            else if ( isPtr )
            {
                SPEW_STRUCTS( (isArray) ? " array of pointers\n" : " pointer\n" );
                opType = (isArray) ? kOpFieldIntArray : kOpFieldInt;
            }
            else
            {
                if ( isNative || isObject )
                {
                    opType = ((isArray) ? kOpFieldByteArray : kOpFieldByte) + baseType;
                }
                else
                {
                    if ( isArray )
                    {
                        *pDst++ = COMPILED_OP( kOpArrayOffset, pEntry[2] );
                    }
                    opType = kOpOffset;
                }
            }
            SPEW_STRUCTS( " opcode 0x%x\n", COMPILED_OP( opType, offset ) );
            *pDst++ = COMPILED_OP( opType, offset );
        }
#if 1
        else if ( isMethod )
        {
            // This is a method which is a non-final accessor field
            // this method must return either a struct or an object
            opType = kOpMethodWithTOS;
            offset = pEntry[0];
            SPEW_STRUCTS( " opcode 0x%x\n", COMPILED_OP( opType, offset ) );
            *pDst++ = COMPILED_OP( opType, offset );
            offset = 0;
            switch ( baseType )
            {
                case kBaseTypeObject:
                    previousWasObject = true;
                    break;

                case kBaseTypeStruct:
                    break;

                default:
                    {
                        // ERROR! method must return object or struct
                        sprintf( errorMsg, "Method %s return value is not an object or struct", pToken );
                        pEngine->SetError( kForthErrorStruct, errorMsg );
                        return false;
                    }
            }

            pStruct = GetStructInfo( CODE_TO_STRUCT_INDEX( typeCode ) );
            if ( pStruct == NULL )
            {
                pEngine->SetError( kForthErrorStruct, "Method return type not found by type manager" );
                return false;
            }
        }
#endif
        else
        {
            //
            // this is not the final accessor field
            //
            if ( isNative )
            {
                // ERROR! a native field must be a final accessor
                sprintf( errorMsg, "Native %s used for non-final accessor", pToken );
                pEngine->SetError( kForthErrorStruct, errorMsg );
                return false;
            }
            // struct: do nothing (offset already added in)
            // ptr to struct: compile offset, compile @
            // array of structs: compile offset, compile array offset
            // array of ptrs to structs: compile offset, compile array offset, compile at
            // object: compile offset, compile d@
            // ptr to object: compile offset, compile @, compile d@
            // array of objects: compile offset, compile array offset, compile d@
            // array of ptrs to objects: compile offset, compile array offset, compile at, compile d@
            if ( isArray || isPtr )
            {
                if ( offset )
                {
                    SPEW_STRUCTS( " offsetOp 0x%x", COMPILED_OP( kOpOffset, offset ) );
                    *pDst++ = COMPILED_OP( kOpOffset, offset );
                    offset = 0;
                }
                if ( isArray )
                {
                    // TBD: verify the element size for arrays of ptrs to structs is 4
                    SPEW_STRUCTS( " arrayOffsetOp 0x%x", COMPILED_OP( kOpArrayOffset, pEntry[2] ) );
                    *pDst++ = COMPILED_OP( kOpArrayOffset, pEntry[2] );
                }
                if ( isPtr )
                {
                    SPEW_STRUCTS( " fetchOp 0x%x", BUILTIN_OP( OP_FETCH ) );
                    *pDst++ = BUILTIN_OP( OP_FETCH );
                }
                if ( isObject )
                {
                    SPEW_STRUCTS( " dfetchOp 0x%x", BUILTIN_OP( OP_DFETCH ) );
                    *pDst++ = BUILTIN_OP( OP_DFETCH );
                    previousWasObject = true;
                }
            }
            else if ( isObject )
            {
                if ( offset )
                {
                    SPEW_STRUCTS( " offsetOp 0x%x", COMPILED_OP( kOpOffset, offset ) );
                    *pDst++ = COMPILED_OP( kOpOffset, offset );
                    offset = 0;
                }
                SPEW_STRUCTS( " dfetchOp 0x%x", BUILTIN_OP( OP_DFETCH ) );
                *pDst++ = BUILTIN_OP( OP_DFETCH );
                previousWasObject = true;
            }
            pStruct = GetStructInfo( CODE_TO_STRUCT_INDEX( typeCode ) );
            if ( pStruct == NULL )
            {
                pEngine->SetError( kForthErrorStruct, "Struct field not found by type manager" );
                return false;
            }
        }
        pToken = pNextToken;
    }

    // when done, either compile (copy) or execute code in mCode buffer
    if ( pEngine->IsCompiling() )
    {
        int nLongs = pDst - &(mCode[0]);
        if ( compileVarop != 0 )
        {
            pEngine->UncompileLastOpcode();
        }
        pDst = pEngine->GetDP();
        pEngine->AllotLongs( nLongs );
        memcpy( pDst, &(mCode[0]), (nLongs << 2) );
    }
    else
    {
        *pDst++ = BUILTIN_OP( OP_DONE );
        exitStatus = pEngine->ExecuteOps( &(mCode[0]) );
    }
    return true;
}

// compile symbol if it is a member variable or method
bool
ForthTypesManager::ProcessMemberSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus )
{
    ForthEngine *pEngine = ForthEngine::GetInstance();
    long *pDst = &(mCode[0]);
    ForthVocabulary *pFoundVocab = NULL;
    ForthCoreState* pCore = pEngine->GetCoreState();

    ForthClassVocabulary* pVocab = GetNewestClass();
    if ( pVocab == NULL )
    {
        // TBD: report no newest class??
        return false;
    }

    long* pEntry = pVocab->FindSymbol( pInfo->GetToken() );
    if ( pEntry )
    {
        long offset = *pEntry;
        long typeCode = pEntry[1];
        long opType;
        bool isNative = CODE_IS_NATIVE( typeCode );
        bool isPtr = CODE_IS_PTR( typeCode );
        bool isArray = CODE_IS_ARRAY( typeCode );
        long baseType = CODE_TO_BASE_TYPE( typeCode );

        if ( CODE_IS_METHOD( typeCode ) )
        {
            // this is a method invocation on current object
            opType = kOpMethodWithThis;
            SPEW_STRUCTS( " opcode 0x%x\n", COMPILED_OP( opType, offset ) );
            *pDst++ = COMPILED_OP( opType, offset );
        }
        else
        {
            // this is a member variable of current object
            if ( isPtr )
            {
                SPEW_STRUCTS( (isArray) ? " array of pointers\n" : " pointer\n" );
                opType = (isArray) ? kOpMemberIntArray : kOpMemberInt;
            }
            else
            {
                if ( baseType == kBaseTypeStruct )
                {
                    if ( isArray )
                    {
                        *pDst++ = COMPILED_OP( kOpMemberRef, offset );
                        opType = kOpArrayOffset;
                        offset = pEntry[2];
                    }
                    else
                    {
                        opType = kOpMemberRef;
                    }
                }
                else
                {
                    opType = ((isArray) ? kOpMemberByteArray : kOpMemberByte) + baseType;
                }
            }
            SPEW_STRUCTS( " opcode 0x%x\n", COMPILED_OP( opType, offset ) );
            *pDst++ = COMPILED_OP( opType, offset );
        }
    }
    else
    {
        // token isn't in current class vocabulary
        return false;
    }
    // when done, compile (copy) the code in mCode buffer
    int nLongs = pDst - &(mCode[0]);
    if ( nLongs )
    {
        pDst = pEngine->GetDP();
        pEngine->AllotLongs( nLongs );
        memcpy( pDst, &(mCode[0]), (nLongs << 2) );
    }
    return true;
}


forthBaseType
ForthTypesManager::GetBaseTypeFromName( const char* typeName )
{
    for ( int i = 0; i <= kBaseTypeObject; i++ )
    {
        if ( strcmp( gpBaseTypes[i]->GetName(), typeName ) == 0 )
        {
            return gpBaseTypes[i]->GetBaseType();
        }
    }
    return kBaseTypeUnknown;
}


// ForthTypesManager::AddBuiltinClasses is defined in ForthBuiltinClasses.cpp

long*
ForthTypesManager::GetClassMethods()
{
    return mpClassMethods;
}


//////////////////////////////////////////////////////////////////////
////
///     ForthStructVocabulary
//
//

ForthStructVocabulary::ForthStructVocabulary( const char    *pName,
                                              int           structIndex )
: ForthVocabulary( pName, NUM_STRUCT_VOCAB_VALUE_LONGS, DEFAULT_VOCAB_STORAGE )
, mStructIndex( structIndex )
, mAlignment( 1 )
, mNumBytes( 0 )
, mMaxNumBytes( 0 )
, mpSearchNext( NULL )
{

}

ForthStructVocabulary::~ForthStructVocabulary()
{
}

// delete symbol entry and all newer entries
// return true IFF symbol was forgotten
bool
ForthStructVocabulary::ForgetSymbol( const char *pSymName )
{
    return false;
}


// forget all ops with a greater op#
void
ForthStructVocabulary::ForgetOp( long op )
{
}

void
ForthStructVocabulary::DefineInstance( void )
{
    // do one of the following:
    // - define a global instance of this struct type
    // - define a local instance of this struct type
    // - define a field of this struct type
    char *pToken = mpEngine->GetNextSimpleToken();
    int nBytes = mMaxNumBytes;
    char *pHere;
    long val = 0;
    ForthVocabulary *pVocab;
    long *pEntry;
    long typeCode;
    bool isPtr = false;
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthCoreState *pCore = mpEngine->GetCoreState();        // so we can GET_VAR_OPERATION

    long numElements = mpEngine->GetArraySize();
    bool isArray = (numElements != 0);
    long arrayFlag = (isArray) ? kDTIsArray : 0;
    mpEngine->SetArraySize( 0 );
    typeCode = STRUCT_TYPE_TO_CODE( arrayFlag, mStructIndex );

    if ( mpEngine->CheckFlag( kEngineFlagIsPointer ) )
    {
        mpEngine->ClearFlag( kEngineFlagIsPointer );
        nBytes = 4;
        typeCode |= kDTIsPtr;
        isPtr = true;
    }

    if ( mpEngine->InStructDefinition() )
    {
        pManager->GetNewestStruct()->AddField( pToken, typeCode, numElements );
        return;
    }

    // get next symbol, add it to vocabulary with type "user op"
    if ( mpEngine->IsCompiling() )
    {
        // define local struct
        pVocab = mpEngine->GetLocalVocabulary();
        if ( isArray )
        {
            mpEngine->SetArraySize( numElements );
            mpEngine->AddLocalArray( pToken, typeCode, nBytes );
        }
        else
        {
            pHere = (char *) (mpEngine->GetDP());
            mpEngine->AddLocalVar( pToken, typeCode, nBytes );
            if ( isPtr )
            {
                // handle "-> ptrTo STRUCT_TYPE pWoof"
                long* pLastIntoOp = mpEngine->GetLastCompiledIntoPtr();
                if ( pLastIntoOp == (((long *) pHere) - 1) )
                {
                    // local var definition was preceeded by "->", so compile the op for this local var
                    //  so it will be initialized
                    long *pEntry = pVocab->GetNewestEntry();
                    mpEngine->CompileOpcode( pEntry[0] );
                }
            }
        }
    }
    else
    {
        // define global struct
        mpEngine->AddUserOp( pToken );
        pEntry = mpEngine->GetDefinitionVocabulary()->GetNewestEntry();
        if ( isArray )
        {
            long padding = 0;
            long alignMask = mAlignment - 1;
            if ( nBytes & alignMask )
            {
                padding = mAlignment - (nBytes & alignMask);
            }
            if ( isPtr )
            {
                mpEngine->CompileOpcode( OP_DO_INT_ARRAY );
            }
            else
            {
                mpEngine->CompileOpcode( OP_DO_STRUCT_ARRAY );
                mpEngine->CompileLong( nBytes + padding );
            }
            pHere = (char *) (mpEngine->GetDP());
            mpEngine->AllotLongs( (((nBytes + padding) * (numElements - 1)) + nBytes + 3) >> 2 );
            memset( pHere, 0, ((nBytes * numElements) + 3) );
        }
        else
        {
            mpEngine->CompileOpcode( isPtr ? OP_DO_INT : OP_DO_STRUCT );
            pHere = (char *) (mpEngine->GetDP());
            mpEngine->AllotLongs( (nBytes  + 3) >> 2 );
            memset( pHere, 0, nBytes );
            if ( isPtr && (GET_VAR_OPERATION == kVarStore) )
            {
                // var definition was preceeded by "->", so initialize var
                mpEngine->ExecuteOneOp( pEntry[0] );
            }
        }
        pEntry[1] = typeCode;
    }
}

void
ForthStructVocabulary::AddField( const char* pName, long fieldType, int numElements )
{
    // a struct vocab entry has the following value fields:
    // - field offset in bytes
    // - field type
    // - padded element size (valid only for array fields)

    long fieldBytes, alignment, alignMask, padding;
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    bool isPtr = CODE_IS_PTR( fieldType );
    bool isNative = CODE_IS_NATIVE( fieldType );

    pManager->GetFieldInfo( fieldType, fieldBytes, alignment );

    if ( mNumBytes == 0 )
    {
        // this is first field in struct, so this field defines structs' alignment
        // to allow union types, the first field in each union subtype can set
        //   the alignment, but it can't reduce the alignment size
        if ( alignment > mAlignment )
        {
            mAlignment = alignment;
        }
    }
    alignMask = alignment - 1;

    // handle alignment of start of this struct (element 0 in array case)
    if ( mNumBytes & alignMask )
    {
        padding = alignment - (mNumBytes & alignMask);
        SPEW_STRUCTS( "AddField padding %d bytes before field\n", padding );
        mNumBytes += padding;
    }

    long *pEntry = AddSymbol( pName, (forthOpType) 0, mNumBytes, false );
    pEntry[1] = fieldType;
    pEntry[2] = fieldBytes;
    if ( numElements != 0 )
    {
        // TBD: handle alignment for elements after 0
        if ( fieldBytes & alignMask )
        {
           padding = alignment - (fieldBytes & alignMask);
           SPEW_STRUCTS( "AddField padding %d bytes between elements\n", padding );
           fieldBytes = (fieldBytes * numElements) + (padding * (numElements - 1));
           pEntry[2] += padding;
        }
        else
        {
           // field size matches alignment
           fieldBytes *= numElements;
        }
    }
    mNumBytes += fieldBytes;
    if ( mNumBytes > mMaxNumBytes )
    {
        mMaxNumBytes = mNumBytes;
    }
    SPEW_STRUCTS( "AddField %s code 0x%x isPtr %d isNative %d elements %d\n",
                  pName, fieldType, isPtr, isNative, numElements );
}

long
ForthStructVocabulary::GetAlignment( void )
{
    // return alignment of first field
    return mAlignment;
}

long
ForthStructVocabulary::GetSize( void )
{
    // return alignment of first field
    return mMaxNumBytes;
}

void
ForthStructVocabulary::StartUnion()
{
    // when a union subtype is started, the current size is zeroed, but
    //   the maximum size is left untouched
    // if this struct is an extension of another struct, the size is set to
    //   the size of the parent struct
    mNumBytes = (mpSearchNext) ? ((ForthStructVocabulary *) mpSearchNext)->GetSize() : 0;
}

void
ForthStructVocabulary::Extends( ForthStructVocabulary *pParentStruct )
{
    // this new struct is an extension of an existing struct - it has all
    //   the fields of the parent struct
    mpSearchNext = pParentStruct;
    mNumBytes = pParentStruct->GetSize();
    mMaxNumBytes = mNumBytes;
    mAlignment = pParentStruct->GetAlignment();
}

const char*
ForthStructVocabulary::GetType( void )
{
    return "struct";
}

// return ptr to vocabulary entry for symbol
long *
ForthStructVocabulary::FindSymbol( const char *pSymName, ulong serial )
{
    long tmpSym[SYM_MAX_LONGS];
    long* pEntry;
    ForthParseInfo parseInfo( tmpSym, SYM_MAX_LONGS );

    parseInfo.SetToken( pSymName );
    ForthStructVocabulary* pVocab = this;
    while ( pVocab )
    {
        pEntry = pVocab->ForthVocabulary::FindSymbol( &parseInfo, serial );
        if ( pEntry )
        {
            return pEntry;
        }
        pVocab = pVocab->mpSearchNext;
    }

    return NULL;
}

bool
ForthStructVocabulary::IsClass( void )
{
    return false;
}

void
ForthStructVocabulary::PrintEntry( long*   pEntry )
{
#define BUFF_SIZE 256
    char buff[BUFF_SIZE];
    char nameBuff[128];
    ForthCoreState* pCore = mpEngine->GetCoreState();
    long typeCode = pEntry[1];

    // print out the base class stuff - name and value fields
    sprintf( buff, "  %08x    ", *pEntry );
    CONSOLE_STRING_OUT( buff );

    for ( int j = 1; j < mValueLongs; j++ )
    {
        sprintf( buff, "%08x    ", pEntry[j] );
        CONSOLE_STRING_OUT( buff );
    }

    GetEntryName( pEntry, nameBuff, sizeof(nameBuff) );
    if ( strlen( nameBuff ) > 32 )
    {
        sprintf( buff, "%s    ", nameBuff );
    }
    else
    {
        sprintf( buff, "%32s    ", nameBuff );
    }
    CONSOLE_STRING_OUT( buff );

    TypecodeToString( typeCode, buff, sizeof(buff) );
    CONSOLE_STRING_OUT( buff );

    sprintf( buff, " @ offset %d", pEntry[0] );
    CONSOLE_STRING_OUT( buff );
}

void
ForthStructVocabulary::TypecodeToString( long typeCode, char* outBuff, size_t outBuffSize )
{
    char buff[BUFF_SIZE];
    char buff2[64];
    buff[0] = '\0';
    if ( CODE_IS_ARRAY( typeCode ) )
    {
        strcpy( buff, "array of " );
    }
    if ( CODE_IS_PTR( typeCode ) )
    {
        strcat( buff, "pointer to " );
    }
    if ( CODE_IS_NATIVE( typeCode ) )
    {
        int baseType = CODE_TO_BASE_TYPE( typeCode );
        sprintf( buff2, "%s", gpBaseTypes[baseType]->GetName() );
        strcat( buff, buff2 );
        if ( baseType == kBaseTypeString )
        {
            sprintf( buff2, " strLen=%d", CODE_TO_STRING_BYTES( typeCode ) );
            strcat( buff, buff2 );
        }
    }
    else
    {
        long baseType = CODE_TO_BASE_TYPE( typeCode );
        switch ( baseType )
        {
        case kBaseTypeObject:
        case kBaseTypeStruct:
            {
                long structIndex = CODE_TO_STRUCT_INDEX( typeCode );
                ForthTypeInfo* pInfo = ForthTypesManager::GetInstance()->GetStructInfo( structIndex );
                if ( pInfo )
                {
                    sprintf( buff2, "%s", pInfo->pVocab->GetName() );
                }
                else
                {
                    sprintf( buff2, "<UNKNOWN STRUCT INDEX %d!>", structIndex );
                }
            }
            break;

        case kBaseTypeUserDefinition:
            strcpy( buff2, "user defined forthop" );
            break;

        case kBaseTypeVoid:
            strcpy( buff2, "void" );
            break;

        default:
            sprintf( buff2, "UNKNOWN BASE TYPE %d", baseType );
            break;
        }
        strcat( buff, buff2 );
    }
    outBuff[ outBuffSize - 1 ] = '\0';
    strncpy( outBuff, buff, outBuffSize - 1 );
}

void ForthStructVocabulary::EndDefinition()
{
    mNumBytes = mMaxNumBytes;
}

//////////////////////////////////////////////////////////////////////
////
///     ForthClassVocabulary
//
//

ForthClassVocabulary::ForthClassVocabulary( const char*     pName,
                                            int             typeIndex )
: ForthStructVocabulary( pName, typeIndex )
, mpParentClass( NULL )
, mCurrentInterface( 0 )
{
    mpClassObject = new ForthClassObject;
    mpClassObject->pVocab = this;
    mpClassObject->newOp = OP_ALLOC_OBJECT;
    ForthInterface* pPrimaryInterface = new ForthInterface( this );
    mInterfaces.push_back( pPrimaryInterface );
}


ForthClassVocabulary::~ForthClassVocabulary()
{
    for ( unsigned int i = 0; i < mInterfaces.size(); i++ )
    {
        delete mInterfaces[i];
    }
    delete mpClassObject;
}


void
ForthClassVocabulary::DefineInstance( void )
{
    // do one of the following:
    // - define a global instance of this class type
    // - define a local instance of this class type
    // - define a field of this class type
    char *pToken = mpEngine->GetNextSimpleToken();
    int nBytes = 8;
    long *pHere;
    long val = 0;
    ForthVocabulary *pVocab;
    long *pEntry;
    long typeCode;
    bool isPtr = false;
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthCoreState *pCore = mpEngine->GetCoreState();

    long numElements = mpEngine->GetArraySize();
    bool isArray = (numElements != 0);
    long arrayFlag = (isArray) ? kDTIsArray : 0;
    mpEngine->SetArraySize( 0 );
    typeCode = OBJECT_TYPE_TO_CODE( arrayFlag, mStructIndex );

    if ( mpEngine->CheckFlag( kEngineFlagIsPointer ) )
    {
        mpEngine->ClearFlag( kEngineFlagIsPointer );
        nBytes = 4;
        typeCode |= kDTIsPtr;
        isPtr = true;
    }

    if ( mpEngine->CheckFlag( kEngineFlagInStructDefinition | kEngineFlagIsMethod ) == kEngineFlagInStructDefinition )
    {
        pManager->GetNewestStruct()->AddField( pToken, typeCode, numElements );
        return;
    }

    // get next symbol, add it to vocabulary with type "user op"
    if ( mpEngine->IsCompiling() )
    {
        // define local object
        pVocab = mpEngine->GetLocalVocabulary();
        if ( isArray )
        {
            mpEngine->SetArraySize( numElements );
            mpEngine->AddLocalArray( pToken, typeCode, nBytes );
        }
        else
        {
            pHere = mpEngine->GetDP();
            mpEngine->AddLocalVar( pToken, typeCode, nBytes );
            long* pLastIntoOp = mpEngine->GetLastCompiledIntoPtr();
            if ( pLastIntoOp == (((long *) pHere) - 1) )
            {
                // local var definition was preceeded by "->", so compile the op for this local var
                //  so it will be initialized
                long *pEntry = pVocab->GetNewestEntry();
                mpEngine->CompileOpcode( COMPILED_OP( (isPtr ? kOpLocalInt : kOpLocalObject), pEntry[0] ) );
            }
        }
    }
    else
    {
        // define global object(s)
        mpEngine->AddUserOp( pToken );
        pEntry = mpEngine->GetDefinitionVocabulary()->GetNewestEntry();
        if ( isArray )
        {
            mpEngine->CompileOpcode( isPtr ? OP_DO_INT_ARRAY : OP_DO_OBJECT_ARRAY );
            pHere = mpEngine->GetDP();
            mpEngine->AllotLongs( (nBytes * numElements) >> 2 );
            memset( pHere, 0, (nBytes * numElements) );
            if ( !(typeCode & kDTIsPtr) )
            {
                for ( int i = 0; i < numElements; i++ )
                {
                    // TBD: fill in vtable pointer
                    pHere[i * 2] = 0;
                }
            }
        }
        else
        {
            mpEngine->CompileOpcode( isPtr ? OP_DO_INT : OP_DO_OBJECT );
            pHere = mpEngine->GetDP();
            mpEngine->AllotLongs( nBytes >> 2 );
            memset( pHere, 0, nBytes );
            if ( !isPtr )
            {
                // TBD: fill in vtable pointer
                *pHere = 0;
            }
            if ( GET_VAR_OPERATION == kVarStore )
            {
                if ( isPtr )
                {
                    *pHere = SPOP;
                }
                else
                {
                    *pHere++ = SPOP;
                    *pHere = SPOP;
                }
                CLEAR_VAR_OPERATION;
            }
        }
        pEntry[1] = typeCode;
    }
}

void
ForthClassVocabulary::DoOp( ForthCoreState *pCore )
{
    if ( GET_VAR_OPERATION == kVocabGetClass )
    {
        // this is invoked at runtime when code explicitly invokes methods on class objects (IE CLASSNAME.new)
        SPUSH( (long) mpClassObject );
        SPUSH( (long) ForthTypesManager::GetInstance()->GetClassMethods() );
        CLEAR_VAR_OPERATION;
    }
    else
    {
        ForthVocabulary::DoOp( pCore );
    }
}

long
ForthClassVocabulary::AddMethod( const char*    pName,
                                 long           op )
{
	ForthInterface* pCurInterface = mInterfaces[ mCurrentInterface ];
	// see if method name is already defined - if so, just overwrite the method longword with op
	// if name is not already defined, add the method name and op
    long methodIndex = pCurInterface->GetMethodIndex( pName );
    if ( methodIndex < 0 )
    {
        // method name was not found in current interface
        if ( mCurrentInterface == 0 )
        {
            // add new method to primary interface
            methodIndex = pCurInterface->AddMethod( op );
        }
        else
        {
            // TBD: report error - trying to add a method to a secondary interface
        }
	}
    else
    {
        // overwrite method
        pCurInterface->SetMethod( methodIndex, op );
    }
    return methodIndex;
}


void
ForthClassVocabulary::Extends( ForthStructVocabulary *pParentStruct )
{
	if ( pParentStruct->IsClass() )
	{
		mpParentClass = reinterpret_cast<ForthClassVocabulary *>(pParentStruct);
		long numInterfaces = mpParentClass->GetNumInterfaces();
		mInterfaces.resize( numInterfaces );
		for ( int i = 0; i < numInterfaces; i++ )
		{
			if ( i != 0 )
			{
				mInterfaces[i] = new ForthInterface;
			}
			mInterfaces[i]->Copy( mpParentClass->GetInterface( i ) );
		}
	}

	ForthStructVocabulary::Extends( pParentStruct );
}


void
ForthClassVocabulary::Implements( const char* pName )
{
	ForthStructVocabulary* pVocab = ForthTypesManager::GetInstance()->GetStructVocabulary( pName );

	if ( pVocab )
	{
		if ( pVocab->IsClass() )
		{
			ForthClassVocabulary* pClassVocab = reinterpret_cast<ForthClassVocabulary *>(pVocab);
            long interfaceIndex = FindInterfaceIndex( pClassVocab->GetClassId() );
            if ( interfaceIndex > 0 )
            {
                // this is an interface we have already inherited from superclass
                mCurrentInterface = interfaceIndex;
            }
            else if ( interfaceIndex < 0 )
            {
                // this is an interface which this class doesn't already have
                ForthInterface* pNewInterface = new ForthInterface( this );
                pNewInterface->Implements( pClassVocab );
                mInterfaces.push_back( pNewInterface );
            }
            else
            {
                // TBD: report error - target of "implements" is same class!
            }
		}
		else
		{
			// TBD: report that vocab is struct, not class
		}
	
	}
	else
	{
		// TBD: report vocabulary not found
	}
}


void
ForthClassVocabulary::EndImplements()
{
	// TBD: report error if not all methods implemented
    mCurrentInterface = 0;
}



bool
ForthClassVocabulary::IsClass( void )
{
	return true;
}

ForthInterface*
ForthClassVocabulary::GetInterface( long index )
{
	return mInterfaces[index];
}

long
ForthClassVocabulary::FindInterfaceIndex( long classId )
{
    for ( unsigned int i = 0; i < mInterfaces.size(); i++ )
    {
        ForthClassVocabulary* pVocab = mInterfaces[i]->GetDefiningClass();
        if ( pVocab->GetClassId() == classId )
        {
            return i;
        }
    }
	return -1;
}


long
ForthClassVocabulary::GetNumInterfaces( void )
{
	return mInterfaces.size();
}


ForthClassObject*
ForthClassVocabulary::GetClassObject( void )
{
    return mpClassObject;
}


void
ForthClassVocabulary::PrintEntry( long*   pEntry )
{
    char buff[BUFF_SIZE];
    char nameBuff[128];
    long methodNum = *pEntry;
    long typeCode = pEntry[1];
    bool isMethod = CODE_IS_METHOD( typeCode );
    if ( !isMethod )
    {
        ForthStructVocabulary::PrintEntry( pEntry );
        return;
    }

    ForthCoreState* pCore = mpEngine->GetCoreState();
    ForthInterface* pPrimaryInterface = GetInterface( 0 );

    sprintf( buff, "  %08x    ", methodNum );
    CONSOLE_STRING_OUT( buff );

    for ( int j = 1; j < mValueLongs; j++ )
    {
        sprintf( buff, "%08x    ", pEntry[j] );
        CONSOLE_STRING_OUT( buff );
    }

    GetEntryName( pEntry, nameBuff, sizeof(nameBuff) );
    if ( strlen( nameBuff ) > 32 )
    {
        sprintf( buff, "%s    ", nameBuff );
    }
    else
    {
        sprintf( buff, "%32s    ", nameBuff );
    }
    CONSOLE_STRING_OUT( buff );

    sprintf( buff, "method # %d returning ", methodNum );
    CONSOLE_STRING_OUT( buff );

    if ( CODE_IS_ARRAY( typeCode ) )
    {
        CONSOLE_STRING_OUT( "array of " );
    }
    if ( CODE_IS_PTR( typeCode ) )
    {
        CONSOLE_STRING_OUT( "pointer to " );
    }
    if ( CODE_IS_NATIVE( typeCode ) )
    {
        int baseType = CODE_TO_BASE_TYPE( typeCode );
        sprintf( buff, "%s ", gpBaseTypes[baseType]->GetName() );
        CONSOLE_STRING_OUT( buff );
        if ( baseType == kBaseTypeString )
        {
            sprintf( buff, "strLen=%d, ", CODE_TO_STRING_BYTES( typeCode ) );
            CONSOLE_STRING_OUT( buff );
        }
    }
    else
    {
        long baseType = CODE_TO_BASE_TYPE( typeCode );
        switch ( baseType )
        {
        case kBaseTypeObject:
        case kBaseTypeStruct:
            {
                long structIndex = CODE_TO_STRUCT_INDEX( typeCode );
                ForthTypeInfo* pInfo = ForthTypesManager::GetInstance()->GetStructInfo( structIndex );
                if ( pInfo )
                {
                    sprintf( buff, "%s ", pInfo->pVocab->GetName() );
                }
                else
                {
                    sprintf( buff, "<UNKNOWN STRUCT INDEX %d!> ", structIndex );
                }
            }
            break;

        case kBaseTypeUserDefinition:
            strcpy( buff, "user defined forthop " );
            break;

        case kBaseTypeVoid:
            strcpy( buff, "void " );
            break;

        default:
            sprintf( buff, "UNKNOWN BASE TYPE %d ", baseType );
            break;
        }
        CONSOLE_STRING_OUT( buff );
    }
    long* pMethod = pPrimaryInterface->GetMethods() + methodNum;
    sprintf( buff, "opcode=%02x:%06x", GetEntryType(pMethod), GetEntryValue(pMethod) );
    CONSOLE_STRING_OUT( buff );
}

// TBD: implement FindSymbol which iterates over all interfaces

//////////////////////////////////////////////////////////////////////
////
///     ForthInterface
//
//

ForthInterface::ForthInterface( ForthClassVocabulary* pDefiningClass )
: mpDefiningClass( pDefiningClass )
, mNumAbstractMethods( 0 )
{
    long classObject = (long) pDefiningClass->GetClassObject();
    mMethods.push_back( classObject );
}


ForthInterface::~ForthInterface()
{
}


void
ForthInterface::Copy( ForthInterface* pInterface )
{
    mpDefiningClass = pInterface->GetDefiningClass();
    mNumAbstractMethods = pInterface->mNumAbstractMethods;
    int numMethods = pInterface->mMethods.size();
    mMethods.resize( numMethods );
    for ( int i = 1; i < numMethods; i++ )
    {
        mMethods[i] = pInterface->mMethods[i];
    }
}


ForthClassVocabulary*
ForthInterface::GetDefiningClass()
{
    return mpDefiningClass;
}


long*
ForthInterface::GetMethods( void )
{
	return &( mMethods[1] );
}


long
ForthInterface::GetMethod( long index )
{
    // TBD: check index in bounds
	return mMethods[index + 1];
}


void
ForthInterface::SetMethod( long index, long method )
{
    // TBD: check index in bounds
    index++;
    if ( mMethods[index] != method )
    {
        if ( method != OP_BAD_OP )
        {
            mNumAbstractMethods--;
        }
    	mMethods[index] = method;
        // NOTE: we don't support the case where the old method was concrete and the new method is abstract
    }
}


void
ForthInterface::Implements( ForthClassVocabulary* pVocab )
{
    ForthInterface* pInterface = pVocab->GetInterface( 0 );
    long numMethods = pInterface->mMethods.size();
    mMethods.resize( numMethods );
	for ( int i = 1; i < numMethods; i++ )
	{
		mMethods[i] = OP_BAD_OP;	// TBD: make this "unimplemented method" opcode
	}
    mNumAbstractMethods = numMethods;
}


long
ForthInterface::AddMethod( long method )
{
    long methodIndex = mMethods.size() - 1;
	mMethods.push_back( method );
    if ( method == OP_BAD_OP )
    {
        mNumAbstractMethods++;
    }
    return methodIndex;
}


long
ForthInterface::GetNumMethods( void )
{
    return static_cast<long>( mMethods.size() - 1 );
}


long
ForthInterface::GetNumAbstractMethods( void )
{
    return mNumAbstractMethods;
}


long
ForthInterface::GetMethodIndex( const char* pName )
{
    // TBD: lookup method name in defining vocabulary and return its method index
    long* pEntry = mpDefiningClass->FindSymbol( pName );
    if ( pEntry )
    {
        long typeCode = pEntry[1];
        if ( CODE_IS_METHOD( typeCode ) )
        {
            return pEntry[0];
        }
    }
    return -1;
}


//////////////////////////////////////////////////////////////////////
////
///     ForthBaseType
//
//

ForthBaseType::ForthBaseType( const char*       pName,
                                  int               numBytes,
                                  forthBaseType   baseType )
: mpName( pName )
, mNumBytes( numBytes )
, mBaseType( baseType )
{
}

ForthBaseType::~ForthBaseType()
{
}

void
ForthBaseType::DefineInstance( ForthEngine *pEngine, void *pInitialVal, long flags )
{
    char *pToken = pEngine->GetNextSimpleToken();
    int nBytes = mNumBytes;
    char *pHere;
    int i;
    long val = 0;
    forthBaseType baseType = mBaseType;
    ForthVocabulary *pVocab;
    long *pEntry;
    long typeCode, len, varOffset, storageLen;
    char *pStr;
    ForthCoreState *pCore = pEngine->GetCoreState();        // so we can SPOP maxbytes
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();

    bool isString = (baseType == kBaseTypeString);

    if ( isString )
    {
        // get maximum string length
        if ( pEngine->IsCompiling() )
        {
            // the symbol just before "string" should have been an integer constant
            if ( !pEngine->GetLastConstant( len ) )
            {
                SET_ERROR( kForthErrorMissingSize );
            }
        }
        else
        {
            len = SPOP;
        }
    }

    long numElements = pEngine->GetArraySize();
    if ( numElements != 0 )
    {
        flags |= kDTIsArray;
    }
    pEngine->SetArraySize( 0 );
    if ( isString )
    {
        typeCode = STRING_TYPE_TO_CODE( flags, len );
    }
    else
    {
        typeCode = NATIVE_TYPE_TO_CODE( flags, baseType );
    }

    if ( pEngine->CheckFlag( kEngineFlagIsPointer ) )
    {
        // outside of a struct definition, any native variable or array defined with "ptrTo"
        //  is the same thing as an int variable or array, since native types have no fields
        pEngine->ClearFlag( kEngineFlagIsPointer );
        baseType = kBaseTypeInt;
        nBytes = 4;
        pInitialVal = &val;
        typeCode |= kDTIsPtr;
        isString = false;
    }

    if ( pEngine->InStructDefinition() )
    {
        pManager->GetNewestStruct()->AddField( pToken, typeCode, numElements );
        return;
    }

    // get next symbol, add it to vocabulary with type "user op"
    if ( !isString )
    {
        if ( pEngine->IsCompiling() )
        {
            // define local variable
            pVocab = pEngine->GetLocalVocabulary();
            if ( numElements )
            {
                pEngine->SetArraySize( numElements );
                pEngine->AddLocalArray( pToken, typeCode, nBytes );
            }
            else
            {
                pHere = (char *) (pEngine->GetDP());
                pEngine->AddLocalVar( pToken, typeCode, nBytes );
                long* pLastIntoOp = pEngine->GetLastCompiledIntoPtr();
                if ( pLastIntoOp == (((long *) pHere) - 1) )
                {
                    // local var definition was preceeded by "->", so compile the op for this local var
                    //  so it will be initialized
                    long *pEntry = pVocab->GetNewestEntry();
                    pEngine->CompileOpcode( pEntry[0] );
                }
            }
        }
        else
        {
            // define global variable
            pEngine->AddUserOp( pToken );
            pEntry = pEngine->GetDefinitionVocabulary()->GetNewestEntry();
            if ( numElements )
            {
                // define global array
                pEngine->CompileOpcode( OP_DO_BYTE_ARRAY + baseType );
                pHere = (char *) (pEngine->GetDP());
                pEngine->AllotLongs( ((nBytes * numElements) + 3) >> 2 );
                for ( i = 0; i < numElements; i++ )
                {
                    memcpy( pHere, pInitialVal, nBytes );
                    pHere += nBytes;
                }
            }
            else
            {
                // define global single variable
                pEngine->CompileOpcode( OP_DO_BYTE + baseType );
                pHere = (char *) (pEngine->GetDP());
                pEngine->AllotLongs( (nBytes  + 3) >> 2 );
                if ( GET_VAR_OPERATION == kVarStore )
                {
                    // var definition was preceeded by "->", so initialize var
                    pEngine->ExecuteOneOp( pEntry[0] );
                }
                else
                {
                    memcpy( pHere, pInitialVal, nBytes );
                }
            }
            pEntry[1] = typeCode;
        }
    }
    else
    {
        // instance is string
        if ( pEngine->IsCompiling() )
        {
            pVocab = pEngine->GetLocalVocabulary();
            // uncompile the integer contant opcode - it is the string maxLen
            pEngine->UncompileLastOpcode();
            storageLen = ((len >> 2) + 3) << 2;

            if ( numElements )
            {
                // define local string array
                varOffset = pEngine->AddLocalArray( pToken, typeCode, storageLen );
                pEngine->CompileOpcode( COMPILED_OP( kOpConstant, numElements ) );
                pEngine->CompileOpcode( COMPILED_OP( kOpConstant, len ) );
                pEngine->CompileOpcode( COMPILED_OP( kOpLocalRef, varOffset - 2) );
                pEngine->CompileOpcode( OP_INIT_STRING_ARRAY );
            }
            else
            {
                // define local string variable
                pHere = (char *) (pEngine->GetDP());
                varOffset = pEngine->AddLocalVar( pToken, typeCode, storageLen );
                // compile initLocalString op
                varOffset = (varOffset << 12) | len;
                // NOTE: do not use CompileOpcode here - it would screw up the OP_INTO check just below
                pEngine->CompileOpcode( COMPILED_OP( kOpInitLocalString, varOffset ) );
                long* pLastIntoOp = pEngine->GetLastCompiledIntoPtr();
                if ( pLastIntoOp == (((long *) pHere) - 1) )
                {
                    // local var definition was preceeded by "->", so compile the op for this local var
                    //  so it will be initialized
                    long *pEntry = pVocab->GetNewestEntry();
                    pEngine->CompileOpcode( pEntry[0] );
                }
            }
        }
        else
        {
            pEngine->AddUserOp( pToken );
            pEntry = pEngine->GetDefinitionVocabulary()->GetNewestEntry();
            if ( numElements )
            {
                // define global string array
                pEngine->CompileOpcode( OP_DO_STRING_ARRAY );
                for ( i = 0; i < numElements; i++ )
                {
                    pEngine->CompileLong( len );
                    pEngine->CompileLong( 0 );
                    pStr = (char *) (pEngine->GetDP());
                    // a length of 4 means room for 4 chars plus a terminating null
                    pEngine->AllotLongs( ((len  + 4) & ~3) >> 2 );
                    *pStr = 0;
                }
            }
            else
            {
                // define global string variable
                pEngine->CompileOpcode( OP_DO_STRING );
                pEngine->CompileLong( len );
                pEngine->CompileLong( 0 );
                pStr = (char *) (pEngine->GetDP());
                // a length of 4 means room for 4 chars plus a terminating null
                pEngine->AllotLongs( ((len  + 4) & ~3) >> 2 );
                *pStr = 0;
                if ( GET_VAR_OPERATION == kVarStore )
                {
                    // var definition was preceeded by "->", so initialize var
                    pEngine->ExecuteOneOp( pEntry[0] );
                }
            }
            pEntry[1] = typeCode;
        }
    }
}


//////////////////////////////////////////////////////////////////////
////
///     globals
//
//

ForthBaseType gBaseTypeByte( "byte", 1, kBaseTypeByte );

ForthBaseType gBaseTypeShort( "short", 2, kBaseTypeShort );

ForthBaseType gBaseTypeInt( "int", 4, kBaseTypeInt );

ForthBaseType gBaseTypeFloat( "float", 4, kBaseTypeFloat );

ForthBaseType gBaseTypeDouble( "double", 8, kBaseTypeDouble );

ForthBaseType gBaseTypeString( "string", 12, kBaseTypeString );

ForthBaseType gBaseTypeOp( "op", 4, kBaseTypeOp );

ForthBaseType gBaseTypeObject( "object", 8, kBaseTypeObject );

ForthBaseType gBaseTypeLong( "long", 8, kBaseTypeLong );

ForthBaseType *gpBaseTypes[] =
{
    &gBaseTypeByte,
    &gBaseTypeShort,
    &gBaseTypeInt,
    &gBaseTypeFloat,
    &gBaseTypeDouble,
    &gBaseTypeString,
    &gBaseTypeOp,
    &gBaseTypeObject,
    &gBaseTypeLong
};
