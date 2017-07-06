//////////////////////////////////////////////////////////////////////
//
// OSystem.cpp: builtin system class
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
#include "ForthThread.h"
#include "ForthPortability.h"

#include "OSystem.h"

extern "C" {
	extern void unimplementedMethodOp(ForthCoreState *pCore);
	extern void illegalMethodOp(ForthCoreState *pCore);
};

namespace OSystem
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 OSystem
	//

    static oSystemStruct gSystemSingleton;

    FORTHOP(oSystemNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        ForthObject obj;
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
        gSystemSingleton.refCount = 1000000;
        obj.pMethodOps = pPrimaryInterface->GetMethods();
        obj.pData = reinterpret_cast<long *>(&gSystemSingleton);
        PUSH_OBJECT(obj);
	}

	FORTHOP(oSystemDeleteMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		pSystem->refCount = 1000000;
        // TODO: warn that something tried to delete system
		METHOD_RETURN;
	}

	FORTHOP(oSystemStatsMethod)
	{
		char buff[512];

		SNPRINTF(buff, sizeof(buff), "pCore %p pEngine %p     DP %p DBase %p    IP %p\n",
			pCore, pCore->pEngine, pCore->pDictionary, pCore->pDictionary->pBase, pCore->IP);
		CONSOLE_STRING_OUT(buff);
		SNPRINTF(buff, sizeof(buff), "SP %p ST %p SLen %d    RP %p RT %p RLen %d\n",
			pCore->SP, pCore->ST, pCore->SLen,
			pCore->RP, pCore->RT, pCore->RLen);
		CONSOLE_STRING_OUT(buff);
		SNPRINTF(buff, sizeof(buff), "%d builtins    %d userops @ %p\n", pCore->numBuiltinOps, pCore->numOps, pCore->ops);
		CONSOLE_STRING_OUT(buff);
		METHOD_RETURN;
	}


	FORTHOP(oSystemGetDefinitionsVocabMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		ForthEngine *pEngine = GET_ENGINE;

		ForthVocabulary* pVocab = pEngine->GetDefinitionVocabulary();
		if (pVocab != NULL)
		{
			PUSH_OBJECT(pVocab->GetVocabularyObject());
		}
		else
		{
			PUSH_PAIR(nullptr, nullptr);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemSetDefinitionsVocabMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		ForthObject vocabObj;
		POP_OBJECT(vocabObj);
		oVocabularyStruct* pVocabStruct = reinterpret_cast<oVocabularyStruct *>(vocabObj.pData);

		ForthVocabulary* pVocab = pVocabStruct->vocabulary;
		if (pVocab != NULL)
		{
			ForthEngine *pEngine = GET_ENGINE;
			pEngine->SetDefinitionVocabulary(pVocab);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemGetSearchVocabDepthMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		ForthEngine *pEngine = GET_ENGINE;
		ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
		SPUSH(pVocabStack->GetDepth());

		METHOD_RETURN;
	}

	FORTHOP(oSystemClearSearchVocabMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		ForthEngine *pEngine = GET_ENGINE;
		ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
		pVocabStack->Clear();

		METHOD_RETURN;
	}

	FORTHOP(oSystemGetSearchVocabAtMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		int vocabStackIndex = SPOP;
		ForthEngine *pEngine = GET_ENGINE;

		ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
		ForthVocabulary* pVocab = pVocabStack->GetElement(vocabStackIndex);
		if (pVocab != NULL)
		{
			PUSH_OBJECT(pVocab->GetVocabularyObject());
		}
		else
		{
			PUSH_PAIR(nullptr, nullptr);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemGetSearchVocabTopMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		ForthEngine *pEngine = GET_ENGINE;

		ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
		ForthVocabulary* pVocab = pVocabStack->GetTop();
		if (pVocab != NULL)
		{
			PUSH_OBJECT(pVocab->GetVocabularyObject());
		}
		else
		{
			PUSH_PAIR(nullptr, nullptr);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemSetSearchVocabTopMethod)
	{
		ForthObject vocabObj;
		POP_OBJECT(vocabObj);
		oVocabularyStruct* pVocabStruct = reinterpret_cast<oVocabularyStruct *>(vocabObj.pData);

		ForthVocabulary* pVocab = pVocabStruct->vocabulary;
		if (pVocab != NULL)
		{
			ForthEngine *pEngine = GET_ENGINE;
			ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
			pVocabStack->SetTop(pVocab);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemPushSearchVocabMethod)
	{
		ForthObject vocabObj;
		POP_OBJECT(vocabObj);
		oVocabularyStruct* pVocabStruct = reinterpret_cast<oVocabularyStruct *>(vocabObj.pData);

		ForthVocabulary* pVocab = pVocabStruct->vocabulary;
		if (pVocab != NULL)
		{
			ForthEngine *pEngine = GET_ENGINE;
			ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
			pVocabStack->DupTop();
			pVocabStack->SetTop(pVocab);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemGetVocabByNameMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		ForthEngine *pEngine = GET_ENGINE;

		const char* pVocabName = reinterpret_cast<const char*>(SPOP);
		ForthVocabulary* pVocab = ForthVocabulary::FindVocabulary(pVocabName);
		if (pVocab != NULL)
		{
			PUSH_OBJECT(pVocab->GetVocabularyObject());
		}
		else
		{
			PUSH_PAIR(nullptr, nullptr);
		}
		METHOD_RETURN;
	}
	

	FORTHOP(oSystemGetOpsTableMethod)
	{
		SPUSH((long)(pCore->ops));
		METHOD_RETURN;
	}

	FORTHOP(oSystemCreateAsyncThreadMethod)
	{
		ForthEngine* pEngine = GET_ENGINE;
		ForthObject asyncThread;
		int returnStackLongs = (int)(SPOP);
		int paramStackLongs = (int)(SPOP);
		long threadOp = SPOP;
		OThread::CreateAsyncThreadObject(asyncThread, pEngine, threadOp, paramStackLongs, returnStackLongs);

		PUSH_OBJECT(asyncThread);
		METHOD_RETURN;
	}

	FORTHOP(oSystemCreateAsyncLockMethod)
	{
		ForthEngine *pEngine = GET_ENGINE;
		ForthObject asyncLock;
		OLock::CreateAsyncLockObject(asyncLock, pEngine);

		PUSH_OBJECT(asyncLock);
		METHOD_RETURN;
	}

	baseMethodEntry oSystemMembers[] =
	{
		METHOD("__newOp", oSystemNew),
		METHOD("delete", oSystemDeleteMethod),
		METHOD("stats", oSystemStatsMethod),
		METHOD_RET("getDefinitionsVocab", oSystemGetDefinitionsVocabMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIVocabulary)),
		METHOD("setDefinitionsVocab", oSystemSetDefinitionsVocabMethod),
		METHOD("clearSearchVocab", oSystemClearSearchVocabMethod),
		METHOD("getSearchVocabDepth", oSystemGetSearchVocabDepthMethod),
		METHOD_RET("getSearchVocabAt", oSystemGetSearchVocabAtMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIVocabulary)),
		METHOD_RET("getSearchVocabTop", oSystemGetSearchVocabTopMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIVocabulary)),
		METHOD("setSearchVocabTop", oSystemSetSearchVocabTopMethod),
		METHOD("pushSearchVocab", oSystemPushSearchVocabMethod),
		METHOD_RET("getVocabByName", oSystemGetVocabByNameMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIVocabulary)),
		METHOD_RET("getOpsTable", oSystemGetOpsTableMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("createAsyncThread", oSystemCreateAsyncThreadMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIAsyncThread)),
		METHOD_RET("createAsyncLock", oSystemCreateAsyncLockMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIAsyncLock)),


		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		pEngine->AddBuiltinClass("System", kBCISystem, kBCIObject, oSystemMembers);
	}

} // namespace OSystem

