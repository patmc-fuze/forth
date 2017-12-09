#pragma once
//////////////////////////////////////////////////////////////////////
//
// OMap.h: builtin map related classes
//
//////////////////////////////////////////////////////////////////////


class ForthClassVocabulary;

namespace OMap
{
	void AddClasses(ForthEngine* pEngine);

    void createLongMapObject(ForthObject& destObj, ForthClassVocabulary *pClassVocab);

    extern ForthClassVocabulary* gpLongMapClassVocab;
}