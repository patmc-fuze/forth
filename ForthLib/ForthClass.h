//////////////////////////////////////////////////////////////////////
//
// ForthClass.h: interface for the ForthClass class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FORTHCLASS_H__4B764080_16F1_11D8_B166_00045A5DC0CC__INCLUDED_)
#define AFX_FORTHCLASS_H__4B764080_16F1_11D8_B166_00045A5DC0CC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


typedef enum {
    kForthClassPrivate,
    kForthClassProtected,
    kForthClassPublic
} eForthClassVisibility;

class ForthVocabulary;
class ForthEngine;

#define CLASS_MAGIC_NUMBER 0xDEADBEEF
#define CLASS_EXTRA_ENTRIES 3
#define CLASS_MAX_METHODS 128

// Each class defined in forth has a table of longwords in memory which
// is used by the inner interpreter to dispatch to class methods.
// The layout of that table is:
//
// long[0]    ptr to ForthClass object for this class
// long[1]    magic number CLASS_MAGIC_NUMBER for error checking
// long[2]    # of method IPs in this table
// long[3]    IP for method 0
// long[4]    IP for method 1
// ...
// long[N]    IP for method (N-3)

class ForthClass  
{
public:
    ForthClass( ForthEngine *pEngine );
    virtual ~ForthClass();

    void                AddMethod( char *pSym, long *pMethodIP );
    void                EndClassDefinition( void );

    void                SetBaseClass( ForthClass *pClass );
    inline ForthClass * GetBaseClass( void ) { return mpBaseClass; };
    void                SetVisibility( eForthClassVisibility vis );
    inline int          GetNumMethods( void ) { return mNumTotalMethods; };

private:
    static ForthClass * mpChainHead;        // head of chain which links all ForthClasses

    ForthClass *        mpChainNext;        // link in chain of all ForthClasses
    ForthClass *        mpBaseClass;        // our base class
    int                 mNumNewMethods;     // # of methods which this class defines, not counting its base class
    int                 mNumTotalMethods;   // # of methods which this class defines, counting its base class
    eForthClassVisibility   mVisibility;        // current visibility setting
    eForthClassVisibility   mvInheritance;      // visibility of inheritance from base class
    ForthVocabulary *       mpPublicVocab;
    ForthVocabulary *       mpProtectedVocab;
    ForthVocabulary *       mpPrivateVocab;
    ForthVocabulary *       mpDefinitionVocab;
    long *              mpClassTable;       // pointer to forth class descriptor
    ForthEngine *       mpEngine;
};


#endif // !defined(AFX_FORTHCLASS_H__4B764080_16F1_11D8_B166_00045A5DC0CC__INCLUDED_)
