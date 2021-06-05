//////////////////////////////////////////////////////////////////////
//
// ForthMemoryManager.cpp: implementation of the Forth memory manager classes.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Forth.h"
#include "ForthEngine.h"

#if defined(LINUX) || defined(MACOSX)
#endif

bool __useStandardMemoryAllocation = true;

ForthMemoryManager* s_memoryManager = nullptr;

void initMemoryAllocation()
{
    if (__useStandardMemoryAllocation)
    {
        s_memoryManager = new PassThruMemoryManager;
    }
    else
    {
        s_memoryManager = new DebugPassThruMemoryManager;
    }
}

//////////////////////////////////////////////////////////////////////
////
///     ForthMemoryManager
//
//

ForthMemoryManager::~ForthMemoryManager()
{
}

ForthMemoryBucket* ForthMemoryManager::getBucketInfo(int bucketSize)
{
    return nullptr;
}


//////////////////////////////////////////////////////////////////////
////
///     PassThruMemoryManager
//
//

void* PassThruMemoryManager::allocate(size_t numBytes)
{
    return ::malloc(numBytes);
}
void* PassThruMemoryManager::resize(void *pMemory, size_t numBytes)
{
    return ::realloc(pMemory, numBytes);
}

void PassThruMemoryManager::free(void* pBlock)
{
    ::free(pBlock);
}


//////////////////////////////////////////////////////////////////////
////
///     DebugPassThruMemoryManager
//
//

void* DebugPassThruMemoryManager::allocate(size_t numBytes)
{
    void * pData = ::malloc(numBytes);
    ForthEngine::GetInstance()->TraceOut("allocate %d bytes @ 0x%p\n", numBytes, pData);
    return pData;
}
void* DebugPassThruMemoryManager::resize(void *pMemory, size_t numBytes)
{
    void *pData = ::realloc(pMemory, numBytes);
    ForthEngine::GetInstance()->TraceOut("resize %d bytes @ 0x%p\n", numBytes, pData);
    return pData;
}

void DebugPassThruMemoryManager::free(void* pBlock)
{
    ForthEngine::GetInstance()->TraceOut("free @ 0x%p\n", pBlock);
    ::free(pBlock);
}

