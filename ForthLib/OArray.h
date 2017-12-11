#pragma once
//////////////////////////////////////////////////////////////////////
//
// OArray.h: builtin array related classes
//
//////////////////////////////////////////////////////////////////////


class ForthClassVocabulary;

namespace OArray
{
	void AddClasses(ForthEngine* pEngine);

    void createArrayObject(ForthObject& destObj, ForthClassVocabulary *pClassVocab);

    extern ForthClassVocabulary* gpArrayClassVocab;
}