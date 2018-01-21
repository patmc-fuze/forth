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
	//                 Map
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


	FORTHOP(oMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oMapStruct, pMap, pClassVocab);
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
		EXIT_IF_OBJECT_ALREADY_SHOWN;
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER;
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
            POP_OBJECT(key.obj);
			ForthObject newObj;
			POP_OBJECT(newObj);
			if (newObj.pMethodOps != NULL)
			{
                SAFE_KEEP(key.obj);
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
        POP_OBJECT(key.obj);
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
        POP_OBJECT(key.obj);
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
                SAFE_KEEP(key.obj);
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
		PUSH_OBJECT(retVal.obj);
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oMapRemoveMethod)
	{
		GET_THIS(oMapStruct, pMap);
		oMap& a = *(pMap->elements);
		stackInt64 key;
        POP_OBJECT(key.obj);
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
        POP_OBJECT(key.obj);
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
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIMapIter, 0);
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
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIMapIter, 0);
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
			ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIMapIter, 0);
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

		METHOD_RET("headIter", oMapHeadIterMethod, RETURNS_OBJECT(kBCIMapIter)),
		METHOD_RET("tailIter", oMapTailIterMethod, RETURNS_OBJECT(kBCIMapIter)),
		METHOD_RET("find", oMapFindMethod, RETURNS_OBJECT(kBCIMapIter)),
		//METHOD_RET( "clone",                oMapCloneMethod, RETURNS_OBJECT(kBCIMap) ),
		METHOD_RET("count", oMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oMapClearMethod),
		METHOD("load", oMapLoadMethod),

        METHOD_RET("get", oMapGetMethod, RETURNS_OBJECT(kBCIContainedType)),
		METHOD("set", oMapSetMethod),
		METHOD_RET("findKey", oMapFindKeyMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oMapRemoveMethod),
		METHOD("unref", oMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 MapIter
	//

	FORTHOP(oMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a MapIter object");
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
			PUSH_OBJECT(key.obj);
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
            PUSH_OBJECT(key.obj);
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
		METHOD_RET("next", oMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oMapIterRemoveMethod),
		METHOD_RET("findNext", oMapIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		//METHOD_RET( "clone",                oMapIterCloneMethod, RETURNS_OBJECT(kBCIMapIter) ),

		METHOD_RET("nextPair", oMapIterNextPairMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prevPair", oMapIterPrevPairMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 IntMap
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


	FORTHOP(oIntMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oIntMapStruct, pMap, pClassVocab);
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
		EXIT_IF_OBJECT_ALREADY_SHOWN;
		char buffer[20];
		GET_THIS(oIntMapStruct, pMap);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER;
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
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIIntMapIter, 0);
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
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIIntMapIter, 0);
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
			ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIIntMapIter, 0);
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

		METHOD_RET("headIter", oIntMapHeadIterMethod, RETURNS_OBJECT(kBCIIntMapIter)),
		METHOD_RET("tailIter", oIntMapTailIterMethod, RETURNS_OBJECT(kBCIIntMapIter)),
		METHOD_RET("find", oIntMapFindMethod, RETURNS_OBJECT(kBCIIntMapIter)),
		//METHOD_RET( "clone",                oIntMapCloneMethod, RETURNS_OBJECT(kBCIMap) ),
		METHOD_RET("count", oIntMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oIntMapClearMethod),
		METHOD("load", oIntMapLoadMethod),

        METHOD_RET("get", oIntMapGetMethod, RETURNS_OBJECT(kBCIContainedType)),
		METHOD("set", oIntMapSetMethod),
		METHOD_RET("findKey", oIntMapFindKeyMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oIntMapRemoveMethod),
		METHOD("unref", oIntMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 IntMapIter
	//

	FORTHOP(oIntMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create an IntMapIter object");
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
		METHOD_RET("next", oIntMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oIntMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oIntMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oIntMapIterRemoveMethod),
		METHOD_RET("findNext", oMapIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		//METHOD_RET( "clone",                oIntMapIterCloneMethod, RETURNS_OBJECT(kBCIIntMapIter) ),

		METHOD_RET("nextPair", oIntMapIterNextPairMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prevPair", oIntMapIterPrevPairMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIIntMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 FloatMap
	//

	typedef std::map<float, ForthObject> oFloatMap;
	struct oFloatMapStruct
	{
		ulong       refCount;
		oFloatMap*		elements;
	};

	struct oFloatMapIterStruct
	{
		ulong				refCount;
		ForthObject			parent;
		oFloatMap::iterator	*cursor;
	};


	FORTHOP(oFloatMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oFloatMapStruct, pMap, pClassVocab);
		pMap->refCount = 0;
		pMap->elements = new oFloatMap;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pMap);
	}

	FORTHOP(oFloatMapShowMethod)
	{
		EXIT_IF_OBJECT_ALREADY_SHOWN;
		char buffer[20];
		GET_THIS(oFloatMapStruct, pMap);
		oFloatMap::iterator iter;
		oFloatMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER;
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

	FORTHOP(oFloatMapLoadMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oFloatMapStruct, pMap);
		oFloatMap::iterator iter;
		oFloatMap& a = *(pMap->elements);
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
			float key = FPOP;
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

	FORTHOP(oFloatMapGetMethod)
	{
		GET_THIS(oFloatMapStruct, pMap);
		oFloatMap& a = *(pMap->elements);
		float key = FPOP;
		oFloatMap::iterator iter = a.find(key);
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

	FORTHOP(oFloatMapSetMethod)
	{
		GET_THIS(oFloatMapStruct, pMap);
		oFloatMap& a = *(pMap->elements);
		float key = FPOP;
		ForthObject newObj;
		POP_OBJECT(newObj);
		oFloatMap::iterator iter = a.find(key);
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

	FORTHOP(oFloatMapHeadIterMethod)
	{
		GET_THIS(oFloatMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oFloatMapIterStruct* pIter = new oFloatMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = new oFloatMap::iterator;
		*(pIter->cursor) = pMap->elements->begin();
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIFloatMapIter, 0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oFloatMapTailIterMethod)
	{
		GET_THIS(oFloatMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oFloatMapIterStruct* pIter = new oFloatMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = new oFloatMap::iterator;
		*(pIter->cursor) = pMap->elements->end();
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIFloatMapIter, 0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oFloatMapFindKeyMethod)
	{
		GET_THIS(oFloatMapStruct, pMap);
		float retVal = -1.0f;
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oFloatMap::iterator iter;
		oFloatMap& a = *(pMap->elements);
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
		FPUSH(retVal);
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oFloatMapRemoveMethod)
	{
		GET_THIS(oFloatMapStruct, pMap);
		oFloatMap& a = *(pMap->elements);
		float key = FPOP;
		oFloatMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& oldObj = iter->second;
			SAFE_RELEASE(pCore, oldObj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oFloatMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oFloatMapStruct, pMap);
		oFloatMap& a = *(pMap->elements);
		float key = FPOP;
		oFloatMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& fobj = iter->second;
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oFloatMapFindMethod)
	{
		GET_THIS(oFloatMapStruct, pMap);
		bool found = false;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oFloatMap::iterator iter;
		oFloatMap& a = *(pMap->elements);
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
			oFloatMapIterStruct* pIter = new oFloatMapIterStruct;
			TRACK_ITER_NEW;
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pMap);
			pIter->cursor = new oFloatMap::iterator;
			*(pIter->cursor) = iter;
			ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIFloatMapIter, 0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		else
		{
			SPUSH(0);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oFloatMapMembers[] =
	{
		METHOD("__newOp", oFloatMapNew),
		METHOD("show", oFloatMapShowMethod),

		METHOD_RET("headIter", oFloatMapHeadIterMethod, RETURNS_OBJECT(kBCIFloatMapIter)),
		METHOD_RET("tailIter", oFloatMapTailIterMethod, RETURNS_OBJECT(kBCIFloatMapIter)),
		METHOD_RET("find", oFloatMapFindMethod, RETURNS_OBJECT(kBCIFloatMapIter)),
		//METHOD_RET( "clone",                oFloatMapCloneMethod, RETURNS_OBJECT(kBCIFloatMap) ),
		METHOD("load", oFloatMapLoadMethod),

        METHOD_RET("get", oFloatMapGetMethod, RETURNS_OBJECT(kBCIContainedType)),
		METHOD("set", oFloatMapSetMethod),
		METHOD_RET("findKey", oFloatMapFindKeyMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oFloatMapRemoveMethod),
		METHOD("unref", oFloatMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 LongMap
	//

    ForthClassVocabulary* gpLongMapClassVocab = nullptr;

    void createLongMapObject(ForthObject& destObj, ForthClassVocabulary *pClassVocab)
    {
        destObj.pMethodOps = pClassVocab->GetInterface(0)->GetMethods();

        MALLOCATE_OBJECT(oLongMapStruct, pMap, pClassVocab);
        pMap->refCount = 0;
        pMap->elements = new oLongMap;
        destObj.pData = (long *)pMap;
    }

    FORTHOP(oLongMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        ForthObject newMap;
        createLongMapObject(newMap, pClassVocab);
        PUSH_OBJECT(newMap);
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
		EXIT_IF_OBJECT_ALREADY_SHOWN;
		char buffer[20];
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER;
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
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCILongMapIter, 0);
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
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCILongMapIter, 0);
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
			ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCILongMapIter, 0);
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

		METHOD_RET("headIter", oLongMapHeadIterMethod, RETURNS_OBJECT(kBCILongMapIter)),
		METHOD_RET("tailIter", oLongMapTailIterMethod, RETURNS_OBJECT(kBCILongMapIter)),
		METHOD_RET("find", oLongMapFindMethod, RETURNS_OBJECT(kBCILongMapIter)),
		//METHOD_RET( "clone",                oLongMapCloneMethod, RETURNS_OBJECT(kBCILongMap) ),
		METHOD_RET("count", oLongMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oLongMapClearMethod),
		METHOD("load", oLongMapLoadMethod),

        METHOD_RET("get", oLongMapGetMethod, RETURNS_OBJECT(kBCIContainedType)),
		METHOD("set", oLongMapSetMethod),
		METHOD_RET("findKey", oLongMapFindKeyMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oLongMapRemoveMethod),
		METHOD("unref", oLongMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 LongMapIter
	//

	FORTHOP(oLongMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a LongMapIter object");
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
		METHOD_RET("next", oLongMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oLongMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oLongMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oLongMapIterRemoveMethod),
		METHOD_RET("findNext", oMapIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		//METHOD_RET( "clone",                oLongMapIterCloneMethod, RETURNS_OBJECT(kBCILongMapIter) ),

		METHOD_RET("nextPair", oLongMapIterNextPairMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prevPair", oLongMapIterPrevPairMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCILongMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 DoubleMap
	//

	typedef std::map<double, ForthObject> oDoubleMap;
	struct oDoubleMapStruct
	{
		ulong       refCount;
		oDoubleMap*	elements;
	};

	struct oDoubleMapIterStruct
	{
		ulong				refCount;
		ForthObject			parent;
		oDoubleMap::iterator*	cursor;
	};


	FORTHOP(oDoubleMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oDoubleMapStruct, pMap, pClassVocab);
		pMap->refCount = 0;
		pMap->elements = new oDoubleMap;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pMap);
	}

	FORTHOP(oDoubleMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap::iterator iter;
		oDoubleMap& a = *(pMap->elements);
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

	FORTHOP(oDoubleMapShowMethod)
	{
		EXIT_IF_OBJECT_ALREADY_SHOWN;
		char buffer[64];
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap::iterator iter;
		oDoubleMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER;
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
				sprintf(buffer, "%g : ", iter->first);
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

	FORTHOP(oDoubleMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap::iterator iter;
		oDoubleMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapLoadMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap::iterator iter;
		oDoubleMap& a = *(pMap->elements);
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
			double key = DPOP;
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

	FORTHOP(oDoubleMapCountMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		SPUSH((long)(pMap->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapGetMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap& a = *(pMap->elements);
		double key = DPOP;
		oDoubleMap::iterator iter = a.find(key);
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

	FORTHOP(oDoubleMapSetMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap& a = *(pMap->elements);
		double key = DPOP;
		ForthObject newObj;
		POP_OBJECT(newObj);
		oDoubleMap::iterator iter = a.find(key);
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

	FORTHOP(oDoubleMapFindKeyMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		double retVal = -1.0;
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oDoubleMap::iterator iter;
		oDoubleMap& a = *(pMap->elements);
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
		DPUSH(retVal);
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapRemoveMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap& a = *(pMap->elements);
		double key = DPOP;
		oDoubleMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& oldObj = iter->second;
			SAFE_RELEASE(pCore, oldObj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap& a = *(pMap->elements);
		double key = DPOP;
		oDoubleMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& fobj = iter->second;
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapHeadIterMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oDoubleMapIterStruct* pIter = new oDoubleMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = new oDoubleMap::iterator;
		*(pIter->cursor) = pMap->elements->begin();
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIDoubleMapIter, 0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapTailIterMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oDoubleMapIterStruct* pIter = new oDoubleMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = new oDoubleMap::iterator;
		*(pIter->cursor) = pMap->elements->end();
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIDoubleMapIter, 0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapFindMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		bool found = false;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oDoubleMap::iterator iter;
		oDoubleMap& a = *(pMap->elements);
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
			oDoubleMapIterStruct* pIter = new oDoubleMapIterStruct;
			TRACK_ITER_NEW;
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pMap);
			pIter->cursor = new oDoubleMap::iterator;
			*(pIter->cursor) = iter;
			ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIDoubleMapIter, 0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		else
		{
			SPUSH(0);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oDoubleMapMembers[] =
	{
		METHOD("__newOp", oDoubleMapNew),
		METHOD("delete", oDoubleMapDeleteMethod),
		METHOD("show", oDoubleMapShowMethod),

		METHOD_RET("headIter", oDoubleMapHeadIterMethod, RETURNS_OBJECT(kBCIDoubleMapIter)),
		METHOD_RET("tailIter", oDoubleMapTailIterMethod, RETURNS_OBJECT(kBCIDoubleMapIter)),
		METHOD_RET("find", oDoubleMapFindMethod, RETURNS_OBJECT(kBCIDoubleMapIter)),
		//METHOD_RET( "clone",                oDoubleMapCloneMethod, RETURNS_OBJECT(kBCIDoubleMap) ),
		METHOD_RET("count", oDoubleMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oDoubleMapClearMethod),
		METHOD("load", oDoubleMapLoadMethod),

        METHOD_RET("get", oDoubleMapGetMethod, RETURNS_OBJECT(kBCIContainedType)),
		METHOD("set", oDoubleMapSetMethod),
		METHOD_RET("findKey", oDoubleMapFindKeyMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oDoubleMapRemoveMethod),
		METHOD("unref", oDoubleMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 DoubleMapIter
	//

	FORTHOP(oDoubleMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a DoubleMapIter object");
	}

	FORTHOP(oDoubleMapIterDeleteMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterSeekNextMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterSeekPrevMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterSeekHeadMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterSeekTailMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterNextMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent.pData);
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

	FORTHOP(oDoubleMapIterPrevMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent.pData);
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

	FORTHOP(oDoubleMapIterCurrentMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent.pData);
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

	FORTHOP(oDoubleMapIterRemoveMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) != pMap->elements->end())
		{
			ForthObject& o = (*pIter->cursor)->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterNextPairMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*pIter->cursor)->second;
			PUSH_OBJECT(o);
			double key = (*pIter->cursor)->first;
			DPUSH(key);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterPrevPairMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent.pData);
		if ((*pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			ForthObject& o = (*pIter->cursor)->second;
			PUSH_OBJECT(o);
			double key = (*pIter->cursor)->first;
			DPUSH(key);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oDoubleMapIterMembers[] =
	{
		METHOD("__newOp", oDoubleMapIterNew),
		METHOD("delete", oDoubleMapIterDeleteMethod),

		METHOD("seekNext", oDoubleMapIterSeekNextMethod),
		METHOD("seekPrev", oDoubleMapIterSeekPrevMethod),
		METHOD("seekHead", oDoubleMapIterSeekHeadMethod),
		METHOD("seekTail", oDoubleMapIterSeekTailMethod),
		METHOD_RET("next", oDoubleMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oDoubleMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oDoubleMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oDoubleMapIterRemoveMethod),
		METHOD_RET("findNext", oMapIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		//METHOD_RET( "clone",                oDoubleMapIterCloneMethod, RETURNS_OBJECT(kBCIMapIter) ),

		METHOD_RET("nextPair", oDoubleMapIterNextPairMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prevPair", oDoubleMapIterPrevPairMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIDoubleMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 StringIntMap
	//

	typedef std::map<std::string, int> oStringIntMap;

	struct oStringIntMapStruct
	{
		ulong       refCount;
		oStringIntMap*	elements;
	};

	struct oStringIntMapIterStruct
	{
		ulong				refCount;
		ForthObject			parent;
		oStringIntMap::iterator	*cursor;
	};



	FORTHOP(oStringIntMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oStringIntMapStruct, pMap, pClassVocab);
		pMap->refCount = 0;
		pMap->elements = new oStringIntMap;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pMap);
	}

	FORTHOP(oStringIntMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringIntMapStruct, pMap);
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapShowMethod)
	{
		EXIT_IF_OBJECT_ALREADY_SHOWN;
		char buffer[16];
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap::iterator iter;
		oStringIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER;
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
				pShowContext->ShowIndent("'");
				pEngine->ConsoleOut(iter->first.c_str());
				pEngine->ConsoleOut("' : ");
				sprintf(buffer, "%d", iter->second);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndIndent();
			pShowContext->EndElement();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}


	FORTHOP(oStringIntMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap::iterator iter;
		oStringIntMap& a = *(pMap->elements);
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapLoadMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap::iterator iter;
		oStringIntMap& a = *(pMap->elements);
		a.clear();
		int n = SPOP;
		for (int i = 0; i < n; i++)
		{
			std::string key;
			key = (const char*)(SPOP);
			int val = SPOP;
			a[key] = val;
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapCountMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		SPUSH((long)(pMap->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapGetMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringIntMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			SPUSH(iter->second);
		}
		else
		{
			SPUSH(0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapSetMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		int newValue = SPOP;
		a[key] = newValue;
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapFindKeyMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		const char* retVal = NULL;
		long found = 0;
		int soughtVal = SPOP;

		oStringIntMap::iterator iter;
		oStringIntMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			if (iter->second == soughtVal)
			{
				found = 1;
				retVal = iter->first.c_str();
				break;
			}
		}
		SPUSH(((long)retVal));
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapRemoveMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringIntMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapHeadIterMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oStringIntMapIterStruct* pIter = new oStringIntMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		oStringIntMap::iterator iter = pMap->elements->begin();
		*(pIter->cursor) = iter;
		//pIter->cursor = pMap->elements->begin();
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIStringIntMapIter, 0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapTailIterMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oStringIntMapIterStruct* pIter = new oStringIntMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		*(pIter->cursor) = pMap->elements->end();
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIStringIntMapIter, 0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapFindMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		bool found = false;
		int soughtInt = SPOP;
		oStringIntMap::iterator iter;
		oStringIntMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			if (soughtInt == iter->second)
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
			oStringIntMapIterStruct* pIter = new oStringIntMapIterStruct;
			TRACK_ITER_NEW;
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pMap);
			pIter->cursor = new oStringIntMap::iterator;
			*(pIter->cursor) = iter;
			ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIStringIntMapIter, 0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		else
		{
			SPUSH(0);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oStringIntMapMembers[] =
	{
		METHOD("__newOp", oStringIntMapNew),
		METHOD("delete", oStringIntMapDeleteMethod),
		METHOD("show", oStringIntMapShowMethod),

		METHOD_RET("headIter", oStringIntMapHeadIterMethod, RETURNS_OBJECT(kBCIStringIntMapIter)),
		METHOD_RET("tailIter", oStringIntMapTailIterMethod, RETURNS_OBJECT(kBCIStringIntMapIter)),
		METHOD_RET("find", oStringIntMapFindMethod, RETURNS_OBJECT(kBCIStringIntMapIter)),
		//METHOD_RET( "clone",                oStringIntMapCloneMethod, RETURNS_OBJECT(kBCIStringIntMap) ),
		METHOD_RET("count", oStringIntMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oStringIntMapClearMethod),
		METHOD("load", oStringIntMapLoadMethod),

		METHOD_RET("get", oStringIntMapGetMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("set", oStringIntMapSetMethod),
		METHOD_RET("findKey", oStringIntMapFindKeyMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oStringIntMapRemoveMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 StringIntMapIter
	//

	FORTHOP(oStringIntMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a StringIntMapIter object");
	}

	FORTHOP(oStringIntMapIterDeleteMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterSeekNextMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterSeekPrevMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterSeekHeadMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterSeekTailMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterNextMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			int val = (*(pIter->cursor))->second;
			SPUSH(val);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterPrevMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			int val = (*(pIter->cursor))->second;
			SPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterCurrentMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			int val = (*(pIter->cursor))->second;
			SPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterRemoveMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) != pMap->elements->end())
		{
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterFindNextMethod)
	{
		SPUSH(0);
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterNextPairMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			int val = (*(pIter->cursor))->second;
			SPUSH(val);
			SPUSH((long)(*(pIter->cursor))->first.c_str());
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterPrevPairMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			int val = (*(pIter->cursor))->second;
			SPUSH(val);
			SPUSH((long)(*(pIter->cursor))->first.c_str());
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oStringIntMapIterMembers[] =
	{
		METHOD("__newOp", oStringIntMapIterNew),
		METHOD("delete", oStringIntMapIterDeleteMethod),

		METHOD("seekNext", oStringIntMapIterSeekNextMethod),
		METHOD("seekPrev", oStringIntMapIterSeekPrevMethod),
		METHOD("seekHead", oStringIntMapIterSeekHeadMethod),
		METHOD("seekTail", oStringIntMapIterSeekTailMethod),
		METHOD_RET("next", oStringIntMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oStringIntMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oStringIntMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oStringIntMapIterRemoveMethod),
		METHOD_RET("findNext", oStringIntMapIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		//METHOD_RET( "clone",                oStringIntMapIterCloneMethod, RETURNS_OBJECT(kBCIMapIter) ),

		METHOD_RET("nextPair", oStringIntMapIterNextPairMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prevPair", oStringIntMapIterPrevPairMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIStringIntMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 StringFloatMap
	//

	FORTHOP(oStringFloatMapShowMethod)
	{
		EXIT_IF_OBJECT_ALREADY_SHOWN;
		char buffer[16];
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap::iterator iter;
		oStringIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER;
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
				pShowContext->ShowIndent("'");
				pEngine->ConsoleOut(iter->first.c_str());
				pEngine->ConsoleOut("' : ");
				float fval = *((float *)&(iter->second));
				sprintf(buffer, "%f", fval);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndIndent();
			pShowContext->EndElement();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}



	baseMethodEntry oStringFloatMapMembers[] =
	{
		METHOD("__newOp", oStringIntMapNew),
		METHOD("show", oStringFloatMapShowMethod),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 StringLongMap
	//

	typedef std::map<std::string, long long> oStringLongMap;

	struct oStringLongMapStruct
	{
		ulong       refCount;
		oStringLongMap*	elements;
	};

	struct oStringLongMapIterStruct
	{
		ulong				refCount;
		ForthObject			parent;
		oStringLongMap::iterator	*cursor;
	};



	FORTHOP(oStringLongMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oStringLongMapStruct, pMap, pClassVocab);
		pMap->refCount = 0;
		pMap->elements = new oStringLongMap;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pMap);
	}

	FORTHOP(oStringLongMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringLongMapStruct, pMap);
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapShowMethod)
	{
		EXIT_IF_OBJECT_ALREADY_SHOWN;
		char buffer[32];
		GET_THIS(oStringLongMapStruct, pMap);
		oStringLongMap::iterator iter;
		oStringLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER;
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
				pShowContext->ShowIndent("'");
				pEngine->ConsoleOut(iter->first.c_str());
				pEngine->ConsoleOut("' : ");
				sprintf(buffer, "%lld", iter->second);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndIndent();
			pShowContext->EndElement();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}


	FORTHOP(oStringLongMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringLongMapStruct, pMap);
		oStringLongMap::iterator iter;
		oStringLongMap& a = *(pMap->elements);
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapLoadMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringLongMapStruct, pMap);
		oStringLongMap::iterator iter;
		oStringLongMap& a = *(pMap->elements);
		a.clear();
		int n = SPOP;
		for (int i = 0; i < n; i++)
		{
			std::string key;
			key = (const char*)(SPOP);
			stackInt64 val;
			LPOP(val);
			a[key] = val.s64;
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapCountMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		SPUSH((long)(pMap->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapGetMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		oStringLongMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringLongMap::iterator iter = a.find(key);
		stackInt64 val;
		if (iter != a.end())
		{
			val.s64 = iter->second;
		}
		else
		{
			val.s64 = 0;
		}
		LPUSH(val);
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapSetMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		oStringLongMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		stackInt64 newValue;
		LPOP(newValue);
		a[key] = newValue.s64;
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapFindKeyMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		const char* retVal = NULL;
		long found = 0;
		stackInt64 soughtVal;
		LPOP(soughtVal);

		oStringLongMap::iterator iter;
		oStringLongMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			if (iter->second == soughtVal.s64)
			{
				found = 1;
				retVal = iter->first.c_str();
				break;
			}
		}
		SPUSH(((long)retVal));
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapRemoveMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		oStringLongMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringLongMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapHeadIterMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oStringLongMapIterStruct* pIter = new oStringLongMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		oStringLongMap::iterator iter = pMap->elements->begin();
		*(pIter->cursor) = iter;
		//pIter->cursor = pMap->elements->begin();
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIStringLongMapIter, 0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapTailIterMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oStringLongMapIterStruct* pIter = new oStringLongMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		*(pIter->cursor) = pMap->elements->end();
		ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIStringLongMapIter, 0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapFindMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		bool found = false;
		stackInt64 soughtVal;
		LPOP(soughtVal);
		oStringLongMap::iterator iter;
		oStringLongMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			if (soughtVal.s64 == iter->second)
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
			oStringLongMapIterStruct* pIter = new oStringLongMapIterStruct;
			TRACK_ITER_NEW;
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pMap);
			pIter->cursor = new oStringLongMap::iterator;
			*(pIter->cursor) = iter;
			ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIStringLongMapIter, 0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		else
		{
			SPUSH(0);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oStringLongMapMembers[] =
	{
		METHOD("__newOp", oStringLongMapNew),
		METHOD("delete", oStringLongMapDeleteMethod),
		METHOD("show", oStringLongMapShowMethod),
		
		METHOD_RET("headIter", oStringLongMapHeadIterMethod, RETURNS_OBJECT(kBCIStringLongMapIter)),
		METHOD_RET("tailIter", oStringLongMapTailIterMethod, RETURNS_OBJECT(kBCIStringLongMapIter)),
		METHOD_RET("find", oStringLongMapFindMethod, RETURNS_OBJECT(kBCIStringLongMapIter)),
		//METHOD_RET( "clone",                oStringLongMapCloneMethod, RETURNS_OBJECT(kBCIStringLongMap) ),
		METHOD_RET("count", oStringLongMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oStringLongMapClearMethod),
		METHOD("load", oStringLongMapLoadMethod),

		METHOD_RET("get", oStringLongMapGetMethod, RETURNS_NATIVE(kBaseTypeLong)),
		METHOD("set", oStringLongMapSetMethod),
		METHOD_RET("findKey", oStringLongMapFindKeyMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oStringLongMapRemoveMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 StringLongMapIter
	//

	FORTHOP(oStringLongMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a StringLongMapIter object");
	}

	FORTHOP(oStringLongMapIterDeleteMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterSeekNextMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterSeekPrevMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterSeekHeadMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterSeekTailMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent.pData);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterNextMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			stackInt64 val;
			val.s64 = (*(pIter->cursor))->second;
			LPUSH(val);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterPrevMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			stackInt64 val;
			val.s64 = (*(pIter->cursor))->second;
			LPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterCurrentMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			stackInt64 val;
			val.s64 = (*(pIter->cursor))->second;
			LPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterRemoveMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) != pMap->elements->end())
		{
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterFindNextMethod)
	{
		SPUSH(0);
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterNextPairMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			stackInt64 val;
			val.s64 = (*(pIter->cursor))->second;
			LPUSH(val);
			SPUSH((long)(*(pIter->cursor))->first.c_str());
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterPrevPairMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent.pData);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			stackInt64 val;
			val.s64 = (*(pIter->cursor))->second;
			LPUSH(val);
			SPUSH((long)(*(pIter->cursor))->first.c_str());
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oStringLongMapIterMembers[] =
	{
		METHOD("__newOp", oStringLongMapIterNew),
		METHOD("delete", oStringLongMapIterDeleteMethod),

		METHOD("seekNext", oStringLongMapIterSeekNextMethod),
		METHOD("seekPrev", oStringLongMapIterSeekPrevMethod),
		METHOD("seekHead", oStringLongMapIterSeekHeadMethod),
		METHOD("seekTail", oStringLongMapIterSeekTailMethod),
		METHOD_RET("next", oStringLongMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oStringLongMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oStringLongMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oStringLongMapIterRemoveMethod),
		METHOD_RET("findNext", oStringLongMapIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		//METHOD_RET( "clone",                oStringLongMapIterCloneMethod, RETURNS_OBJECT(kBCIMapIter) ),

		METHOD_RET("nextPair", oStringLongMapIterNextPairMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prevPair", oStringLongMapIterPrevPairMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIStringLongMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 StringDoubleMap
	//

	FORTHOP(oStringDoubleMapShowMethod)
	{
		EXIT_IF_OBJECT_ALREADY_SHOWN;
		char buffer[32];
		GET_THIS(oStringLongMapStruct, pMap);
		oStringLongMap::iterator iter;
		oStringLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER;
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
				pShowContext->ShowIndent("'");
				pEngine->ConsoleOut(iter->first.c_str());
				pEngine->ConsoleOut("' : ");

				stackInt64 val;
				val.s64 = iter->second;
				stackInt64 valb;
				valb.s32[0] = val.s32[1];
				valb.s32[1] = val.s32[0];
				double dvalb = *((double *)&valb);
				sprintf(buffer, "%f", dvalb);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndIndent();
			pShowContext->EndElement();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}


	baseMethodEntry oStringDoubleMapMembers[] =
	{
		METHOD("__newOp", oStringLongMapNew),
		METHOD("show", oStringDoubleMapShowMethod),

		// following must be last in table
		END_MEMBERS
	};


	// TODO: string-double map iter, it can just be string-long map iter, but parent member should be type kBCIStringDoubleMap

	void AddClasses(ForthEngine* pEngine)
	{
		pEngine->AddBuiltinClass("Map", kBCIMap, kBCIIterable, oMapMembers);
		pEngine->AddBuiltinClass("MapIter", kBCIMapIter, kBCIIter, oMapIterMembers);

		pEngine->AddBuiltinClass("IntMap", kBCIIntMap, kBCIIterable, oIntMapMembers);
		pEngine->AddBuiltinClass("IntMapIter", kBCIIntMapIter, kBCIIter, oIntMapIterMembers);

		pEngine->AddBuiltinClass("FloatMap", kBCIFloatMap, kBCIIntMap, oFloatMapMembers);
		pEngine->AddBuiltinClass("FloatMapIter", kBCIFloatMapIter, kBCIIter, oIntMapIterMembers);

        gpLongMapClassVocab = pEngine->AddBuiltinClass("LongMap", kBCILongMap, kBCIIterable, oLongMapMembers);
		pEngine->AddBuiltinClass("LongMapIter", kBCILongMapIter, kBCIIter, oLongMapIterMembers);

		pEngine->AddBuiltinClass("DoubleMap", kBCIDoubleMap, kBCILongMap, oDoubleMapMembers);
		pEngine->AddBuiltinClass("DoubleMapIter", kBCIDoubleMapIter, kBCIIter, oLongMapIterMembers);

		pEngine->AddBuiltinClass("StringIntMap", kBCIStringIntMap, kBCIIterable, oStringIntMapMembers);
		pEngine->AddBuiltinClass("StringIntMapIter", kBCIStringIntMapIter, kBCIIter, oStringIntMapIterMembers);

		pEngine->AddBuiltinClass("StringFloatMap", kBCIStringFloatMap, kBCIStringIntMap, oStringFloatMapMembers);
		pEngine->AddBuiltinClass("StringFloatMapIter", kBCIStringFloatMapIter, kBCIIter, oStringIntMapIterMembers);

		pEngine->AddBuiltinClass("StringLongMap", kBCIStringLongMap, kBCIIterable, oStringLongMapMembers);
		pEngine->AddBuiltinClass("StringLongMapIter", kBCIStringLongMapIter, kBCIIter, oLongMapIterMembers);

		pEngine->AddBuiltinClass("StringDoubleMap", kBCIStringDoubleMap, kBCIStringLongMap, oStringDoubleMapMembers);
		pEngine->AddBuiltinClass("StringDoubleMapIter", kBCIStringDoubleMapIter, kBCIIter, oLongMapIterMembers);
	}

} // namespace OMap

