#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthObjectReader.h: interfaces for the JSON Object reader.
//
//////////////////////////////////////////////////////////////////////

#include <map>
#include <string>
#include "ForthEngine.h"
#include "ForthBuiltinClasses.h"

class ForthClassVocabulary;

class ForthObjectReader
{
public:
    ForthObjectReader();
    ~ForthObjectReader();

    // returns true if there were no errors
    bool ReadObjects(ForthObject& inStream, ForthObject& outObjects);
    const std::string& GetError() const { return mError; }

private:

    char getChar();
    char getRawChar();
    void getRequiredChar(char ch);
    void ungetChar(char ch);
    void getName(std::string& name);
    void getString(std::string& str);
    void getNumber(std::string& str);
    void skipWhitespace();
    void getObject();
    void getStruct();
    void processElement(const std::string& name);
    void throwError(const char* message);
    void throwError(const std::string& message);

    typedef std::map<std::string, int> knownObjectMap;
    typedef struct
    {
        ForthStructVocabulary* pVocab;
        int objIndex;
        char* pData;
    } readerContext;

    ForthObject mInStreamObject;
    ForthObject mOutArrayObject;
    oInStreamStruct* mInStream;
    oArrayStruct* mOutArray;
    char* mObjectData;
    int mCurrentObjectIndex;
    std::vector<ForthObject> mObjects;
    std::vector<readerContext> mContextStack;

    ForthClassVocabulary* mCurrentVocab;

    std::vector<int> mObjectStack;

    knownObjectMap mKnownObjects;

    ForthEngine *mpEngine;
    ForthCoreState* mpCore;

    std::string mError;

    char mSavedChar;
    bool mHaveSavedChar;

    int mLineNum;
    int mCharOffset;
};
