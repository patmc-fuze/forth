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
    void BeginElement(const char* pName);
    // BeginRawElement doesn't print quotes around pName
    void BeginRawElement(const char* pName);
    void BeginLinkElement(const ForthObject& obj);
    void BeginArrayElement();
    void BeginFirstElement(const char* pText);
    void BeginNextElement(const char* pText);
	void EndElement(const char* pEndText = NULL);
	void ShowHeader(ForthCoreState* pCore, const char* pTypeName, const void* pData);
	void ShowID(const char* pTypeName, const void* pData);
	void ShowIDElement (const char* pTypeName, const void* pData);
    void ShowObjectLink(const ForthObject& obj);
    void ShowText(const char* pText);
    void ShowTextReturn(const char* pText = nullptr);

    void BeginArray();
    void EndArray();
    void BeginObject(const char* pName, const void* pData);
    void EndObject();

    void AddObject(ForthObject& obj);
	// returns true IFF object has already been shown
	bool ObjectAlreadyShown(ForthObject& obj);

	std::vector<ForthObject>& GetObjects();

	void SetShowIDElement(bool inShow) { mShowIDElement = inShow; }
	bool GetShowIDElement(void) { return mShowIDElement;  }
	void SetShowRefCount(bool inShow) { mShowRefCount = inShow; }
	bool GetShowRefCount(void) { return mShowRefCount; }

    int GetNumShown() { return mNumShown; }
    int IncrementNumShown() { ++mNumShown; return mNumShown; }

    // show code calls BeginNestedShow before showing a nested object,
    // and calls EndNestedShow after showing the nested object
    void BeginNestedShow();
    void EndNestedShow();

private:
	ulong mDepth;
    int mNumShown; // num elements shown at current depth
	std::set<void *> mShownObjects;
	std::vector<ForthObject> mObjects;
    std::vector<int> mNumShownStack;
	ForthEngine* mpEngine;
	bool mShowIDElement;
	bool mShowRefCount;
    bool mShowSpaces;
};

