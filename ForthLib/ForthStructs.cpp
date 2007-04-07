//////////////////////////////////////////////////////////////////////
//
// ForthStructs.cpp: support for user-defined structures
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthVocabulary.h"
#include "ForthEngine.h"
#include "ForthShell.h"
#include "ForthForgettable.h"

//############################################################################
//
//   vocabulary/symbol manipulation
//
//############################################################################


// symbol entry layout:
// offset   contents
//  0..3    4-byte symbol value - high byte is symbol type - usually this is opcode for symbol
//  ...     0 or more extra symbol value fields
//  N       1-byte symbol length (not including padding)
//  N+1     symbol characters
//
//  The total symbol entry length is always a multiple of 4, padded with 0s
//  if necessary.  This allows faster dictionary searches.

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
        mpStructInfo = (ForthStructInfo *) realloc( mpStructInfo, sizeof(ForthStructInfo) * mMaxStructs );
    }
    ForthStructInfo *pInfo = &(mpStructInfo[mNumStructs]);

    long *pEntry = pEngine->StartOpDefinition( pName, true );
    pInfo->pVocab = new ForthStructVocabulary( pName, mNumStructs );
    pInfo->op = *pEntry;

    mNumStructs++;
//    pInfo->pVocab = new ForthStructVocabulary( pengin
    return pInfo->pVocab;
}

ForthStructInfo*
ForthStructsManager::GetStructInfo( int structIndex )
{
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
    // TBD: handle alignment
    // TBD: handle strings
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
}

void
ForthStructVocabulary::AddField( const char* pName, long fieldType, int numElements )
{
    // a struct vocab entry has the following value fields:
    // - field offset in bytes
    // - field type
    // - element count (valid only for array fields)

    long *pEntry = AddSymbol( pName, (forthOpType) 0, mNumBytes, false );
    pEntry[1] = fieldType;
    pEntry[2] = numElements;
    long fieldBytes, alignment;
    ForthStructsManager::GetFieldInfo( fieldType, fieldBytes, alignment );
    // TBD: handle alignment
    if ( numElements != 0 )
    {
        fieldBytes *= numElements;
    }
    mNumBytes += fieldBytes;
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
    long numElements = pEngine->GetArraySize();
    int val = 0;
    forthNativeType nativeType = mNativeType;
    ForthVocabulary *pVocab;
    long *pEntry;

    if ( pEngine->GetCompileFlags() & kFECompileFlagIsPointer )
    {
        // outside of a struct definition, any variable or array defined with "ptrTo"
        //  is the same thing as an int variable or array
        pEngine->ClearCompileFlag( kFECompileFlagIsPointer );
        nativeType = kNativeInt;
        nBytes = 4;
        pInitialVal = &val;
    }

    // get next symbol, add it to vocabulary with type "user op"
    if ( pEngine->IsCompiling() )
    {
        // define local variable
        pVocab = pEngine->GetLocalVocabulary();
        if ( numElements )
        {
            pEngine->AddLocalArray( pToken, nativeType, nBytes );
        }
        else
        {
            pEngine->AddLocalVar( pToken, nativeType, nBytes );
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
            pEngine->SetArraySize( 0 );
            for ( i = 0; i < numElements; i++ )
            {
                memcpy( pHere, pInitialVal, nBytes );
                pHere += nBytes;
            }
            // TBD: support strings...
            pEntry[1] = NATIVE_TYPE_TO_CODE( kDTNativeArray, nativeType );
        }
        else
        {
            pEngine->CompileLong( OP_DO_BYTE + nativeType );
            pHere = (char *) (pEngine->GetDP());
            pEngine->AllotLongs( (nBytes  + 3) >> 2 );
            memcpy( pHere, pInitialVal, nBytes );
            // TBD: support strings...
            pEntry[1] = NATIVE_TYPE_TO_CODE( kDTNativeVariable, nativeType );
        }
    }
}


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
    long numElements = pEngine->GetArraySize();
    ForthCoreState *pCore = pEngine->GetCoreState();

    // TBD: set type info in vocab entry[1]

    if ( pEngine->IsCompiling() )
    {
        // the symbol just before "string" should have been an integer constant
        if ( pEngine->GetLastConstant( len ) )
        {
            // uncompile the integer contant opcode
            pEngine->UncompileLastOpcode();
            storageLen = ((len >> 2) + 3) << 2;

            if ( numElements )
            {
                varOffset = pEngine->AddLocalArray( pToken, kNativeString, storageLen );
                pEngine->CompileLong( COMPILED_OP( kOpConstant, numElements ) );
                pEngine->CompileLong( COMPILED_OP( kOpConstant, len ) );
                pEngine->CompileLong( COMPILED_OP( kOpLocalRef, varOffset - 2) );
                pEngine->CompileLong( OP_INIT_STRING_ARRAY );
            }
            else
            {
                // define local variable
                varOffset = pEngine->AddLocalVar( pToken, kNativeString, storageLen );
                // compile initLocalString op
                varOffset = (varOffset << 12) | len;
                pEngine->CompileLong( COMPILED_OP( kOpInitLocalString, varOffset ) );
            }
        }
        else
        {
            SET_ERROR( kForthErrorMissingSize );
        }
    }
    else
    {
        len = SPOP;
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
    }
}

//////////////////////////////////////////////////////////////////////
////
///     globals
//
//

ForthNativeType gNativeByte( "byte", 1, kNativeByte );

ForthNativeType gNativeShort( "short", 2, kNativeShort );

ForthNativeType gNativeInt( "int", 1, kNativeInt );

ForthNativeType gNativeFloat( "float", 4, kNativeFloat );

ForthNativeType gNativeDouble( "double", 8, kNativeDouble );

ForthNativeStringType gNativeString( "string", 12, kNativeString );

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

