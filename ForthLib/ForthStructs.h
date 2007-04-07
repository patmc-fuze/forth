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

class ForthStructsManager : public ForthForgettable
{
public:
    ForthStructsManager();
    ~ForthStructsManager();

    virtual void    ForgetCleanup( void *pForgetLimit, long op );

    // add a new structure type
    virtual ForthStructVocabulary*  AddStructType( const char *pName );
    static ForthStructsManager*     GetInstance( void );

    // return info structure for struct type specified by structIndex
    virtual ForthStructInfo*        GetStructInfo( int structIndex );

    static void GetFieldInfo( long fieldType, long& fieldBytes, long& alignment );

protected:
    // mpStructInfo points to an array with an entry for each defined structure type
    ForthStructInfo                 *mpStructInfo;
    int                             mNumStructs;
    int                             mMaxStructs;
    static ForthStructsManager*     mpInstance;
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

    // handle invocation of a struct op - define a local/global struct or struct array, or define a field
    void                DefineInstance( void );

    void                AddField( const char* pName, long fieldType, int numElements );

protected:
    int                 mNumBytes;
    int                 mStructIndex;
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

protected:
    const char*         mpName;
    int                 mNumBytes;
    forthNativeType     mNativeType;
};

class ForthNativeStringType : public ForthNativeType
{
public:
    ForthNativeStringType( const char* pName, int numBytes, forthNativeType nativeType );
    virtual ~ForthNativeStringType();
    virtual void DefineInstance( ForthEngine *pEngine, void *pInitialVal );
};

extern ForthNativeType gNativeByte, gNativeShort, gNativeInt, gNativeFloat, gNativeDouble, gNativeOp;
extern ForthNativeStringType gNativeString;

#endif
