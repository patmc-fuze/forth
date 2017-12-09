//////////////////////////////////////////////////////////////////////
//
// ONumber.cpp: builtin number related classes
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

#include "ONumber.h"

namespace ONumber
{

	//////////////////////////////////////////////////////////////////////
	///
	//                 oInt
	//

	struct oIntStruct
	{
		ulong       refCount;
		int			val;
	};


	FORTHOP(oIntNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oIntStruct, pInt, pClassVocab);
		pInt->refCount = 0;
		pInt->val = 0;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pInt);
	}

    FORTHOP(oIntGetMethod)
    {
        GET_THIS(oIntStruct, pInt);
        SPUSH(pInt->val);
        METHOD_RETURN;
    }

    FORTHOP(oIntGetSignedByteMethod)
    {
        GET_THIS(oIntStruct, pInt);
        int val = pInt->val & 0xFF;
        if (val > 0x7F)
        {
            val |= 0xFFFFFF00;
        }
        SPUSH(val);
        METHOD_RETURN;
    }

    FORTHOP(oIntGetUnsignedByteMethod)
    {
        GET_THIS(oIntStruct, pInt);
        int val = pInt->val & 0xFF;
        SPUSH(val);
        METHOD_RETURN;
    }

    FORTHOP(oIntGetSignedShortMethod)
    {
        GET_THIS(oIntStruct, pInt);
        int val = pInt->val & 0xFF;
        if (val > 0x7FFF)
        {
            val |= 0xFFFF0000;
        }
        SPUSH(val);
        METHOD_RETURN;
    }

    FORTHOP(oIntGetUnsignedShortMethod)
    {
        GET_THIS(oIntStruct, pInt);
        int val = pInt->val & 0xFFFF;
        SPUSH(val);
        METHOD_RETURN;
    }

    FORTHOP(oIntSetMethod)
	{
		GET_THIS(oIntStruct, pInt);
		pInt->val = SPOP;
		METHOD_RETURN;
	}

	FORTHOP(oIntShowMethod)
	{
		char buff[32];
		GET_THIS(oIntStruct, pInt);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_OBJ_HEADER;
		pShowContext->ShowIndent("'value' : ");
		sprintf(buff, "%d", pInt->val);
		pShowContext->EndElement(buff);
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oIntCompareMethod)
	{
		GET_THIS(oIntStruct, pInt);
		ForthObject compObj;
		POP_OBJECT(compObj);
		oIntStruct* pComp = (oIntStruct *)compObj.pData;
		int retVal = 0;
		if (pInt->val != pComp->val)
		{
			retVal = (pInt->val > pComp->val) ? 1 : -1;
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	baseMethodEntry oIntMembers[] =
	{
		METHOD("__newOp", oIntNew),

		METHOD("set", oIntSetMethod),
		METHOD_RET("get", oIntGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
        METHOD_RET("getByte", oIntGetSignedByteMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte)),
        METHOD_RET("getUByte", oIntGetUnsignedByteMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeUByte)),
        METHOD_RET("getShort", oIntGetSignedShortMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort)),
        METHOD_RET("getUShort", oIntGetUnsignedShortMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeUShort)),
        METHOD("show", oIntShowMethod),
		METHOD_RET("compare", oIntCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),

		MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oLong
	//

	struct oLongStruct
	{
		ulong       refCount;
        int dummy;
		long long	val;
	};


	FORTHOP(oLongNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oLongStruct, pLong, pClassVocab);
		pLong->refCount = 0;
		pLong->val = 0;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pLong);
	}

	FORTHOP(oLongGetMethod)
	{
		GET_THIS(oLongStruct, pLong);
		stackInt64 val;
		val.s64 = pLong->val;
		LPUSH(val);
		METHOD_RETURN;
	}

	FORTHOP(oLongSetMethod)
	{
		GET_THIS(oLongStruct, pLong);
		stackInt64 a64;
		LPOP(a64);
		pLong->val = a64.s64;
		METHOD_RETURN;
	}

	FORTHOP(oLongShowMethod)
	{
		char buff[32];
		GET_THIS(oLongStruct, pLong);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_OBJ_HEADER;
		pShowContext->ShowIndent("'value' : ");
		sprintf(buff, "%lld", pLong->val);
		pShowContext->EndElement(buff);
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oLongCompareMethod)
	{
		GET_THIS(oLongStruct, pLong);
		ForthObject compObj;
		POP_OBJECT(compObj);
		oLongStruct* pComp = (oLongStruct *)compObj.pData;
		int retVal = 0;
		if (pLong->val != pComp->val)
		{
			retVal = (pLong->val > pComp->val) ? 1 : -1;
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	baseMethodEntry oLongMembers[] =
	{
		METHOD("__newOp", oLongNew),

		METHOD("set", oLongSetMethod),
		METHOD_RET("get", oLongGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong)),
		METHOD("show", oLongShowMethod),
		METHOD_RET("compare", oLongCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),

        MEMBER_VAR("__dummy", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
        MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, kBaseTypeLong)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oFloat
	//

	struct oFloatStruct
	{
		ulong       refCount;
		float		val;
	};


	FORTHOP(oFloatNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oFloatStruct, pFloat, pClassVocab);
		pFloat->refCount = 0;
		pFloat->val = 0.0f;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pFloat);
	}

	FORTHOP(oFloatGetMethod)
	{
		GET_THIS(oFloatStruct, pFloat);
		FPUSH(pFloat->val);
		METHOD_RETURN;
	}

	FORTHOP(oFloatSetMethod)
	{
		GET_THIS(oFloatStruct, pFloat);
		pFloat->val = FPOP;
		METHOD_RETURN;
	}

	FORTHOP(oFloatShowMethod)
	{
		char buff[32];
		GET_THIS(oFloatStruct, pFloat);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_OBJ_HEADER;
		pShowContext->ShowIndent("'value' : ");
		sprintf(buff, "%f", pFloat->val);
		pShowContext->EndElement(buff);
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oFloatCompareMethod)
	{
		GET_THIS(oFloatStruct, pFloat);
		ForthObject compObj;
		POP_OBJECT(compObj);
		oFloatStruct* pComp = (oFloatStruct *)compObj.pData;
		int retVal = 0;
		if (pFloat->val != pComp->val)
		{
			retVal = (pFloat->val > pComp->val) ? 1 : -1;
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	baseMethodEntry oFloatMembers[] =
	{
		METHOD("__newOp", oFloatNew),

		METHOD("set", oFloatSetMethod),
		METHOD_RET("get", oFloatGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeFloat)),
		METHOD("show", oFloatShowMethod),
		METHOD_RET("compare", oFloatCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),

        MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, kBaseTypeFloat)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oDouble
	//

	struct oDoubleStruct
	{
		ulong       refCount;
        int         dummy;
		double		val;
	};


	FORTHOP(oDoubleNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oDoubleStruct, pDouble, pClassVocab);
		pDouble->refCount = 0;
		pDouble->val = 0.0;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pDouble);
	}

	FORTHOP(oDoubleGetMethod)
	{
		GET_THIS(oDoubleStruct, pDouble);
		DPUSH(pDouble->val);
		METHOD_RETURN;
	}

	FORTHOP(oDoubleSetMethod)
	{
		GET_THIS(oDoubleStruct, pDouble);
		pDouble->val = DPOP;
		METHOD_RETURN;
	}

	FORTHOP(oDoubleShowMethod)
	{
		char buff[32];
		GET_THIS(oDoubleStruct, pDouble);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_OBJ_HEADER;
		pShowContext->ShowIndent("'value' : ");
		sprintf(buff, "%f", pDouble->val);
		pShowContext->EndElement(buff);
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oDoubleCompareMethod)
	{
		GET_THIS(oDoubleStruct, pDouble);
		int retVal = 0;
		ForthObject compObj;
		POP_OBJECT(compObj);
		oDoubleStruct* pComp = (oDoubleStruct *)compObj.pData;
		if (pDouble->val != pComp->val)
		{
			retVal = (pDouble->val > pComp->val) ? 1 : -1;
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	baseMethodEntry oDoubleMembers[] =
	{
		METHOD("__newOp", oDoubleNew),

		METHOD("set", oDoubleSetMethod),
		METHOD_RET("get", oDoubleGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeDouble)),
		METHOD("show", oDoubleShowMethod),
		METHOD_RET("compare", oDoubleCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),

        MEMBER_VAR("__dummy", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
        MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, kBaseTypeDouble)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		pEngine->AddBuiltinClass("Int", kBCIInt, kBCIObject, oIntMembers);
		pEngine->AddBuiltinClass("Long", kBCILong, kBCIObject, oLongMembers);
		pEngine->AddBuiltinClass("Float", kBCIFloat, kBCIObject, oFloatMembers);
		pEngine->AddBuiltinClass("Double", kBCIDouble, kBCIObject, oDoubleMembers);
	}

} // namespace ONumber
