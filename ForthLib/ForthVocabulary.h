//////////////////////////////////////////////////////////////////////
//
// ForthVocabulary.h: interface for the ForthVocabulary class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FORTHVOCABULARY_H__C43FADC1_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_)
#define AFX_FORTHVOCABULARY_H__C43FADC1_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"

class ForthParseInfo;

// default initial vocab size in bytes
#define DEFAULT_VOCAB_STORAGE 512

// by default, the value field of a vocabulary entry is 1 longword
//   but this can be overridden in the constructor
#define DEFAULT_VALUE_FIELD_LONGS 1

class ForthVocabulary  
{
public:
    ForthVocabulary( ForthEngine *pEngine, const char *pName=NULL,
                     int valueLongs=DEFAULT_VALUE_FIELD_LONGS, int storageBytes=DEFAULT_VOCAB_STORAGE );
    virtual ~ForthVocabulary();

    void                SetName( const char *pVocabName );
    char *              GetName( void );

    void                Empty( void );

    void                SetNextSearchVocabulary( ForthVocabulary *pNext );
    ForthVocabulary *   GetNextSearchVocabulary( void );

    // add symbol to symbol table, return symvalue
    virtual long        AddSymbol( const char *pSymName, int symType, long symValue, bool addToEngineOps );

    // copy a symbol table entry, presumably from another vocabulary
    void                CopyEntry( const void *pEntry );

    // delete single symbol table entry
    void                DeleteEntry( void *pEntry );

    // delete symbol entry and all newer entries
    // return true IFF symbol was forgotten
    virtual bool        ForgetSymbol( const char   *pSymName );

    // forget all ops with a greater op#
    virtual void        ForgetOp( long op );

    // return pointer to symbol entry, NULL if not found
    virtual void *      FindSymbol( const char *pSymName );

    // return pointer to symbol entry, NULL if not found
    // pSymName is required to be a longword aligned address, and to be padded with 0's
    // to the next longword boundary
    virtual void *      FindSymbol( ForthParseInfo *pInfo );

    // the symbol for the word which is currently under construction is "smudged"
    // so that if you try to reference that symbol in its own definition, the match
    // will fail, and an earlier symbol with the same name will be compiled instead
    // this is to allow you to re-define a symbol by layering on top of an earlier
    // symbol of the same name
    void                SmudgeNewestSymbol( void );
    // unsmudge a symbol when its definition is complete
    void                UnSmudgeNewestSymbol( void );

    inline void *       GetFirstEntry( void ) {
        return (void *) mpStorageBottom;
    };

    inline int          GetNumEntries( void )
    {
        return mNumSymbols;
    };

    // find next entry in vocabulary
    // invoker is responsible for not going past end of vocabulary
    inline void *       NextEntry( const void *pEntry ) {
        long *pTmp = ((long *) pEntry) + mValueLongs;
        // add in 1 for length, and 3 for longword rounding
        return (void *) (pTmp + ( (( ((char *) pTmp)[0] ) + 4) >> 2));
    };

    static inline forthOpType   GetEntryType( const void *pEntry ) {
        return FORTH_OP_TYPE( *((long *) pEntry) );

    };

    static inline void          SetEntryType( void *pEntry, forthOpType opType ) {
        *((long *) pEntry) = COMPILED_OP( opType, FORTH_OP_VALUE( *((long *) pEntry) ) );
    };

    static inline long          GetEntryValue( const void *pEntry ) {
        return FORTH_OP_VALUE( *((long *) pEntry) );
    };

    inline char *               GetEntryName( const void *pEntry ) {
        return ((char *) pEntry) + (mValueLongs << 2) + 1;
    };

    inline int                  GetEntryNameLength( const void *pEntry ) {
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


private:

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

class ForthLocalVarVocabulary : public ForthVocabulary
{
public:
    ForthLocalVarVocabulary( ForthEngine *pEngine, const char *pName=NULL, int storageBytes=DEFAULT_VOCAB_STORAGE );
    virtual ~ForthLocalVarVocabulary();

    // delete symbol entry and all newer entries
    // return true IFF symbol was forgotten
    virtual bool        ForgetSymbol( const char   *pSymName );

    // forget all ops with a greater op#
    virtual void        ForgetOp( long op );
};



#endif // !defined(AFX_FORTHVOCABULARY_H__C43FADC1_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_)
