#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthObject.h: forth base object definitions
//
//////////////////////////////////////////////////////////////////////

#define METHOD_RETURN     SET_TPM( (long *) RPOP );     SET_TPD( (long *) RPOP )

#define RPUSH_OBJECT_PAIR( _methods, _data ) RPUSH( ((long) (_data)));  RPUSH( ((long) (_methods)))
#define RPUSH_OBJECT( _object ) RPUSH_OBJECT_PAIR( (_object).pMethodOps, (_object).pData )
#define RPUSH_THIS  RPUSH_OBJECT_PAIR( GET_TPM, GET_TPD )
#define SET_THIS_PAIR( _methods, _data ) SET_TPM( (_methods) );  SET_TPD( (_data) )
#define SET_THIS( _object) SET_TPM( (_object).pMethodOps );  SET_TPD( (_object).pData) )

#define METHOD( NAME, VALUE  )          { NAME, (ulong) VALUE, NATIVE_TYPE_TO_CODE( kDTIsMethod, kBaseTypeVoid ) }
#define METHOD_RET( NAME, VAL, RVAL )   { NAME, (ulong) VAL, RVAL }
#define MEMBER_VAR( NAME, TYPE )        { NAME, 0, (ulong) TYPE }
#define MEMBER_ARRAY( NAME, TYPE, NUM ) { NAME, NUM, (ulong) (TYPE | kDTIsArray) }

#define END_MEMBERS { NULL, 0, 0 }

#define INVOKE_METHOD( _pCore, _obj, _methodNum ) ForthEngine::GetInstance()->ExecuteOneMethod( _pCore, _obj, _methodNum )
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

#define GET_THIS( THIS_TYPE, THIS_NAME ) THIS_TYPE* THIS_NAME = reinterpret_cast<THIS_TYPE *>(GET_TPD);

#define SAFE_RELEASE( _pCore, _obj ) \
	if ( (_obj).pMethodOps != NULL ) { \
		*(_obj).pData -= 1; \
		if ( *(_obj).pData == 0 ) {		INVOKE_METHOD( (_pCore), (_obj), kMethodDelete );  } \
	} TRACK_RELEASE

#define SAFE_KEEP( _obj )       if ( (_obj).pMethodOps != NULL ) { *(_obj).pData += 1; } TRACK_KEEP

//#define OBJECTS_DIFFERENT( OLDOBJ, NEWOBJ ) ( (OLDOBJ.pData != NEWOBJ.pData) || (OLDOBJ.pMethodOps != NEWOBJ.pMethodOps) )
//#define OBJECTS_SAME( OLDOBJ, NEWOBJ ) ( (OLDOBJ.pData == NEWOBJ.pData) && (OLDOBJ.pMethodOps == NEWOBJ.pMethodOps) )
#define OBJECTS_DIFFERENT( OLDOBJ, NEWOBJ ) (OLDOBJ.pData != NEWOBJ.pData)
#define OBJECTS_SAME( OLDOBJ, NEWOBJ ) (OLDOBJ.pData == NEWOBJ.pData)

#define CLEAR_OBJECT( _obj )             (_obj).pMethodOps = NULL; (_obj).pData = NULL

#define OBJECT_ASSIGN( _pCore, _dstObj, _srcObj ) if ( OBJECTS_DIFFERENT( (_dstObj), (_srcObj) ) ) { SAFE_KEEP( (_srcObj) ); SAFE_RELEASE( (_pCore), (_dstObj) ); }

enum
{
    // all objects have methods 0..5
    kMethodDelete,
    kMethodShow,
    kMethodGetClass,
    kMethodCompare,
	kMethodKeep,
	kMethodRelease,
	kNumBaseMethods
};


// debug tracking of object allocations

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

#define MALLOCATE( _type, _ptr ) _type* _ptr = (_type *) __MALLOC( sizeof(_type) );

#define MALLOCATE_OBJECT( _type, _ptr, _vocab )   _type* _ptr = (_type *) __MALLOC( _vocab->GetSize() );  TRACK_NEW
#define FREE_OBJECT( _obj )  __FREE( _obj );  TRACK_DELETE
#define MALLOCATE_LINK( _type, _ptr )  MALLOCATE( _type, _ptr );  TRACK_LINK_NEW
#define FREE_LINK( _link )  __FREE( _link );  TRACK_LINK_DELETE
#define MALLOCATE_ITER( _type, _ptr, _vocab )  MALLOCATE_OBJECT( _type, _ptr, _vocab );  TRACK_ITER_NEW
#define FREE_ITER( _link )  FREE_OBJECT( _link );  TRACK_ITER_DELETE


