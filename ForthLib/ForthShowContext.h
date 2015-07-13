#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthShowContext.h: interface for the ForthShowContext class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"
#include <set>
#include <vector>

class ForthEngine;

class ForthShowContext
{
public:
	ForthShowContext();
	~ForthShowContext();

	void Reset();
	ulong GetDepth();

	void BeginIndent();
	void EndIndent();
	void ShowIndent(const char* pText = NULL);
	void EndElement(const char* pEndText = NULL);
	void ShowHeader(ForthCoreState* pCore, const char* pTypeName, const void* pData);
	void ShowID(const char* pTypeName, const void* pData);
	void ShowIDElement (const char* pTypeName, const void* pData);

	// returns true IFF object has already been shown
	bool AddObject(ForthObject& obj);

	std::vector<ForthObject>& GetObjects();

private:
	ulong mDepth;
	std::set<void *> mShownObjects;
	std::vector<ForthObject> mObjects;
	ForthEngine* mpEngine;
};

