//////////////////////////////////////////////////////////////////////
//
// ForthObjectReader.cpp: implementation of the JSON Object reader.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <stdexcept>
#include "ForthObjectReader.h"
//#include "ForthEngine.h"

/*
    todos:
    - handle right hand sides
    - how to handle current object data pointer when changing levels
    - run _init op for classes that have one
    ? look for elements in same order as they would be printed, or just search for them?
*/

ForthObjectReader::ForthObjectReader()
: mOutArray(nullptr)
, mInStream(nullptr)
, mSavedChar('\0')
, mHaveSavedChar(false)
, mOutArrayObject({ nullptr, nullptr })
, mInStreamObject({ nullptr, nullptr })
, mObjectData(nullptr)
, mCurrentVocab(nullptr)
, mCurrentObjectIndex(-1)
{
    mpEngine = ForthEngine::GetInstance();
    mpCore = mpEngine->GetCoreState();
}

ForthObjectReader::~ForthObjectReader()
{
}

bool ForthObjectReader::ReadObjects(ForthObject& inStream, ForthObject& outArray)
{
    mInStreamObject = inStream;
    mOutArrayObject = outArray;
    mInStream = reinterpret_cast<oInStreamStruct *>(inStream.pData);
    mOutArray = reinterpret_cast<oArrayStruct *>(mOutArrayObject.pData);
    mObjectStack.clear();
    mCurrentObjectIndex = -1;
    mCurrentVocab = nullptr;
    mLineNum = 1;
    mCharOffset = 0;

    bool itWorked = true;
    mError.clear();

    try
    {
        if ((mInStream->pInFuncs == nullptr) || (mInStream->pInFuncs->inChar == nullptr))
        {
            throwError("unusable input stream");
        }
        getObject();
    }
    catch (const std::exception& ex)
    {
        
        mError.assign(ex.what());
        itWorked = false;
    }

    return itWorked;
}


char ForthObjectReader::getRawChar()
{
    int ch;
    mInStream->pInFuncs->inChar(mpCore, mInStreamObject.pData, ch);
    if (ch == -1)
    {
        throwError("unexpected EOF");
    }

    if (ch == '\n')
    {
        mLineNum++;
        mCharOffset = 0;
    }
    else
    {
        mCharOffset++;
    }

    return (char)ch;
}


char ForthObjectReader::getChar()
{
    if (mHaveSavedChar)
    {
        mHaveSavedChar = false;
        return mSavedChar;
    }

    skipWhitespace();
    if (mHaveSavedChar)
    {
        mHaveSavedChar = false;
        return mSavedChar;
    }
    return getRawChar();
}

void ForthObjectReader::getRequiredChar(char requiredChar)
{
    char actualChar = getChar();
    if (actualChar != requiredChar)
    {
        throwError("unexpected character");
    }
}

void ForthObjectReader::ungetChar(char ch)
{
    if (mHaveSavedChar)
    {
        throwError("can't unget more than one char");
    }
    else
    {
        mHaveSavedChar = true;
        mSavedChar = ch;
    }
}

void ForthObjectReader::getName(std::string& name)
{
    getString(name);
}

void ForthObjectReader::getString(std::string& str)
{
    getRequiredChar('\"');
    str.clear();
    char ch;
    while ((ch = getRawChar()) != '\"')
    {
        str.push_back(ch);
    }
}

void ForthObjectReader::getNumber(std::string& str)
{
    str.clear();
    skipWhitespace();
    while (true)
    {
        char ch = getChar();
        if (ch < '0' || ch > '9')
        {
            if (ch != '-' && ch != '.' && ch != 'e')
            {
                ungetChar(ch);
                break;
            }
        }
        str.push_back(ch);
    }
}

void ForthObjectReader::skipWhitespace()
{
    while (true)
    {
        char ch = getRawChar();
        if (strchr(" \t\r\n", ch) == nullptr)
        {
            ungetChar(ch);
            break;
        }
    }
}

void ForthObjectReader::getObject()
{
    getRequiredChar('{');
    mObjectStack.push_back(mCurrentObjectIndex);
    mCurrentVocab = nullptr;
    bool done = false;
    while (!done)
    {
        char ch = getChar();

        if (ch == '}')
        {
            done = true;
        }
        else if (ch == '\"')
        {
            std::string elementName;
            ungetChar(ch);
            getName(elementName);
            getRequiredChar(':');
            processElement(elementName);
            char nextChar = getChar();
            if (nextChar != ',')
            {
                done = true;
                ungetChar(nextChar);
            }
        }
        else
        {
            throwError("unexpected char in getObject");
        }
    }
    getRequiredChar('}');
    mCurrentObjectIndex = mObjectStack.back();
    mObjectStack.pop_back();
    if (mCurrentObjectIndex >= 0)
    {
        ForthObject& currentObject = mOutArray->elements->at(mCurrentObjectIndex);
        ForthClassObject* pClassObject = (ForthClassObject *)(currentObject.pMethodOps[-1]);
        mCurrentVocab = pClassObject->pVocab;
        mObjectData = (char *)currentObject.pData;
    }
    else
    {
        mCurrentVocab = nullptr;
        mObjectData = nullptr;
    }
}

void ForthObjectReader::processElement(const std::string& name)
{
    if (mCurrentVocab == nullptr)
    {
        // name has to be __id, value has to be string with form CLASSNAME_ID
        if (name == "__id")
        {
            // class name is part of __id up to last underscore
            std::string classId;
            getString(classId);
            size_t lastUnderscore = classId.find_last_of('_');
            if (lastUnderscore == classId.npos)
            {
                throwError("__id missing underscore");
            }
            if (lastUnderscore < 1)
            {
                throwError("__id has empty class name");
            }
            std::string className = classId.substr(0, lastUnderscore);

            ForthEngine* pEngine = ForthEngine::GetInstance();
            ForthCoreState *pCore = pEngine->GetCoreState();
            ForthStructVocabulary* newClassVocab = ForthTypesManager::GetInstance()->GetStructVocabulary(className.c_str());
            if (newClassVocab->IsClass())
            {
                mCurrentVocab = static_cast<ForthClassVocabulary *>(newClassVocab);
                mCurrentObjectIndex = mOutArray->elements->size();

                long initOpcode = mCurrentVocab->GetInitOpcode();
                SPUSH((long)mCurrentVocab);
                pEngine->FullyExecuteOp(pCore, mCurrentVocab->GetClassObject()->newOp);
                if (initOpcode != 0)
                {
                    // copy object data pointer to TOS to be used by init 
                    long a = (GET_SP)[1];
                    SPUSH(a);
                    mpEngine->FullyExecuteOp(pCore, initOpcode);
                }

                ForthObject newObject;
                POP_OBJECT(newObject);
                // new object has refcount of 1, since it is in outArray
                *newObject.pData = 1;
                mOutArray->elements->push_back(newObject);
                mObjectData = (char *) newObject.pData;
            }
            else
            {
                throwError("not a class vocabulary");
            }
        }
        else
        {
            throwError("first element is not __id");
        }
    }
    else
    {
        // ignore __refcount
        if (name == "__refCount")
        {
            // discard following ref count
            std::string unusedCount;
            getNumber(unusedCount);
        }
        else
        {
            // lookup name in current vocabulary to see how to process
            long* pEntry = mCurrentVocab->FindSymbol(name.c_str());
            if (pEntry != nullptr)
            {
                // TODO - handle number, string, object, array
                long elementSize = VOCABENTRY_TO_ELEMENT_SIZE(pEntry);
                if (elementSize == 0)
                {
                    throwError("attempt to read zero size element");
                }
                long typeCode = VOCABENTRY_TO_TYPECODE(pEntry);
                long byteOffset = VOCABENTRY_TO_FIELD_OFFSET(pEntry);
                long baseType = CODE_TO_BASE_TYPE(typeCode);
                //bool isNative = CODE_IS_NATIVE(typeCode);
                bool isPtr = CODE_IS_PTR(typeCode);
                bool isArray = CODE_IS_ARRAY(typeCode);
                if (isPtr)
                {
                    baseType = kBaseTypeInt;
                    isArray = false;
                }
                float fval;
                double dval;
                int ival;
                long long lval;
                std::string str;
                char *pDst = mObjectData + byteOffset;
                int roomLeft = mCurrentVocab->GetSize() - byteOffset;

                if (isArray)
                {
                    getRequiredChar('[');
                }

                bool done = false;
                while (!done)
                {
                    int bytesConsumed = 0;
                    switch (baseType)
                    {
                    case kBaseTypeByte:          // 0 - byte
                    case kBaseTypeUByte:         // 1 - ubyte
                    {
                        getNumber(str);
                        if (roomLeft < 1)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%d", &ival) != 1)
                        {
                            throwError("failed to parse byte");
                        }
                        *pDst = (char)ival;
                        bytesConsumed = 1;
                        break;
                    }

                    case kBaseTypeShort:         // 2 - short
                    case kBaseTypeUShort:        // 3 - ushort
                    {
                        getNumber(str);
                        if (roomLeft < 2)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%d", &ival) != 1)
                        {
                            throwError("failed to parse short");
                        }
                        *(short *)pDst = (short)ival;
                        bytesConsumed = 2;
                        break;
                    }

                    case kBaseTypeInt:           // 4 - int
                    case kBaseTypeUInt:          // 5 - uint
                    case kBaseTypeOp:            // 11 - op
                    {
                        getNumber(str);
                        if (roomLeft < 4)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%d", &ival) != 1)
                        {
                            throwError("failed to parse int");
                        }
                        *(int *)pDst = ival;
                        bytesConsumed = 4;
                        break;
                    }

                    case kBaseTypeLong:          // 6 - long
                    case kBaseTypeULong:         // 7 - ulong
                    {
                        getNumber(str);
                        if (roomLeft < 8)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%lld", &lval) != 1)
                        {
                            throwError("failed to parse long");
                        }
                        *(long long *)pDst = lval;
                        bytesConsumed = 8;
                        break;
                    }

                    case kBaseTypeFloat:         // 8 - float
                    {
                        getNumber(str);
                        if (roomLeft < 4)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%g", &fval) != 1)
                        {
                            throwError("failed to parse float");
                        }
                        *(float *)pDst = (short)fval;
                        bytesConsumed = 4;
                        break;
                    }

                    case kBaseTypeDouble:        // 9 - double
                    {
                        getNumber(str);
                        if (roomLeft < 8)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%g", &dval) != 1)
                        {
                            throwError("failed to parse double");
                        }
                        *(double *)pDst = dval;
                        bytesConsumed = 8;
                        break;
                    }

                    case kBaseTypeString:        // 10 - string
                    {
                        getString(str);
                        int maxBytes = CODE_TO_STRING_BYTES(typeCode);
                        if (str.size() >= maxBytes)
                        {
                            throwError("string too big");
                        }
                        strcpy(pDst + 8, str.c_str());
                        bytesConsumed = 8 + (((maxBytes + 3) >> 2) << 2);
                        break;
                    }

                    case kBaseTypeObject:      // 12 - object
                    {
                        char ch = getChar();
                        if (ch == '\"')
                        {
                            // this must be an object link
                            ungetChar(ch);
                            getString(str);
                            auto foundObj = mKnownObjects.find(str);
                            if (foundObj != mKnownObjects.end())
                            {
                                ForthObject linkedObject = mOutArray->elements->at(foundObj->second);
                                *(ForthObject*)pDst = linkedObject;
                                bytesConsumed = 8;

                                // bump linked object refcount
                                *(linkedObject.pData) += 1;
                            }
                            else
                            {
                                throwError("unknown linked object");
                            }
                        }
                        else if (ch == '{')
                        {
                            // this must be an object
                            ungetChar(ch);
                            getObject();
                            *(ForthObject*)pDst = mOutArray->elements->back();
                            bytesConsumed = 8;
                        }
                        else
                        {
                            throwError("unexpected char at start of object");
                        }
                        break;
                    }

                    case kBaseTypeStruct:                        // 13 - struct
                        // TODO!
                        throwError("structs not supported yet");
                        break;

                    default:
                        throwError("unexpected type found");
                        break;
                    }
                    roomLeft -= bytesConsumed;
                    pDst += bytesConsumed;

                    if (isArray)
                    {
                        char endCh = getChar();
                        if (endCh == ']')
                        {
                            ungetChar(endCh);
                            done = true;
                        }
                        else if (endCh != ',')
                        {
                            ungetChar(endCh);
                        }
                    }
                    else
                    {
                        done = true;
                    }
                }

                if (isArray)
                {
                    getRequiredChar(']');
                }
            }
            else
            {
                throwError("name not found");
            }
        }
    }
}

void ForthObjectReader::throwError(const char* message)
{
    // TODO: add line, offset to message
    char buffer[128];
    sprintf(buffer, "%s at line %d character %d",
        message, mLineNum, mCharOffset);

    throw std::runtime_error(buffer);
}

void ForthObjectReader::throwError(const std::string& message)
{
    // TODO: add line, offset to message
    throwError(message.c_str());
}
