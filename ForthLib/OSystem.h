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
        forthop*        pMethods;
        ucell           refCount;
        ForthObject     namedObjects;
        ForthObject     args;
        ForthObject     env;
        ForthObject     shellStack;
    };

    void AddClasses(ForthEngine* pEngine);
    void Shutdown(ForthEngine* pEngine);
}
