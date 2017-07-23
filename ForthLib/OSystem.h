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
        ForthObject     namedObjects;
        ForthObject     args;
        ForthObject     env;
    };

    void AddClasses(ForthEngine* pEngine);
    void Shutdown(ForthEngine* pEngine);
}
