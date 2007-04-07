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

// symbol entry layout:
// offset   contents
//  0..3    4-byte symbol value - high byte is symbol type - usually this is opcode for symbol
//  ...     0 or more extra symbol value fields
//  N       1-byte symbol length (not including padding)
//  N+1     symbol characters
//
//  The total symbol entry length is always a multiple of 4, padded with 0s
//  if necessary.  This allows faster dictionary searches.

extern ForthNativeType *gpNativeTypes[];
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
{
    ASSERT( mpInstance == NULL );
    mpInstance = this;
}

ForthStructsManager::~ForthStructsManager()
{
    delete mpStructInfo;
}

void
ForthStructsManager::ForgetCleanup( void *pForgetLimit, long op )
{
    // TBD: remove struct info for forgotten struct types
    int i;

    for ( i = mNumStructs - 1; i >= 0; i-- )
    {
        if ( mpStructInfo[i].op >= op )
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
ForthStructsManager::AddStructType( const char *pName )
{
    // TBD
    ForthEngine *pEngine = ForthEngine::GetInstance();
    ForthVocabulary* pDefinitionsVocab = pEngine->GetDefinitionVocabulary();

    if ( mNumStructs == mMaxStructs )
    {
        mMaxStructs += STRUCTS_EXPANSION_INCREMENT;
        SPEW_STRUCTS( "AddStructType expanding structs table to %d entries\n", mMaxStructs );
        mpStructInfo = (ForthStructInfo *) realloc( mpStructInfo, sizeof(ForthStructInfo) * mMaxStructs );
    }
    ForthStructInfo *pInfo = &(mpStructInfo[mNumStructs]);

    long *pEntry = pEngine->StartOpDefinition( pName, true );
    pInfo->pVocab = new ForthStructVocabulary( pName, mNumStructs );
    pInfo->op = *pEntry;
    SPEW_STRUCTS( "AddStructType %s struct index %d\n", pName, mNumStructs );
    mNumStructs++;
    return pInfo->pVocab;
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
        if ( subType == kNativeString )
        {
            // add in for maxLen, curLen fields & terminating null byte
            fieldBytes = 9 + CODE_TO_STRING_BYTES( fieldType );
        }
        else
        {
            fieldBytes = gpNativeTypes[subType]->GetSize();
        }
        alignment = gpNativeTypes[subType]->GetAlignment();
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
    long *pEntry = pEngine->GetSearchVocabulary()->FindSymbol( mToken );
    if ( pEntry == NULL )
    {
        pEntry = pEngine->GetLocalVocabulary()->FindSymbol( mToken );
    }
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
        *pDst++ = pEntry[0];
        if ( CODE_IS_PTR( typeCode ) )
        {
            SPEW_STRUCTS( "Pointer field fetch\n" );
            *pDst++ = BUILTIN_OP( OP_FETCH );
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
    long offset = 0;

    ///////////////////////////////////
    //
    //  process tokens after first
    //

    // TBD: process each accessor token, compiling code in mCode
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
        if ( isFinal )
        {
            //
            // this is final accessor field
            //
            SPEW_STRUCTS( " FINAL" );
            if ( isPtr )
            {
                SPEW_STRUCTS( (isArray) ? " array of pointers\n" : " pointer\n" );
                opType = (isArray) ? kOpFieldInt : kOpFieldIntArray;
            }
            else
            {
                if ( isNative )
                {
                    opType = ((isArray) ? kOpFieldByteArray : kOpFieldByte) + CODE_TO_NATIVE_TYPE( typeCode );
                }
                else
                {
                    if ( isArray )
                    {
                        // TBD: we need a "field struct array" op...
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
            if ( isNative )
            {
                // ERROR! a native field must be a final accessor
                char blah[256];
                sprintf( blah, "Native %s used for non-final accessor", pToken );
                pEngine->SetError( kForthErrorStruct, blah );
                return false;
            }
            // struct: do nothing (offset already added in
            // ptr to struct: compile offset, compile at
            // array of structs: compile offset, compile array offset
            // array of ptrs to structs: compile offset, compile array offset, compile at
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

//////////////////////////////////////////////////////////////////////
////
///     ForthStructVocabulary
//
//

ForthStructVocabulary::ForthStructVocabulary( const char    *pName,
                                              int           structIndex )
: ForthVocabulary( pName, NUM_STRUCT_VOCAB_VALUE_LONGS, DEFAULT_VOCAB_STORAGE )
, mStructIndex( structIndex )
, mAlignment( 4 )
, mNumBytes( 0 )
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
    // TBD: do one of the following:
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
    ForthStructsManager* pManager = ForthStructsManager::GetInstance();

    long numElements = mpEngine->GetArraySize();
    mpEngine->SetArraySize( 0 );
    typeCode = STRUCT_TYPE_TO_CODE( ((numElements) ? kDTArray : kDTSingle), mStructIndex );

    if ( mpEngine->GetCompileFlags() & kFECompileFlagIsPointer )
    {
        mpEngine->ClearCompileFlag( kFECompileFlagIsPointer );
        nBytes = 4;
        typeCode |= kDTIsPtr;
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
            mpEngine->AddLocalVar( pToken, typeCode, nBytes );
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
            mpEngine->CompileLong( OP_DO_STRUCT_ARRAY );
            mpEngine->CompileLong( nBytes + padding );
            pHere = (char *) (mpEngine->GetDP());
            mpEngine->AllotLongs( (((nBytes + padding) * (numElements - 1)) + nBytes + 3) >> 2 );
            memset( pHere, 0, ((nBytes * numElements) + 3) );
        }
        else
        {
            mpEngine->CompileLong( OP_DO_STRUCT );
            pHere = (char *) (mpEngine->GetDP());
            mpEngine->AllotLongs( (nBytes  + 3) >> 2 );
            memset( pHere, 0, nBytes );
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

    if ( mNumSymbols == 0 )
    {
        // this is first field in struct, so this field defines structs' alignment
        mAlignment = alignment;
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
    return mNumBytes;
}

//////////////////////////////////////////////////////////////////////
////
///     ForthNativeType
//
//

ForthNativeType::ForthNativeType( const char*       pName,
                                  int               numBytes,
                                  forthNativeType   nativeType )
: mpName( pName )
, mNumBytes( numBytes )
, mNativeType( nativeType )
{
}

ForthNativeType::~ForthNativeType()
{
}

void
ForthNativeType::DefineInstance( ForthEngine *pEngine, void *pInitialVal )
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

    bool isString = (nativeType == kNativeString);

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

    if ( pEngine->GetCompileFlags() & kFECompileFlagIsPointer )
    {
        // outside of a struct definition, any native variable or array defined with "ptrTo"
        //  is the same thing as an int variable or array, since native types have no fields
        pEngine->ClearCompileFlag( kFECompileFlagIsPointer );
        nativeType = kNativeInt;
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
                pEngine->AddLocalVar( pToken, typeCode, nBytes );
            }
        }
        else
        {
            // define global variable
            pEngine->AddUserOp( pToken );
            pEntry = pEngine->GetDefinitionVocabulary()->GetNewestEntry();
            if ( numElements )
            {
                pEngine->CompileLong( OP_DO_BYTE_ARRAY + nativeType );
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
                pEngine->CompileLong( OP_DO_BYTE + nativeType );
                pHere = (char *) (pEngine->GetDP());
                pEngine->AllotLongs( (nBytes  + 3) >> 2 );
                memcpy( pHere, pInitialVal, nBytes );
            }
            pEntry[1] = typeCode;
        }
    }
    else
    {
        // instance is string
        if ( pEngine->IsCompiling() )
        {
            // uncompile the integer contant opcode
            pEngine->UncompileLastOpcode();
            storageLen = ((len >> 2) + 3) << 2;

            if ( numElements )
            {
                varOffset = pEngine->AddLocalArray( pToken, typeCode, storageLen );
                pEngine->CompileLong( COMPILED_OP( kOpConstant, numElements ) );
                pEngine->CompileLong( COMPILED_OP( kOpConstant, len ) );
                pEngine->CompileLong( COMPILED_OP( kOpLocalRef, varOffset - 2) );
                pEngine->CompileLong( OP_INIT_STRING_ARRAY );
            }
            else
            {
                // define local variable
                varOffset = pEngine->AddLocalVar( pToken, typeCode, storageLen );
                // compile initLocalString op
                varOffset = (varOffset << 12) | len;
                pEngine->CompileLong( COMPILED_OP( kOpInitLocalString, varOffset ) );
            }
        }
        else
        {
            pEngine->AddUserOp( pToken );
            pEntry = pEngine->GetDefinitionVocabulary()->GetNewestEntry();
            if ( numElements )
            {
                // define global array
                pEngine->CompileLong( OP_DO_STRING_ARRAY );
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
                // define global variable
                pEngine->CompileLong( OP_DO_STRING );
                pEngine->CompileLong( len );
                pEngine->CompileLong( 0 );
                pStr = (char *) (pEngine->GetDP());
                // a length of 4 means room for 4 chars plus a terminating null
                pEngine->AllotLongs( ((len  + 4) & ~3) >> 2 );
                *pStr = 0;
            }
            pEntry[1] = typeCode;
        }
    }
}


#if 0
//////////////////////////////////////////////////////////////////////
////
///     ForthNativeStringType
//
//

ForthNativeStringType::ForthNativeStringType( const char*       pName,
                                              int               numBytes,
                                              forthNativeType   nativeType )
: ForthNativeType( pName, numBytes, nativeType )
{
}

ForthNativeStringType::~ForthNativeStringType()
{
}

void
ForthNativeStringType::DefineInstance( ForthEngine *pEngine, void *pInitialVal )
{
    char *pToken = pEngine->GetNextSimpleToken();
    long len, varOffset, storageLen;
    char *pStr;
    int i;
    long val = 0;
    ForthCoreState *pCore = pEngine->GetCoreState();        // so we can SPOP maxbytes

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
    long numElements = pEngine->GetArraySize();
    pEngine->SetArraySize( 0 );
    long typeCode = STRING_TYPE_TO_CODE( ((numElements) ? kDTArray : kDTSingle), len );

    if ( pEngine->GetCompileFlags() & kFECompileFlagIsPointer )
    {
        // outside of a struct definition, any native variable or array defined with "ptrTo"
        //  is the same thing as an int variable or array, since native types have no fields
        pEngine->ClearCompileFlag( kFECompileFlagIsPointer );
        nativeType = kNativeInt;
        nBytes = 4;
        pInitialVal = &val;
        typeCode |= kDTIsPtr;
    }

    if ( pEngine->InStructDefinition() )
    {
        ForthStructsManager::GetNewestStruct()->AddField( pToken, typeCode, numElements );
        return;
    }

    if ( pEngine->IsCompiling() )
    {
        // uncompile the integer contant opcode
        pEngine->UncompileLastOpcode();
        storageLen = ((len >> 2) + 3) << 2;

        if ( numElements )
        {
            varOffset = pEngine->AddLocalArray( pToken, typeCode, storageLen );
            pEngine->CompileLong( COMPILED_OP( kOpConstant, numElements ) );
            pEngine->CompileLong( COMPILED_OP( kOpConstant, len ) );
            pEngine->CompileLong( COMPILED_OP( kOpLocalRef, varOffset - 2) );
            pEngine->CompileLong( OP_INIT_STRING_ARRAY );
        }
        else
        {
            // define local variable
            varOffset = pEngine->AddLocalVar( pToken, typeCode, storageLen );
            // compile initLocalString op
            varOffset = (varOffset << 12) | len;
            pEngine->CompileLong( COMPILED_OP( kOpInitLocalString, varOffset ) );
        }
    }
    else
    {
        pEngine->AddUserOp( pToken );
        if ( numElements )
        {
            // define global array
            pEngine->CompileLong( OP_DO_STRING_ARRAY );
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
            // define global variable
            pEngine->CompileLong( OP_DO_STRING );
            pEngine->CompileLong( len );
            pEngine->CompileLong( 0 );
            pStr = (char *) (pEngine->GetDP());
            // a length of 4 means room for 4 chars plus a terminating null
            pEngine->AllotLongs( ((len  + 4) & ~3) >> 2 );
            *pStr = 0;
        }
        pEntry[1] = typeCode;
    }
}
#endif

//////////////////////////////////////////////////////////////////////
////
///     globals
//
//

ForthNativeType gNativeByte( "byte", 1, kNativeByte );

ForthNativeType gNativeShort( "short", 2, kNativeShort );

ForthNativeType gNativeInt( "int", 4, kNativeInt );

ForthNativeType gNativeFloat( "float", 4, kNativeFloat );

ForthNativeType gNativeDouble( "double", 8, kNativeDouble );

ForthNativeType gNativeString( "string", 12, kNativeString );

ForthNativeType gNativeOp( "op", 4, kNativeOp );


ForthNativeType *gpNativeTypes[] =
{
    &gNativeByte,
    &gNativeShort,
    &gNativeInt,
    &gNativeFloat,
    &gNativeDouble,
    &gNativeString,
    &gNativeOp
};
