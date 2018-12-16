//////////////////////////////////////////////////////////////////////
//
// ForthShowContext.cpp: implementation of the ForthShowContext class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ForthShowContext.h"

#include "ForthEngine.h"

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthShowContext
// 

ForthShowContext::ForthShowContext()
	: mDepth(0)
	, mShowIDElement(true)
	, mShowRefCount(true)
    , mShowSpaces(true)
{
	mpEngine = ForthEngine::GetInstance();
}

ForthShowContext::~ForthShowContext()
{
}

void ForthShowContext::Reset()
{
	mShownObjects.clear();
	mObjects.clear();
	mDepth = 0;
}

ulong ForthShowContext::GetDepth()
{
	return mDepth;
}


void ForthShowContext::BeginIndent()
{
	mDepth++;
}

void ForthShowContext::EndIndent()
{
	mDepth--;
	if (mDepth == 0)
	{
		Reset();
	}
}

void ForthShowContext::ShowIndent(const char* pText)
{
    if (mShowSpaces)
    {
        for (uint32 i = 0; i < mDepth; i++)
        {
            ShowText("  ");
        }

    }

	if (pText != NULL)
	{
		ShowText(pText);
	}
}

void ForthShowContext::BeginElement(const char* pName)
{
    if (mNumShown != 0)
    {
        ShowTextReturn(",");
    }

    ShowIndent("\"");
    ShowText(pName);
    ShowText(mShowSpaces ? "\" : " : "\":");
    mNumShown++;
}

void ForthShowContext::BeginRawElement(const char* pName)
{
    if (mNumShown != 0)
    {
        ShowTextReturn(",");
    }

    ShowIndent();
    ShowText(pName);
    ShowText(mShowSpaces ? " : " : ":");
    mNumShown++;
}

void ForthShowContext::BeginLinkElement(const ForthObject& obj)
{
    if (mNumShown != 0)
    {
        ShowTextReturn(",");
    }

    ShowIndent();
    ShowObjectLink(obj);
    ShowText(mShowSpaces ? " : " : ":");
    mNumShown++;
}

void ForthShowContext::BeginArrayElement()
{
    if (mNumShown != 0)
    {
        ShowText(mShowSpaces ? ", " : ",");
    }
    mNumShown++;
}

void ForthShowContext::BeginFirstElement(const char* pText)
{
	ShowIndent("\"");
	ShowText(pText);
    ShowText(mShowSpaces ? "\" : " : "\":");
}

void ForthShowContext::BeginNextElement(const char* pText)
{
	EndElement(",");
	ShowIndent("\"");
	ShowText(pText);
    ShowText(mShowSpaces ? "\" : " : "\":");
}

void ForthShowContext::EndElement(const char* pEndText)
{
    ShowText(pEndText);
}

void ForthShowContext::AddObject(ForthObject& obj)
{
	if (mShownObjects.insert(obj.pData).second)
	{
		mObjects.push_back(obj);
	}
}

bool ForthShowContext::ObjectAlreadyShown(ForthObject& obj)
{
	return mShownObjects.find(obj.pData) != mShownObjects.end();
}

std::vector<ForthObject>& ForthShowContext::GetObjects()
{
	return mObjects;
}

void ForthShowContext::ShowHeader(ForthCoreState* pCore, const char* pTypeName, const void* pData)
{
	char buffer[16];

	ShowTextReturn("{");
    mNumShown = 0;

    ShowIDElement(pTypeName, pData);

	if (mShowRefCount)
	{
        if (mNumShown != 0)
        {
            EndElement(",");
        }

        ShowIndent();
        ShowText(mShowSpaces ? "\"__refCount\" : " : "\"__refCount\":");
		sprintf(buffer, "%d,", *(int *)(pData));
		EndElement(buffer);
        mNumShown++;
	}
}

void ForthShowContext::ShowID(const char* pTypeName, const void* pData)
{
	char buffer[16];

	ShowText(pTypeName);
	sprintf(buffer, "_%08x", pData);
	ShowText(buffer);
}

void ForthShowContext::ShowIDElement(const char* pTypeName, const void* pData)
{
	if (mShowIDElement)
	{
        ShowIndent(mShowSpaces ? "\"__id\" : \"" : "\"__id\":\"");
        ShowID(pTypeName, pData);
        ShowText("\"");
        mNumShown++;
    }
}

void ForthShowContext::ShowText(const char* pText)
{
    if (pText != NULL)
    {
        mpEngine->ConsoleOut(pText);
    }
}

void ForthShowContext::ShowTextReturn(const char* pText)
{
    ShowText(pText);
    if (mShowSpaces)
    {
        ShowText("\n");
    }
}

void ForthShowContext::BeginNestedShow()
{
    mNumShownStack.push_back(mNumShown);
    mNumShown = 0;
}

void ForthShowContext::EndNestedShow()
{
    mNumShown = mNumShownStack.back();
    mNumShownStack.pop_back();
}

void ForthShowContext::BeginArray()
{
    EndElement("[");
    BeginIndent();
    ShowIndent();
}

void ForthShowContext::EndArray()
{
    EndIndent();
    EndElement();
    ShowIndent("]");
}


void ForthShowContext::BeginObject(const char* pName, const void* pData)
{
    ShowTextReturn("{");
    BeginIndent();
    ShowIDElement(pName, pData);
}


void ForthShowContext::EndObject()
{
    EndIndent();
    ShowTextReturn();
    ShowIndent("}");
}

void ForthShowContext::ShowObjectLink(const ForthObject& obj)
{
    const ForthClassObject* pClassObject = (const ForthClassObject *)(*((obj.pMethodOps) - 1));
    const char* pTypeName = pClassObject->pVocab->GetName();
    ShowText("\"@");
    ShowID(pTypeName, obj.pData);
    ShowText("\"");
}

