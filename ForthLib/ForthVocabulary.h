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

// default initial vocab size in bytes
#define DEFAULT_VOCAB_STORAGE 512

// by default, the value field of a vocabulary entry is 1 longword
//   but this can be overridden in the constructor
#define DEFAULT_VALUE_FIELD_LONGS 1

class ForthVocabulary : public ForthForgettable
{
public:
    ForthVocabulary( ForthEngine *pEngine, const char *pName=NULL,
                     int valueLongs=DEFAULT_VALUE_FIELD_LONGS, int storageBytes=DEFAULT_VOCAB_STORAGE, void* pForgetLimt=NULL, long op=0 );
    virtual ~ForthVocabulary();

    virtual void        ForgetCleanup( void *pForgetLimit, long op );

    void                SetName( const char *pVocabName );
    char *              GetName( void );

    void                Empty( void );

    void                SetNextSearchVocabulary( ForthVocabulary *pNext );
    ForthVocabulary *   GetNextSearchVocabulary( void );

    // add symbol to symbol table, return symvalue
    virtual long        AddSymbol( const char *pSymName, forthOpType symType, long symValue, bool addToEngineOps );

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

    // compile/interpret symbol if recognized
    // return pointer to symbol entry, NULL if not found
    // pSymName is required to be a longword aligned address, and to be padded with 0's
    // to the next longword boundary
    virtual void *      ProcessSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus );

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

    inline void *               GetNewestEntry( void )
    {
        return mpNewestSymbol;
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
    ForthPrecedenceVocabulary( ForthEngine *pEngine, const char *pName=NULL,
                               int valueLongs=DEFAULT_VALUE_FIELD_LONGS, int storageBytes=DEFAULT_VOCAB_STORAGE );
    virtual ~ForthPrecedenceVocabulary();

    // interpret symbol if recognized
    // return pointer to symbol entry, NULL if not found
    // pSymName is required to be a longword aligned address, and to be padded with 0's
    // to the next longword boundary
    virtual void *      ProcessSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus );
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


#endif
