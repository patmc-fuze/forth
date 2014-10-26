#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.h: builtin classes
//
//////////////////////////////////////////////////////////////////////

#include "ForthForgettable.h"
#include "ForthObject.h"

// structtype indices for builtin classes
typedef enum {
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
} kBuiltinClassIndex;

class ForthForgettableGlobalObject : public ForthForgettable
{
public:
    ForthForgettableGlobalObject( void *pOpAddress, long op, int numElements = 1 );
    virtual ~ForthForgettableGlobalObject();

protected:
    virtual void    ForgetCleanup( void *pForgetLimit, long op );

	int		mNumElements;
};
