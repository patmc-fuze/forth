#pragma once
//////////////////////////////////////////////////////////////////////
//
// OSystem.h: builtin system class
//
//////////////////////////////////////////////////////////////////////


class ForthClassVocabulary;

namespace OSystem
{
	extern ForthClassVocabulary* gpOSystemClassVocab;

	struct oSystemStruct
	{
		ulong			refCount;
	};

	void AddClasses(ForthEngine* pEngine);
}