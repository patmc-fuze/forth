//////////////////////////////////////////////////////////////////////
//
// OMap.cpp: builtin map related classes
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

#include "OMap.h"

extern "C" {
	extern void unimplementedMethodOp(ForthCoreState *pCore);
	extern void illegalMethodOp(ForthCoreState *pCore);
};

namespace OMap
{

	//////////////////////////////////////////////////////////////////////
	///
	//                 oMap
	//

	typedef std::map<long long, ForthObject> oMap;
	struct oMapStruct
	{
		ulong       refCount;
		oMap*	elements;
	};

	struct oMapIterStruct
	{
		ulong				refCount;
		ForthObject			parent;
		oMap::iterator*		cursor;
	};

	static ForthClassVocabulary* gpMapIterClassVocab = NULL;


	FORTHOP(oMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oMapStruct, pMap);
		pMap->refCount = 0;
		pMap->elements = new oMap;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pMap);
	}

	FORTHOP(oMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			stackInt64 key;
			key.s64 = iter->first;
			SAFE_RELEASE(pCore, key.obj);
			ForthObject& val = iter->second;
			SAFE_RELEASE(pCore, val);
		}
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oMapShowMethod)
	{
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER("oMap");
		pShowContext->ShowIndent("'map' : {");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
				stackInt64 key;
				key.s64 = iter->first;
				if (iter != a.begin())
				{
					pShowContext->EndElement(",");
				}
				pShowContext->ShowIndent();
				ForthShowObject(key.obj, pCore);
				pEngine->ConsoleOut(" : ");
				ForthShowObject(iter->second, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			stackInt64 key;
			key.s64 = iter->first;
			SAFE_RELEASE(pCore, key.obj);
			ForthObject& val = iter->second;
			SAFE_RELEASE(pCore, val);
		}
		a.clear();
		METHOD_RETURN;
	}


	FORTHOP(oMapLoadMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			stackInt64 key;
			key.s64 = iter->first;
			SAFE_RELEASE(pCore, key.obj);
			ForthObject& val = iter->second;
			SAFE_RELEASE(pCore, val);
		}
		a.clear();
		int n = SPOP;
		for (int i = 0; i < n; i++)
		{
			stackInt64 key;
			LPOP(key);
			ForthObject newObj;
			POP_OBJECT(newObj);
			if (newObj.pMethodOps != NULL)
			{
				SAFE_KEEP(newObj);
			}
			a[key.s64] = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapCountMethod)
	{
		GET_THIS(oMapStruct, pMap);
		SPUSH((long)(pMap->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oMapGetMethod)
	{
		GET_THIS(oMapStruct, pMap);
		oMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			ForthObject fobj = iter->second;
			PUSH_OBJECT(fobj);
		}
		else
		{
			PUSH_PAIR(0, 0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapSetMethod)
	{
		GET_THIS(oMapStruct, pMap);
		oMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		ForthObject newObj;
		POP_OBJECT(newObj);
		oMap::iterator iter = a.find(key.s64);
		if (newObj.pMethodOps != NULL)
		{
			if (iter != a.end())
			{
				ForthObject oldObj = iter->second;
				if (OBJECTS_DIFFERENT(oldObj, newObj))
				{
					SAFE_KEEP(newObj);
					SAFE_RELEASE(pCore, oldObj);
				}
			}
			else
			{
				SAFE_KEEP(newObj);
			}
			a[key.s64] = newObj;
		}
		else
		{
			// remove element associated with key from map
			if (iter != a.end())
			{
				stackInt64 key;
				key.s64 = iter->first;
				SAFE_RELEASE(pCore, key.obj);
				ForthObject& val = iter->second;
				SAFE_RELEASE(pCore, val);
				a.erase(iter);
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapFindKeyMethod)
	{
		GET_THIS(oMapStruct, pMap);
		stackInt64 retVal;
		retVal.s64 = 0L;
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = 1;
				retVal.s64 = iter->first;
				break;
			}
		}
		LPUSH(retVal);
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oMapRemoveMethod)
	{
		GET_THIS(oMapStruct, pMap);
		oMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			stackInt64 key;
			key.s64 = iter->first;
			SAFE_RELEASE(pCore, key.obj);
			ForthObject& val = iter->second;
			SAFE_RELEASE(pCore, val);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oMapStruct, pMap);
		oMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			stackInt64 key;
			key.s64 = iter->first;
			unrefObject(key.obj);
			ForthObject& val = iter->second;
			unrefObject(val);
			PUSH_OBJECT(val);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapHeadIterMethod)
	{
		GET_THIS(oMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oMapIterStruct* pIter = new oMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = new oMap::iterator;
		*(pIter->cursor) = pMap->elements->begin();
		ForthInterface* pPrimaryInterface = gpMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oMapTailIterMethod)
	{
		GET_THIS(oMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oMapIterStruct* pIter = new oMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = new oMap::iterator;
		*(pIter->cursor) = pMap->elements->end();
		ForthInterface* pPrimaryInterface = gpMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oMapFindMethod)
	{
		GET_THIS(oMapStruct, pMap);
		bool found = false;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = true;
				break;
			}
		}
		if (found)
		{
			pMap->refCount++;
			TRACK_KEEP;
			// needed to use new instead of malloc otherwise the iterator isn't setup right and
			//   a crash happens when you assign to it
			oMapIterStruct* pIter = new oMapIterStruct;
			TRACK_ITER_NEW;
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pMap);
			pIter->cursor = new oMap::iterator;
			*(pIter->cursor) = iter;
			ForthInterface* pPrimaryInterface = gpMapIterClassVocab->GetInterface(0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		else
		{
			SPUSH(0);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oMapMembers[] =
	{
		METHOD("__newOp", oMapNew),
		METHOD("delete", oMapDeleteMethod),
		METHOD("show", oMapShowMethod),

		METHOD_RET("headIter", oMapHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("tailIter", oMapTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("find", oMapFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		//METHOD_RET( "clone",                oMapCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMap) ),
		METHOD_RET("count", oMapCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oMapClearMethod),
		METHOD("load", oMapLoadMethod),

		METHOD_RET("get", oMapGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject)),
		METHOD("set", oMapSetMethod),
		METHOD_RET("findKey", oMapFindKeyMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oMapRemoveMethod),
		METHOD("unref", oMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oMapIter
	//

	FORTHOP(oMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oMapIter object");
	}

	FORTHOP(oMapIterDeleteMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekNextMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekPrevMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekHeadMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekTailMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

	FORTHOP(oMapIterNextMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterPrevMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterCurrentMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterRemoveMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) != pMap->elements->end())
		{
			stackInt64 key;
			key.s64 = (*pIter->cursor)->first;
			SAFE_RELEASE(pCore, key.obj);
			ForthObject& o = (*pIter->cursor)->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterFindNextMethod)
	{
		SPUSH(0);
		METHOD_RETURN;
	}

	FORTHOP(oMapIterNextPairMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*pIter->cursor)->second;
			PUSH_OBJECT(o);
			stackInt64 key;
			key.s64 = (*pIter->cursor)->first;
			LPUSH(key);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterPrevPairMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			ForthObject& o = (*pIter->cursor)->second;
			PUSH_OBJECT(o);
			stackInt64 key;
			key.s64 = (*pIter->cursor)->first;
			LPUSH(key);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oMapIterMembers[] =
	{
		METHOD("__newOp", oMapIterNew),
		METHOD("delete", oMapIterDeleteMethod),

		METHOD("seekNext", oMapIterSeekNextMethod),
		METHOD("seekPrev", oMapIterSeekPrevMethod),
		METHOD("seekHead", oMapIterSeekHeadMethod),
		METHOD("seekTail", oMapIterSeekTailMethod),
		METHOD_RET("next", oMapIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oMapIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oMapIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oMapIterRemoveMethod),
		METHOD_RET("findNext", oMapIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		//METHOD_RET( "clone",                oMapIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),

		METHOD_RET("nextPair", oMapIterNextPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prevPair", oMapIterPrevPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),

		MEMBER_VAR("parent", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 oIntMap
	//

	typedef std::map<long, ForthObject> oIntMap;
	struct oIntMapStruct
	{
		ulong       refCount;
		oIntMap*		elements;
	};

	struct oIntMapIterStruct
	{
		ulong				refCount;
		ForthObject			parent;
		oIntMap::iterator	*cursor;
	};
	static ForthClassVocabulary* gpIntMapIterClassVocab = NULL;



	FORTHOP(oIntMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oIntMapStruct, pMap);
		pMap->refCount = 0;
		pMap->elements = new oIntMap;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pMap);
	}

	FORTHOP(oIntMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oIntMapStruct, pMap);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oIntMapShowMethod)
	{
		char buffer[20];
		GET_THIS(oIntMapStruct, pMap);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER("oIntMap");
		pShowContext->ShowIndent("'map' : {");
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
				sprintf(buffer, "%d : ", iter->first);
				pShowContext->ShowIndent(buffer);
				ForthShowObject(iter->second, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oIntMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oIntMapStruct, pMap);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oIntMapLoadMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oIntMapStruct, pMap);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		int n = SPOP;
		for (int i = 0; i < n; i++)
		{
			long key = SPOP;
			ForthObject newObj;
			POP_OBJECT(newObj);
			if (newObj.pMethodOps != NULL)
			{
				SAFE_KEEP(newObj);
			}
			a[key] = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapCountMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
		SPUSH((long)(pMap->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oIntMapGetMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
		oIntMap& a = *(pMap->elements);
		long key = SPOP;
		oIntMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject fobj = iter->second;
			PUSH_OBJECT(fobj);
		}
		else
		{
			PUSH_PAIR(0, 0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapSetMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
		oIntMap& a = *(pMap->elements);
		long key = SPOP;
		ForthObject newObj;
		POP_OBJECT(newObj);
		oIntMap::iterator iter = a.find(key);
		if (newObj.pMethodOps != NULL)
		{
			if (iter != a.end())
			{
				ForthObject oldObj = iter->second;
				if (OBJECTS_DIFFERENT(oldObj, newObj))
				{
					SAFE_KEEP(newObj);
					SAFE_RELEASE(pCore, oldObj);
				}
			}
			else
			{
				SAFE_KEEP(newObj);
			}
			a[key] = newObj;
		}
		else
		{
			// remove element associated with key from map
			if (iter != a.end())
			{
				ForthObject& oldObj = iter->second;
				SAFE_RELEASE(pCore, oldObj);
				a.erase(iter);
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapFindKeyMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
		long retVal = -1;
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = 1;
				retVal = iter->first;
				break;
			}
		}
		SPUSH(retVal);
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oIntMapRemoveMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
		oIntMap& a = *(pMap->elements);
		long key = SPOP;
		oIntMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& oldObj = iter->second;
			SAFE_RELEASE(pCore, oldObj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oIntMapStruct, pMap);
		oIntMap& a = *(pMap->elements);
		long key = SPOP;
		oIntMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& fobj = iter->second;
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapHeadIterMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oIntMapIterStruct* pIter = new oIntMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		oIntMap::iterator iter = pMap->elements->begin();
		pIter->cursor = new oIntMap::iterator;
		*(pIter->cursor) = pMap->elements->begin();
		ForthInterface* pPrimaryInterface = gpIntMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oIntMapTailIterMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oIntMapIterStruct* pIter = new oIntMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = new oIntMap::iterator;
		*(pIter->cursor) = pMap->elements->end();
		ForthInterface* pPrimaryInterface = gpIntMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oIntMapFindMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
		bool found = false;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = true;
				break;
			}
		}
		if (found)
		{
			pMap->refCount++;
			TRACK_KEEP;
			// needed to use new instead of malloc otherwise the iterator isn't setup right and
			//   a crash happens when you assign to it
			oIntMapIterStruct* pIter = new oIntMapIterStruct;
			TRACK_ITER_NEW;
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pMap);
			pIter->cursor = new oIntMap::iterator;
			*(pIter->cursor) = iter;
			ForthInterface* pPrimaryInterface = gpIntMapIterClassVocab->GetInterface(0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		else
		{
			SPUSH(0);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oIntMapMembers[] =
	{
		METHOD("__newOp", oIntMapNew),
		METHOD("delete", oIntMapDeleteMethod),
		METHOD("show", oIntMapShowMethod),

		METHOD_RET("headIter", oIntMapHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("tailIter", oIntMapTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("find", oIntMapFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		//METHOD_RET( "clone",                oIntMapCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMap) ),
		METHOD_RET("count", oIntMapCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oIntMapClearMethod),
		METHOD("load", oIntMapLoadMethod),

		METHOD_RET("get", oIntMapGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject)),
		METHOD("set", oIntMapSetMethod),
		METHOD_RET("findKey", oIntMapFindKeyMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oIntMapRemoveMethod),
		METHOD("unref", oIntMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oIntMapIter
	//

	FORTHOP(oIntMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oIntMapIter object");
	}

	FORTHOP(oIntMapIterDeleteMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterSeekNextMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterSeekPrevMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterSeekHeadMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterSeekTailMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterNextMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterPrevMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterCurrentMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterRemoveMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) != pMap->elements->end())
		{
			ForthObject& o = (*pIter->cursor)->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterNextPairMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*pIter->cursor)->second;
			PUSH_OBJECT(o);
			SPUSH((*pIter->cursor)->first);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterPrevPairMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			ForthObject& o = (*pIter->cursor)->second;
			PUSH_OBJECT(o);
			SPUSH((*pIter->cursor)->first);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oIntMapIterMembers[] =
	{
		METHOD("__newOp", oIntMapIterNew),
		METHOD("delete", oIntMapIterDeleteMethod),

		METHOD("seekNext", oIntMapIterSeekNextMethod),
		METHOD("seekPrev", oIntMapIterSeekPrevMethod),
		METHOD("seekHead", oIntMapIterSeekHeadMethod),
		METHOD("seekTail", oIntMapIterSeekTailMethod),
		METHOD_RET("next", oIntMapIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oIntMapIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oIntMapIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oIntMapIterRemoveMethod),
		METHOD_RET("findNext", oMapIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		//METHOD_RET( "clone",                oIntMapIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),

		METHOD_RET("nextPair", oIntMapIterNextPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prevPair", oIntMapIterPrevPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),

		MEMBER_VAR("parent", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 oFloatMap
	//
	static ForthClassVocabulary* gpFloatMapIterClassVocab = NULL;

	FORTHOP(oFloatMapShowMethod)
	{
		char buffer[20];
		GET_THIS(oIntMapStruct, pMap);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER("oFloatMap");
		pShowContext->ShowIndent("'map' : {");
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
				float fval = *((float *)(&(iter->first)));
				sprintf(buffer, "%f : ", fval);
				pShowContext->ShowIndent(buffer);
				ForthShowObject(iter->second, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}


	baseMethodEntry oFloatMapMembers[] =
	{
		METHOD("__newOp", oIntMapNew),
		METHOD("show", oFloatMapShowMethod),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oLongMap
	//

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

	static ForthClassVocabulary* gpLongMapIterClassVocab = NULL;


	FORTHOP(oLongMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oLongMapStruct, pMap);
		pMap->refCount = 0;
		pMap->elements = new oLongMap;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pMap);
	}

	FORTHOP(oLongMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oLongMapShowMethod)
	{
		char buffer[20];
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER("oLongMap");
		pShowContext->ShowIndent("'map' : {");
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
				sprintf(buffer, "%lld : ", iter->first);
				pShowContext->ShowIndent(buffer);
				ForthShowObject(iter->second, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oLongMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oLongMapLoadMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		int n = SPOP;
		for (int i = 0; i < n; i++)
		{
			stackInt64 key;
			LPOP(key);
			ForthObject newObj;
			POP_OBJECT(newObj);
			if (newObj.pMethodOps != NULL)
			{
				SAFE_KEEP(newObj);
			}
			a[key.s64] = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapCountMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		SPUSH((long)(pMap->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oLongMapGetMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		oLongMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oLongMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			ForthObject fobj = iter->second;
			PUSH_OBJECT(fobj);
		}
		else
		{
			PUSH_PAIR(0, 0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapSetMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		oLongMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		ForthObject newObj;
		POP_OBJECT(newObj);
		oLongMap::iterator iter = a.find(key.s64);
		if (newObj.pMethodOps != NULL)
		{
			if (iter != a.end())
			{
				ForthObject oldObj = iter->second;
				if (OBJECTS_DIFFERENT(oldObj, newObj))
				{
					SAFE_KEEP(newObj);
					SAFE_RELEASE(pCore, oldObj);
				}
			}
			else
			{
				SAFE_KEEP(newObj);
			}
			a[key.s64] = newObj;
		}
		else
		{
			// remove element associated with key from map
			if (iter != a.end())
			{
				ForthObject& oldObj = iter->second;
				SAFE_RELEASE(pCore, oldObj);
				a.erase(iter);
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapFindKeyMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		stackInt64 retVal;
		retVal.s64 = -1L;
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = 1;
				retVal.s64 = iter->first;
				break;
			}
		}
		LPUSH(retVal);
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oLongMapRemoveMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		oLongMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oLongMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			ForthObject& oldObj = iter->second;
			SAFE_RELEASE(pCore, oldObj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oLongMapStruct, pMap);
		oLongMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oLongMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			ForthObject& fobj = iter->second;
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapHeadIterMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oLongMapIterStruct* pIter = new oLongMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = new oLongMap::iterator;
		*(pIter->cursor) = pMap->elements->begin();
		ForthInterface* pPrimaryInterface = gpLongMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oLongMapTailIterMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oLongMapIterStruct* pIter = new oLongMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = new oLongMap::iterator;
		*(pIter->cursor) = pMap->elements->end();
		ForthInterface* pPrimaryInterface = gpLongMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oLongMapFindMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		bool found = false;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = true;
				break;
			}
		}
		if (found)
		{
			pMap->refCount++;
			TRACK_KEEP;
			// needed to use new instead of malloc otherwise the iterator isn't setup right and
			//   a crash happens when you assign to it
			oLongMapIterStruct* pIter = new oLongMapIterStruct;
			TRACK_ITER_NEW;
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pMap);
			pIter->cursor = new oLongMap::iterator;
			*(pIter->cursor) = iter;
			ForthInterface* pPrimaryInterface = gpLongMapIterClassVocab->GetInterface(0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		else
		{
			SPUSH(0);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oLongMapMembers[] =
	{
		METHOD("__newOp", oLongMapNew),
		METHOD("delete", oLongMapDeleteMethod),
		METHOD("show", oLongMapShowMethod),

		METHOD_RET("headIter", oLongMapHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("tailIter", oLongMapTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("find", oLongMapFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		//METHOD_RET( "clone",                oLongMapCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMap) ),
		METHOD_RET("count", oLongMapCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oLongMapClearMethod),
		METHOD("load", oLongMapLoadMethod),

		METHOD_RET("get", oLongMapGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject)),
		METHOD("set", oLongMapSetMethod),
		METHOD_RET("findKey", oLongMapFindKeyMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oLongMapRemoveMethod),
		METHOD("unref", oLongMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oLongMapIter
	//

	FORTHOP(oLongMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oLongMapIter object");
	}

	FORTHOP(oLongMapIterDeleteMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekNextMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekPrevMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekHeadMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekTailMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterNextMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterPrevMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterCurrentMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterRemoveMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) != pMap->elements->end())
		{
			ForthObject& o = (*pIter->cursor)->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterNextPairMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*pIter->cursor)->second;
			PUSH_OBJECT(o);
			stackInt64 key;
			key.s64 = (*pIter->cursor)->first;
			LPUSH(key);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterPrevPairMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			ForthObject& o = (*pIter->cursor)->second;
			PUSH_OBJECT(o);
			stackInt64 key;
			key.s64 = (*pIter->cursor)->first;
			LPUSH(key);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oLongMapIterMembers[] =
	{
		METHOD("__newOp", oLongMapIterNew),
		METHOD("delete", oLongMapIterDeleteMethod),

		METHOD("seekNext", oLongMapIterSeekNextMethod),
		METHOD("seekPrev", oLongMapIterSeekPrevMethod),
		METHOD("seekHead", oLongMapIterSeekHeadMethod),
		METHOD("seekTail", oLongMapIterSeekTailMethod),
		METHOD_RET("next", oLongMapIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oLongMapIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oLongMapIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oLongMapIterRemoveMethod),
		METHOD_RET("findNext", oMapIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		//METHOD_RET( "clone",                oLongMapIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),

		METHOD_RET("nextPair", oLongMapIterNextPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prevPair", oLongMapIterPrevPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),

		MEMBER_VAR("parent", NATIVE_TYPE_TO_CODE(0, kBaseTypeObject)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oDoubleMap
	//
	static ForthClassVocabulary* gpDoubleMapIterClassVocab = NULL;

	FORTHOP(oDoubleMapShowMethod)
	{
		char buffer[32];
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER("oDoubleMap");
		pShowContext->ShowIndent("'map' : {");
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
				double dval = *((double *)(&(iter->first)));
				sprintf(buffer, "%g : ", dval);
				pShowContext->ShowIndent(buffer);
				ForthShowObject(iter->second, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}


	baseMethodEntry oDoubleMapMembers[] =
	{
		METHOD("__newOp", oLongMapNew),
		METHOD("show", oDoubleMapShowMethod),
		// following must be last in table
		END_MEMBERS
	};

	void AddClasses(ForthEngine* pEngine)
	{
		ForthClassVocabulary* pOMapClass = pEngine->AddBuiltinClass("OMap", gpOIterableClassVocab, oMapMembers);
		gpMapIterClassVocab = pEngine->AddBuiltinClass("OMapIter", gpOIterClassVocab, oMapIterMembers);

		ForthClassVocabulary* pOIntMapClass = pEngine->AddBuiltinClass("OIntMap", gpOIterableClassVocab, oIntMapMembers);
		gpIntMapIterClassVocab = pEngine->AddBuiltinClass("OIntMapIter", gpOIterClassVocab, oIntMapIterMembers);

		ForthClassVocabulary* pOFloatMapClass = pEngine->AddBuiltinClass("OFloatMap", pOIntMapClass, oFloatMapMembers);
		gpFloatMapIterClassVocab = pEngine->AddBuiltinClass("OFloatMapIter", gpOIterClassVocab, oIntMapIterMembers);

		ForthClassVocabulary* pOLongMapClass = pEngine->AddBuiltinClass("OLongMap", gpOIterableClassVocab, oLongMapMembers);
		gpLongMapIterClassVocab = pEngine->AddBuiltinClass("OLongMapIter", gpOIterClassVocab, oLongMapIterMembers);

		ForthClassVocabulary* pODoubleMapClass = pEngine->AddBuiltinClass("ODoubleMap", pOLongMapClass, oDoubleMapMembers);
		gpDoubleMapIterClassVocab = pEngine->AddBuiltinClass("ODoubleMapIter", gpOIterClassVocab, oLongMapIterMembers);

	}

} // namespace OMap

