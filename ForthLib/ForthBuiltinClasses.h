#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.h: builtin classes
//
//////////////////////////////////////////////////////////////////////

#include "ForthForgettable.h"
#include "ForthObject.h"

// structtype indices for builtin classes
typedef enum
{
    kBCIObject,
    kBCIClass,
	kBCIIter,
	kBCIIterable,
	kBCIArray,
	kBCIArrayIter,
	kBCIList,
	kBCIListIter,
	kBCIMap,
	kBCIMapIter,
	kBCIString,
	kBCIPair,
	kBCIPairIter,
	kBCITriple,
	kBCITripleIter,
	kBCIByteArray,
	kBCIByteArrayIter,
	kBCIShortArray,
	kBCIShortArrayIter,
	kBCIIntArray,
	kBCIIntArrayIter,
	kBCILongArray,
	kBCILongArrayIter,
	kBCIInt,
	kBCILong,
	kBCIFloat,
	kBCIDouble,
	kBCIThread
} eBuiltinClassIndex;

typedef enum
{
	kOMDelete,
	kOMShow,
	kOMGetClass,
	kOMCompare
} eObjectMethod;
#define kObjectShowMethodIndex 1

void ForthShowObject(ForthObject& obj, ForthCoreState* pCore);

class ForthForgettableGlobalObject : public ForthForgettable
{
public:
    ForthForgettableGlobalObject( const char* pName, void *pOpAddress, long op, int numElements = 1 );
    virtual ~ForthForgettableGlobalObject();

    virtual const char* GetTypeName();
    virtual const char* GetName();
protected:
    char* mpName;
    virtual void    ForgetCleanup( void *pForgetLimit, long op );

	int		mNumElements;
};
