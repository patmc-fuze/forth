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

#define SHOW_OBJ_HEADER(_TYPENAME)  pShowContext->ShowHeader(pCore, _TYPENAME, GET_TPD)
void ForthShowObject(ForthObject& obj, ForthCoreState* pCore);

void unrefObject(ForthObject& fobj);

extern ForthClassVocabulary* gpObjectClassVocab;
extern ForthClassVocabulary* gpClassClassVocab;
extern ForthClassVocabulary* gpOIterClassVocab;
extern ForthClassVocabulary* gpOIterableClassVocab;


// oOutStream is an abstract output stream class

struct OutStreamFuncs
{
	streamCharOutRoutine		outChar;
	streamBytesOutRoutine		outBytes;
	streamStringOutRoutine		outString;
};


struct oOutStreamStruct
{
	ulong               refCount;
	OutStreamFuncs*     pOutFuncs;
	void*               pUserData;
};

enum
{
	kOutStreamPutCharMethod = kNumBaseMethods,
	kOutStreamPutBytesMethod = kNumBaseMethods + 1,
	kOutStreamPutStringMethod = kNumBaseMethods + 2
};

typedef std::vector<ForthObject> oArray;
struct oArrayStruct
{
	ulong       refCount;
	oArray*    elements;
};

struct oListElement
{
	oListElement*	prev;
	oListElement*	next;
	ForthObject		obj;
};

struct oListStruct
{
	ulong			refCount;
	oListElement*	head;
	oListElement*	tail;
};

struct oArrayIterStruct
{
	ulong			refCount;
	ForthObject		parent;
	ulong			cursor;
};

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
