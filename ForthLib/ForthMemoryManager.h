#pragma once

#include "Forth.h"

// memory allocation wrappers
#define __MALLOC s_memoryManager->allocate
#define __REALLOC s_memoryManager->resize
#define __FREE s_memoryManager->free

extern void initMemoryAllocation();

struct ForthMemoryBucket
{
    int numAllocs;
    int numFrees;
    int maxUsed;   // maximum of numAllocs - numFrees over time
    int size;
    long * freeChain;
};

class ForthMemoryManager
{
public:
    virtual ~ForthMemoryManager();
    virtual void* allocate(size_t numBytes) = 0;
    virtual void* resize(void *pMemory, size_t numBytes) = 0;
    virtual void free(void* pBlock) = 0;
    virtual ForthMemoryBucket* getBucketInfo(int bucketSize);
};

extern ForthMemoryManager* s_memoryManager;

class PassThruMemoryManager : public ForthMemoryManager
{
public:
    void* allocate(size_t numBytes) override;
    void* resize(void *pMemory, size_t numBytes) override;
    void free(void* pBlock) override;
};

class DebugPassThruMemoryManager : public ForthMemoryManager
{
public:
    void* allocate(size_t numBytes) override;
    void* resize(void *pMemory, size_t numBytes) override;
    void free(void* pBlock) override;
};
