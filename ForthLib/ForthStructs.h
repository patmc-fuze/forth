//////////////////////////////////////////////////////////////////////
//
// ForthStructs.h: support for user-defined structures
//
//////////////////////////////////////////////////////////////////////

#if !defined(_FORTH_STRUCTS_H_INCLUDED_)
#define _FORTH_STRUCTS_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"
#include "ForthForgettable.h"

class ForthEngine;
class ForthStructVocabulary;

// each new structure type definition is assigned a unique index
// the struct type index is:
// - recorded in the type info field of vocabulary entries for global struct instances
// - recorded in the type info field of struct vocabulary entries for members of this type
// - used to get from a struct field to the struct vocabulary which defines its subfields

typedef struct
{
    ForthStructVocabulary*      pVocab;
    long                        op;
} ForthStructInfo;

// a struct accessor compound operation must be less than this length in longs
#define MAX_ACCESSOR_LONGS  64

class ForthStructsManager : public ForthForgettable
{
public:
    ForthStructsManager();
    ~ForthStructsManager();

    virtual void    ForgetCleanup( void *pForgetLimit, long op );

    // compile/interpret symbol if it is a valid structure accessor
    virtual bool    ProcessSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus );

    // add a new structure type
    ForthStructVocabulary*  AddStructType( const char *pName );
    static ForthStructsManager*     GetInstance( void );

    // return info structure for struct type specified by structIndex
    ForthStructInfo*        GetStructInfo( int structIndex );

    // return vocabulary for a struct type given its opcode
    ForthStructVocabulary*  GetStructVocabulary( long op );

    void GetFieldInfo( long fieldType, long& fieldBytes, long& alignment );

    ForthStructVocabulary*   GetNewestStruct( void );

protected:
    // mpStructInfo points to an array with an entry for each defined structure type
    ForthStructInfo                 *mpStructInfo;
    int                             mNumStructs;
    int                             mMaxStructs;
    static ForthStructsManager*     mpInstance;
    char                            mToken[ DEFAULT_INPUT_BUFFER_LEN ];
    long                            mCode[ MAX_ACCESSOR_LONGS ];
};

class ForthStructVocabulary : public ForthVocabulary
{
public:
    ForthStructVocabulary( const char* pName, int structIndex );
    virtual ~ForthStructVocabulary();

    // delete symbol entry and all newer entries
    // return true IFF symbol was forgotten
    virtual bool        ForgetSymbol( const char   *pSymName );

    // forget all ops with a greater op#
    virtual void        ForgetOp( long op );

    virtual const char* GetType( void );

    // handle invocation of a struct op - define a local/global struct or struct array, or define a field
    void                DefineInstance( void );

    void                AddField( const char* pName, long fieldType, int numElements );
    long                GetAlignment( void );
    long                GetSize( void );
    void                StartUnion( void );
    void                Extends( ForthStructVocabulary *pParentStruct );

protected:
    long                mNumBytes;
    long                mMaxNumBytes;
    long                mStructIndex;
    long                mAlignment;
};


class ForthNativeType
{
public:
    ForthNativeType( const char* pName, int numBytes, forthNativeType nativeType );
    virtual ~ForthNativeType();
    virtual void DefineInstance( ForthEngine *pEngine, void *pInitialVal );

    inline long GetGlobalOp( void ) { return mNativeType + OP_DO_BYTE; };
    inline long GetGlobalArrayOp( void ) { return mNativeType + OP_DO_BYTE_ARRAY; };
    inline long GetLocalOp( void ) { return mNativeType + kOpLocalByte; };
    inline long GetLocalArrayOp( void ) { return mNativeType + kOpLocalByteArray; };
    inline long GetFieldOp( void ) { return mNativeType + kOpFieldByte; };
    inline long GetFieldArrayOp( void ) { return mNativeType + kOpFieldByteArray; };
    inline long GetAlignment( void ) { return (mNumBytes > 4) ? 4 : mNumBytes; };
    inline long GetSize( void ) { return mNumBytes; };

protected:
    const char*         mpName;
    int                 mNumBytes;
    forthNativeType     mNativeType;
};

#if 0
class ForthNativeStringType : public ForthNativeType
{
public:
    ForthNativeStringType( const char* pName, int numBytes, forthNativeType nativeType );
    virtual ~ForthNativeStringType();
    virtual void DefineInstance( ForthEngine *pEngine, void *pInitialVal );
};
#endif

extern ForthNativeType gNativeByte, gNativeShort, gNativeInt, gNativeFloat, gNativeDouble, gNativeString, gNativeOp;

#endif
