//////////////////////////////////////////////////////////////////////
//
// ForthStructs.cpp: support for user-defined structures
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthShell.h"
#include "ForthForgettable.h"
#include "ForthBuiltinClasses.h"
#include "ForthParseInfo.h"

// symbol entry layout for struct vocabulary (fields and method symbols
// offset   contents
//  0..3    field offset or method index
//  4..7    field data type or method return type
//  8..12   element size for arrays
//  13      1-byte symbol length (not including padding)
//  14..    symbol characters
//
// see forth.h for most current description of struct symbol entry fields

#define STRUCTS_EXPANSION_INCREMENT     16

ForthNativeType gNativeTypeByte( "byte", 1, kBaseTypeByte );

ForthNativeType gNativeTypeUByte( "ubyte", 1, kBaseTypeUByte );

ForthNativeType gNativeTypeShort( "short", 2, kBaseTypeShort );

ForthNativeType gNativeTypeUShort( "ushort", 2, kBaseTypeUShort );

ForthNativeType gNativeTypeInt( "int", 4, kBaseTypeInt );

ForthNativeType gNativeTypeUInt( "uint", 4, kBaseTypeUInt );

ForthNativeType gNativeTypeLong( "long", 8, kBaseTypeLong );

ForthNativeType gNativeTypeULong( "ulong", 8, kBaseTypeULong );

ForthNativeType gNativeTypeFloat( "float", 4, kBaseTypeFloat );

ForthNativeType gNativeTypeDouble( "double", 8, kBaseTypeDouble );

ForthNativeType gNativeTypeString( "string", 12, kBaseTypeString );

ForthNativeType gNativeTypeOp( "op", 4, kBaseTypeOp );

ForthNativeType gNativeTypeObject( "object", 8, kBaseTypeObject );

ForthNativeType *gpNativeTypes[] =
{
    &gNativeTypeByte,
    &gNativeTypeUByte,
    &gNativeTypeShort,
    &gNativeTypeUShort,
    &gNativeTypeInt,
    &gNativeTypeUInt,
    &gNativeTypeLong,
    &gNativeTypeULong,
    &gNativeTypeFloat,
    &gNativeTypeDouble,
    &gNativeTypeString,
    &gNativeTypeOp,
    &gNativeTypeObject
};

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
	mpCodeGenerator = new ForthStructCodeGenerator( this );
}

ForthTypesManager::~ForthTypesManager()
{
	delete mpCodeGenerator;
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
            fieldBytes = gpNativeTypes[subType]->GetSize();
        }
        alignment = gpNativeTypes[subType]->GetAlignment();
    }
    else
    {
        ForthTypeInfo* pInfo = GetStructInfo( CODE_TO_STRUCT_INDEX( fieldType ) );
        if ( pInfo )
        {
            alignment = pInfo->pVocab->GetAlignment();
			fieldBytes = pInfo->pVocab->IsClass() ? sizeof(ForthObject) : pInfo->pVocab->GetSize();
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
    ForthEngine *pEngine = ForthEngine::GetInstance();
    ForthCoreState* pCore = pEngine->GetCoreState();
    ForthVocabulary *pFoundVocab = NULL;
    // ProcessSymbol will compile opcodes into temporary buffer mCode
    long *pDst = &(mCode[0]);

	bool result = mpCodeGenerator->Generate( pInfo, pDst, MAX_ACCESSOR_LONGS );
	if ( result )
	{
		// when done, either compile (copy) or execute code in mCode buffer
		if ( pEngine->IsCompiling() )
		{
			int nLongs = pDst - &(mCode[0]);
			if ( mpCodeGenerator->UncompileLastOpcode() )
			{
				pEngine->UncompileLastOpcode();
			}
			for ( int i = 0; i < nLongs; i++ )
			{
				pEngine->CompileOpcode( mCode[i] );
			}
		}
		else
		{
			*pDst++ = gCompiledOps[ OP_DONE ];
			exitStatus = pEngine->ExecuteOps( &(mCode[0]) );
		}
	}
    return result;
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

    const char* pToken = pInfo->GetToken();
    bool isRef = false;
    long* pEntry = pVocab->FindSymbol( pToken );
    if ( (pEntry == NULL) && (pToken[0] == '&') && (pToken[1] != '\0') )
    {
        pEntry = pVocab->FindSymbol( pToken + 1);
        isRef = (pEntry != NULL);
    }
    if ( pEntry )
    {
        long offset = *pEntry;
        long typeCode = pEntry[1];
        long opType;
        bool isNative = CODE_IS_NATIVE( typeCode );
        bool isPtr = CODE_IS_PTR( typeCode );
        bool isArray = CODE_IS_ARRAY( typeCode );
        long baseType = CODE_TO_BASE_TYPE( typeCode );

		if ( CODE_IS_USER_DEFINITION( typeCode ) )
		{
			// this is a normal forthop, let outer interpreter process it
			return false;
		}
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
            if ( isRef )
            {
                opType = kOpMemberRef;
            }
            else
            {
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
                        opType = ((isArray) ? kOpMemberByteArray : kOpMemberByte) + CODE_TO_BASE_TYPE( typeCode );
                    }
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


ForthNativeType*
ForthTypesManager::GetNativeTypeFromName( const char* typeName )
{
    for ( int i = 0; i <= kBaseTypeObject; i++ )
    {
        if ( strcmp( gpNativeTypes[i]->GetName(), typeName ) == 0 )
        {
            return gpNativeTypes[i];
        }
    }
    return NULL;
}


forthBaseType
ForthTypesManager::GetBaseTypeFromName( const char* typeName )
{
	ForthNativeType* pNative = GetNativeTypeFromName( typeName );
	return (pNative != NULL) ? pNative->GetBaseType() : kBaseTypeUnknown;
}


long
ForthTypesManager::GetBaseTypeSizeFromName( const char* typeName )
{
    for ( int i = 0; i <= kBaseTypeObject; i++ )
    {
        if ( strcmp( gpNativeTypes[i]->GetName(), typeName ) == 0 )
        {
            return gpNativeTypes[i]->GetSize();
        }
    }
    return -1;
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
		if ( mpEngine->InStructDefinition() )
		{
			pManager->GetNewestStruct()->AddField( pToken, typeCode, numElements );
			return;
		}

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
                mpEngine->CompileBuiltinOpcode( OP_DO_INT_ARRAY );
            }
            else
            {
                mpEngine->CompileBuiltinOpcode( OP_DO_STRUCT_ARRAY );
                mpEngine->CompileLong( nBytes + padding );
            }
            pHere = (char *) (mpEngine->GetDP());
            mpEngine->AllotLongs( (((nBytes + padding) * (numElements - 1)) + nBytes + 3) >> 2 );
            memset( pHere, 0, ((nBytes * numElements) + 3) );
        }
        else
        {
            mpEngine->CompileBuiltinOpcode( isPtr ? OP_DO_INT : OP_DO_STRUCT );
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
    // return size of struct
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
        sprintf( buff2, "%s", gpNativeTypes[baseType]->GetName() );
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

bool
ForthStructVocabulary::IsStruct()
{
	return true;
}

const char *
ForthStructVocabulary::GetTypeName( void )
{
    return "structVocabulary";
}

void
ForthStructVocabulary::ShowData(const void* pData)
{
	char buffer[32];
	const char* pStruct = (const char*)pData;
	ForthStructVocabulary* pVocab = this;
	bool notFirstTime = false;
	mpEngine->ConsoleOut("{ ");
	mpEngine->ConsoleOut("'__type' : '");
	mpEngine->ConsoleOut( GetName() );
	mpEngine->ConsoleOut("', ");
	bool foundSomething;

	while (pVocab != NULL)
	{
		long* pEntry = pVocab->GetNewestEntry();
		long* pEntriesEnd = pVocab->GetEntriesEnd();
		while (pEntry < pEntriesEnd)
		{
			long typeCode = VOCABENTRY_TO_TYPECODE(pEntry);
			long byteOffset = VOCABENTRY_TO_FIELD_OFFSET(pEntry);
			long numElements = VOCABENTRY_TO_NUM_ELEMENTS(pEntry);

			if (!CODE_IS_METHOD(typeCode))
			{
				bool isNative = CODE_IS_NATIVE(typeCode);
				bool isPtr = CODE_IS_PTR(typeCode);
				bool isArray = CODE_IS_ARRAY(typeCode);
				long baseType = CODE_TO_BASE_TYPE(typeCode);
				int sval;
				unsigned int uval;

				if (notFirstTime && foundSomething)
				{
					mpEngine->ConsoleOut(", ");
				}

				//mpEngine->ConsoleOut("\"");
				mpEngine->ConsoleOut("'");
				pVocab->GetEntryName(pEntry, buffer, sizeof(buffer));
				mpEngine->ConsoleOut(buffer);
				//mpEngine->ConsoleOut("\" : ");
				mpEngine->ConsoleOut("' : ");

				// mark buffer as empty by default
				buffer[0] = 0;

				foundSomething = true;
				int numElements = 1;
				if (isArray)
				{
					mpEngine->ConsoleOut("[ ");
				}
				else
				{
					numElements = 1;
				}
				if (isPtr)
				{
					// hack to print all pointers in hex
					baseType = kBaseTypeOp;
				}
				switch (baseType)
				{
				case kBaseTypeByte:
					sval = *((const char*)(pStruct + byteOffset));
					sprintf(buffer, "%d", sval);
					break;

				case kBaseTypeUByte:
					uval = *((const unsigned char*)(pStruct + byteOffset));
					sprintf(buffer, "%u", uval);
					break;

				case kBaseTypeShort:
					sval = *((const short*)(pStruct + byteOffset));
					sprintf(buffer, "%d", sval);
					break;

				case kBaseTypeUShort:
					uval = *((const unsigned short*)(pStruct + byteOffset));
					sprintf(buffer, "%u", uval);
					break;

				case kBaseTypeInt:
					sval = *((const int*)(pStruct + byteOffset));
					sprintf(buffer, "%d", sval);
					break;

				case kBaseTypeUInt:
					uval = *((const unsigned int*)(pStruct + byteOffset));
					sprintf(buffer, "%u", uval);
					break;

				case kBaseTypeLong:
					sprintf(buffer, "%lld", *((const long long*)(pStruct + byteOffset)));
					break;

				case kBaseTypeULong:
					sprintf(buffer, "%llu", *((const unsigned long long*)(pStruct + byteOffset)));
					break;

				case kBaseTypeFloat:
					sprintf(buffer, "%f", *((const float*)(pStruct + byteOffset)));
					break;

				case kBaseTypeDouble:
					sprintf(buffer, "%f", *((const double*)(pStruct + byteOffset)));
					break;

				case kBaseTypeString:
					mpEngine->ConsoleOut("'");
					mpEngine->ConsoleOut(pStruct + byteOffset + 8);
					mpEngine->ConsoleOut("'");
					break;

				case kBaseTypeOp:
					sprintf(buffer, "0x%x", pEntry[0]);
					break;

					/*
					kBaseTypeObject = kNumNativeTypes,      // 12 - object
					kBaseTypeStruct,                        // 13 - struct
					kBaseTypeUserDefinition,                // 14 - user defined forthop
					kBaseTypeVoid,							// 15 - void
					*/
				default:
					foundSomething = false;
					break;
				}
				if (foundSomething)
				{
					notFirstTime = true;
				}
				// if something was put in the buffer, print it
				if (buffer[0])
				{
					mpEngine->ConsoleOut(buffer);
				}
				if (isArray)
				{
					mpEngine->ConsoleOut(" ]");
				}
			}
			pEntry = NextEntry(pEntry);
		}

		pVocab = pVocab->BaseVocabulary();
	}
	mpEngine->ConsoleOut(" }");

}


//////////////////////////////////////////////////////////////////////
////
///     ForthClassVocabulary
//
//
ForthClassVocabulary* ForthClassVocabulary::smpObjectClass = NULL;

ForthClassVocabulary::ForthClassVocabulary( const char*     pName,
                                            int             typeIndex )
: ForthStructVocabulary( pName, typeIndex )
, mpParentClass( NULL )
, mCurrentInterface( 0 )
{
    mpClassObject = new ForthClassObject;
    mpClassObject->pVocab = this;
    mpClassObject->newOp = gCompiledOps[OP_ALLOC_OBJECT];
    ForthInterface* pPrimaryInterface = new ForthInterface( this );
    mInterfaces.push_back( pPrimaryInterface );

	if ( strcmp( pName, "object" ) == 0 )
	{
		smpObjectClass = this;
	}
	else
	{
		if ( strcmp( pName, "class" ) != 0 )
		{
			Extends( smpObjectClass );
		}
	}
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
                mpEngine->CompileOpcode( (isPtr ? kOpLocalInt : kOpLocalObject), pEntry[0] );
            }
        }
    }
    else
    {
		if ( mpEngine->CheckFlag( kEngineFlagInStructDefinition ) )
		{
			pManager->GetNewestStruct()->AddField( pToken, typeCode, numElements );
			return;
		}

        // define global object(s)
        long newGlobalOp = mpEngine->AddUserOp( pToken );

		// create object which will release object referenced by this global when it is forgotten
		new ForthForgettableGlobalObject( pToken, mpEngine->GetDP(), newGlobalOp, isArray ? numElements : 1 );

        pEntry = mpEngine->GetDefinitionVocabulary()->GetNewestEntry();
        if ( isArray )
        {
            mpEngine->CompileBuiltinOpcode( isPtr ? OP_DO_INT_ARRAY : OP_DO_OBJECT_ARRAY );
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

            mpEngine->CompileBuiltinOpcode( isPtr ? OP_DO_INT : OP_DO_OBJECT );
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
					// bump objects refcount
                    *pHere++ = SPOP;
					long* pData = (long *) SPOP;
					if ( pData != NULL )
					{
						*pData += 1;
					}
                    *pHere = (long) pData;
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
								 long			methodIndex,
                                 long           op )
{
	ForthInterface* pCurInterface = mInterfaces[ mCurrentInterface ];
	// see if method name is already defined - if so, just overwrite the method longword with op
	// if name is not already defined, add the method name and op
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

long
ForthClassVocabulary::FindMethod( const char* pName )
{
	ForthInterface* pCurInterface = mInterfaces[ mCurrentInterface ];
	// see if method name is already defined - if so, just overwrite the method longword with op
	// if name is not already defined, add the method name and op
    return pCurInterface->GetMethodIndex( pName );
}


void
ForthClassVocabulary::Extends( ForthClassVocabulary *pParentClass )
{
	if ( pParentClass->IsClass() )
	{
		long numInterfaces = pParentClass->GetNumInterfaces();
		for ( int i = 1; i < numInterfaces; i++ )
		{
			delete mInterfaces[i];
			mInterfaces[i] = NULL;
		}
		mpParentClass = pParentClass;
		numInterfaces = mpParentClass->GetNumInterfaces();
		mInterfaces.resize( numInterfaces );
		bool isPrimaryInterface = true;
		for ( int i = 0; i < numInterfaces; i++ )
		{
			if ( !isPrimaryInterface )
			{
				mInterfaces[i] = new ForthInterface;
			}
			mInterfaces[i]->Copy( mpParentClass->GetInterface( i ), isPrimaryInterface );
		}
	}

	ForthStructVocabulary::Extends( pParentClass );
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
        sprintf( buff, "%s ", gpNativeTypes[baseType]->GetName() );
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

ForthClassVocabulary*
ForthClassVocabulary::ParentClass( void )
{
	return mpSearchNext->IsClass() ? (ForthClassVocabulary *) mpSearchNext : NULL;
}

const char *
ForthClassVocabulary::GetTypeName( void )
{
    return "classVocabulary";
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
ForthInterface::Copy( ForthInterface* pInterface, bool isPrimaryInterface )
{
	if ( !isPrimaryInterface )
	{
	    mpDefiningClass = pInterface->GetDefiningClass();
	}
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
	return &(mMethods[0]) + 1;
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
		if ( method != gCompiledOps[OP_BAD_OP] )
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
		mMethods[i] = gCompiledOps[OP_BAD_OP];	// TBD: make this "unimplemented method" opcode
	}
    mNumAbstractMethods = numMethods;
}


long
ForthInterface::AddMethod( long method )
{
    long methodIndex = mMethods.size() - 1;
	mMethods.push_back( method );
    if ( method == gCompiledOps[OP_BAD_OP] )
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
	long* pEntry = NULL;
	bool done = false;
	long methodIndex = -1;
	ForthStructVocabulary* pVocab = mpDefiningClass;

	while ( !done )
	{
		pEntry = pVocab->FindNextSymbol( pName, pEntry );
		if ( pEntry )
		{
			long typeCode = pEntry[1];
			if ( CODE_IS_METHOD( typeCode ) )
			{
				methodIndex = pEntry[0];
				done = true;
			}
		}
		else
		{
			pVocab = pVocab->BaseVocabulary();
			if ( (pVocab == NULL) || !pVocab->IsClass() )
			{
				done = true;
			}
		}
	}
	return methodIndex;
}


//////////////////////////////////////////////////////////////////////
////
///     ForthNativeType
//
//

ForthNativeType::ForthNativeType( const char*       pName,
                                  int               numBytes,
                                  forthBaseType   baseType )
: mpName( pName )
, mNumBytes( numBytes )
, mBaseType( baseType )
{
}

ForthNativeType::~ForthNativeType()
{
}

void
ForthNativeType::DefineInstance( ForthEngine *pEngine, void *pInitialVal, long flags )
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

    if ( pEngine->InStructDefinition() && !pEngine->IsCompiling() )
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
                pEngine->CompileBuiltinOpcode( OP_DO_BYTE_ARRAY + CODE_TO_BASE_TYPE( typeCode ) );
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
                pEngine->CompileBuiltinOpcode( OP_DO_BYTE + CODE_TO_BASE_TYPE( typeCode ) );
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
                pEngine->CompileOpcode( kOpConstant, numElements );
                pEngine->CompileOpcode( kOpConstant, len );
                pEngine->CompileOpcode( kOpLocalRef, varOffset - 2);
                pEngine->CompileBuiltinOpcode( OP_INIT_STRING_ARRAY );
            }
            else
            {
                // define local string variable
                pHere = (char *) (pEngine->GetDP());
                varOffset = pEngine->AddLocalVar( pToken, typeCode, storageLen );
                // compile initLocalString op
                varOffset = (varOffset << 12) | len;
                // NOTE: do not use CompileOpcode here - it would screw up the OP_INTO check just below
                pEngine->CompileOpcode( kOpLocalStringInit, varOffset );
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
                pEngine->CompileBuiltinOpcode( OP_DO_STRING_ARRAY );
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
                pEngine->CompileBuiltinOpcode( OP_DO_STRING );
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


