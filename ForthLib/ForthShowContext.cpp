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
	for (uint32 i = 0; i < mDepth; i++)
	{
		mpEngine->ConsoleOut("  ");
	}
	if (pText != NULL)
	{
		mpEngine->ConsoleOut(pText);
	}
}

void ForthShowContext::EndElement(const char* pEndText)
{
	if (pEndText != NULL)
	{
		mpEngine->ConsoleOut(pEndText);
	}
	// TODO: do this based on prettyPrint flag
	mpEngine->ConsoleOut("\n");
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

	EndElement("{");
	ShowIDElement(pTypeName, pData);
	ShowIndent();
	mpEngine->ConsoleOut("'__refCount' : ");
	sprintf(buffer, "%d,", *(int *)(pData));
	EndElement(buffer);
}

void ForthShowContext::ShowID(const char* pTypeName, const void* pData)
{
	char buffer[16];

	mpEngine->ConsoleOut(pTypeName);
	sprintf(buffer, "_%08x", pData);
	mpEngine->ConsoleOut(buffer);
}

void ForthShowContext::ShowIDElement(const char* pTypeName, const void* pData)
{
	ShowIndent("'__id' : '");
	ShowID(pTypeName, pData);
	mpEngine->ConsoleOut("',");
	EndElement();
}

