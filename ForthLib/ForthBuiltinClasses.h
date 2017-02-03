#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.h: builtin classes
//
//////////////////////////////////////////////////////////////////////

#include <vector>

#include "ForthForgettable.h"
#include "ForthObject.h"

class ForthClassVocabulary;

// structtype indices for builtin classes
typedef enum
{
	kBCIInvalid,
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
	kBCIIntMap,
	kBCIIntMapIter,
	kBCIFloatMap,
	kBCIFloatMapIter,
	kBCILongMap,
	kBCILongMapIter,
	kBCIDoubleMap,
	kBCIDoubleMapIter,
	kBCIStringIntMap,
	kBCIStringIntMapIter,
	kBCIStringFloatMap,
	kBCIStringFloatMapIter,
	kBCIStringLongMap,
	kBCIStringLongMapIter,
	kBCIStringDoubleMap,
	kBCIStringDoubleMapIter,
	kBCIString,
	kBCIStringMap,
	kBCIStringMapIter,
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
	kBCIFloatArray,
	kBCIFloatArrayIter,
	kBCIDoubleArray,
	kBCIDoubleArrayIter,
	kBCILongArray,
	kBCILongArrayIter,
	kBCIInt,
	kBCILong,
	kBCIFloat,
	kBCIDouble,
	kBCIThread,
	kBCISystem,
	kBCIVocabulary,
	kBCIVocabularyIter,
	kBCIInStream,
	kBCIFileInStream,
	kBCIConsoleInStream,
	kBCIOutStream,
	kBCIFileOutStream,
	kBCIStringOutStream,
	kBCIConsoleOutStream,
	kBCIFunctionOutStream,
	kNumBuiltinClasses		// must be last
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
// ForthShowAlreadyShownObject returns true if object was already shown (or null), does display for those cases
bool ForthShowAlreadyShownObject(ForthObject* obj, ForthCoreState* pCore, bool addIfUnshown);
void ForthShowObject(ForthObject& obj, ForthCoreState* pCore);

#define EXIT_IF_OBJECT_ALREADY_SHOWN if (ForthShowAlreadyShownObject(GET_THIS_PTR, pCore, true)) { METHOD_RETURN; return; }

void unrefObject(ForthObject& fobj);

#define GET_BUILTIN_INTERFACE(BCI_INDEX, INTERFACE_INDEX) ForthTypesManager::GetInstance()->GetClassVocabulary(BCI_INDEX)->GetInterface(INTERFACE_INDEX)

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
	void*               pUserData;
	OutStreamFuncs*     pOutFuncs;
	char				eolChars[4];
};

enum
{
	kOutStreamPutCharMethod = kNumBaseMethods,
	kOutStreamPutBytesMethod = kNumBaseMethods + 1,
	kOutStreamPutStringMethod = kNumBaseMethods + 2,
	kInStreamGetCharMethod = kNumBaseMethods,
	kInStreamGetBytesMethod = kNumBaseMethods + 1,
	kInStreamGetLineMethod = kNumBaseMethods + 2,
	kInStreamAtEOFMethod = kNumBaseMethods + 3
};

struct oInStreamStruct
{
	ulong               refCount;
	void*               pUserData;
	int					bTrimEOL;
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
