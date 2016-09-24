//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.cpp: builtin classes
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <stdio.h>
#include <string.h>
#include <map>

#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthObject.h"
#include "ForthBuiltinClasses.h"
#include "ForthShowContext.h"

#include "OArray.h"
#include "OList.h"
#include "OString.h"
#include "OMap.h"
#include "OStream.h"
#include "ONumber.h"

#ifdef TRACK_OBJECT_ALLOCATIONS
long gStatNews = 0;
long gStatDeletes = 0;
long gStatLinkNews = 0;
long gStatLinkDeletes = 0;
long gStatIterNews = 0;
long gStatIterDeletes = 0;
long gStatKeeps = 0;
long gStatReleases = 0;
#endif

// first time OString:printf fails due to overflow, it buffer is increased to this size
#define OSTRING_PRINTF_FIRST_OVERFLOW_SIZE 256
// this is size limit of buffer expansion upon OString:printf overflow
#define OSTRING_PRINTF_LAST_OVERFLOW_SIZE 0x2000000

extern "C" {
	unsigned long SuperFastHash (const char * data, int len, unsigned long hash);
	extern void unimplementedMethodOp( ForthCoreState *pCore );
	extern void illegalMethodOp( ForthCoreState *pCore );
	extern int oStringFormatSub( ForthCoreState* pCore, const char* pBuffer, int bufferSize );
};

#ifdef _WINDOWS
float __cdecl cdecl_boohoo(int aa, int bb, int cc)
{
	return (float)((aa + bb) * cc);
}

float __stdcall stdcall_boohoo(int aa, int bb, int cc)
{
	return (float)((aa + bb) * cc);
}

float boohoo(int aa, int bb, int cc)
{
	return (float)((aa + bb) * cc);
}
#endif

#ifdef LINUX
#define SNPRINTF snprintf
#else
#define SNPRINTF _snprintf
#endif

void* ForthAllocateBlock(size_t numBytes)
{
	void* pData = malloc(numBytes);
	ForthEngine::GetInstance()->TraceOut("malloc %d bytes @ 0x%p\n", numBytes, pData);
	return pData;
}

void* ForthReallocateBlock(void *pMemory, size_t numBytes)
{
	void* pData = realloc(pMemory, numBytes);
	ForthEngine::GetInstance()->TraceOut("realloc %d bytes @ 0x%p\n", numBytes, pData);
	return pData;
}

void ForthFreeBlock(void* pBlock)
{
	free(pBlock);
	ForthEngine::GetInstance()->TraceOut("free @ 0x%p\n", pBlock);
}

void unrefObject(ForthObject& fobj)
{
	ForthEngine *pEngine = ForthEngine::GetInstance();
	if (fobj.pData != NULL)
	{
		ulong refCount = *((ulong *)fobj.pData);
		if (refCount == 0)
		{
			pEngine->SetError(kForthErrorBadReferenceCount, " unref with refcount already zero");
		}
		else
		{
			*fobj.pData -= 1;
		}
	}
}

ForthClassVocabulary* gpObjectClassVocab;
ForthClassVocabulary* gpClassClassVocab;
ForthClassVocabulary* gpOIterClassVocab;
ForthClassVocabulary* gpOIterableClassVocab;

namespace
{

	//////////////////////////////////////////////////////////////////////
	///
	//                 object
	//
	FORTHOP(objectNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		long nBytes = pClassVocab->GetSize();
		long* pData = (long *)__MALLOC(nBytes);
		// clear the entire object area - this handles both its refcount and any object pointers it might contain
		memset(pData, 0, nBytes);
		TRACK_NEW;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pData);
	}

	FORTHOP(objectDeleteMethod)
	{
		FREE_OBJECT(GET_TPD);
		METHOD_RETURN;
	}

	FORTHOP(objectShowMethod)
	{
		ForthObject obj;
		obj.pData = GET_TPD;
		obj.pMethodOps = GET_TPM;
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		ForthClassObject* pClassObject = (ForthClassObject *)(*((GET_TPM)-1));
		ForthEngine *pEngine = ForthEngine::GetInstance();

		if (pShowContext->AddObject(obj))
		{
			pEngine->ConsoleOut("'@");
			pShowContext->ShowID(pClassObject->pVocab->GetName(), obj.pData);
			pEngine->ConsoleOut("'");
		}
		else
		{
			pClassObject->pVocab->ShowData(GET_TPD, pCore);
			if (pShowContext->GetDepth() == 0)
			{
				pEngine->ConsoleOut("\n");
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(objectClassMethod)
	{
		// this is the big gaping hole - where should the pointer to the class vocabulary be stored?
		// we could store it in the slot for method 0, but that would be kind of clunky - also,
		// would slot 0 of non-primary interfaces also have to hold it?
		// the class object is stored in the long before mehod 0
		PUSH_PAIR(ForthTypesManager::GetInstance()->GetClassMethods(), (GET_TPM)-1);
		METHOD_RETURN;
	}

	FORTHOP(objectCompareMethod)
	{
		long thisVal = (long)(GET_TPD);
		long thatVal = SPOP;
		long result = 0;
		if (thisVal != thatVal)
		{
			result = (thisVal > thatVal) ? 1 : -1;
		}
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(objectKeepMethod)
	{
		ulong* pRefCount = (ulong*)(GET_TPD);      // member at offset 0 is refcount
		(*pRefCount) += 1;
		TRACK_KEEP;
		METHOD_RETURN;
	}

	FORTHOP(objectReleaseMethod)
	{
		ulong* pRefCount = (ulong*)(GET_TPD);      // member at offset 0 is refcount
		(*pRefCount) -= 1;
		TRACK_RELEASE;
		if (*pRefCount != 0)
		{
			METHOD_RETURN;
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			ulong deleteOp = GET_TPM[kMethodDelete];
			pEngine->ExecuteOneOp(deleteOp);
			// we are effectively chaining to the delete op, its method return will pop TPM & TPD for us
		}
	}

	baseMethodEntry objectMembers[] =
	{
		METHOD("__newOp", objectNew),
		METHOD("delete", objectDeleteMethod),
		METHOD("show", objectShowMethod),
		METHOD_RET("getClass", objectClassMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIClass)),
		METHOD_RET("compare", objectCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("keep", objectKeepMethod),
		METHOD("release", objectReleaseMethod),
		MEMBER_VAR("__refCount", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 class
	//
	FORTHOP(classCreateMethod)
	{
		ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
		SPUSH((long)pClassObject->pVocab);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->ExecuteOneOp(pClassObject->newOp);
		METHOD_RETURN;
	}

	FORTHOP(classSuperMethod)
	{
		ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
		ForthClassVocabulary* pClassVocab = pClassObject->pVocab;
		// what should happen if a class is derived from a struct?
		PUSH_PAIR(GET_TPM, pClassVocab->BaseVocabulary());
		METHOD_RETURN;
	}

	FORTHOP(classNameMethod)
	{
		ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
		ForthClassVocabulary* pClassVocab = pClassObject->pVocab;
		const char* pName = pClassVocab->GetName();
		SPUSH((long)pName);
		METHOD_RETURN;
	}

	FORTHOP(classVocabularyMethod)
	{
		ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
		SPUSH((long)(pClassObject->pVocab));
		METHOD_RETURN;
	}

	FORTHOP(classGetInterfaceMethod)
	{
		// TOS is ptr to name of desired interface
		ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
		ForthClassVocabulary* pClassVocab = pClassObject->pVocab;
		const char* pName = (const char*)(SPOP);
		long* foundInterface = NULL;
		long numInterfaces = pClassVocab->GetNumInterfaces();
		for (int i = 0; i < numInterfaces; i++)
		{
			ForthInterface* pInterface = pClassVocab->GetInterface(i);
			if (strcmp(pInterface->GetDefiningClass()->GetName(), pName) == 0)
			{
				foundInterface = (long *)pInterface;
				break;
			}
		}
		PUSH_PAIR(foundInterface, GET_TPD);
		METHOD_RETURN;
	}

	FORTHOP(classDeleteMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot delete a class object");
		METHOD_RETURN;
	}

	FORTHOP(classSetNewMethod)
	{
		ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
		pClassObject->newOp = SPOP;
		METHOD_RETURN;
	}

	baseMethodEntry classMembers[] =
	{
		METHOD("delete", classDeleteMethod),
		METHOD_RET("create", classCreateMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject)),
		METHOD_RET("getParent", classSuperMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIClass)),
		METHOD_RET("getName", classNameMethod, NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeByte)),
		METHOD_RET("getVocabulary", classVocabularyMethod, NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeInt)),
		METHOD_RET("getInterface", classGetInterfaceMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject)),
		METHOD("setNew", classSetNewMethod),

		MEMBER_VAR("vocab", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("newOp", NATIVE_TYPE_TO_CODE(0, kBaseTypeOp)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oIter
	//

	// oIter is an abstract iterator class

	baseMethodEntry oIterMembers[] =
	{
		METHOD("seekNext", unimplementedMethodOp),
		METHOD("seekPrev", unimplementedMethodOp),
		METHOD("seekHead", unimplementedMethodOp),
		METHOD("seekTail", unimplementedMethodOp),
		METHOD_RET("next", unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", unimplementedMethodOp),
		METHOD_RET("unref", unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject)),
		METHOD_RET("findNext", unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("clone", unimplementedMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIter)),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oIterable
	//

	// oIterable is an abstract iterable class, containers should be derived from oIterable

	baseMethodEntry oIterableMembers[] =
	{
		METHOD_RET("headIter", unimplementedMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIter)),
		METHOD_RET("tailIter", unimplementedMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIter)),
		METHOD_RET("find", unimplementedMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIter)),
		METHOD_RET("clone", unimplementedMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIterable)),
		METHOD_RET("count", unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", unimplementedMethodOp),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oThread
	//

	struct oThreadStruct
	{
		ulong			refCount;
		ForthThread		*pThread;
	};

	// TODO: add tracking of run state of thread - this should be done inside ForthThread, not here
	FORTHOP(oThreadNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oThreadStruct, pThreadStruct);
		pThreadStruct->refCount = 0;
		pThreadStruct->pThread = NULL;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pThreadStruct);
	}

	FORTHOP(oThreadDeleteMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		if (pThreadStruct->pThread != NULL)
		{
			GET_ENGINE->DestroyThread(pThreadStruct->pThread);
			pThreadStruct->pThread = NULL;
		}
		FREE_OBJECT(pThreadStruct);
		METHOD_RETURN;
	}

	FORTHOP(oThreadInitMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		ForthEngine* pEngine = GET_ENGINE;
		if (pThreadStruct->pThread != NULL)
		{
			pEngine->DestroyThread(pThreadStruct->pThread);
		}
		long threadOp = SPOP;
		int returnStackSize = (int)(SPOP);
		int paramStackSize = (int)(SPOP);
		pThreadStruct->pThread = GET_ENGINE->CreateThread(threadOp, paramStackSize, returnStackSize);
		pThreadStruct->pThread->Reset();
		METHOD_RETURN;
	}

	FORTHOP(oThreadStartMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		long result = pThreadStruct->pThread->Start();
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oThreadExitMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Exit();
		METHOD_RETURN;
	}

	FORTHOP(oThreadPushMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Push(SPOP);
		METHOD_RETURN;
	}

	FORTHOP(oThreadPopMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		long val = pThreadStruct->pThread->Pop();
		SPUSH(val);
		METHOD_RETURN;
	}

	FORTHOP(oThreadRPushMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->RPush(SPOP);
		METHOD_RETURN;
	}

	FORTHOP(oThreadRPopMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		long val = pThreadStruct->pThread->RPop();
		SPUSH(val);
		METHOD_RETURN;
	}

	FORTHOP(oThreadGetStateMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		SPUSH((long)(pThreadStruct->pThread->GetCore()));
		METHOD_RETURN;
	}

	FORTHOP(oThreadStepMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		ForthThread* pThread = pThreadStruct->pThread;
		ForthCoreState* pThreadCore = pThread->GetCore();
		long op = *(pThreadCore->IP)++;
		long result;
		ForthEngine *pEngine = GET_ENGINE;
#ifdef ASM_INNER_INTERPRETER
		if (pEngine->GetFastMode())
		{
			result = (long)InterpretOneOpFast(pThreadCore, op);
		}
		else
#endif
		{
			result = (long)InterpretOneOp(pThreadCore, op);
		}
		if (result == kResultDone)
		{
			pThread->ResetIP();
		}
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oThreadResetMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Reset();
		METHOD_RETURN;
	}

	FORTHOP(oThreadResetIPMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->ResetIP();
		METHOD_RETURN;
	}

	baseMethodEntry oThreadMembers[] =
	{
		METHOD("__newOp", oThreadNew),
		METHOD("delete", oThreadDeleteMethod),
		METHOD("init", oThreadInitMethod),
		METHOD_RET("start", oThreadStartMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("exit", oThreadExitMethod),
		METHOD("push", oThreadPushMethod),
		METHOD_RET("pop", oThreadPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("rpush", oThreadRPushMethod),
		METHOD_RET("rpop", oThreadRPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("getState", oThreadGetStateMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("step", oThreadStepMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("reset", oThreadResetMethod),
		METHOD("resetIP", oThreadResetIPMethod),

		MEMBER_VAR("__thread", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

} // namespace

void ForthShowObject(ForthObject& obj, ForthCoreState* pCore)
{
	ForthEngine* pEngine = ForthEngine::GetInstance();
	if (obj.pData != NULL)
	{
		pEngine->ExecuteOneMethod(pCore, obj, kOMShow);
	}
	else
	{
		pEngine->ConsoleOut("'@");
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->ShowID("nullObject", NULL);
		pEngine->ConsoleOut("'");
	}
}


//////////////////////////////////////////////////////////////////////
///
//                 oVocabulary
//
struct oVocabularyStruct
{
	ulong				refCount;
	ForthVocabulary*	vocabulary;
	long*				cursor;
};

FORTHOP(oVocabularyNew)
{
	ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
	ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
	MALLOCATE_OBJECT(oVocabularyStruct, pVocabulary);
	pVocabulary->refCount = 0;
	pVocabulary->vocabulary = NULL;
	pVocabulary->cursor = NULL;
	PUSH_PAIR(pPrimaryInterface->GetMethods(), pVocabulary);
}

FORTHOP(oVocabularyOpenMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	pVocabulary->vocabulary = (ForthVocabulary*)(SPOP);
	pVocabulary->cursor = (pVocabulary->vocabulary != NULL) ? pVocabulary->vocabulary->GetNewestEntry() : NULL;
	METHOD_RETURN;
}

FORTHOP(oVocabularyCloseMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	pVocabulary->vocabulary = NULL;
	pVocabulary->cursor = NULL;
	METHOD_RETURN;
}

FORTHOP(oVocabularyGetNameMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	SPUSH((pVocab != NULL) ? (long)(pVocab->GetName()) : NULL);
	METHOD_RETURN;
}

FORTHOP(oVocabularySetDefinitionsVocabMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	if (pVocab != NULL)
	{
		ForthEngine *pEngine = GET_ENGINE;
		ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
		pEngine->SetDefinitionVocabulary(pVocabStack->GetTop());
	}
	METHOD_RETURN;
}
FORTHOP(oVocabularySetSearchVocabMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	if (pVocab != NULL)
	{
		ForthEngine *pEngine = GET_ENGINE;
		ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
		pVocabStack = pEngine->GetVocabularyStack();
		pVocabStack->SetTop(pVocab);
	}
	METHOD_RETURN;
}

FORTHOP(oVocabularySeekNewestMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	long* pEntry = NULL;
	if (pVocab != NULL)
	{
		pVocabulary->cursor = pVocab->GetNewestEntry();
		pEntry = pVocabulary->cursor;
	}
	SPUSH((long)pEntry);
	METHOD_RETURN;
}

FORTHOP(oVocabularySeekNextMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	long* pEntry = NULL;
	if ((pVocab != NULL) && (pVocabulary->cursor != NULL))
	{
		pVocabulary->cursor = pVocab->NextEntrySafe(pVocabulary->cursor);
		pEntry = pVocabulary->cursor;
	}
	SPUSH((long)pEntry);
	METHOD_RETURN;
}

FORTHOP(oVocabularyFindEntryByNameMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	long* pEntry = NULL;
	const char* pName = (const char *)(SPOP);
	if (pVocab != NULL)
	{
		pVocabulary->cursor = pVocab->FindSymbol(pName);
		pEntry = pVocabulary->cursor;
	}
	SPUSH((long)pEntry);
	METHOD_RETURN;
}

FORTHOP(oVocabularyFindNextEntryByNameMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	long* pEntry = NULL;
	const char* pName = (const char *)(SPOP);
	if (pVocab != NULL)
	{
		pVocabulary->cursor = pVocab->FindNextSymbol(pName, pEntry);
		pEntry = pVocabulary->cursor;
	}
	SPUSH((long)pEntry);
	METHOD_RETURN;
}

FORTHOP(oVocabularyFindEntryByValueMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	long* pEntry = NULL;
	long val = SPOP;
	if (pVocab != NULL)
	{
		pVocabulary->cursor = pVocab->FindSymbolByValue(val);
		pEntry = pVocabulary->cursor;
	}
	SPUSH((long)pEntry);
	METHOD_RETURN;
}

FORTHOP(oVocabularyFindNextEntryByValueMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	long* pEntry = NULL;
	long val = SPOP;
	if (pVocab != NULL)
	{
		pVocabulary->cursor = pVocab->FindNextSymbolByValue(val, pEntry);
		pEntry = pVocabulary->cursor;
	}
	SPUSH((long)pEntry);
	METHOD_RETURN;
}

FORTHOP(oVocabularyGetNumEntriesMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	SPUSH((pVocab != NULL) ? (long)(pVocab->GetNumEntries()) : 0);
	METHOD_RETURN;
}

FORTHOP(oVocabularyGetValueLengthMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	SPUSH((pVocab != NULL) ? (long)(pVocab->GetValueLength()) : 0);
	METHOD_RETURN;
}

FORTHOP(oVocabularyAddSymbolMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	long addToEngineFlags = SPOP;
	long opVal = SPOP;
	long opType = SPOP;
	char* pSymbol = (char *)(SPOP);
	bool addToEngineOps = (addToEngineFlags < 0) ? (opType <= kOpDLLEntryPoint) : (addToEngineFlags > 0);
	if (pVocab != NULL)
	{
		pVocab->AddSymbol(pSymbol, opType, opVal, addToEngineOps);
		pVocabulary->cursor = pVocab->GetNewestEntry();
	}
	METHOD_RETURN;
}

FORTHOP(oVocabularyRemoveEntryMethod)
{
	GET_THIS(oVocabularyStruct, pVocabulary);
	ForthVocabulary* pVocab = pVocabulary->vocabulary;
	long* pEntry = (long *)(SPOP);
	if ((pEntry != NULL) && (pVocab != NULL) && (pVocabulary->cursor != NULL))
	{
		pVocab->DeleteEntry(pEntry);
		pVocabulary->cursor = pVocab->GetNewestEntry();
	}
	METHOD_RETURN;
}

baseMethodEntry oVocabularyMembers[] =
{
	METHOD("__newOp", oVocabularyNew),
	METHOD("open", oVocabularyOpenMethod),
	//METHOD("openByName", oVocabularyOpenByNameMethod),
	METHOD("close", oVocabularyCloseMethod),
	METHOD("getName", oVocabularyGetNameMethod),
	METHOD("setDefinitionsVocab", oVocabularySetDefinitionsVocabMethod),
	METHOD("setSearchVocab", oVocabularySetSearchVocabMethod),
	METHOD("seekNewestEntry", oVocabularySeekNewestMethod),
	METHOD("seekNextEntry", oVocabularySeekNextMethod),
	METHOD("findEntryByName", oVocabularyFindEntryByNameMethod),
	METHOD("findNextEntryByName", oVocabularyFindNextEntryByNameMethod),
	METHOD("findEntryByValue", oVocabularyFindEntryByValueMethod),
	METHOD("findNextEntryByValue", oVocabularyFindNextEntryByValueMethod),
	METHOD("getNumEntries", oVocabularyGetNumEntriesMethod),
	METHOD("getValueLength", oVocabularyGetValueLengthMethod),
	METHOD("addSymbol", oVocabularyAddSymbolMethod),
	METHOD("removeEntry", oVocabularyRemoveEntryMethod),

	MEMBER_VAR("vocabulary", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeInt)),
	MEMBER_VAR("cursor", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeInt)),

	// following must be last in table
	END_MEMBERS
};


//////////////////////////////////////////////////////////////////////
////
///     ForthForgettableGlobalObject - handles forgetting of global forth objects
//
// 
ForthForgettableGlobalObject::ForthForgettableGlobalObject( const char* pName, void* pOpAddress, long op, int numElements )
: ForthForgettable( pOpAddress, op )
,	mNumElements( numElements )
{
    int nameLen = strlen( pName );
    mpName = (char *) __MALLOC(nameLen + 1);
    strcpy( mpName, pName );
}

ForthForgettableGlobalObject::~ForthForgettableGlobalObject()
{
    __FREE( mpName );
}

const char *
ForthForgettableGlobalObject::GetName( void )
{
    return mpName;
}

const char *
ForthForgettableGlobalObject::GetTypeName( void )
{
    return "globalObject";
}

void ForthForgettableGlobalObject::ForgetCleanup( void* pForgetLimit, long op )
{
	// first longword is OP_DO_OBJECT or OP_DO_OBJECT_ARRAY, after that are object elements
	if ((ulong)mpOpAddress > (ulong)pForgetLimit)
	{
		ForthObject* pObject = (ForthObject *)((long *)mpOpAddress + 1);
		ForthCoreState* pCore = ForthEngine::GetInstance()->GetCoreState();
		for (int i = 0; i < mNumElements; i++)
		{
			// TODO: release each 
			SAFE_RELEASE(pCore, *pObject);
			pObject->pData = NULL;
			pObject->pMethodOps = NULL;
			pObject++;
		}
	}
}

const char *
ForthTypesManager::GetName( void )
{
    return GetTypeName();
}

const char *
ForthTypesManager::GetTypeName( void )
{
    return "typesManager";
}

void
ForthTypesManager::AddBuiltinClasses(ForthEngine* pEngine)
{
	gpObjectClassVocab = pEngine->AddBuiltinClass("Object", NULL, objectMembers);
	gpClassClassVocab = pEngine->AddBuiltinClass("Class", gpObjectClassVocab, classMembers);

	gpOIterClassVocab = pEngine->AddBuiltinClass("OIter", gpObjectClassVocab, oIterMembers);
	gpOIterableClassVocab = pEngine->AddBuiltinClass("OIterable", gpObjectClassVocab, oIterableMembers);

	OArray::AddClasses(pEngine);
	OList::AddClasses(pEngine);
	OMap::AddClasses(pEngine);
	OString::AddClasses(pEngine);
	OStream::AddClasses(pEngine);
	ONumber::AddClasses(pEngine);

	ForthClassVocabulary* pOThreadClass = pEngine->AddBuiltinClass("OThread", gpObjectClassVocab, oThreadMembers);

	ForthClassVocabulary* pOVocabularyClass = pEngine->AddBuiltinClass("OVocabulary", gpObjectClassVocab, oVocabularyMembers);

	mpClassMethods = gpClassClassVocab->GetInterface(0)->GetMethods();
}

