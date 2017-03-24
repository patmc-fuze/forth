//////////////////////////////////////////////////////////////////////
//
// OList.cpp: builtin list related classes
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

#include "OList.h"
#include "OArray.h"

extern "C" {
	extern void unimplementedMethodOp(ForthCoreState *pCore);
	extern void illegalMethodOp(ForthCoreState *pCore);
};

namespace OList
{

	//////////////////////////////////////////////////////////////////////
	///
	//                 oList
	//

	FORTHOP(oListNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oListStruct, pList, pClassVocab);
		pList->refCount = 0;
		pList->head = NULL;
		pList->tail = NULL;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pList);
	}

	FORTHOP(oListDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		while (pCur != NULL)
		{
			oListElement* pNext = pCur->next;
			SAFE_RELEASE(pCore, pCur->obj);
			FREE_LINK(pCur);
			pCur = pNext;
		}
		FREE_OBJECT(pList);
		METHOD_RETURN;
	}

	FORTHOP(oListShowMethod)
	{
		EXIT_IF_OBJECT_ALREADY_SHOWN;
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER;
		pShowContext->ShowIndent("'elements' : [");
		if (pCur != NULL)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			while (pCur != NULL)
			{
				oListElement* pNext = pCur->next;
				if (pCur != pList->head)
				{
					pShowContext->EndElement(",");
				}
				pShowContext->ShowIndent();
				ForthShowObject(pCur->obj, pCore);
				pCur = pNext;
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

	FORTHOP(oListIsEmptyMethod)
	{
		GET_THIS(oListStruct, pList);
		int result = 0;
		if (pList->head == NULL)
		{
			ASSERT(pList->tail == NULL);
			result = ~0;
		}
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oListHeadMethod)
	{
		GET_THIS(oListStruct, pList);
		if (pList->head == NULL)
		{
			ASSERT(pList->tail == NULL);
			PUSH_PAIR(NULL, NULL);
		}
		else
		{
			PUSH_OBJECT(pList->head->obj);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListTailMethod)
	{
		GET_THIS(oListStruct, pList);
		if (pList->tail == NULL)
		{
			ASSERT(pList->head == NULL);
			PUSH_PAIR(NULL, NULL);
		}
		else
		{
			PUSH_OBJECT(pList->tail->obj);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListAddHeadMethod)
	{
		GET_THIS(oListStruct, pList);
		MALLOCATE_LINK(oListElement, newElem);
		POP_OBJECT(newElem->obj);
		SAFE_KEEP(newElem->obj);
		newElem->prev = NULL;
		oListElement* oldHead = pList->head;
		if (oldHead == NULL)
		{
			ASSERT(pList->tail == NULL);
			pList->tail = newElem;
		}
		else
		{
			ASSERT(oldHead->prev == NULL);
			oldHead->prev = newElem;
		}
		newElem->next = oldHead;
		pList->head = newElem;
		METHOD_RETURN;
	}

	FORTHOP(oListAddTailMethod)
	{
		GET_THIS(oListStruct, pList);
		MALLOCATE_LINK(oListElement, newElem);
		POP_OBJECT(newElem->obj);
		SAFE_KEEP(newElem->obj);
		newElem->next = NULL;
		oListElement* oldTail = pList->tail;
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
		pList->tail = newElem;
		METHOD_RETURN;
	}

	FORTHOP(oListRemoveHeadMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* oldHead = pList->head;
		if (oldHead != NULL)
		{
			ForthObject& obj = oldHead->obj;
			if (oldHead == pList->tail)
			{
				// removed only element in list
				pList->head = NULL;
				pList->tail = NULL;
			}
			else
			{
				ASSERT(oldHead->prev == NULL);
				oListElement* newHead = oldHead->next;
				ASSERT(newHead != NULL);
				newHead->prev = NULL;
				pList->head = newHead;
			}
			SAFE_RELEASE(pCore, obj);
			FREE_LINK(oldHead);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListRemoveTailMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* oldTail = pList->tail;
		if (oldTail != NULL)
		{
			ForthObject& obj = oldTail->obj;
			if (pList->head == oldTail)
			{
				// removed only element in list
				pList->head = NULL;
				pList->tail = NULL;
			}
			else
			{
				ASSERT(oldTail->next == NULL);
				oListElement* newTail = oldTail->prev;
				ASSERT(newTail != NULL);
				newTail->next = NULL;
				pList->tail = newTail;
			}
			SAFE_RELEASE(pCore, obj);
			FREE_LINK(oldTail);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListUnrefHeadMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* oldHead = pList->head;
		if (oldHead == NULL)
		{
			ASSERT(pList->tail == NULL);
			PUSH_PAIR(NULL, NULL);
		}
		else
		{
			ForthObject& obj = oldHead->obj;
			if (oldHead == pList->tail)
			{
				// removed only element in list
				pList->head = NULL;
				pList->tail = NULL;
			}
			else
			{
				ASSERT(oldHead->prev == NULL);
				oListElement* newHead = oldHead->next;
				ASSERT(newHead != NULL);
				newHead->prev = NULL;
				pList->head = newHead;
			}
			unrefObject(obj);
			PUSH_OBJECT(obj);
			FREE_LINK(oldHead);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListUnrefTailMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* oldTail = pList->tail;
		if (oldTail == NULL)
		{
			ASSERT(pList->head == NULL);
			PUSH_PAIR(NULL, NULL);
		}
		else
		{
			ForthObject& obj = oldTail->obj;
			if (pList->head == oldTail)
			{
				// removed only element in list
				pList->head = NULL;
				pList->tail = NULL;
			}
			else
			{
				ASSERT(oldTail->next == NULL);
				oListElement* newTail = oldTail->prev;
				ASSERT(newTail != NULL);
				newTail->next = NULL;
				pList->tail = newTail;
			}
			unrefObject(obj);
			PUSH_OBJECT(obj);
			FREE_LINK(oldTail);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListHeadIterMethod)
	{
		GET_THIS(oListStruct, pList);
		pList->refCount++;
		TRACK_KEEP;
		ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIListIter);
		MALLOCATE_ITER(oListIterStruct, pIter, pIterVocab);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pList);
		pIter->cursor = pList->head;
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIListIter, 0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oListTailIterMethod)
	{
		GET_THIS(oListStruct, pList);
		pList->refCount++;
		TRACK_KEEP;
		ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIListIter);
		MALLOCATE_ITER(oListIterStruct, pIter, pIterVocab);
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pList);
		pIter->cursor = NULL;
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIListIter, 0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oListFindMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		while (pCur != NULL)
		{
			ForthObject& o = pCur->obj;
			if (OBJECTS_SAME(o, soughtObj))
			{
				break;
			}
			pCur = pCur->next;
		}
		if (pCur == NULL)
		{
			SPUSH(0);
		}
		else
		{
			pList->refCount++;
			TRACK_KEEP;
			ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIListIter);
			MALLOCATE_ITER(oListIterStruct, pIter, pIterVocab);
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pList);
			pIter->cursor = pCur;
			ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIListIter, 0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListCountMethod)
	{
		GET_THIS(oListStruct, pList);
		long count = 0;
		oListElement* pCur = pList->head;
		while (pCur != NULL)
		{
			count++;
			pCur = pCur->next;
		}
		SPUSH(count);
		METHOD_RETURN;
	}

	FORTHOP(oListClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		while (pCur != NULL)
		{
			oListElement* pNext = pCur->next;
			SAFE_RELEASE(pCore, pCur->obj);
			FREE_LINK(pCur);
			pCur = pNext;
		}
		pList->head = NULL;
		pList->tail = NULL;
		METHOD_RETURN;
	}

	FORTHOP(oListLoadMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		while (pCur != NULL)
		{
			oListElement* pNext = pCur->next;
			SAFE_RELEASE(pCore, pCur->obj);
			FREE_LINK(pCur);
			pCur = pNext;
		}
		pList->head = NULL;
		pList->tail = NULL;

		int n = SPOP;
		for (int i = 0; i < n; i++)
		{
			MALLOCATE_LINK(oListElement, newElem);
			POP_OBJECT(newElem->obj);
			SAFE_KEEP(newElem->obj);
			newElem->next = NULL;
			oListElement* oldTail = pList->tail;
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
			pList->tail = newElem;
		}
		METHOD_RETURN;
	}

	FORTHOP(oListToArrayMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		ForthClassVocabulary *pArrayVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIArray);
		MALLOCATE_OBJECT(oArrayStruct, pArray, pArrayVocab);
		pArray->refCount = 0;
		pArray->elements = new oArray;

		while (pCur != NULL)
		{
			ForthObject& o = pCur->obj;
			SAFE_KEEP(o);
			pArray->elements->push_back(o);
			pCur = pCur->next;
		}

		// push array on TOS
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIArray, 0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pArray);
		METHOD_RETURN;
	}

	/*


	Cut( oListIter start, oListIter end )
	remove a sublist from this list
	end is one past last element to delete, end==NULL deletes to end of list
	Splice( oList soList, listElement* insertBefore
	insert soList into this list before position specified by insertBefore
	if insertBefore is null, insert at tail of list
	after splice, soList is empty

	? replace an elements object
	? isEmpty
	*/

	FORTHOP(oListCloneMethod)
	{
		// create an empty list
		ForthClassVocabulary *pListVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIList);
		MALLOCATE_OBJECT(oListStruct, pCloneList, pListVocab);
		pCloneList->refCount = 0;
		pCloneList->head = NULL;
		pCloneList->tail = NULL;
		// go through all elements and add a reference to any which are not null and add to clone list
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		oListElement* oldTail = NULL;
		while (pCur != NULL)
		{
			oListElement* pNext = pCur->next;
			MALLOCATE_LINK(oListElement, newElem);
			newElem->obj = pCur->obj;
			SAFE_KEEP(newElem->obj);
			if (oldTail == NULL)
			{
				pCloneList->head = newElem;
			}
			else
			{
				oldTail->next = newElem;
			}
			newElem->prev = oldTail;
			oldTail = newElem;
			pCur = pNext;
		}
		if (oldTail != NULL)
		{
			oldTail->next = NULL;
		}
		PUSH_PAIR(GET_TPM, pCloneList);
		METHOD_RETURN;
	}

	FORTHOP(oListRemoveMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		while (pCur != NULL)
		{
			ForthObject& o = pCur->obj;
			if (OBJECTS_SAME(o, soughtObj))
			{
				break;
			}
			pCur = pCur->next;
		}
		if (pCur != NULL)
		{
			oListElement* pPrev = pCur->prev;
			oListElement* pNext = pCur->next;
			if (pCur == pList->head)
			{
				pList->head = pNext;
			}
			if (pCur == pList->tail)
			{
				pList->tail = pPrev;
			}
			if (pNext != NULL)
			{
				pNext->prev = pPrev;
			}
			if (pPrev != NULL)
			{
				pPrev->next = pNext;
			}
			SAFE_RELEASE(pCore, pCur->obj);
			FREE_LINK(pCur);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oListMembers[] =
	{
		METHOD("__newOp", oListNew),
		METHOD("delete", oListDeleteMethod),
		METHOD("show", oListShowMethod),

		METHOD_RET("headIter", oListHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIListIter)),
		METHOD_RET("tailIter", oListTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIListIter)),
		METHOD_RET("find", oListFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIListIter)),
		METHOD_RET("clone", oListCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIList)),
		METHOD_RET("count", oListCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oListClearMethod),
		METHOD("load", oListLoadMethod),
		METHOD_RET("toArray", oListToArrayMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArray)),

		METHOD_RET("isEmpty", oListIsEmptyMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("head", oListHeadMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject)),
		METHOD_RET("tail", oListTailMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject)),
		METHOD("addHead", oListAddHeadMethod),
		METHOD("addTail", oListAddTailMethod),
		METHOD("removeHead", oListRemoveHeadMethod),
		METHOD("removeTail", oListRemoveTailMethod),
		METHOD("unrefHead", oListUnrefHeadMethod),
		METHOD("unrefTail", oListUnrefTailMethod),
		METHOD("remove", oListRemoveMethod),

		MEMBER_VAR("__head", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("__tail", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oListIter
	//

	FORTHOP(oListIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oListIter object");
	}

	FORTHOP(oListIterDeleteMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oListIterShowMethod)
	{
		EXIT_IF_OBJECT_ALREADY_SHOWN;
		GET_THIS(oListIterStruct, pIter);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER;
		pShowContext->ShowIndent("'cursor : ");
		ForthShowObject(pIter->cursor->obj, pCore);
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

	FORTHOP(oListIterSeekNextMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		if (pIter->cursor != NULL)
		{
			pIter->cursor = pIter->cursor->next;
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterSeekPrevMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		if (pIter->cursor != NULL)
		{
			pIter->cursor = pIter->cursor->prev;
		}
		else
		{
			pIter->cursor = reinterpret_cast<oListStruct *>(pIter->parent.pData)->tail;
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterSwapNextMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		oListElement* pCursor = pIter->cursor;
		if (pCursor != NULL)
		{
			oListElement* pNext = pCursor->next;
			if (pNext != NULL)
			{
				ForthObject nextObj = pNext->obj;
				ForthObject obj = pCursor->obj;
				pCursor->obj = nextObj;
				pNext->obj = obj;
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterSwapPrevMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		oListElement* pCursor = pIter->cursor;
		if (pCursor != NULL)
		{
			oListElement* pPrev = pCursor->prev;
			if (pPrev != NULL)
			{
				ForthObject prevObj = pPrev->obj;
				ForthObject obj = pCursor->obj;
				pCursor->obj = prevObj;
				pPrev->obj = obj;
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterSeekHeadMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		pIter->cursor = reinterpret_cast<oListStruct *>(pIter->parent.pData)->head;
		METHOD_RETURN;
	}

	FORTHOP(oListIterSeekTailMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		pIter->cursor = NULL;
		METHOD_RETURN;
	}

	FORTHOP(oListIterNextMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		if (pIter->cursor == NULL)
		{
			SPUSH(0);
		}
		else
		{
			PUSH_OBJECT(pIter->cursor->obj);
			pIter->cursor = pIter->cursor->next;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterPrevMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		// special case: NULL cursor means tail of list
		if (pIter->cursor == NULL)
		{
			pIter->cursor = reinterpret_cast<oListStruct *>(pIter->parent.pData)->tail;
		}
		else
		{
			pIter->cursor = pIter->cursor->prev;
		}
		if (pIter->cursor == NULL)
		{
			SPUSH(0);
		}
		else
		{
			PUSH_OBJECT(pIter->cursor->obj);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterCurrentMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		if (pIter->cursor == NULL)
		{
			SPUSH(0);
		}
		else
		{
			PUSH_OBJECT(pIter->cursor->obj);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterRemoveMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		oListElement* pCur = pIter->cursor;
		if (pCur != NULL)
		{
			oListStruct* pList = reinterpret_cast<oListStruct *>(pIter->parent.pData);
			oListElement* pPrev = pCur->prev;
			oListElement* pNext = pCur->next;
			if (pCur == pList->head)
			{
				pList->head = pNext;
			}
			if (pCur == pList->tail)
			{
				pList->tail = pPrev;
			}
			if (pNext != NULL)
			{
				pNext->prev = pPrev;
			}
			if (pPrev != NULL)
			{
				pPrev->next = pNext;
			}
			pIter->cursor = pNext;
			SAFE_RELEASE(pCore, pCur->obj);
			FREE_LINK(pCur);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterUnrefMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		oListElement* pCur = pIter->cursor;
		if (pCur != NULL)
		{
			oListStruct* pList = reinterpret_cast<oListStruct *>(pIter->parent.pData);
			oListElement* pPrev = pCur->prev;
			oListElement* pNext = pCur->next;
			if (pCur == pList->head)
			{
				pList->head = pNext;
			}
			if (pCur == pList->tail)
			{
				pList->tail = pPrev;
			}
			if (pNext != NULL)
			{
				pNext->prev = pPrev;
			}
			if (pPrev != NULL)
			{
				pPrev->next = pNext;
			}
			pIter->cursor = pNext;
			PUSH_OBJECT(pCur->obj);
			unrefObject(pCur->obj);
			FREE_LINK(pCur);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterFindNextMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		oListStruct* pList = reinterpret_cast<oListStruct *>(pIter->parent.pData);
		oListElement* pCur = pIter->cursor;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		if (pCur != NULL)
		{
			pCur = pCur->next;
		}
		while (pCur != NULL)
		{
			ForthObject& o = pCur->obj;
			if (OBJECTS_SAME(o, soughtObj))
			{
				break;
			}
			pCur = pCur->next;
		}
		if (pCur == NULL)
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor = pCur;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterSplitMethod)
	{
		GET_THIS(oListIterStruct, pIter);

		// create an empty list
		ForthClassVocabulary *pListVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIList);
		MALLOCATE_OBJECT(oListStruct, pNewList, pListVocab);
		pNewList->refCount = 0;
		pNewList->head = NULL;
		pNewList->tail = NULL;

		oListElement* pCursor = pIter->cursor;
		oListStruct* pOldList = reinterpret_cast<oListStruct *>(pIter->parent.pData);
		// if pCursor is NULL, iter cursor is past tail, new list is just empty list, leave old list alone
		if (pCursor != NULL)
		{
			if (pCursor == pOldList->head)
			{
				// iter cursor is start of list, make old list empty, new list is entire old list
				pNewList->head = pOldList->head;
				pNewList->tail = pOldList->tail;
				pOldList->head = NULL;
				pOldList->tail = NULL;
			}
			else
			{
				// head of new list is iter cursor
				pNewList->head = pCursor;
				pNewList->tail = pOldList->tail;
				// fix old list tail
				pOldList->tail = pNewList->head->prev;
				pOldList->tail->next = NULL;
			}
		}

		// split leaves iter cursor past tail
		pIter->cursor = NULL;

		PUSH_PAIR(pIter->parent.pMethodOps, pNewList);
		METHOD_RETURN;
	}

	baseMethodEntry oListIterMembers[] =
	{
		METHOD("__newOp", oListIterNew),
		METHOD("delete", oListIterDeleteMethod),

		METHOD("seekNext", oListIterSeekNextMethod),
		METHOD("seekPrev", oListIterSeekPrevMethod),
		METHOD("seekHead", oListIterSeekHeadMethod),
		METHOD("seekTail", oListIterSeekTailMethod),
		METHOD_RET("next", oListIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oListIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oListIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oListIterRemoveMethod),
		METHOD("unref", oListIterUnrefMethod),
		METHOD_RET("findNext", oListIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		//METHOD_RET( "clone",                oListIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIListIter) ),

		METHOD("swapNext", oListIterSwapNextMethod),
		METHOD("swapPrev", oListIterSwapPrevMethod),
		METHOD("split", oListIterSplitMethod),


		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIList)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		pEngine->AddBuiltinClass("OList", kBCIList, kBCIIterable, oListMembers);
		pEngine->AddBuiltinClass("OListIter", kBCIListIter, kBCIIter, oListIterMembers);
	}

} // namespace OList
