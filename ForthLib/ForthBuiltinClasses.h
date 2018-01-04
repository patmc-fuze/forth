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
    kBCIContainedType,
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
    kBCIStructArray,
    kBCIStructArrayIter,
    kBCIInt,
	kBCILong,
	kBCIFloat,
	kBCIDouble,
	kBCIAsyncThread,
	kBCIThread,
	kBCIAsyncLock,
	kBCILock,
    kBCIAsyncSemaphore,
    kBCISemaphore,
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
	kBCITraceOutStream,
    kBCIBlockFile,
    kBCIBag,
    kBCIBagIter,
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

#define SHOW_OBJ_HEADER  pShowContext->ShowHeader(pCore, ((ForthClassObject *)(*((GET_TPM)-1)))->pVocab->GetName(), GET_TPD)
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
	kInStreamGetStringMethod = kNumBaseMethods + 3,
	kInStreamAtEOFMethod = kNumBaseMethods + 4
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

#define DEFAULT_STRING_DATA_BYTES 32

struct oString
{
	long		maxLen;
	long		curLen;
	char		data[DEFAULT_STRING_DATA_BYTES];
};

struct oStringStruct
{
	ulong		refCount;
	ulong		hash;
	oString*	str;
};

typedef std::map<long long, ForthObject> oLongMap;
struct oLongMapStruct
{
    ulong       refCount;
    oLongMap*	elements;
};

struct oLongMapIterStruct
{
    ulong				refCount;
    ForthObject			parent;
    oLongMap::iterator*	cursor;
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
