//////////////////////////////////////////////////////////////////////
//
// OArray.cpp: builtin array related classes
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

extern "C" {
	extern void unimplementedMethodOp(ForthCoreState *pCore);
	extern void illegalMethodOp(ForthCoreState *pCore);
};

namespace OArray
{

	//////////////////////////////////////////////////////////////////////
	///
	//                 oArray
	//

	ForthClassVocabulary* gpOArrayClassVocab;
	ForthClassVocabulary* gpArrayIterClassVocab;

	FORTHOP(oArrayNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oArrayStruct, pArray);
		pArray->refCount = 0;
		pArray->elements = new oArray;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pArray);
	}

	FORTHOP(oArrayDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oArrayStruct, pArray);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = *iter;
			SAFE_RELEASE(pCore, o);
		}
		delete pArray->elements;
		FREE_OBJECT(pArray);
		METHOD_RETURN;
	}

	FORTHOP(oArrayShowMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER("oArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
				if (iter != a.begin())
				{
					pShowContext->EndElement(",");
				}
				ForthObject& o = *iter;
				pShowContext->ShowIndent();
				ForthShowObject(o, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oArrayResizeMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oArrayStruct, pArray);
		oArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		if (a.size() != newSize)
		{
			if (a.size() > newSize)
			{
				// shrinking - release elements which are beyond new end of array
				for (ulong i = newSize; i < a.size(); i++)
				{
					SAFE_RELEASE(pCore, a[i]);
				}
				a.resize(newSize);
			}
			else
			{
				// growing - add null objects to end of array
				a.resize(newSize);
				ForthObject o;
				o.pMethodOps = NULL;
				o.pData = NULL;
				for (ulong i = a.size(); i < newSize; i++)
				{
					a[i] = o;
				}
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayLoadMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oArrayStruct, pArray);
		oArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		if (a.size() != newSize)
		{
			if (a.size() > newSize)
			{
				// shrinking - release elements which are beyond new end of array
				for (ulong i = newSize; i < a.size(); i++)
				{
					SAFE_RELEASE(pCore, a[i]);
				}
				a.resize(newSize);
			}
			else
			{
				// growing - add null objects to end of array
				a.resize(newSize);
				ForthObject o;
				o.pMethodOps = NULL;
				o.pData = NULL;
				for (ulong i = a.size(); i < newSize; i++)
				{
					a[i] = o;
				}
			}
		}
		if (newSize > 0)
		{
			for (int i = newSize - 1; i >= 0; i--)
			{
				ForthObject& oldObj = a[i];
				ForthObject newObj;
				POP_OBJECT(newObj);
				if (OBJECTS_DIFFERENT(oldObj, newObj))
				{
					SAFE_KEEP(newObj);
					SAFE_RELEASE(pCore, oldObj);
				}
				a[i] = newObj;
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayToListMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();

		MALLOCATE_OBJECT(oListStruct, pList);
		pList->refCount = 0;
		pList->head = NULL;
		pList->tail = NULL;
		oListElement* oldTail = NULL;

		// bump reference counts of all valid elements in this array
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = *iter;
			SAFE_KEEP(o);
			MALLOCATE_LINK(oListElement, newElem);
			newElem->obj = o;
			if (oldTail == NULL)
			{
				ASSERT(pList->head == NULL);
				pList->head = newElem;
			}
			else
			{
				ASSERT(oldTail->next == NULL);
				oldTail->next = newElem;
			}
			newElem->prev = oldTail;
			newElem->next = NULL;
			pList->tail = newElem;
			oldTail = newElem;
		}

		// push list on TOS
		ForthInterface* pPrimaryInterface = OList::gpOListClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pList);
		METHOD_RETURN;
	}

	FORTHOP(oArrayClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oArrayStruct, pArray);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = *iter;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oArrayCountMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		SPUSH((long)(pArray->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oArrayRefMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			SPUSH((long)&(a[ix]));
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayUnrefMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			ForthObject fobj = a[ix];
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a[ix].pData = NULL;
			a[ix].pMethodOps = NULL;
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayGetMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			ForthObject fobj = a[ix];
			PUSH_OBJECT(fobj);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oArraySetMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			ForthObject& oldObj = a[ix];
			ForthObject newObj;
			POP_OBJECT(newObj);
			OBJECT_ASSIGN(pCore, oldObj, newObj);
			a[ix] = newObj;
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayFindIndexMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		long retVal = -1;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		for (ulong i = 0; i < a.size(); i++)
		{
			ForthObject& o = a[i];
			if (OBJECTS_SAME(o, soughtObj))
			{
				retVal = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	FORTHOP(oArrayPushMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray& a = *(pArray->elements);
		ForthObject fobj;
		POP_OBJECT(fobj);
		SAFE_KEEP(fobj);
		a.push_back(fobj);
		METHOD_RETURN;
	}

	FORTHOP(oArrayPopUnrefMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray& a = *(pArray->elements);
		if (a.size() > 0)
		{
			ForthObject fobj = a.back();
			a.pop_back();
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " pop of empty oArray");
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayHeadIterMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oArrayIterStruct, pIter);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = 0;
		ForthInterface* pPrimaryInterface = gpArrayIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oArrayTailIterMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oArrayIterStruct, pIter);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = pArray->elements->size();
		ForthInterface* pPrimaryInterface = gpArrayIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oArrayFindMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		long retVal = -1;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		for (ulong i = 0; i < a.size(); i++)
		{
			ForthObject& o = a[i];
			if (OBJECTS_SAME(o, soughtObj))
			{
				retVal = i;
				break;
			}
		}
		if (retVal < 0)
		{
			SPUSH(0);
		}
		else
		{
			pArray->refCount++;
			TRACK_KEEP;
			MALLOCATE_ITER(oArrayIterStruct, pIter);
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pArray);
			pIter->cursor = retVal;
			ForthInterface* pPrimaryInterface = gpArrayIterClassVocab->GetInterface(0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayCloneMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		// bump reference counts of all valid elements in this array
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = *iter;
			SAFE_KEEP(o);
		}
		// create clone array and set is size to match this array
		MALLOCATE_OBJECT(oArrayStruct, pCloneArray);
		pCloneArray->refCount = 0;
		pCloneArray->elements = new oArray;
		pCloneArray->elements->resize(a.size());
		// copy this array contents to clone array
		memcpy(&(pCloneArray->elements[0]), &(a[0]), a.size() << 3);
		// push cloned array on TOS
		PUSH_PAIR(GET_TPM, pCloneArray);
		METHOD_RETURN;
	}

	FORTHOP(oArrayInsertMethod)
	{
		GET_THIS(oArrayStruct, pArray);

		oArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ulong ix = (ulong)SPOP;
		ulong oldSize = a.size();
		if (oldSize >= ix)
		{
			ForthObject newObj;
			POP_OBJECT(newObj);
			// add dummy object to end of array
			a.resize(oldSize + 1);
			if ((oldSize > 0) && (ix < oldSize))
			{
				// move old entries up by size of ForthObject
				memmove(&(a[ix + 1]), &(a[ix]), sizeof(ForthObject) * (oldSize - ix));
			}
			a[ix] = newObj;
			SAFE_KEEP(newObj);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	baseMethodEntry oArrayMembers[] =
	{
		METHOD("__newOp", oArrayNew),
		METHOD("delete", oArrayDeleteMethod),
		METHOD("show", oArrayShowMethod),

		METHOD_RET("headIter", oArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter)),
		METHOD_RET("tailIter", oArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter)),
		METHOD_RET("find", oArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter)),
		METHOD_RET("clone", oArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArray)),
		METHOD_RET("count", oArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oArrayClearMethod),
		METHOD("resize", oArrayResizeMethod),
		METHOD("load", oArrayLoadMethod),
		METHOD_RET("toList", oArrayToListMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIList)),
		METHOD_RET("ref", oArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject | kDTIsPtr)),
		METHOD_RET("unref", oArrayUnrefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject)),
		METHOD_RET("get", oArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject)),
		METHOD("set", oArraySetMethod),
		METHOD_RET("findIndex", oArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("push", oArrayPushMethod),
		METHOD_RET("popUnref", oArrayPopUnrefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject)),
		METHOD("insert", oArrayInsertMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 oArrayIter
	//

	FORTHOP(oArrayIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oArrayIter object");
	}

	FORTHOP(oArrayIterDeleteMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterShowMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		oArray& a = *(pArray->elements);
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER("oArrayIter");
		pShowContext->ShowIndent("'cursor : ");
		ForthShowObject(a[pIter->cursor], pCore);
		pShowContext->EndElement(",");
		pShowContext->ShowIndent("'parent' : '@");
		long* pParentData = pIter->parent.pData;
		long* pParentMethods = pIter->parent.pMethodOps;
		ForthClassObject* pClassObject = (ForthClassObject *)(*((pParentMethods)-1));
		pShowContext->ShowID(pClassObject->pVocab->GetName(), pParentData);
		pShowContext->EndElement("'");
		pShowContext->EndIndent();
		pEngine->ConsoleOut("}");
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterSeekNextMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		if (pIter->cursor < pArray->elements->size())
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterSeekPrevMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterSeekHeadMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterSeekTailMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		pIter->cursor = pArray->elements->size();
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterNextMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		oArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			PUSH_OBJECT(a[pIter->cursor]);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterPrevMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		oArray& a = *(pArray->elements);
		if (pIter->cursor == 0)
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			PUSH_OBJECT(a[pIter->cursor]);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterCurrentMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		oArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			PUSH_OBJECT(a[pIter->cursor]);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterRemoveMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		oArray& a = *(pArray->elements);
		if (pIter->cursor < a.size())
		{
			// TODO!
			ForthObject o = a[pIter->cursor];
			SAFE_RELEASE(pCore, o);
			pArray->elements->erase(pArray->elements->begin() + pIter->cursor);
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterUnrefMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		oArray& a = *(pArray->elements);
		ForthObject fobj;
		fobj.pData = NULL;
		fobj.pMethodOps = NULL;
		if (pIter->cursor < a.size())
		{
			ForthObject fobj = a[pIter->cursor];
			unrefObject(fobj);
			a[pIter->cursor].pData = NULL;
			a[pIter->cursor].pMethodOps = NULL;
		}
		PUSH_OBJECT(fobj);
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterFindNextMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		long retVal = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		oArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while (i < a.size())
		{
			ForthObject& o = a[i];
			if (OBJECTS_SAME(o, soughtObj))
			{
				retVal = ~0;
				pIter->cursor = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterCloneMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		MALLOCATE_ITER(oArrayIterStruct, pNewIter);
		pNewIter->refCount = 0;
		pNewIter->parent.pMethodOps = pIter->parent.pMethodOps;
		pNewIter->parent.pData = pIter->parent.pData;
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		pArray->refCount++;
		TRACK_KEEP;
		pNewIter->cursor = pIter->cursor;
		PUSH_PAIR(GET_TPM, pNewIter);
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterInsertMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);

		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		oArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ulong ix = pIter->cursor;
		ulong oldSize = a.size();
		if (oldSize >= ix)
		{
			ForthObject newObj;
			POP_OBJECT(newObj);
			// add dummy object to end of array
			a.resize(oldSize + 1);
			if ((oldSize > 0) && (ix < oldSize))
			{
				// move old entries up by size of ForthObject
				memmove(&(a[ix + 1]), &(a[ix]), sizeof(ForthObject) * (oldSize - ix));
			}
			a[ix] = newObj;
			SAFE_KEEP(newObj);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	baseMethodEntry oArrayIterMembers[] =
	{
		METHOD("__newOp", oArrayIterNew),
		METHOD("delete", oArrayIterDeleteMethod),
		METHOD("show", oArrayIterShowMethod),

		METHOD("seekNext", oArrayIterSeekNextMethod),
		METHOD("seekPrev", oArrayIterSeekPrevMethod),
		METHOD("seekHead", oArrayIterSeekHeadMethod),
		METHOD("seekTail", oArrayIterSeekTailMethod),
		METHOD_RET("next", oArrayIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oArrayIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oArrayIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oArrayIterRemoveMethod),
		METHOD_RET("unref", oArrayIterUnrefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject)),
		METHOD_RET("findNext", oArrayIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("clone", oArrayIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter)),
		METHOD("insert", oArrayIterInsertMethod),

		MEMBER_VAR("parent", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oByteArray
	//

	typedef std::vector<char> oByteArray;
	struct oByteArrayStruct
	{
		ulong       refCount;
		oByteArray*    elements;
	};

	struct oByteArrayIterStruct
	{
		ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
	};
	static ForthClassVocabulary* gpByteArraryIterClassVocab = NULL;

	FORTHOP(oByteArrayNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oByteArrayStruct, pArray);
		pArray->refCount = 0;
		pArray->elements = new oByteArray;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pArray);
	}

	FORTHOP(oByteArrayDeleteMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		delete pArray->elements;
		FREE_OBJECT(pArray);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayShowMethod)
	{
		char buffer[8];
		GET_THIS(oByteArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oByteArray& a = *(pArray->elements);
		SHOW_OBJ_HEADER("oByteArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				sprintf(buffer, "%u", a[i] & 0xFF);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayResizeMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		ulong oldSize = a.size();
		a.resize(newSize);
		if (oldSize < newSize)
		{
			// growing - add null bytes to end of array
			char* pElement = &(a[oldSize]);
			memset(pElement, 0, newSize - oldSize);
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		char* pElement = &(a[0]);
		memset(pElement, 0, a.size());
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayLoadMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		a.resize(newSize);
		if (newSize > 0)
		{
			for (int i = newSize - 1; i >= 0; i--)
			{
				int c = SPOP;
				a[i] = (char)c;
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayCountMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		SPUSH((long)(pArray->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayRefMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			SPUSH((long)&(a[ix]));
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayGetMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			SPUSH((long)a[ix]);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArraySetMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			a[ix] = (char)SPOP;
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayFindIndexMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		long retVal = -1;
		char val = (char)SPOP;
		oByteArray& a = *(pArray->elements);
		char* pElement = &(a[0]);
		char* pFound = (char *)memchr(pElement, val, a.size());
		if (pFound)
		{
			retVal = pFound - pElement;
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayPushMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		char val = (char)SPOP;
		a.push_back(val);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayPopMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		if (a.size() > 0)
		{
			char val = a.back();
			a.pop_back();
			SPUSH((long)val);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " pop of empty oByteArray");
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayHeadIterMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oByteArrayIterStruct, pIter);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = 0;
		ForthInterface* pPrimaryInterface = gpByteArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayTailIterMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oByteArrayIterStruct, pIter);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = pArray->elements->size();
		ForthInterface* pPrimaryInterface = gpByteArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayFindMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		long retVal = -1;
		char soughtByte = (char)(SPOP);
		oByteArray::iterator iter;
		oByteArray& a = *(pArray->elements);
		for (unsigned int i = 0; i < a.size(); i++)
		{
			if (soughtByte == a[i])
			{
				retVal = i;
				break;
			}
		}
		if (retVal < 0)
		{
			SPUSH(0);
		}
		else
		{
			pArray->refCount++;
			TRACK_KEEP;
			MALLOCATE_ITER(oByteArrayIterStruct, pIter);
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pArray);
			pIter->cursor = retVal;
			ForthInterface* pPrimaryInterface = gpByteArraryIterClassVocab->GetInterface(0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayCloneMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		// create clone array and set is size to match this array
		MALLOCATE_OBJECT(oByteArrayStruct, pCloneArray);
		pCloneArray->refCount = 0;
		pCloneArray->elements = new oByteArray;
		pCloneArray->elements->resize(a.size());
		// copy this array contents to clone array
		if (a.size() > 0)
		{
			memcpy(&(pCloneArray->elements[0]), &(a[0]), a.size());
		}
		// push cloned array on TOS
		PUSH_PAIR(GET_TPM, pCloneArray);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayBaseMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		SPUSH(a.size() > 0 ? (long)&(a[0]) : NULL);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayFromStringMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		const char* pSrc = (const char *)(SPOP);
		int numBytes = ((int)strlen(pSrc)) + 1;
		a.resize(numBytes);
		memcpy(&(a[0]), pSrc, numBytes);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayFromBytesMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		int offset = SPOP;
		int numBytes = SPOP;
		const char* pSrc = (const char *)(SPOP);
		ulong copyEnd = (ulong)(numBytes + offset);
		if (copyEnd > a.size())
		{
			a.resize(numBytes);
		}
		char* pDst = &(a[0]) + offset;
		memcpy(pDst, pSrc, numBytes);
		METHOD_RETURN;
	}

	baseMethodEntry oByteArrayMembers[] =
	{
		METHOD("__newOp", oByteArrayNew),
		METHOD("delete", oByteArrayDeleteMethod),
		METHOD("show", oByteArrayShowMethod),

		METHOD_RET("headIter", oByteArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArrayIter)),
		METHOD_RET("tailIter", oByteArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArrayIter)),
		METHOD_RET("find", oByteArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArrayIter)),
		METHOD_RET("clone", oByteArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArray)),
		METHOD_RET("count", oByteArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oByteArrayClearMethod),
		METHOD("load", oByteArrayLoadMethod),

		METHOD("resize", oByteArrayResizeMethod),
		METHOD_RET("ref", oByteArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte | kDTIsPtr)),
		METHOD_RET("get", oByteArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte)),
		METHOD("set", oByteArraySetMethod),
		METHOD_RET("findIndex", oByteArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("push", oByteArrayPushMethod),
		METHOD_RET("pop", oByteArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte)),
		METHOD_RET("base", oByteArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte | kDTIsPtr)),
		METHOD("setFromString", oByteArrayFromStringMethod),
		METHOD("setFromBytes", oByteArrayFromBytesMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oByteArrayIter
	//

	FORTHOP(oByteArrayIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oByteArrayIter object");
	}

	FORTHOP(oByteArrayIterDeleteMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterSeekNextMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent.pData);
		if (pIter->cursor < pArray->elements->size())
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterSeekPrevMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterSeekHeadMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterSeekTailMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent.pData);
		pIter->cursor = pArray->elements->size();
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterNextMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent.pData);
		oByteArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			char val = a[pIter->cursor];
			SPUSH((long)val);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterPrevMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent.pData);
		oByteArray& a = *(pArray->elements);
		if (pIter->cursor == 0)
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			char val = a[pIter->cursor];
			SPUSH((long)val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterCurrentMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent.pData);
		oByteArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			char ch = a[pIter->cursor];
			SPUSH((long)ch);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterRemoveMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent.pData);
		oByteArray& a = *(pArray->elements);
		if (pIter->cursor < a.size())
		{
			pArray->elements->erase(pArray->elements->begin() + pIter->cursor);
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterFindNextMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		long retVal = 0;
		char soughtByte = (char)(SPOP);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent.pData);
		oByteArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while (i < a.size())
		{
			if (a[i] == soughtByte)
			{
				retVal = ~0;
				pIter->cursor = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterCloneMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent.pData);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oByteArrayIterStruct, pNewIter);
		pNewIter->refCount = 0;
		pNewIter->parent.pMethodOps = pIter->parent.pMethodOps;
		pNewIter->parent.pData = reinterpret_cast<long *>(pArray);
		pNewIter->cursor = pIter->cursor;
		ForthInterface* pPrimaryInterface = gpByteArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(GET_TPM, pNewIter);
		METHOD_RETURN;
	}

	baseMethodEntry oByteArrayIterMembers[] =
	{
		METHOD("__newOp", oByteArrayIterNew),
		METHOD("delete", oByteArrayIterDeleteMethod),

		METHOD("seekNext", oByteArrayIterSeekNextMethod),
		METHOD("seekPrev", oByteArrayIterSeekPrevMethod),
		METHOD("seekHead", oByteArrayIterSeekHeadMethod),
		METHOD("seekTail", oByteArrayIterSeekTailMethod),
		METHOD_RET("next", oByteArrayIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oByteArrayIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oByteArrayIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oByteArrayIterRemoveMethod),
		METHOD_RET("findNext", oByteArrayIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("clone", oByteArrayIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArrayIter)),

		MEMBER_VAR("parent", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};



	//////////////////////////////////////////////////////////////////////
	///
	//                 oShortArray
	//

	typedef std::vector<short> oShortArray;
	struct oShortArrayStruct
	{
		ulong       refCount;
		oShortArray*    elements;
	};

	struct oShortArrayIterStruct
	{
		ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
	};
	static ForthClassVocabulary* gpShortArraryIterClassVocab = NULL;

	FORTHOP(oShortArrayNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oShortArrayStruct, pArray);
		pArray->refCount = 0;
		pArray->elements = new oShortArray;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pArray);
	}

	FORTHOP(oShortArrayDeleteMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		delete pArray->elements;
		FREE_OBJECT(pArray);
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayShowMethod)
	{
		char buffer[16];
		GET_THIS(oShortArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oShortArray& a = *(pArray->elements);
		SHOW_OBJ_HEADER("oShortArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				sprintf(buffer, "%d", a[i]);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayResizeMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		ulong oldSize = a.size();
		a.resize(newSize);
		if (oldSize < newSize)
		{
			// growing - add zeros to end of array
			short* pElement = &(a[oldSize]);
			memset(pElement, 0, ((newSize - oldSize) << 1));
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		short* pElement = &(a[0]);
		memset(pElement, 0, (a.size() << 1));
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayLoadMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		a.resize(newSize);
		if (newSize > 0)
		{
			for (int i = newSize - 1; i >= 0; i--)
			{
				int c = SPOP;
				a[i] = (short)c;
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayCountMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		SPUSH((long)(pArray->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayRefMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			SPUSH((long)&(a[ix]));
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayGetMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			SPUSH((long)a[ix]);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArraySetMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			a[ix] = (short)SPOP;
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayFindIndexMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		long retVal = -1;
		short val = (short)SPOP;
		oShortArray& a = *(pArray->elements);
		for (ulong i = 0; i < a.size(); i++)
		{
			if (val == a[i])
			{
				retVal = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayPushMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		short val = (short)SPOP;
		a.push_back(val);
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayPopMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		if (a.size() > 0)
		{
			short val = a.back();
			a.pop_back();
			SPUSH((long)val);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " pop of empty oShortArray");
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayHeadIterMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oShortArrayIterStruct, pIter);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = 0;
		ForthInterface* pPrimaryInterface = gpShortArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayTailIterMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oShortArrayIterStruct, pIter);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = pArray->elements->size();
		ForthInterface* pPrimaryInterface = gpShortArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayFindMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		long retVal = -1;
		short soughtShort = (short)(SPOP);
		oShortArray::iterator iter;
		oShortArray& a = *(pArray->elements);
		for (unsigned int i = 0; i < a.size(); i++)
		{
			if (soughtShort == a[i])
			{
				retVal = i;
				break;
			}
		}
		if (retVal < 0)
		{
			SPUSH(0);
		}
		else
		{
			pArray->refCount++;
			TRACK_KEEP;
			MALLOCATE_ITER(oShortArrayIterStruct, pIter);
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pArray);
			pIter->cursor = retVal;
			ForthInterface* pPrimaryInterface = gpShortArraryIterClassVocab->GetInterface(0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayCloneMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		// create clone array and set is size to match this array
		MALLOCATE_OBJECT(oShortArrayStruct, pCloneArray);
		pCloneArray->refCount = 0;
		pCloneArray->elements = new oShortArray;
		pCloneArray->elements->resize(a.size());
		// copy this array contents to clone array
		memcpy(&(pCloneArray->elements[0]), &(a[0]), a.size() << 1);
		// push cloned array on TOS
		PUSH_PAIR(GET_TPM, pCloneArray);
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayBaseMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		SPUSH((long)&(a[0]));
		METHOD_RETURN;
	}


	FORTHOP(oShortArrayFromMemoryMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		int offset = SPOP;
		int numShorts = SPOP;
		const short* pSrc = (const short *)(SPOP);
		ulong copyEnd = (ulong)(numShorts + offset);
		if (copyEnd > a.size())
		{
			a.resize(numShorts);
		}
		short* pDst = &(a[0]) + offset;
		memcpy(pDst, pSrc, numShorts << 1);
		METHOD_RETURN;
	}

	baseMethodEntry oShortArrayMembers[] =
	{
		METHOD("__newOp", oShortArrayNew),
		METHOD("delete", oShortArrayDeleteMethod),
		METHOD("show", oShortArrayShowMethod),

		METHOD_RET("headIter", oShortArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArrayIter)),
		METHOD_RET("tailIter", oShortArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArrayIter)),
		METHOD_RET("find", oShortArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArrayIter)),
		METHOD_RET("clone", oShortArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArray)),
		METHOD_RET("count", oShortArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oShortArrayClearMethod),
		METHOD("load", oShortArrayLoadMethod),

		METHOD("resize", oShortArrayResizeMethod),
		METHOD_RET("ref", oShortArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort | kDTIsPtr)),
		METHOD_RET("get", oShortArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort)),
		METHOD("set", oShortArraySetMethod),
		METHOD_RET("findIndex", oShortArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("push", oShortArrayPushMethod),
		METHOD_RET("pop", oShortArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort)),
		METHOD_RET("base", oShortArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort | kDTIsPtr)),
		METHOD("fromMemory", oShortArrayFromMemoryMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oShortArrayIter
	//

	FORTHOP(oShortArrayIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oShortArrayIter object");
	}

	FORTHOP(oShortArrayIterDeleteMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterSeekNextMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent.pData);
		if (pIter->cursor < pArray->elements->size())
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterSeekPrevMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterSeekHeadMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterSeekTailMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent.pData);
		pIter->cursor = pArray->elements->size();
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterNextMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent.pData);
		oShortArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			short val = a[pIter->cursor];
			SPUSH((long)val);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterPrevMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent.pData);
		oShortArray& a = *(pArray->elements);
		if (pIter->cursor == 0)
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			short val = a[pIter->cursor];
			SPUSH((long)val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterCurrentMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent.pData);
		oShortArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			short val = a[pIter->cursor];
			SPUSH((long)val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterRemoveMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent.pData);
		oShortArray& a = *(pArray->elements);
		if (pIter->cursor < a.size())
		{
			pArray->elements->erase(pArray->elements->begin() + pIter->cursor);
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterFindNextMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		long retVal = 0;
		char soughtShort = (char)(SPOP);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent.pData);
		oShortArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while (i < a.size())
		{
			if (a[i] == soughtShort)
			{
				retVal = ~0;
				pIter->cursor = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterCloneMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent.pData);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oShortArrayIterStruct, pNewIter);
		pNewIter->refCount = 0;
		pNewIter->parent.pMethodOps = pIter->parent.pMethodOps;
		pNewIter->parent.pData = reinterpret_cast<long *>(pArray);
		pNewIter->cursor = pIter->cursor;
		ForthInterface* pPrimaryInterface = gpShortArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(GET_TPM, pNewIter);
		METHOD_RETURN;
	}

	baseMethodEntry oShortArrayIterMembers[] =
	{
		METHOD("__newOp", oShortArrayIterNew),
		METHOD("delete", oShortArrayIterDeleteMethod),
		METHOD("seekNext", oShortArrayIterSeekNextMethod),
		METHOD("seekPrev", oShortArrayIterSeekPrevMethod),
		METHOD("seekHead", oShortArrayIterSeekHeadMethod),
		METHOD("seekTail", oShortArrayIterSeekTailMethod),
		METHOD_RET("next", oShortArrayIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oShortArrayIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oShortArrayIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oShortArrayIterRemoveMethod),
		METHOD_RET("findNext", oShortArrayIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("clone", oShortArrayIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArrayIter)),

		MEMBER_VAR("parent", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 oIntArray
	//

	typedef std::vector<int> oIntArray;
	struct oIntArrayStruct
	{
		ulong       refCount;
		oIntArray*    elements;
	};

	struct oIntArrayIterStruct
	{
		ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
	};
	static ForthClassVocabulary* gpIntArraryIterClassVocab = NULL;

	FORTHOP(oIntArrayNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oIntArrayStruct, pArray);
		pArray->refCount = 0;
		pArray->elements = new oIntArray;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pArray);
	}

	FORTHOP(oIntArrayDeleteMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		delete pArray->elements;
		FREE_OBJECT(pArray);
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayShowMethod)
	{
		char buffer[16];
		GET_THIS(oIntArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oIntArray& a = *(pArray->elements);
		SHOW_OBJ_HEADER("oIntArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				sprintf(buffer, "%d", a[i]);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayResizeMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		ulong oldSize = a.size();
		a.resize(newSize);
		if (oldSize < newSize)
		{
			// growing - add zeros to end of array
			int* pElement = &(a[oldSize]);
			memset(pElement, 0, ((newSize - oldSize) << 2));
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		int* pElement = &(a[0]);
		memset(pElement, 0, (a.size() << 2));
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayLoadMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		a.resize(newSize);
		if (newSize > 0)
		{
			for (int i = newSize - 1; i >= 0; i--)
			{
				int c = SPOP;
				a[i] = c;
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayCountMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		SPUSH((long)(pArray->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayRefMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			SPUSH((long)&(a[ix]));
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayGetMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			SPUSH(a[ix]);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArraySetMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			a[ix] = SPOP;
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayFindIndexMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		long retVal = -1;
		int val = SPOP;
		oIntArray& a = *(pArray->elements);
		for (ulong i = 0; i < a.size(); i++)
		{
			if (val == a[i])
			{
				retVal = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayPushMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		int val = SPOP;
		a.push_back(val);
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayPopMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		if (a.size() > 0)
		{
			long val = a.back();
			a.pop_back();
			SPUSH(val);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " pop of empty oIntArray");
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayHeadIterMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oIntArrayIterStruct, pIter);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = 0;
		ForthInterface* pPrimaryInterface = gpIntArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayTailIterMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oIntArrayIterStruct, pIter);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = pArray->elements->size();
		ForthInterface* pPrimaryInterface = gpIntArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayFindMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		long retVal = -1;
		int soughtInt = SPOP;
		oIntArray::iterator iter;
		oIntArray& a = *(pArray->elements);
		for (unsigned int i = 0; i < a.size(); i++)
		{
			if (soughtInt == a[i])
			{
				retVal = i;
				break;
			}
		}
		if (retVal < 0)
		{
			SPUSH(0);
		}
		else
		{
			pArray->refCount++;
			TRACK_KEEP;
			MALLOCATE_ITER(oIntArrayIterStruct, pIter);
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pArray);
			pIter->cursor = retVal;
			ForthInterface* pPrimaryInterface = gpIntArraryIterClassVocab->GetInterface(0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayCloneMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		// create clone array and set is size to match this array
		MALLOCATE_OBJECT(oIntArrayStruct, pCloneArray);
		pCloneArray->refCount = 0;
		pCloneArray->elements = new oIntArray;
		pCloneArray->elements->resize(a.size());
		// copy this array contents to clone array
		memcpy(&(pCloneArray->elements[0]), &(a[0]), a.size() << 2);
		// push cloned array on TOS
		PUSH_PAIR(GET_TPM, pCloneArray);
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayBaseMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		SPUSH((long)&(a[0]));
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayFromMemoryMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		int offset = SPOP;
		int numInts = SPOP;
		const int* pSrc = (const int *)(SPOP);
		ulong copyEnd = (ulong)(numInts + offset);
		if (copyEnd > a.size())
		{
			a.resize(numInts);
		}
		int* pDst = &(a[0]) + offset;
		memcpy(pDst, pSrc, numInts << 2);
		METHOD_RETURN;
	}

	baseMethodEntry oIntArrayMembers[] =
	{
		METHOD("__newOp", oIntArrayNew),
		METHOD("delete", oIntArrayDeleteMethod),
		METHOD("show", oIntArrayShowMethod),

		METHOD_RET("headIter", oIntArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArrayIter)),
		METHOD_RET("tailIter", oIntArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArrayIter)),
		METHOD_RET("find", oIntArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArrayIter)),
		METHOD_RET("clone", oIntArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArray)),
		METHOD_RET("count", oIntArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oIntArrayClearMethod),
		METHOD("load", oIntArrayLoadMethod),

		METHOD("resize", oIntArrayResizeMethod),
		METHOD_RET("ref", oIntArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt | kDTIsPtr)),
		METHOD_RET("get", oIntArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("set", oIntArraySetMethod),
		METHOD_RET("findIndex", oIntArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("push", oIntArrayPushMethod),
		METHOD_RET("pop", oIntArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("base", oIntArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt | kDTIsPtr)),
		METHOD("fromMemory", oIntArrayFromMemoryMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oIntArrayIter
	//

	FORTHOP(oIntArrayIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oIntArrayIter object");
	}

	FORTHOP(oIntArrayIterDeleteMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterSeekNextMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent.pData);
		if (pIter->cursor < pArray->elements->size())
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterSeekPrevMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterSeekHeadMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterSeekTailMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent.pData);
		pIter->cursor = pArray->elements->size();
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterNextMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent.pData);
		oIntArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			SPUSH(a[pIter->cursor]);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterPrevMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent.pData);
		oIntArray& a = *(pArray->elements);
		if (pIter->cursor == 0)
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			SPUSH(a[pIter->cursor]);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterCurrentMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent.pData);
		oIntArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			SPUSH(a[pIter->cursor]);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterRemoveMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent.pData);
		oIntArray& a = *(pArray->elements);
		if (pIter->cursor < a.size())
		{
			pArray->elements->erase(pArray->elements->begin() + pIter->cursor);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterFindNextMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		long retVal = 0;
		char soughtInt = (char)(SPOP);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent.pData);
		oIntArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while (i < a.size())
		{
			if (a[i] == soughtInt)
			{
				retVal = ~0;
				pIter->cursor = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterCloneMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent.pData);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oIntArrayIterStruct, pNewIter);
		pNewIter->refCount = 0;
		pNewIter->parent.pMethodOps = pIter->parent.pMethodOps;
		pNewIter->parent.pData = reinterpret_cast<long *>(pArray);
		pNewIter->cursor = pIter->cursor;
		ForthInterface* pPrimaryInterface = gpIntArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(GET_TPM, pNewIter);
		METHOD_RETURN;
	}

	baseMethodEntry oIntArrayIterMembers[] =
	{
		METHOD("__newOp", oIntArrayIterNew),
		METHOD("delete", oIntArrayIterDeleteMethod),
		METHOD("seekNext", oIntArrayIterSeekNextMethod),
		METHOD("seekPrev", oIntArrayIterSeekPrevMethod),
		METHOD("seekHead", oIntArrayIterSeekHeadMethod),
		METHOD("seekTail", oIntArrayIterSeekTailMethod),
		METHOD_RET("next", oIntArrayIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oIntArrayIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oIntArrayIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oIntArrayIterRemoveMethod),
		METHOD_RET("findNext", oIntArrayIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("clone", oIntArrayIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArrayIter)),

		MEMBER_VAR("parent", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oFloatArray
	//
	static ForthClassVocabulary* gpFloatArraryIterClassVocab = NULL;

	FORTHOP(oFloatArrayShowMethod)
	{
		char buffer[32];
		GET_THIS(oIntArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oIntArray& a = *(pArray->elements);
		SHOW_OBJ_HEADER("oFloatArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				float fval = *((float *)(&(a[i])));
				sprintf(buffer, "%f", fval);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	baseMethodEntry oFloatArrayMembers[] =
	{
		METHOD("__newOp", oIntArrayNew),
		METHOD("show", oFloatArrayShowMethod),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oLongArray
	//

	typedef std::vector<long long> oLongArray;
	struct oLongArrayStruct
	{
		ulong       refCount;
		oLongArray*    elements;
	};

	struct oLongArrayIterStruct
	{
		ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
	};
	static ForthClassVocabulary* gpLongArraryIterClassVocab = NULL;

	FORTHOP(oLongArrayNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oLongArrayStruct, pArray);
		pArray->refCount = 0;
		pArray->elements = new oLongArray;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pArray);
	}

	FORTHOP(oLongArrayDeleteMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		delete pArray->elements;
		FREE_OBJECT(pArray);
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayShowMethod)
	{
		char buffer[32];
		GET_THIS(oLongArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oLongArray& a = *(pArray->elements);
		SHOW_OBJ_HEADER("oLongArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				sprintf(buffer, "%lld", a[i]);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayResizeMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		ulong oldSize = a.size();
		a.resize(newSize);
		if (oldSize < newSize)
		{
			// growing - add zeros to end of array
			long long* pElement = &(a[oldSize]);
			memset(pElement, 0, ((newSize - oldSize) << 3));
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		long long* pElement = &(a[0]);
		memset(pElement, 0, (a.size() << 3));
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayLoadMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		a.resize(newSize);
		stackInt64 a64;
		if (newSize > 0)
		{
			for (int i = newSize - 1; i >= 0; i--)
			{
				LPOP(a64);
				a[i] = a64.s64;
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayCountMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		SPUSH((long)(pArray->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayRefMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			SPUSH((long)&(a[ix]));
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayGetMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			stackInt64 a64;
			a64.s64 = a[ix];
			LPUSH(a64);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArraySetMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		stackInt64 a64;
		if (a.size() > ix)
		{
			LPOP(a64);
			a[ix] = a64.s64;
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " array index out of range");
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayFindIndexMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		long retVal = -1;
		stackInt64 a64;
		LPOP(a64);
		long long val = a64.s64;
		oLongArray& a = *(pArray->elements);
		for (ulong i = 0; i < a.size(); i++)
		{
			if (val == a[i])
			{
				retVal = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayPushMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		stackInt64 a64;
		LPOP(a64);
		a.push_back(a64.s64);
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayPopMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		if (a.size() > 0)
		{
			stackInt64 a64;
			a64.s64 = a.back();
			a.pop_back();
			LPUSH(a64);
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			pEngine->SetError(kForthErrorBadParameter, " pop of empty oLongArray");
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayHeadIterMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oLongArrayIterStruct, pIter);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = 0;
		ForthInterface* pPrimaryInterface = gpLongArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayTailIterMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oLongArrayIterStruct, pIter);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = pArray->elements->size();
		ForthInterface* pPrimaryInterface = gpLongArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayFindMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		long retVal = -1;
		stackInt64 a64;
		LPOP(a64);
		long long soughtLong = a64.s64;
		oLongArray::iterator iter;
		oLongArray& a = *(pArray->elements);
		for (unsigned int i = 0; i < a.size(); i++)
		{
			if (soughtLong == a[i])
			{
				retVal = i;
				break;
			}
		}
		if (retVal < 0)
		{
			SPUSH(0);
		}
		else
		{
			pArray->refCount++;
			TRACK_KEEP;
			MALLOCATE_ITER(oLongArrayIterStruct, pIter);
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pArray);
			pIter->cursor = retVal;
			ForthInterface* pPrimaryInterface = gpLongArraryIterClassVocab->GetInterface(0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayCloneMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		// create clone array and set is size to match this array
		MALLOCATE_OBJECT(oLongArrayStruct, pCloneArray);
		pCloneArray->refCount = 0;
		pCloneArray->elements = new oLongArray;
		pCloneArray->elements->resize(a.size());
		// copy this array contents to clone array
		memcpy(&(pCloneArray->elements[0]), &(a[0]), a.size() << 3);
		// push cloned array on TOS
		PUSH_PAIR(GET_TPM, pCloneArray);
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayBaseMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		SPUSH((long)&(a[0]));
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayFromMemoryMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		int offset = SPOP;
		int numLongs = SPOP;
		const long long* pSrc = (const long long*)(SPOP);
		ulong copyEnd = (ulong)(numLongs + offset);
		if (copyEnd > a.size())
		{
			a.resize(numLongs);
		}
		long long* pDst = &(a[0]) + offset;
		memcpy(pDst, pSrc, numLongs << 3);
		METHOD_RETURN;
	}

	baseMethodEntry oLongArrayMembers[] =
	{
		METHOD("__newOp", oLongArrayNew),
		METHOD("delete", oLongArrayDeleteMethod),
		METHOD("show", oLongArrayShowMethod),

		METHOD_RET("headIter", oLongArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArrayIter)),
		METHOD_RET("tailIter", oLongArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArrayIter)),
		METHOD_RET("find", oLongArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArrayIter)),
		METHOD_RET("clone", oLongArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArray)),
		METHOD_RET("count", oLongArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oLongArrayClearMethod),
		METHOD("long", oLongArrayLoadMethod),

		METHOD("resize", oLongArrayResizeMethod),
		METHOD_RET("ref", oLongArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong | kDTIsPtr)),
		METHOD_RET("get", oLongArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong)),
		METHOD("set", oLongArraySetMethod),
		METHOD_RET("findIndex", oLongArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("push", oLongArrayPushMethod),
		METHOD_RET("pop", oLongArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong)),
		METHOD_RET("base", oLongArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong | kDTIsPtr)),
		METHOD("fromMemory", oLongArrayFromMemoryMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oLongArrayIter
	//

	FORTHOP(oLongArrayIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oLongArrayIter object");
	}

	FORTHOP(oLongArrayIterDeleteMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterSeekNextMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent.pData);
		if (pIter->cursor < pArray->elements->size())
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterSeekPrevMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterSeekHeadMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterSeekTailMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent.pData);
		pIter->cursor = pArray->elements->size();
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterNextMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent.pData);
		oLongArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			stackInt64 val;
			val.s64 = a[pIter->cursor];
			LPUSH(val);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterPrevMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent.pData);
		oLongArray& a = *(pArray->elements);
		if (pIter->cursor == 0)
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			stackInt64 val;
			val.s64 = a[pIter->cursor];
			LPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterCurrentMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent.pData);
		oLongArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			stackInt64 val;
			val.s64 = a[pIter->cursor];
			LPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterRemoveMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent.pData);
		oLongArray& a = *(pArray->elements);
		if (pIter->cursor < a.size())
		{
			pArray->elements->erase(pArray->elements->begin() + pIter->cursor);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterFindNextMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		long retVal = 0;
		stackInt64 a64;
		LPOP(a64);
		long long soughtLong = a64.s64;
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent.pData);
		oLongArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while (i < a.size())
		{
			if (a[i] == soughtLong)
			{
				retVal = ~0;
				pIter->cursor = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterCloneMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent.pData);
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER(oLongArrayIterStruct, pNewIter);
		pNewIter->refCount = 0;
		pNewIter->parent.pMethodOps = pIter->parent.pMethodOps;
		pNewIter->parent.pData = reinterpret_cast<long *>(pArray);
		pNewIter->cursor = pIter->cursor;
		ForthInterface* pPrimaryInterface = gpLongArraryIterClassVocab->GetInterface(0);
		PUSH_PAIR(GET_TPM, pNewIter);
		METHOD_RETURN;
	}

	baseMethodEntry oLongArrayIterMembers[] =
	{
		METHOD("__newOp", oLongArrayIterNew),
		METHOD("delete", oLongArrayIterDeleteMethod),

		METHOD("seekNext", oLongArrayIterSeekNextMethod),
		METHOD("seekPrev", oLongArrayIterSeekPrevMethod),
		METHOD("seekHead", oLongArrayIterSeekHeadMethod),
		METHOD("seekTail", oLongArrayIterSeekTailMethod),
		METHOD_RET("next", oLongArrayIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oLongArrayIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oLongArrayIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oLongArrayIterRemoveMethod),
		METHOD_RET("findNext", oLongArrayIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("clone", oLongArrayIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArrayIter)),

		MEMBER_VAR("parent", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oDoubleArray
	//
	static ForthClassVocabulary* gpDoubleArrayIterClassVocab = NULL;

	FORTHOP(oDoubleArrayShowMethod)
	{
		char buffer[32];
		GET_THIS(oLongArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oLongArray& a = *(pArray->elements);
		SHOW_OBJ_HEADER("oDoubleArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				double dval = *((double *)(&(a[i])));
				sprintf(buffer, "%g", dval);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}


	baseMethodEntry oDoubleArrayMembers[] =
	{
		METHOD("__newOp", oLongArrayNew),
		METHOD("show", oDoubleArrayShowMethod),
		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 oPair
	//

	struct oPairStruct
	{
		ulong       refCount;
		ForthObject	a;
		ForthObject	b;
	};

	struct oPairIterStruct
	{
		ulong				refCount;
		ForthObject			parent;
		int					cursor;
	};

	static ForthClassVocabulary* gpPairIterClassVocab = NULL;


	FORTHOP(oPairNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oPairStruct, pPair);
		pPair->refCount = 0;
		pPair->a.pData = NULL;
		pPair->a.pMethodOps = NULL;
		pPair->b.pData = NULL;
		pPair->b.pMethodOps = NULL;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pPair);
	}

	FORTHOP(oPairDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oPairStruct, pPair);
		ForthObject& oa = pPair->a;
		SAFE_RELEASE(pCore, oa);
		ForthObject& ob = pPair->b;
		SAFE_RELEASE(pCore, ob);
		FREE_OBJECT(pPair);
		METHOD_RETURN;
	}

	FORTHOP(oPairShowMethod)
	{
		GET_THIS(oPairStruct, pPair);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_OBJ_HEADER("oPair");
		pShowContext->BeginIndent();
		pShowContext->ShowIndent("'a' : ");
		ForthShowObject(pPair->a, pCore);
		pShowContext->EndElement(",");
		pShowContext->ShowIndent("'b' : ");
		ForthShowObject(pPair->b, pCore);
		pShowContext->EndElement();
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oPairGetAMethod)
	{
		GET_THIS(oPairStruct, pPair);
		PUSH_OBJECT(pPair->a);
		METHOD_RETURN;
	}

	FORTHOP(oPairSetAMethod)
	{
		GET_THIS(oPairStruct, pPair);
		ForthObject newObj;
		POP_OBJECT(newObj);
		ForthObject& oldObj = pPair->a;
		if (OBJECTS_DIFFERENT(oldObj, newObj))
		{
			SAFE_RELEASE(pCore, oldObj);
			pPair->a = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oPairGetBMethod)
	{
		GET_THIS(oPairStruct, pPair);
		PUSH_OBJECT(pPair->b);
		METHOD_RETURN;
	}

	FORTHOP(oPairSetBMethod)
	{
		GET_THIS(oPairStruct, pPair);
		ForthObject newObj;
		POP_OBJECT(newObj);
		ForthObject& oldObj = pPair->b;
		if (OBJECTS_DIFFERENT(oldObj, newObj))
		{
			SAFE_RELEASE(pCore, oldObj);
			pPair->b = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oPairHeadIterMethod)
	{
		GET_THIS(oPairStruct, pPair);
		pPair->refCount++;
		TRACK_KEEP;
		oPairIterStruct* pIter = new oPairIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pPair);
		pIter->cursor = 0;
		ForthInterface* pPrimaryInterface = gpPairIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oPairTailIterMethod)
	{
		GET_THIS(oPairStruct, pPair);
		pPair->refCount++;
		TRACK_KEEP;
		oPairIterStruct* pIter = new oPairIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pPair);
		pIter->cursor = 2;
		ForthInterface* pPrimaryInterface = gpPairIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}


	baseMethodEntry oPairMembers[] =
	{
		METHOD("__newOp", oPairNew),
		METHOD("delete", oPairDeleteMethod),
		METHOD("show", oPairShowMethod),

		METHOD_RET("headIter", oPairHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIPairIter)),
		METHOD_RET("tailIter", oPairTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIPairIter)),
		//METHOD_RET( "find",                 oPairFindMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIPairIter) ),
		//METHOD_RET( "clone",                oPairCloneMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIPair) ),
		//METHOD_RET( "count",                oPairCountMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
		//METHOD(     "clear",                oPairClearMethod ),

		METHOD("setA", oPairSetAMethod),
		METHOD_RET("getA", oPairGetAMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject)),
		METHOD("setB", oPairSetBMethod),
		METHOD_RET("getB", oPairGetBMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject)),

		MEMBER_VAR("objectA", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("objectB", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oPairIter
	//

	FORTHOP(oPairIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oPairIter object");
	}

	FORTHOP(oPairIterDeleteMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oPairIterSeekNextMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		if (pIter->cursor < 2)
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oPairIterSeekPrevMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oPairIterSeekHeadMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oPairIterSeekTailMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		pIter->cursor = 2;
		METHOD_RETURN;
	}

	FORTHOP(oPairIterNextMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		oPairStruct* pPair = reinterpret_cast<oPairStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 0:
			pIter->cursor++;
			PUSH_OBJECT(pPair->a);
			SPUSH(~0);
			break;
		case 1:
			pIter->cursor++;
			PUSH_OBJECT(pPair->b);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

	FORTHOP(oPairIterPrevMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		oPairStruct* pPair = reinterpret_cast<oPairStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 1:
			pIter->cursor--;
			PUSH_OBJECT(pPair->a);
			SPUSH(~0);
			break;
		case 2:
			pIter->cursor--;
			PUSH_OBJECT(pPair->b);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

	FORTHOP(oPairIterCurrentMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		oPairStruct* pPair = reinterpret_cast<oPairStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 0:
			PUSH_OBJECT(pPair->a);
			SPUSH(~0);
			break;
		case 1:
			PUSH_OBJECT(pPair->b);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

	baseMethodEntry oPairIterMembers[] =
	{
		METHOD("__newOp", oPairIterNew),
		METHOD("delete", oPairIterDeleteMethod),

		METHOD("seekNext", oPairIterSeekNextMethod),
		METHOD("seekPrev", oPairIterSeekPrevMethod),
		METHOD("seekHead", oPairIterSeekHeadMethod),
		METHOD("seekTail", oPairIterSeekTailMethod),
		METHOD_RET("next", oPairIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oPairIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oPairIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		//METHOD(     "remove",				oPairIterRemoveMethod ),
		//METHOD_RET( "findNext",				oPairIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
		//METHOD_RET( "clone",                oPairIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIPairIter) ),

		MEMBER_VAR("parent", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 oTriple
	//

	struct oTripleStruct
	{
		ulong       refCount;
		ForthObject	a;
		ForthObject	b;
		ForthObject	c;
	};

	struct oTripleIterStruct
	{
		ulong				refCount;
		ForthObject			parent;
		int					cursor;
	};
	static ForthClassVocabulary* gpTripleIterClassVocab = NULL;


	FORTHOP(oTripleNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oTripleStruct, pTriple);
		pTriple->refCount = 0;
		pTriple->a.pData = NULL;
		pTriple->a.pMethodOps = NULL;
		pTriple->b.pData = NULL;
		pTriple->b.pMethodOps = NULL;
		pTriple->c.pData = NULL;
		pTriple->c.pMethodOps = NULL;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pTriple);
	}

	FORTHOP(oTripleDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oTripleStruct, pTriple);
		ForthObject& oa = pTriple->a;
		SAFE_RELEASE(pCore, oa);
		ForthObject& ob = pTriple->b;
		SAFE_RELEASE(pCore, ob);
		ForthObject& oc = pTriple->c;
		SAFE_RELEASE(pCore, oc);
		FREE_OBJECT(pTriple);
		METHOD_RETURN;
	}

	FORTHOP(oTripleShowMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_OBJ_HEADER("oTriple");
		pShowContext->BeginIndent();
		pShowContext->ShowIndent("'a' : ");
		ForthShowObject(pTriple->a, pCore);
		pShowContext->EndElement(",");
		pShowContext->ShowIndent("'b' : ");
		ForthShowObject(pTriple->b, pCore);
		pShowContext->EndElement();
		pShowContext->ShowIndent("'c' : ");
		ForthShowObject(pTriple->c, pCore);
		pShowContext->EndElement();
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oTripleGetAMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		PUSH_OBJECT(pTriple->a);
		METHOD_RETURN;
	}

	FORTHOP(oTripleSetAMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		ForthObject newObj;
		POP_OBJECT(newObj);
		ForthObject& oldObj = pTriple->a;
		if (OBJECTS_DIFFERENT(oldObj, newObj))
		{
			SAFE_RELEASE(pCore, oldObj);
			pTriple->a = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleGetBMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		PUSH_OBJECT(pTriple->b);
		METHOD_RETURN;
	}

	FORTHOP(oTripleSetBMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		ForthObject newObj;
		POP_OBJECT(newObj);
		ForthObject& oldObj = pTriple->b;
		if (OBJECTS_DIFFERENT(oldObj, newObj))
		{
			SAFE_RELEASE(pCore, oldObj);
			pTriple->b = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleGetCMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		PUSH_OBJECT(pTriple->c);
		METHOD_RETURN;
	}

	FORTHOP(oTripleSetCMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		ForthObject newObj;
		POP_OBJECT(newObj);
		ForthObject& oldObj = pTriple->b;
		if (OBJECTS_DIFFERENT(oldObj, newObj))
		{
			SAFE_RELEASE(pCore, oldObj);
			pTriple->c = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleHeadIterMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		pTriple->refCount++;
		TRACK_KEEP;
		oTripleIterStruct* pIter = new oTripleIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pTriple);
		pIter->cursor = 0;
		ForthInterface* pPrimaryInterface = gpTripleIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oTripleTailIterMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		pTriple->refCount++;
		TRACK_KEEP;
		oTripleIterStruct* pIter = new oTripleIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pTriple);
		pIter->cursor = 3;
		ForthInterface* pPrimaryInterface = gpTripleIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	baseMethodEntry oTripleMembers[] =
	{
		METHOD("__newOp", oTripleNew),
		METHOD("delete", oTripleDeleteMethod),
		METHOD("show", oTripleShowMethod),

		METHOD_RET("headIter", oTripleHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCITripleIter)),
		METHOD_RET("tailIter", oTripleTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCITripleIter)),
		//METHOD_RET( "find",                 oTripleFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCITripleIter) ),
		//METHOD_RET( "clone",                oTripleCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCITriple) ),
		//METHOD_RET( "count",                oTripleCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
		//METHOD(     "clear",                oTripleClearMethod ),

		METHOD("setA", oTripleSetAMethod),
		METHOD_RET("getA", oTripleGetAMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject)),
		METHOD("setB", oTripleSetBMethod),
		METHOD_RET("getB", oTripleGetBMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject)),
		METHOD("setC", oTripleSetCMethod),
		METHOD_RET("getC", oTripleGetCMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject)),

		MEMBER_VAR("objectA", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("objectB", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("objectC", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oTripleIter
	//

	FORTHOP(oTripleIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oTripleIter object");
	}

	FORTHOP(oTripleIterDeleteMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterSeekNextMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		if (pIter->cursor < 3)
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterSeekPrevMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterSeekHeadMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterSeekTailMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		pIter->cursor = 3;
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterNextMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 0:
			pIter->cursor++;
			PUSH_OBJECT(pTriple->a);
			SPUSH(~0);
			break;
		case 1:
			pIter->cursor++;
			PUSH_OBJECT(pTriple->b);
			SPUSH(~0);
			break;
		case 2:
			pIter->cursor++;
			PUSH_OBJECT(pTriple->c);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterPrevMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 1:
			pIter->cursor--;
			PUSH_OBJECT(pTriple->a);
			SPUSH(~0);
			break;
		case 2:
			pIter->cursor--;
			PUSH_OBJECT(pTriple->b);
			SPUSH(~0);
			break;
		case 3:
			pIter->cursor--;
			PUSH_OBJECT(pTriple->c);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterCurrentMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 0:
			PUSH_OBJECT(pTriple->a);
			SPUSH(~0);
			break;
		case 1:
			PUSH_OBJECT(pTriple->b);
			SPUSH(~0);
			break;
		case 2:
			PUSH_OBJECT(pTriple->c);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

	baseMethodEntry oTripleIterMembers[] =
	{
		METHOD("__newOp", oTripleIterNew),
		METHOD("delete", oTripleIterDeleteMethod),

		METHOD("seekNext", oTripleIterSeekNextMethod),
		METHOD("seekPrev", oTripleIterSeekPrevMethod),
		METHOD("seekHead", oTripleIterSeekHeadMethod),
		METHOD("seekTail", oTripleIterSeekTailMethod),
		METHOD_RET("next", oTripleIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oTripleIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oTripleIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		//METHOD(     "remove",				oTripleIterRemoveMethod ),
		//METHOD_RET( "findNext",				oTripleIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
		//METHOD_RET( "clone",                oTripleIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCITripleIter) ),

		MEMBER_VAR("parent", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		gpOArrayClassVocab = pEngine->AddBuiltinClass("OArray", gpOIterableClassVocab, oArrayMembers);
		gpArrayIterClassVocab = pEngine->AddBuiltinClass("OArrayIter", gpOIterClassVocab, oArrayIterMembers);

		ForthClassVocabulary* pOByteArrayClass = pEngine->AddBuiltinClass("OByteArray", gpOArrayClassVocab, oByteArrayMembers);
		gpByteArraryIterClassVocab = pEngine->AddBuiltinClass("OByteArrayIter", gpOIterClassVocab, oByteArrayIterMembers);

		ForthClassVocabulary* pOShortArrayClass = pEngine->AddBuiltinClass("OShortArray", gpOArrayClassVocab, oShortArrayMembers);
		gpShortArraryIterClassVocab = pEngine->AddBuiltinClass("OShortArrayIter", gpOIterClassVocab, oShortArrayIterMembers);

		ForthClassVocabulary* pOIntArrayClass = pEngine->AddBuiltinClass("OIntArray", gpOArrayClassVocab, oIntArrayMembers);
		gpIntArraryIterClassVocab = pEngine->AddBuiltinClass("OIntArrayIter", gpOIterClassVocab, oIntArrayIterMembers);

		ForthClassVocabulary* pOFloatArrayClass = pEngine->AddBuiltinClass("OFloatArray", pOIntArrayClass, oFloatArrayMembers);
		gpFloatArraryIterClassVocab = pEngine->AddBuiltinClass("OFloatArrayIter", gpOIterClassVocab, oIntArrayIterMembers);

		ForthClassVocabulary* pOLongArrayClass = pEngine->AddBuiltinClass("OLongArray", gpOArrayClassVocab, oLongArrayMembers);
		gpLongArraryIterClassVocab = pEngine->AddBuiltinClass("OLongArrayIter", gpOIterClassVocab, oLongArrayIterMembers);

		ForthClassVocabulary* pODoubleArrayClass = pEngine->AddBuiltinClass("ODoubleArray", pOLongArrayClass, oDoubleArrayMembers);
		gpDoubleArrayIterClassVocab = pEngine->AddBuiltinClass("ODoubleArrayIter", gpOIterClassVocab, oLongArrayIterMembers);

		ForthClassVocabulary* pOPairClass = pEngine->AddBuiltinClass("OPair", gpOIterableClassVocab, oPairMembers);
		gpPairIterClassVocab = pEngine->AddBuiltinClass("OPairIter", gpOIterClassVocab, oPairIterMembers);

		ForthClassVocabulary* pOTripleClass = pEngine->AddBuiltinClass("OTriple", gpOIterableClassVocab, oTripleMembers);
		gpTripleIterClassVocab = pEngine->AddBuiltinClass("OTripleIter", gpOIterClassVocab, oTripleIterMembers);
	}

} // namespace OArray
