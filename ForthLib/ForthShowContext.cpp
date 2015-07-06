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
	ForthEngine* pEngine = ForthEngine::GetInstance();

	for (int i = 0; i < mDepth; i++)
	{
		pEngine->ConsoleOut("  ");
	}
	if (pText != NULL)
	{
		pEngine->ConsoleOut(pText);
	}
}

void ForthShowContext::EndElement(const char* pEndText)
{
	ForthEngine* pEngine = ForthEngine::GetInstance();

	if (pEndText != NULL)
	{
		pEngine->ConsoleOut(pEndText);
	}
	// TODO: do this based on prettyPrint flag
	pEngine->ConsoleOut("\n");
}

bool ForthShowContext::AddObject(ForthObject& obj)
{
	if (mShownObjects.find(obj.pData) == mShownObjects.end())
	{
		mShownObjects.insert(obj.pData);
		mObjects.push_back(obj);
		return false;
	}
	return true;
}

std::vector<ForthObject>& ForthShowContext::GetObjects()
{
	return mObjects;
}

