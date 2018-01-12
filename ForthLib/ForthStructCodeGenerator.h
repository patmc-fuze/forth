#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthStructs.h: support for user-defined structures
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

class ForthTypesManager;
class ForthStructVocabulary;
class ForthParseInfo;

class ForthStructCodeGenerator {

public:
	ForthStructCodeGenerator( ForthTypesManager* pTypeManager );
	~ForthStructCodeGenerator();
	
	bool Generate( ForthParseInfo *pInfo, long*& pDst, int dstLongs );
	bool UncompileLastOpcode() { return mCompileVarop != 0; }

protected:
	bool HandleFirst();
	bool HandleMiddle();
	bool HandleLast();
	bool IsLast();
	void HandlePreceedingVarop();
	
	long mTOSTypeCode;
	long mTypeCode;

	ForthParseInfo* mpParseInfo;
    ForthStructVocabulary* mpStructVocab;
    ForthStructVocabulary* mpContainedClassVocab;
    ForthTypesManager* mpTypeManager;
	long* mpDst;
	long* mpDstBase;
	int	mDstLongs;
	char* mpBuffer;
	int mBufferBytes;
	char* mpToken;
	char* mpNextToken;
	ulong mCompileVarop;
	ulong mOffset;
	char mErrorMsg[ 512 ];
    bool mUsesSuper;
};


