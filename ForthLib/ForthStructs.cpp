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
///     ForthStructsManager
//
//

ForthStructsManager *ForthStructsManager::mpInstance = NULL;

ForthStructsManager::ForthStructsManager()
: ForthForgettable( NULL, 0 )
, mNumStructs( 0 )
, mMaxStructs( 0 )
, mpStructInfo( NULL )
, mpSavedDefinitionVocab( NULL )
{
    ASSERT( mpInstance == NULL );
    mpInstance = this;
}

ForthStructsManager::~ForthStructsManager()
{
    mpInstance = NULL;
    delete mpStructInfo;
}

void
ForthStructsManager::ForgetCleanup( void *pForgetLimit, long op )
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
ForthStructsManager::StartStructDefinition( const char *pName )
{
    ForthEngine *pEngine = ForthEngine::GetInstance();
    ForthVocabulary* pDefinitionsVocab = pEngine->GetDefinitionVocabulary();

    if ( mNumStructs == mMaxStructs )
    {
        mMaxStructs += STRUCTS_EXPANSION_INCREMENT;
        SPEW_STRUCTS( "StartStructDefinition expanding structs table to %d entries\n", mMaxStructs );
        mpStructInfo = (ForthStructInfo *) realloc( mpStructInfo, sizeof(ForthStructInfo) * mMaxStructs );
    }
    ForthStructInfo *pInfo = &(mpStructInfo[mNumStructs]);

    long *pEntry = pEngine->StartOpDefinition( pName, true, kOpUserDefImmediate );
    pInfo->pVocab = new ForthStructVocabulary( pName, mNumStructs );
    pInfo->op = *pEntry;
    SPEW_STRUCTS( "StartStructDefinition %s struct index %d\n", pName, mNumStructs );
    mNumStructs++;
    return pInfo->pVocab;
}

void
ForthStructsManager::EndStructDefinition()
{
    ForthEngine *pEngine = ForthEngine::GetInstance();
    pEngine->EndOpDefinition( true );
}

ForthClassVocabulary*
ForthStructsManager::StartClassDefinition( const char *pName )
{
    ForthEngine *pEngine = ForthEngine::GetInstance();
    ForthVocabulary* pDefinitionsVocab = pEngine->GetDefinitionVocabulary();

    if ( mNumStructs == mMaxStructs )
    {
        mMaxStructs += STRUCTS_EXPANSION_INCREMENT;
        SPEW_STRUCTS( "StartClassDefinition expanding structs table to %d entries\n", mMaxStructs );
        mpStructInfo = (ForthStructInfo *) realloc( mpStructInfo, sizeof(ForthStructInfo) * mMaxStructs );
    }
    ForthStructInfo *pInfo = &(mpStructInfo[mNumStructs]);

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
ForthStructsManager::EndClassDefinition()
{
    ForthEngine *pEngine = ForthEngine::GetInstance();
    pEngine->EndOpDefinition( false );
    pEngine->SetDefinitionVocabulary( mpSavedDefinitionVocab );
    mpSavedDefinitionVocab = NULL;
}

ForthStructVocabulary*
ForthStructsManager::GetStructVocabulary( long op )
{
    int i;
    ForthStructInfo *pInfo;

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
ForthStructsManager::GetStructVocabulary( const char* pName )
{
    int i;
    ForthStructInfo *pInfo;

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

ForthStructInfo*
ForthStructsManager::GetStructInfo( int structIndex )
{
    if ( structIndex >= mNumStructs )
    {
        SPEW_STRUCTS( "GetStructInfo error: structIndex is %d, only %d structs exist\n",
                        structIndex, mNumStructs );
        return NULL;
    }
    return &(mpStructInfo[ structIndex ]);
}

ForthStructsManager*
ForthStructsManager::GetInstance( void )
{
    ASSERT( mpInstance != NULL );
    return mpInstance;
}

void
ForthStructsManager::GetFieldInfo( long fieldType, long& fieldBytes, long& alignment )
{
    long subType;
    if ( CODE_IS_PTR( fieldType ) )
    {
        fieldBytes = 4;
        alignment = 4;
    }
    else if ( CODE_IS_NATIVE( fieldType ) )
    {
        subType = CODE_TO_NATIVE_TYPE( fieldType );
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
        ForthStructInfo* pInfo = GetStructInfo( CODE_TO_STRUCT_INDEX( fieldType ) );
        if ( pInfo )
        {
            alignment = pInfo->pVocab->GetAlignment();
            fieldBytes = pInfo->pVocab->GetSize();
        }
    }
}

ForthStructVocabulary *
ForthStructsManager::GetNewestStruct( void )
{
    ForthStructsManager* pThis = GetInstance();
    return pThis->mpStructInfo[ pThis->mNumStructs - 1 ].pVocab;
}

ForthClassVocabulary *
ForthStructsManager::GetNewestClass( void )
{
    ForthStructsManager* pThis = GetInstance();
    ForthStructVocabulary* pVocab = pThis->mpStructInfo[ pThis->mNumStructs - 1 ].pVocab;
    if ( pVocab && !pVocab->IsClass() )
    {
        pVocab = NULL;
    }
    return (ForthClassVocabulary *) pVocab;
}

// compile/interpret symbol if is a valid structure accessor
bool
ForthStructsManager::ProcessSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus )
{
    long typeCode;
    ForthEngine *pEngine = ForthEngine::GetInstance();
    long *pDst = &(mCode[0]);
    strcpy( mToken, pInfo->GetToken() );
    // get first token
    char *pToken = &(mToken[0]);
    char *pNextToken = strchr( mToken, '.' );
    ForthVocabulary *pFoundVocab = NULL;
    ForthCoreState* pCore = pEngine->GetCoreState();


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
    long *pEntry = pEngine->GetLocalVocabulary()->FindSymbol( mToken );
    if ( pEntry == NULL )
    {
        pEntry = pEngine->GetVocabularyStack()->FindSymbol( mToken, &pFoundVocab );
    }
    bool tosIsObject = false;
    if ( pEntry )
    {
        // see if token is a struct
        typeCode = pEntry[1];
        if ( CODE_IS_NATIVE( typeCode ) )
        {
            SPEW_STRUCTS( "Native type cant have fields\n" );
            return false;
        }
        SPEW_STRUCTS( "Struct first field %s op 0x%x\n", pToken, pEntry[0] );
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
                *pDst++ = *pLastOp;
                *pLastOp = pEntry[0];
            }
            else
            {
                *pDst++ = pEntry[0];
            }
        }
        else
        {
            // we are interpreting, clear any existing varAction, but compile an op to set it after first op
            ulong varMode = GET_VAR_OPERATION;
            *pDst++ = pEntry[0];
            if ( varMode )
            {
                CLEAR_VAR_OPERATION;
                *pDst++ = OP_ADDRESS_OF + (varMode - kVarRef);
            }
        }
    }
    else
    {
        // token not found in search or local vocabs
        return false;
    }

    ForthStructInfo* pStruct = GetStructInfo( CODE_TO_STRUCT_INDEX( typeCode ) );
    if ( pStruct == NULL )
    {
        SPEW_STRUCTS( "Struct first field not found by structs manager\n" );
        return false;
    }
    else
    {
        SPEW_STRUCTS( "Struct first field of type %s\n", pStruct->pVocab->GetName() );
    }
    if ( pStruct->pVocab->IsClass() )
    {
        tosIsObject = true;
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
        bool isPtr = CODE_IS_PTR( typeCode );
        bool isArray = CODE_IS_ARRAY( typeCode );
        long nativeType = CODE_TO_NATIVE_TYPE( typeCode );
        bool isObject = isNative && (nativeType == kBaseTypeObject);
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
        if ( tosIsObject )
        {
            if ( isMethod )
            {
                isFinal = true;
            }
            else
            {
                // TOS is object pair, discard vtable ptr since this is a member field access
                *pDst++ = OP_DROP;
                tosIsObject = false;
            }
        }
        if ( isFinal )
        {
            //
            // this is final accessor field
            //
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
                if ( isNative )
                {
                    opType = ((isArray) ? kOpFieldByteArray : kOpFieldByte) + nativeType;
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
        else
        {
            //
            // this is not the final accessor field
            //
            if ( isNative && !isObject )
            {
                // ERROR! a native field must be a final accessor
                char blah[256];
                sprintf( blah, "Native %s used for non-final accessor", pToken );
                pEngine->SetError( kForthErrorStruct, blah );
                return false;
            }
            // struct: do nothing (offset already added in
            // ptr to struct: compile offset, compile @
            // array of structs: compile offset, compile array offset
            // array of ptrs to structs: compile offset, compile array offset, compile at
            // object: compile offset, compile d@
            // ptr to object: compile offset, compile @, compile d@
            // array of objects: compile offset, compile array offset, compile d@
            // array of ptrs to objects: compile offset, compile array offset, compile at, compile d@
            if ( isArray || isPtr )
            {
                SPEW_STRUCTS( " offsetOp 0x%x", COMPILED_OP( kOpOffset, offset ) );
                *pDst++ = COMPILED_OP( kOpOffset, offset );
                offset = 0;
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
                    tosIsObject = true;
                }
            }
            else if ( isObject )
            {
                SPEW_STRUCTS( " offsetOp 0x%x", COMPILED_OP( kOpOffset, offset ) );
                *pDst++ = COMPILED_OP( kOpOffset, offset );
                offset = 0;
                SPEW_STRUCTS( " dfetchOp 0x%x", BUILTIN_OP( OP_DFETCH ) );
                *pDst++ = BUILTIN_OP( OP_DFETCH );
                tosIsObject = true;
            }
            pStruct = GetStructInfo( CODE_TO_STRUCT_INDEX( typeCode ) );
            if ( pStruct == NULL )
            {
                SPEW_STRUCTS( "Struct field not found by structs manager\n" );
                return false;
            }
        }
        pToken = pNextToken;
    }
    // when done, either compile (copy) or execute code in mCode buffer
    if ( pEngine->IsCompiling() )
    {
        int nLongs = pDst - &(mCode[0]);
        pDst = pEngine->GetDP();
        pEngine->AllotLongs( nLongs );
        memcpy( pDst, &(mCode[0]), (nLongs << 2) );
    }
    else
    {
        *pDst++ = BUILTIN_OP( OP_DONE );
        pEngine->ExecuteOps( &(mCode[0]) );
    }
    return true;
}

// compile symbol if it is a member variable or method
bool
ForthStructsManager::ProcessMemberSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus )
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
            bool isNative = CODE_IS_NATIVE( typeCode );
            bool isPtr = CODE_IS_PTR( typeCode );
            bool isArray = CODE_IS_ARRAY( typeCode );
            if ( isPtr )
            {
                SPEW_STRUCTS( (isArray) ? " array of pointers\n" : " pointer\n" );
                opType = (isArray) ? kOpMemberIntArray : kOpMemberInt;
            }
            else
            {
                if ( isNative )
                {
                    opType = ((isArray) ? kOpMemberByteArray : kOpMemberByte) + CODE_TO_NATIVE_TYPE( typeCode );
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
    int nBytes = mNumBytes;
    char *pHere;
    long val = 0;
    ForthVocabulary *pVocab;
    long *pEntry;
    long typeCode;
    bool isPtr = false;
    ForthStructsManager* pManager = ForthStructsManager::GetInstance();
    ForthCoreState *pCore = mpEngine->GetCoreState();        // so we can GET_VAR_OPERATION

    long numElements = mpEngine->GetArraySize();
    mpEngine->SetArraySize( 0 );
    typeCode = STRUCT_TYPE_TO_CODE( ((numElements) ? kDTArray : kDTSingle), mStructIndex );

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
        if ( numElements )
        {
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
        if ( numElements )
        {
            long padding = 0;
            long alignMask = mAlignment - 1;
            if ( nBytes & alignMask )
            {
                padding = mAlignment - (nBytes & alignMask);
            }
            mpEngine->CompileOpcode( (typeCode & kDTIsPtr) ? OP_DO_INT_ARRAY : OP_DO_STRUCT_ARRAY );
            mpEngine->CompileLong( nBytes + padding );
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
    ForthStructsManager* pManager = ForthStructsManager::GetInstance();
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
    // print out the base class stuff - name and value fields
    sprintf( buff, "%02x:%06x    ", GetEntryType( pEntry ), GetEntryValue( pEntry ) );
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

    long typeCode = pEntry[1];
    if ( CODE_IS_METHOD( typeCode ) )
    {
        CONSOLE_STRING_OUT( "Method returning " );
    }
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
        int nativeType = CODE_TO_NATIVE_TYPE( typeCode );
        if ( nativeType < kNumBaseTypes )
        {
            sprintf( buff, "%s ", gpBaseTypes[nativeType]->GetName() );
            CONSOLE_STRING_OUT( buff );
            if ( nativeType == kBaseTypeString )
            {
                sprintf( buff, "strLen=%d, ", CODE_TO_STRING_BYTES( typeCode ) );
                CONSOLE_STRING_OUT( buff );
            }
        }
        else
        {
            sprintf( buff, "<INVALID NATIVE TYPE %d!> ", nativeType );
        }
    }
    else
    {
        long structIndex = CODE_TO_STRUCT_INDEX( typeCode );
        ForthStructInfo* pInfo = ForthStructsManager::GetInstance()->GetStructInfo( structIndex );
        if ( pInfo )
        {
            sprintf( buff, "%s ", pInfo->pVocab->GetName() );
        }
        else
        {
            sprintf( buff, "<UNKNOWN STRUCT INDEX %d!> ", structIndex );
        }

        CONSOLE_STRING_OUT( buff );
    }
    if ( CODE_IS_METHOD( typeCode ) )
    {
        sprintf( buff, "method # %d", pEntry[0] );
    }
    else
    {
        sprintf( buff, "@ offset %d", pEntry[0] );
    }
    CONSOLE_STRING_OUT( buff );
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
    ForthInterface* pPrimaryInterface = new ForthInterface( this );
    mInterfaces.Add( pPrimaryInterface );
}


ForthClassVocabulary::~ForthClassVocabulary()
{
    for ( int i = 0; i < mInterfaces.GetSize(); i++ )
    {
        delete mInterfaces[i];
    }
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
    ForthStructsManager* pManager = ForthStructsManager::GetInstance();
    ForthCoreState *pCore = mpEngine->GetCoreState();

    long numElements = mpEngine->GetArraySize();
    bool isArray = (numElements != 0);
    mpEngine->SetArraySize( 0 );
    typeCode = STRUCT_TYPE_TO_CODE( (isArray ? kDTArray : kDTSingle), mStructIndex );

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
        if ( numElements )
        {
            mpEngine->CompileOpcode( OP_DO_OBJECT_ARRAY );
            mpEngine->CompileLong( nBytes );
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
            mpEngine->CompileOpcode( OP_DO_OBJECT );
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
		mInterfaces.SetSize( numInterfaces );
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
	ForthStructVocabulary* pVocab = ForthStructsManager::GetInstance()->GetStructVocabulary( pName );

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
                ForthInterface* pNewInterface = new ForthInterface;
                pNewInterface->Implements( pClassVocab );
                mInterfaces.Add( pNewInterface );
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
    for ( int i = 0; i < mInterfaces.GetSize(); i++ )
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
	return mInterfaces.GetSize();
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
}


ForthInterface::~ForthInterface()
{
}


void
ForthInterface::Copy( ForthInterface* pInterface )
{
    mpDefiningClass = pInterface->GetDefiningClass();
    mNumAbstractMethods = pInterface->mNumAbstractMethods;
    int numMethods = pInterface->mMethods.GetSize();
    mMethods.SetSize( numMethods );
    for ( int i = 0; i < numMethods; i++ )
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
	return &( mMethods[0] );
}


long
ForthInterface::GetMethod( long index )
{
    // TBD: check index in bounds
	return mMethods[index];
}


void
ForthInterface::SetMethod( long index, long method )
{
    // TBD: check index in bounds
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
    long numMethods = pInterface->GetNumMethods();
    mMethods.SetSize( numMethods );
	for ( int i = 0; i < numMethods; i++ )
	{
		mMethods[i] = OP_BAD_OP;	// TBD: make this "unimplemented method" opcode
	}
    mNumAbstractMethods = numMethods;
}


long
ForthInterface::AddMethod( long method )
{
    long methodIndex = mMethods.GetSize();
	mMethods.Add( method );
    if ( method == OP_BAD_OP )
    {
        mNumAbstractMethods++;
    }
    return methodIndex;
}


long
ForthInterface::GetNumMethods( void )
{
    return static_cast<long>( mMethods.GetSize() );
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
                                  forthNativeType   nativeType )
: mpName( pName )
, mNumBytes( numBytes )
, mNativeType( nativeType )
{
}

ForthBaseType::~ForthBaseType()
{
}

void
ForthBaseType::DefineInstance( ForthEngine *pEngine, void *pInitialVal )
{
    char *pToken = pEngine->GetNextSimpleToken();
    int nBytes = mNumBytes;
    char *pHere;
    int i;
    long val = 0;
    forthNativeType nativeType = mNativeType;
    ForthVocabulary *pVocab;
    long *pEntry;
    long typeCode, len, varOffset, storageLen;
    char *pStr;
    ForthCoreState *pCore = pEngine->GetCoreState();        // so we can SPOP maxbytes
    ForthStructsManager* pManager = ForthStructsManager::GetInstance();

    bool isString = (nativeType == kBaseTypeString);

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
    pEngine->SetArraySize( 0 );
    if ( isString )
    {
        typeCode = STRING_TYPE_TO_CODE( ((numElements) ? kDTArray : kDTSingle), len );
    }
    else
    {
        typeCode = NATIVE_TYPE_TO_CODE( ((numElements) ? kDTArray : kDTSingle), nativeType );
    }

    if ( pEngine->CheckFlag( kEngineFlagIsPointer ) )
    {
        // outside of a struct definition, any native variable or array defined with "ptrTo"
        //  is the same thing as an int variable or array, since native types have no fields
        pEngine->ClearFlag( kEngineFlagIsPointer );
        nativeType = kBaseTypeInt;
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
                pEngine->CompileOpcode( OP_DO_BYTE_ARRAY + nativeType );
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
                pEngine->CompileOpcode( OP_DO_BYTE + nativeType );
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


ForthBaseType *gpBaseTypes[] =
{
    &gBaseTypeByte,
    &gBaseTypeShort,
    &gBaseTypeInt,
    &gBaseTypeFloat,
    &gBaseTypeDouble,
    &gBaseTypeString,
    &gBaseTypeOp,
    &gBaseTypeObject
};
