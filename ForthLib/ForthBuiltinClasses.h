#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.h: builtin classes
//
//////////////////////////////////////////////////////////////////////

#include "ForthForgettable.h"

// structtype indices for builtin classes
typedef enum {
    kBCIObject,
    kBCIClass,
	kBCIIter,
	kBCIIterable,
	kBCIArray,
	kBCIArrayIter,
	kBCIList,
	kBCIListIter,
	kBCIMap,
	kBCIMapIter,
	kBCIString,
	kBCIPair,
	kBCIPairIter,
	kBCITriple,
	kBCITripleIter,
	kBCIByteArray,
	kBCIByteArrayIter,
	kBCIShortArray,
	kBCIShortArrayIter,
	kBCIIntArray,
	kBCIIntArrayIter,
	kBCILongArray,
	kBCILongArrayIter,
	kBCIInt,
	kBCILong,
	kBCIFloat,
	kBCIDouble,
	kBCIThread
} kBuiltinClassIndex;

#define TRACK_OBJECT_ALLOCATIONS
#ifdef TRACK_OBJECT_ALLOCATIONS
extern long gStatNews;
extern long gStatDeletes;
extern long gStatLinkNews;
extern long gStatLinkDeletes;
extern long gStatIterNews;
extern long gStatIterDeletes;
extern long gStatKeeps;
extern long gStatReleases;

#define TRACK_NEW			gStatNews++
#define TRACK_DELETE		gStatDeletes++
#define TRACK_LINK_NEW		gStatLinkNews++
#define TRACK_LINK_DELETE	gStatLinkDeletes++
#define TRACK_ITER_NEW		gStatIterNews++
#define TRACK_ITER_DELETE	gStatIterDeletes++
#define TRACK_KEEP			gStatKeeps++
#define TRACK_RELEASE		gStatReleases++
#else
#define TRACK_NEW
#define TRACK_DELETE
#define TRACK_LINK_NEW
#define TRACK_LINK_DELETE
#define TRACK_ITER_NEW
#define TRACK_ITER_DELETE
#define TRACK_KEEP
#define TRACK_RELEASE
#endif

#define METHOD_RETURN \
    SET_TPM( (long *) RPOP ); \
    SET_TPD( (long *) RPOP )

#define METHOD( NAME, VALUE  )          { NAME, (ulong) VALUE, NATIVE_TYPE_TO_CODE( kDTIsMethod, kBaseTypeVoid ) }
#define METHOD_RET( NAME, VAL, RVAL )   { NAME, (ulong) VAL, RVAL }
#define MEMBER_VAR( NAME, TYPE )        { NAME, 0, (ulong) TYPE }
#define MEMBER_ARRAY( NAME, TYPE, NUM ) { NAME, NUM, (ulong) (TYPE | kDTIsArray) }

#define END_MEMBERS { NULL, 0, 0 }

#define INVOKE_METHOD( _obj, _methodNum ) ForthEngine::GetInstance()->ExecuteOneMethod( _obj, _methodNum )
/*
#define INVOKE_METHOD( _obj, _methodNum ) \
    RPUSH( ((long) GET_TPD) ); \
    RPUSH( ((long) GET_TPM) ); \
    SET_TPM( (_obj).pMethodOps ); \
    SET_TPD( (_obj).pData ); \
    ForthEngine::GetInstance()->ExecuteOneOp( (_obj).pMethodOps[ (_methodNum) ] )
*/

#define PUSH_PAIR( _methods, _data )    SPUSH( (long) (_data) ); SPUSH( (long) (_methods) )
#define POP_PAIR( _methods, _data )     (_methods) = (long *) SPOP; (_data) = (long *) SPOP
#define PUSH_OBJECT( _obj )             PUSH_PAIR( (_obj).pMethodOps, (_obj).pData )
#define POP_OBJECT( _obj )              POP_PAIR( (_obj).pMethodOps, (_obj).pData )

#define MALLOCATE( _type, _ptr ) _type* _ptr = (_type *) malloc( sizeof(_type) );

#define GET_THIS( THIS_TYPE, THIS_NAME ) THIS_TYPE* THIS_NAME = reinterpret_cast<THIS_TYPE *>(GET_TPD);

//#define SAFE_RELEASE( _obj )       if ( (_obj).pMethodOps != NULL ) { INVOKE_METHOD( (_obj), kMethodRelease ); }
//#define SAFE_KEEP( _obj )       if ( (_obj).pMethodOps != NULL ) { INVOKE_METHOD( (_obj), kMethodKeep ); }
#define SAFE_RELEASE( _obj ) \
	if ( (_obj).pMethodOps != NULL ) { \
		*(_obj).pData -= 1; \
		if ( *(_obj).pData == 0 ) {		INVOKE_METHOD( (_obj), kMethodDelete );  } \
	} TRACK_RELEASE

#define SAFE_KEEP( _obj )       if ( (_obj).pMethodOps != NULL ) { *(_obj).pData += 1; } TRACK_KEEP

//#define OBJECTS_DIFFERENT( OLDOBJ, NEWOBJ ) ( (OLDOBJ.pData != NEWOBJ.pData) || (OLDOBJ.pMethodOps != NEWOBJ.pMethodOps) )
//#define OBJECTS_SAME( OLDOBJ, NEWOBJ ) ( (OLDOBJ.pData == NEWOBJ.pData) && (OLDOBJ.pMethodOps == NEWOBJ.pMethodOps) )
#define OBJECTS_DIFFERENT( OLDOBJ, NEWOBJ ) (OLDOBJ.pData != NEWOBJ.pData)
#define OBJECTS_SAME( OLDOBJ, NEWOBJ ) (OLDOBJ.pData == NEWOBJ.pData)

enum
{
    // all objects have 0..5
    kMethodDelete,
    kMethodShow,
    kMethodGetClass,
    kMethodCompare,
	kMethodKeep,
	kMethodRelease
};


class ForthForgettableGlobalObject : public ForthForgettable
{
public:
    ForthForgettableGlobalObject( void *pOpAddress, long op, int numElements = 1 );
    virtual ~ForthForgettableGlobalObject();

protected:
    virtual void    ForgetCleanup( void *pForgetLimit, long op );

	int		mNumElements;
};
