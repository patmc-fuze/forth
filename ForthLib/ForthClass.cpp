//////////////////////////////////////////////////////////////////////
//
// ForthClass.cpp: implementation of the ForthClass class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthClass.h"
#include "ForthVocabulary.h"

//////////////////////////////////////////////////////////////////////
////
///     ForthClass - there is one of these defined for each "class" which
//          the user defines in forth
//
//

// TBDS:
// - need to modify "int", "float", "double" and "string" to define member vars
// - need to modify outer interpreter to recognize objName.methodName syntax
// - need to modify outer interpreter to compile methods and member references
//   while inside a class definition
// - need to design constructor/destructor stuff
// ? is there an "object" base class with new/delete/addr_of methods
// - need to work out how "forget" interacts with classes

// all ForthClasses are linked by this chain
ForthClass * ForthClass::mpChainHead = NULL;


ForthClass::ForthClass( ForthEngine *pEngine )
: mNumTotalMethods( 0 )
, mNumNewMethods( 0 )
, mVisibility( kForthClassPublic )
, mpEngine( pEngine )
, mpBaseClass( NULL )
{
    mpChainNext = mpChainHead;
    mpChainHead = this;

    mpClassTable = new long [CLASS_MAX_METHODS + CLASS_EXTRA_ENTRIES];
    mpClassTable[0] = (long) this;
    mpClassTable[1] = 0;        // bad magic number
    mpClassTable[2] = 0;        // zero methods defined

    mpPrivateVocab = new ForthVocabulary( mpEngine, "private", DEFAULT_VALUE_FIELD_LONGS, 64 );
    mpProtectedVocab = new ForthVocabulary( mpEngine, "protected", DEFAULT_VALUE_FIELD_LONGS, 64 );
    mpPublicVocab = new ForthVocabulary( mpEngine, "public", DEFAULT_VALUE_FIELD_LONGS, 64 );
    SetVisibility( mVisibility );
}


ForthClass::~ForthClass()
{
    // remove ourself from chain of all classes
    // TBD: do we have to worry about case where a base class can be
    //   deleted before classes which are derived from it
    ForthClass **mppNext = &mpChainHead;
    while ( mppNext != NULL ) {
        if ( *mppNext == this ) {
            *mppNext = mpChainNext;
            break;
        }
        mppNext = &((*mppNext)->mpChainNext);
    }

    delete [] mpClassTable;

    delete mpProtectedVocab;
    delete mpPublicVocab;
    if ( mpPrivateVocab != NULL ) {
        delete mpPrivateVocab;
    }
}


// TBD! need to deal with member variable symbols...

void
ForthClass::AddMethod( char *pSym, long *pMethodIP )
{
    mpDefinitionVocab->AddSymbol( pSym, 0, mNumTotalMethods++, false );
    mpClassTable[mNumNewMethods++] = (long) pMethodIP;
}


void
ForthClass::EndClassDefinition( void )
{
    int i;
    long *pSrc, *pDst, *pNewTable;

    pNewTable = new long[mNumTotalMethods + CLASS_EXTRA_ENTRIES];
    pNewTable[0] = (long) this;
    pNewTable[1] = CLASS_MAGIC_NUMBER;
    pNewTable[2] = mNumTotalMethods;
    pDst = pNewTable + CLASS_EXTRA_ENTRIES;

    if ( mpBaseClass != NULL ) {
        // fill in table with base class methods
        pSrc = mpBaseClass->mpClassTable + CLASS_EXTRA_ENTRIES;
        for ( i = 0; i < mpBaseClass->mNumTotalMethods; i++ ) {
            *pDst++ = *pSrc++;
        }
    }
    pSrc = mpClassTable + CLASS_EXTRA_ENTRIES;
    for ( i = 0; i < mNumNewMethods; i++ ) {
        *pDst++ = *pSrc++;
    }
    delete [] mpClassTable;
    mpClassTable = pNewTable;

    delete mpPrivateVocab;
    mpPrivateVocab = NULL;
}


void
ForthClass::SetBaseClass( ForthClass *pClass )
{
    // TBD! any class methods which are defined before this is called
    //   will have wrong method numbers...
    mpBaseClass = pClass;
    mNumTotalMethods = pClass->mNumTotalMethods + mNumNewMethods;
}


void
ForthClass::SetVisibility( eForthClassVisibility vis )
{
    mVisibility = vis;

    switch( vis ) {
    case kForthClassPrivate:
        mpDefinitionVocab = mpPrivateVocab;
        break;
    case kForthClassProtected:
        mpDefinitionVocab = mpProtectedVocab;
        break;
    case kForthClassPublic:
        mpDefinitionVocab = mpPublicVocab;
        break;
    }
};
