//////////////////////////////////////////////////////////////////////
//
// ForthVocabulary.h: interface for the ForthVocabulary class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_FORTH_VOCABULARY_H_INCLUDED_)
#define _FORTH_VOCABULARY_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"
#include "ForthForgettable.h"

class ForthParseInfo;
class ForthEngine;

// default initial vocab size in bytes
#define DEFAULT_VOCAB_STORAGE 512

// by default, the value field of a vocabulary entry is 2 longwords
//   but this can be overridden in the constructor
#define DEFAULT_VALUE_FIELD_LONGS 2

class ForthVocabulary : public ForthForgettable
{
public:
    ForthVocabulary( const char *pName = NULL,
                     int valueLongs = DEFAULT_VALUE_FIELD_LONGS,
                     int storageBytes = DEFAULT_VOCAB_STORAGE,
                     void* pForgetLimit = NULL,
                     long op = 0 );
    virtual ~ForthVocabulary();

    virtual void        ForgetCleanup( void *pForgetLimit, long op );

    void                SetName( const char *pVocabName );
    char *              GetName( void );

    void                Empty( void );

    void                        SetNextSearchVocabulary( ForthVocabulary *pNext );
    virtual ForthVocabulary *   GetNextSearchVocabulary( void );

    // add symbol to symbol table, return ptr to new symbol entry
    virtual long*       AddSymbol( const char *pSymName, long symType, long symValue, bool addToEngineOps );

    // copy a symbol table entry, presumably from another vocabulary
    void                CopyEntry( long *pEntry );

    // delete single symbol table entry
    void                DeleteEntry( long *pEntry );

    // delete symbol entry and all newer entries
    // return true IFF symbol was forgotten
    virtual bool        ForgetSymbol( const char   *pSymName );

    // forget all ops with a greater op#
    virtual void        ForgetOp( long op );

    // return pointer to symbol entry, NULL if not found
    // ppFoundVocab will be set to the vocabulary the symbol was actually found in
    // set ppFoundVocab to NULL to search just this vocabulary (not the search chain)
    virtual long *      FindSymbol( const char *pSymName, ForthVocabulary** ppFoundVocab=NULL );
    // return pointer to symbol entry, NULL if not found, given its value
    virtual long *      FindSymbolByValue( long val, ForthVocabulary** ppFoundVocab=NULL );

    // return pointer to symbol entry, NULL if not found
    // pSymName is required to be a longword aligned address, and to be padded with 0's
    // to the next longword boundary
    virtual long *      FindSymbol( ForthParseInfo *pInfo, ForthVocabulary** ppFoundVocab=NULL );

    // compile/interpret symbol if recognized
    // return pointer to symbol entry, NULL if not found
    // pSymName is required to be a longword aligned address, and to be padded with 0's
    // to the next longword boundary
    virtual long *      ProcessSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus );

    // return a string telling the type of library
    virtual const char* GetType( void );

    // the symbol for the word which is currently under construction is "smudged"
    // so that if you try to reference that symbol in its own definition, the match
    // will fail, and an earlier symbol with the same name will be compiled instead
    // this is to allow you to re-define a symbol by layering on top of an earlier
    // symbol of the same name
    void                SmudgeNewestSymbol( void );
    // unsmudge a symbol when its definition is complete
    void                UnSmudgeNewestSymbol( void );

    inline long *       GetFirstEntry( void ) {
        return mpStorageBottom;
    };

    inline int          GetNumEntries( void )
    {
        return mNumSymbols;
    };

    // find next entry in vocabulary
    // invoker is responsible for not going past end of vocabulary
    inline long *       NextEntry( long *pEntry ) {
        long *pTmp = pEntry + mValueLongs;
        // add in 1 for length, and 3 for longword rounding
        return (pTmp + ( (( ((char *) pTmp)[0] ) + 4) >> 2));
    };

    static inline forthOpType   GetEntryType( const long *pEntry ) {
        return FORTH_OP_TYPE( *pEntry );

    };

    static inline void          SetEntryType( long *pEntry, forthOpType opType ) {
        *pEntry = COMPILED_OP( opType, FORTH_OP_VALUE( *pEntry ) );
    };

    static inline long          GetEntryValue( const long *pEntry ) {
        return FORTH_OP_VALUE( *pEntry );
    };

    inline long *               GetNewestEntry( void )
    {
        return mpNewestSymbol;
    };

    inline char *               GetEntryName( const long *pEntry ) {
        return ((char *) pEntry) + (mValueLongs << 2) + 1;
    };

    inline int                  GetEntryNameLength( const long *pEntry ) {
        return (int) *(((char *) pEntry) + (mValueLongs << 2));
    };

    static inline ForthVocabulary *GetVocabularyChainHead( void ) {
        return mpChainHead;
    };

    inline ForthVocabulary *GetNextChainVocabulary( void ) {
        return mpChainNext;
    };

    inline char *NewestSymbol( void ) {
        return &(mNewestSymbol[0]);
    };

protected:

    static ForthVocabulary *mpChainHead;
    ForthEngine         *mpEngine;
    ForthVocabulary     *mpChainNext;
    ForthVocabulary     *mpSearchNext;
    int                 mNumSymbols;
    int                 mStorageLongs;
    long                *mpStorageBase;
    long                *mpStorageTop;
    long                *mpStorageBottom;
    long                *mpNewestSymbol;
    char                *mpName;
    int                 mValueLongs;
    char                mNewestSymbol[ 256 ];
};

// the only difference between ForthPrecedenceVocabulary and ForthVocabulary is
//  that opcodes in this vocabulary are executed even when in compile mode
class ForthPrecedenceVocabulary : public ForthVocabulary
{
public:
    ForthPrecedenceVocabulary( const char *pName=NULL,
                               int valueLongs=NUM_FORTH_VOCAB_VALUE_LONGS, int storageBytes=DEFAULT_VOCAB_STORAGE );
    virtual ~ForthPrecedenceVocabulary();

    // interpret symbol if recognized
    // return pointer to symbol entry, NULL if not found
    // pSymName is required to be a longword aligned address, and to be padded with 0's
    // to the next longword boundary
    virtual long *      ProcessSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus );

    virtual ForthVocabulary *   GetNextSearchVocabulary( void );
};

class ForthLocalVocabulary : public ForthVocabulary
{
public:
    ForthLocalVocabulary( const char *pName=NULL,
                               int valueLongs=NUM_LOCALS_VOCAB_VALUE_LONGS, int storageBytes=DEFAULT_VOCAB_STORAGE );
    virtual ~ForthLocalVocabulary();

    // return a string telling the type of library
    virtual const char* GetType( void );
};

class ForthDLLVocabulary : public ForthVocabulary
{
public:
    ForthDLLVocabulary( const char *pVocabName,
                        const char *pDLLName,
                        int valueLongs = DEFAULT_VALUE_FIELD_LONGS,
                        int storageBytes = DEFAULT_VOCAB_STORAGE,
                        void* pForgetLimit = NULL,
                        long op = 0 );
    virtual ~ForthDLLVocabulary();

    // return a string telling the type of library
    virtual const char* GetType( void );

    long                LoadDLL( void );
    void                UnloadDLL( void );
    long *              AddEntry( const char* pFuncName, long numArgs );
protected:
    char *              mpDLLName;
    HINSTANCE           mhDLL;
};



#endif
