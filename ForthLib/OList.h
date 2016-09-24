#pragma once
//////////////////////////////////////////////////////////////////////
//
// OList.h: builtin list related classes
//
//////////////////////////////////////////////////////////////////////


class ForthClassVocabulary;

namespace OList
{
	extern ForthClassVocabulary* gpOListClassVocab;
	extern ForthClassVocabulary* gpOListIterClassVocab;

	struct oListIterStruct
	{
		ulong			refCount;
		ForthObject		parent;
		oListElement*	cursor;
	};

	void AddClasses(ForthEngine* pEngine);
}