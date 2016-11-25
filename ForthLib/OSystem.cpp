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

#include "OSystem.h"

extern "C" {
	extern void unimplementedMethodOp(ForthCoreState *pCore);
	extern void illegalMethodOp(ForthCoreState *pCore);
};

namespace OSystem
{
	static ForthObject gSystemSingleton;

	//////////////////////////////////////////////////////////////////////
	///
	//                 OSystem
	//

	FORTHOP(oSystemNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		if (gSystemSingleton.pMethodOps == nullptr)
		{
			ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
			MALLOCATE_OBJECT(oSystemStruct, pSystem);
			pSystem->refCount = 1000000;
			gSystemSingleton.pMethodOps = pPrimaryInterface->GetMethods();
			gSystemSingleton.pData = reinterpret_cast<long *>(pSystem);
		}
		PUSH_OBJECT(gSystemSingleton);
	}

	FORTHOP(oSystemDeleteMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		pSystem->refCount = 1000000;
        // TODO: warn that something tried to delete system
		METHOD_RETURN;
	}

	FORTHOP(oSystemShowMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_OBJ_HEADER("OSystem");
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

	baseMethodEntry oSystemMembers[] =
	{
		METHOD("__newOp", oSystemNew),
		METHOD("delete", oSystemDeleteMethod),
		METHOD("show", oSystemShowMethod),
		METHOD("getDefinitionsVocab", oSystemGetDefinitionsVocabMethod),
		METHOD("setDefinitionsVocab", oSystemSetDefinitionsVocabMethod),
		METHOD("clearSearchVocab", oSystemClearSearchVocabMethod),
		METHOD("getSearchVocabDepth", oSystemGetSearchVocabDepthMethod),
		METHOD("getSearchVocabAt", oSystemGetSearchVocabAtMethod),
		METHOD("getSearchVocabTop", oSystemGetSearchVocabTopMethod),
		METHOD("setSearchVocabTop", oSystemSetSearchVocabTopMethod),
		METHOD("pushSearchVocab", oSystemPushSearchVocabMethod),
		METHOD("getVocabByName", oSystemGetVocabByNameMethod),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		gSystemSingleton.pMethodOps = nullptr;
		gSystemSingleton.pData = nullptr;
		pEngine->AddBuiltinClass("OSystem", kBCISystem, kBCIObject, oSystemMembers);
	}

} // namespace OSystem

