//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.cpp: builtin classes
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthBuiltinClasses.h"
#include <map>

#define METHOD_RETURN \
    SET_TPM( (long *) RPOP ); \
    SET_TPD( (long *) RPOP )

#define METHOD( NAME, VALUE  )          { NAME, (ulong) VALUE, NATIVE_TYPE_TO_CODE( kDTIsMethod, kBaseTypeVoid ) }
#define METHOD_RET( NAME, VAL, RVAL )   { NAME, (ulong) VAL, RVAL }
#define MEMBER_VAR( NAME, TYPE )        { NAME, 0, (ulong) TYPE }
#define MEMBER_ARRAY( NAME, TYPE, NUM ) { NAME, NUM, (ulong) (TYPE | kDTIsArray) }

#define END_MEMBERS { NULL, 0, 0 }

#define INVOKE_METHOD( _obj, _methodNum ) \
    RPUSH( ((long) GET_TPD) ); \
    RPUSH( ((long) GET_TPM) ); \
    SET_TPM( (_obj).pMethodOps ); \
    SET_TPD( (_obj).pData ); \
    ForthEngine::GetInstance()->ExecuteOneOp( (_obj).pMethodOps[ (_methodNum) ] )

#define PUSH_PAIR( _methods, _data )    SPUSH( (long) (_data) ); SPUSH( (long) (_methods) )
#define POP_PAIR( _methods, _data )     (_methods) = (long *) SPOP; (_data) = (long *) SPOP
#define PUSH_OBJECT( _obj )             PUSH_PAIR( (_obj).pMethodOps, (_obj).pData )
#define POP_OBJECT( _obj )              POP_PAIR( (_obj).pMethodOps, (_obj).pData )

#define MALLOCATE( _type, _ptr ) _type* _ptr = (_type *) malloc( sizeof(_type) );

#define GET_THIS( THIS_TYPE, THIS_NAME ) THIS_TYPE* THIS_NAME = reinterpret_cast<THIS_TYPE *>(GET_TPD);

#define SAFE_RELEASE( _obj )    if ( (_obj).pMethodOps != NULL ) { INVOKE_METHOD( (_obj), kMethodRelease ); }
#define SAFE_KEEP( _obj )       if ( (_obj).pMethodOps != NULL ) { INVOKE_METHOD( (_obj), kMethodKeep ); }

enum
{
    // all objects have 0..3
    kMethodDelete,
    kMethodShow,
    kMethodGetClass,
    kMethodCompare,
    // refcounted objects have 4 & 5
    kMethodKeep,
    kMethodRelease
};


namespace
{
#define TRACK_OBJECT_ALLOCATIONS
#ifdef TRACK_OBJECT_ALLOCATIONS
	long gStatNews = 0;
	long gStatDeletes = 0;
	long gStatLinkNews = 0;
	long gStatLinkDeletes = 0;
	long gStatIterNews = 0;
	long gStatIterDeletes = 0;
	long gStatKeeps = 0;
	long gStatReleases = 0;

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

#define MALLOCATE_OBJECT( _type, _ptr )  MALLOCATE( _type, _ptr );  TRACK_NEW
#define FREE_OBJECT( _obj )  free( _obj );  TRACK_DELETE
#define MALLOCATE_LINK( _type, _ptr )  MALLOCATE( _type, _ptr );  TRACK_LINK_NEW
#define FREE_LINK( _link )  free( _link );  TRACK_LINK_DELETE
#define MALLOCATE_ITER( _type, _ptr )  MALLOCATE_OBJECT( _type, _ptr );  TRACK_ITER_NEW
#define FREE_ITER( _link )  FREE_OBJECT( _link );  TRACK_ITER_DELETE

    //////////////////////////////////////////////////////////////////////
    ///
    //                 object
    //

    FORTHOP( objectDeleteMethod )
    {
        FREE_OBJECT( GET_TPD );
        METHOD_RETURN;
    }

    FORTHOP( objectShowMethod )
    {
        char buff[128];
        ForthClassObject* pClassObject = (ForthClassObject *)(*((GET_TPM) - 1));
        sprintf( buff, "object %s  METHODS=0x%08x  DATA=0x%08x", pClassObject->pVocab->GetName(), GET_TPM, GET_TPD );
        CONSOLE_STRING_OUT( buff );
        METHOD_RETURN;
    }

    FORTHOP( objectClassMethod )
    {
        // this is the big gaping hole - where should the pointer to the class vocabulary be stored?
        // we could store it in the slot for method 0, but that would be kind of clunky - also,
        // would slot 0 of non-primary interfaces also have to hold it?
        // the class object is stored in the long before mehod 0
        PUSH_PAIR( ForthTypesManager::GetInstance()->GetClassMethods(), (GET_TPM) - 1 );
        METHOD_RETURN;
    }

    FORTHOP( objectCompareMethod )
    {
        long thisVal = (long)(GET_TPD);
        long thatVal = SPOP;
        long result = 0;
        if ( thisVal != thatVal )
        {
            result = ( thisVal > thatVal ) ? 1 : -1;
        }
        SPUSH( result );
        METHOD_RETURN;
    }

    baseMethodEntry objectMembers[] =
    {
        METHOD(     "delete",           objectDeleteMethod ),
        METHOD(     "show",             objectShowMethod ),
        METHOD_RET( "getClass",         objectClassMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIClass) ),
        METHOD_RET( "compare",          objectCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 class
    //
    FORTHOP( classCreateMethod )
    {
        ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
        SPUSH( (long) pClassObject->pVocab );
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->ExecuteOneOp( pClassObject->newOp );
        METHOD_RETURN;
    }

    FORTHOP( classSuperMethod )
    {
        ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
        ForthClassVocabulary* pClassVocab = pClassObject->pVocab;
        // what should happen if a class is derived from a struct?
        PUSH_PAIR( GET_TPM, pClassVocab->BaseVocabulary() );
        METHOD_RETURN;
    }

    FORTHOP( classNameMethod )
    {
        ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
        ForthClassVocabulary* pClassVocab = pClassObject->pVocab;
        const char* pName = pClassVocab->GetName();
        SPUSH( (long) pName );
        METHOD_RETURN;
    }

    FORTHOP( classVocabularyMethod )
    {
        ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
        SPUSH( (long)(pClassObject->pVocab) );
        METHOD_RETURN;
    }

    FORTHOP( classGetInterfaceMethod )
    {
        // TOS is ptr to name of desired interface
        ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
        ForthClassVocabulary* pClassVocab = pClassObject->pVocab;
        const char* pName = (const char*)(SPOP);
        long* foundInterface = NULL;
	    long numInterfaces = pClassVocab->GetNumInterfaces();
	    for ( int i = 0; i < numInterfaces; i++ )
	    {
            ForthInterface* pInterface = pClassVocab->GetInterface( i );
            if ( strcmp( pInterface->GetDefiningClass()->GetName(), pName ) == 0 )
            {
                foundInterface = (long *) pInterface;
                break;
		    }
	    }
        PUSH_PAIR( foundInterface, GET_TPD );
        METHOD_RETURN;
    }

    FORTHOP( classDeleteMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot delete a class object" );
        METHOD_RETURN;
    }

    FORTHOP( classSetNewMethod )
    {
        ForthClassObject* pClassObject = (ForthClassObject *)(GET_TPD);
        pClassObject->newOp = SPOP;
        METHOD_RETURN;
    }

    baseMethodEntry classMembers[] =
    {
        METHOD(     "delete",           classDeleteMethod ),
        METHOD_RET( "create",           classCreateMethod,          OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject) ),
        METHOD_RET( "getParent",        classSuperMethod,           OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIClass) ),
        METHOD_RET( "getName",          classNameMethod,            NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeByte) ),
        METHOD_RET( "getVocabulary",    classVocabularyMethod,      NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeInt) ),
        METHOD_RET( "getInterface",     classGetInterfaceMethod,    OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject) ),
        METHOD(     "setNew",           classSetNewMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 rcObject
    //


    FORTHOP( rcObjectNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        long nBytes = pClassVocab->GetSize();
        long* pData = (long *) malloc( nBytes );
		TRACK_NEW;
        *pData = 1;     // initialize refCount to 1
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pData );
    }

    FORTHOP( rcObjectShowMethod )
    {
        char buff[128];
        ForthClassObject* pClassObject = (ForthClassObject *)(*((GET_TPM) - 1));
        ulong* pRefCount = (ulong*)(GET_TPD);      // member at offset 0 is refcount
        sprintf( buff, "%s  refCount=%d  METHODS=0x%08x  DATA=0x%08x", pClassObject->pVocab->GetName(), *pRefCount, GET_TPM, GET_TPD );
        CONSOLE_STRING_OUT( buff );
        METHOD_RETURN;
    }

    FORTHOP( rcObjectKeepMethod )
    {
        ulong* pRefCount = (ulong*)(GET_TPD);      // member at offset 0 is refcount
        (*pRefCount) += 1;
		TRACK_KEEP;
        METHOD_RETURN;
    }

    FORTHOP( rcObjectReleaseMethod )
    {
        ulong* pRefCount = (ulong*)(GET_TPD);      // member at offset 0 is refcount
        (*pRefCount) -= 1;
		TRACK_RELEASE;
        if ( *pRefCount == 0 )
        {
            // we are effectively chaining to the delete op, its method return will pop TPM & TPD for us
            ForthEngine *pEngine = ForthEngine::GetInstance();
            ulong deleteOp = *( GET_TPM );      // method 0 is delete
            pEngine->ExecuteOneOp( deleteOp );
            // we are effectively chaining to the delete op, its method return will pop TPM & TPD for us
        }
        else
        {
            METHOD_RETURN;
        }
    }

    baseMethodEntry rcObjectMembers[] =
    {
        METHOD(     "new",                  rcObjectNew ),
        METHOD(     "show",					rcObjectShowMethod ),
        METHOD(     "keep",                 rcObjectKeepMethod ),
        METHOD(     "release",              rcObjectReleaseMethod ),
        MEMBER_VAR( "refCount",             NATIVE_TYPE_TO_CODE(0, kBaseTypeInt) ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 rcIter
    //

	// rcIter is an abstract iterator class

    baseMethodEntry rcIterMembers[] =
    {
        METHOD(     "seekNext",             NULL ),
        METHOD(     "seekPrev",             NULL ),
        METHOD(     "seekHead",             NULL ),
        METHOD(     "seekTail",             NULL ),
        METHOD(     "next",					NULL ),
        METHOD(     "prev",                 NULL ),
        METHOD(     "current",				NULL ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 rcArray
    //

    typedef std::vector<ForthObject> rcArray;
    struct rcArrayStruct
    {
        ulong       refCount;
        rcArray*    elements;
    };

	struct rcArrayIterStruct
    {
        ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
    };
	static ForthClassVocabulary* gpArraryIterClassVocab = NULL;

    FORTHOP( rcArrayNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( rcArrayStruct, pArray );
        pArray->refCount = 1;
        pArray->elements = new rcArray;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pArray );
    }

    FORTHOP( rcArrayDeleteMethod )
    {
        // go through all elements and release any which are not null
		GET_THIS( rcArrayStruct, pArray );
        rcArray::iterator iter;
        rcArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = *iter;
            SAFE_RELEASE( o );
        }
        delete pArray->elements;
        FREE_OBJECT( pArray );
        METHOD_RETURN;
    }

    FORTHOP( rcArrayResizeMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS( rcArrayStruct, pArray );
        rcArray& a = *(pArray->elements);
        ulong newSize = SPOP;
        if ( a.size() != newSize )
        {
            if ( a.size() > newSize )
            {
                // shrinking - release elements which are beyond new end of array
                for ( ulong i = newSize; i < a.size(); i++ )
                {
                    SAFE_RELEASE( a[i] );
                }
                a.resize( newSize );
            }
            else
            {
                // growing - add null objects to end of array
                a.resize( newSize );
                ForthObject o;
                o.pMethodOps = NULL;
                o.pData = NULL;
                for ( ulong i = a.size(); i < newSize; i++ )
                {
                    a[i] = o;
                }
            }
        }
        METHOD_RETURN;
    }

    FORTHOP( rcArrayClearMethod )
    {
        // go through all elements and release any which are not null
		GET_THIS( rcArrayStruct, pArray );
        rcArray::iterator iter;
        rcArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = *iter;
            SAFE_RELEASE( o );
        }
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP( rcArrayCountMethod )
    {
		GET_THIS( rcArrayStruct, pArray );
		SPUSH( (long) (pArray->elements->size()) );
        METHOD_RETURN;
    }

    FORTHOP( rcArrayGetMethod )
    {
		GET_THIS( rcArrayStruct, pArray );
        rcArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
            ForthObject fobj = a[ix];
            PUSH_OBJECT( fobj );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    FORTHOP( rcArraySetMethod )
    {
		GET_THIS( rcArrayStruct, pArray );
        rcArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
            ForthObject& oldObj = a[ix];
            ForthObject newObj;
            POP_OBJECT( newObj );
            if ( (oldObj.pMethodOps != newObj.pMethodOps) || (oldObj.pData != newObj.pData) )
            {
                SAFE_KEEP( newObj );
                SAFE_RELEASE( oldObj );
            }
            a[ix] = newObj;
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    FORTHOP( rcArrayFindIndexMethod )
    {
		GET_THIS( rcArrayStruct, pArray );
        long retVal = -1;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
        rcArray::iterator iter;
        rcArray& a = *(pArray->elements);
        for ( ulong i = 0; i < a.size(); i++ )
        {
            ForthObject& o = a[i];
            if ( (o.pMethodOps == soughtObj.pMethodOps) && (o.pData == soughtObj.pData) )
            {
                retVal = i;
            }
        }
        SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( rcArrayAddMethod )
    {
		GET_THIS( rcArrayStruct, pArray );
        rcArray& a = *(pArray->elements);
        ForthObject fobj;
        POP_OBJECT( fobj );
        SAFE_KEEP( fobj );
        a.push_back( fobj );
        METHOD_RETURN;
    }

    FORTHOP( rcArrayHeadIterMethod )
    {
        GET_THIS( rcArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( rcArrayIterStruct, pIter );
		pIter->refCount = 1;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = 0;
        ForthInterface* pPrimaryInterface = gpArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( rcArrayTailIterMethod )
    {
        GET_THIS( rcArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( rcArrayIterStruct, pIter );
		pIter->refCount = 1;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = pArray->elements->size();
        ForthInterface* pPrimaryInterface = gpArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( rcArrayRemoveMethod )
    {
        METHOD_RETURN;
    }

    baseMethodEntry rcArrayMembers[] =
    {
        METHOD(     "new",                  rcArrayNew ),
        METHOD(     "delete",               rcArrayDeleteMethod ),
        METHOD(     "clear",                rcArrayClearMethod ),
        METHOD(     "resize",               rcArrayResizeMethod ),
        METHOD(     "count",                rcArrayCountMethod ),
        METHOD(     "get",                  rcArrayGetMethod ),
        METHOD(     "set",                  rcArraySetMethod ),
        METHOD(     "findIndex",            rcArrayFindIndexMethod ),
        METHOD(     "add",                  rcArrayAddMethod ),
        METHOD(     "headIter",             rcArrayHeadIterMethod ),
        METHOD(     "tailIter",             rcArrayTailIterMethod ),
        METHOD(     "remove",               rcArrayRemoveMethod ),
        //METHOD(     "insert",               rcArrayInsertMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 rcArrayIter
    //

    FORTHOP( rcArrayIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a rcArrayIter object" );
    }

    FORTHOP( rcArrayIterDeleteMethod )
    {
        GET_THIS( rcArrayIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		FREE_ITER( pIter );
        METHOD_RETURN;
    }

    FORTHOP( rcArrayIterSeekNextMethod )
    {
        GET_THIS( rcArrayIterStruct, pIter );
		rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct *>( pIter->parent.pData );
		if ( pIter->cursor < pArray->elements->size() )
		{
			pIter->cursor++;
		}
        METHOD_RETURN;
    }

    FORTHOP( rcArrayIterSeekPrevMethod )
    {
        GET_THIS( rcArrayIterStruct, pIter );
		if ( pIter->cursor > 0 )
		{
			pIter->cursor--;
		}
        METHOD_RETURN;
    }

    FORTHOP( rcArrayIterSeekHeadMethod )
    {
        GET_THIS( rcArrayIterStruct, pIter );
		pIter->cursor = 0;
        METHOD_RETURN;
    }

    FORTHOP( rcArrayIterSeekTailMethod )
    {
        GET_THIS( rcArrayIterStruct, pIter );
		rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct *>( pIter->parent.pData );
		pIter->cursor = pArray->elements->size();
        METHOD_RETURN;
    }

    FORTHOP( rcArrayIterNextMethod )
    {
        GET_THIS( rcArrayIterStruct, pIter );
		rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct *>( pIter->parent.pData );
        rcArray& a = *(pArray->elements);
		if ( pIter->cursor >= a.size() )
		{
			SPUSH( 0 );
		}
		else
		{
			PUSH_OBJECT( a[ pIter->cursor ] );
			pIter->cursor++;
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcArrayIterPrevMethod )
    {
        GET_THIS( rcArrayIterStruct, pIter );
		rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct *>( pIter->parent.pData );
        rcArray& a = *(pArray->elements);
		if ( pIter->cursor == 0 )
		{
			SPUSH( 0 );
		}
		else
		{
			pIter->cursor--;
			PUSH_OBJECT( a[ pIter->cursor ] );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcArrayIterCurrentMethod )
    {
        GET_THIS( rcArrayIterStruct, pIter );
		rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct *>( pIter->parent.pData );
        rcArray& a = *(pArray->elements);
		if ( pIter->cursor >= a.size() )
		{
			SPUSH( 0 );
		}
		else
		{
			PUSH_OBJECT( a[ pIter->cursor ] );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    baseMethodEntry rcArrayIterMembers[] =
    {
        METHOD(     "new",                  rcArrayIterNew ),
        METHOD(     "delete",               rcArrayIterDeleteMethod ),
        METHOD(     "seekNext",             rcArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             rcArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             rcArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             rcArrayIterSeekTailMethod ),
        METHOD(     "next",					rcArrayIterNextMethod ),
        METHOD(     "prev",                 rcArrayIterPrevMethod ),
        METHOD(     "current",				rcArrayIterCurrentMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 rcList
    //


	struct rcListElement
	{
		rcListElement*	prev;
		rcListElement*	next;
		ForthObject		obj;
	};

    struct rcListStruct
    {
        ulong			refCount;
		rcListElement*	head;
		rcListElement*	tail;
    };

	struct rcListIterStruct
    {
        ulong			refCount;
		ForthObject		parent;
		rcListElement*	cursor;
    };
	static ForthClassVocabulary* gpListIterClassVocab = NULL;


    FORTHOP( rcListNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( rcListStruct, pList );
        pList->refCount = 1;
		pList->head = NULL;
		pList->tail = NULL;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pList );
    }

    FORTHOP( rcListDeleteMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( rcListStruct, pList );
        rcListElement* pCur = pList->head;
		while ( pCur != NULL )
		{
			rcListElement* pNext = pCur->next;
			SAFE_RELEASE( pCur->obj );
			FREE_LINK( pCur );
			pCur = pNext;
		}
        FREE_OBJECT( pList );
        METHOD_RETURN;
    }

    FORTHOP( rcListHeadMethod )
    {
        GET_THIS( rcListStruct, pList );
		if ( pList->head == NULL )
		{
			ASSERT( pList->tail == NULL );
			PUSH_PAIR( NULL, NULL );
		}
		else
		{
			PUSH_OBJECT( pList->head->obj );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcListTailMethod )
    {
        GET_THIS( rcListStruct, pList );
		if ( pList->tail == NULL )
		{
			ASSERT( pList->head == NULL );
			PUSH_PAIR( NULL, NULL );
		}
		else
		{
			PUSH_OBJECT( pList->tail->obj );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcListAddHeadMethod )
    {
        GET_THIS( rcListStruct, pList );
		MALLOCATE_LINK( rcListElement, newElem );
        POP_OBJECT( newElem->obj );
		SAFE_KEEP( newElem->obj );
		newElem->prev = NULL;
		rcListElement* oldHead = pList->head;
		if ( oldHead == NULL )
		{
			ASSERT( pList->tail == NULL );
			pList->tail = newElem;
		}
		else
		{
			ASSERT( oldHead->prev == NULL );
			oldHead->prev = newElem;
		}
		newElem->next = oldHead;
		pList->head = newElem;
        METHOD_RETURN;
    }

    FORTHOP( rcListAddTailMethod )
    {
        GET_THIS( rcListStruct, pList );
		MALLOCATE_LINK( rcListElement, newElem );
        POP_OBJECT( newElem->obj );
		SAFE_KEEP( newElem->obj );
		newElem->next = NULL;
		rcListElement* oldTail = pList->tail;
		if ( oldTail == NULL )
		{
			ASSERT( pList->head == NULL );
			pList->head = newElem;
		}
		else
		{
			ASSERT( oldTail->next == NULL );
			oldTail->next = newElem;
		}
		newElem->prev = oldTail;
		pList->tail = newElem;
        METHOD_RETURN;
    }

    FORTHOP( rcListRemoveHeadMethod )
    {
        GET_THIS( rcListStruct, pList );
		rcListElement* oldHead = pList->head;
		if ( oldHead == NULL )
		{
			ASSERT( pList->tail == NULL );
			PUSH_PAIR( NULL, NULL );
		}
		else
		{
			ForthObject& obj = oldHead->obj;
			if ( oldHead == pList->tail )
			{
				// removed only element in list
				pList->head = NULL;
				pList->tail = NULL;
			}
			else
			{
				ASSERT( oldHead->prev == NULL );
				rcListElement* newHead = oldHead->next;
				ASSERT( newHead != NULL );
				newHead->prev = NULL;
				pList->head = newHead;
			}
			SAFE_RELEASE( obj );
			PUSH_OBJECT( obj );
			FREE_LINK( oldHead );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcListRemoveTailMethod )
    {
        GET_THIS( rcListStruct, pList );
		rcListElement* oldTail = pList->tail;
		if ( oldTail == NULL )
		{
			ASSERT( pList->head == NULL );
			PUSH_PAIR( NULL, NULL );
		}
		else
		{
			ForthObject& obj = oldTail->obj;
			if ( pList->head == oldTail )
			{
				// removed only element in list
				pList->head = NULL;
				pList->tail = NULL;
			}
			else
			{
				ASSERT( oldTail->next == NULL );
				rcListElement* newTail = oldTail->prev;
				ASSERT( newTail != NULL );
				newTail->next = NULL;
				pList->tail = newTail;
			}
			SAFE_RELEASE( obj );
			PUSH_OBJECT( obj );
			FREE_LINK( oldTail );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcListHeadIterMethod )
    {
        GET_THIS( rcListStruct, pList );
		pList->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( rcListIterStruct, pIter );
		pIter->refCount = 1;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pList);
		pIter->cursor = pList->head;
        ForthInterface* pPrimaryInterface = gpListIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( rcListTailIterMethod )
    {
        GET_THIS( rcListStruct, pList );
		pList->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( rcListIterStruct, pIter );
		pIter->refCount = 1;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pList);
		pIter->cursor = NULL;
        ForthInterface* pPrimaryInterface = gpListIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( rcListCountMethod )
    {
        GET_THIS( rcListStruct, pList );
		long count = 0;
        rcListElement* pCur = pList->head;
		while ( pCur != NULL )
		{
			count++;
			pCur = pCur->next;
		}
        SPUSH( count );
        METHOD_RETURN;
    }

	/*


	Clone
	  create a copy of the list
	Cut( rcListIter start, rcListIter end )
	  remove a sublist from this list
	  end is one past last element to delete, end==NULL deletes to end of list
	Splice( rcList srcList, listElement* insertBefore
	  insert srcList into this list before position specified by insertBefore
	  if insertBefore is null, insert at tail of list
	  after splice, srcList is empty

	? replace an elements object
	? isEmpty
	*/

    FORTHOP( rcListFindMethod )
    {
        GET_THIS( rcListStruct, pList );
        rcListElement* pCur = pList->head;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
		while ( pCur != NULL )
		{
			ForthObject& o = pCur->obj;
			if ( (o.pMethodOps == soughtObj.pMethodOps) && (o.pData == soughtObj.pMethodOps) )
			{
				break;
			}
			pCur = pCur->next;
		}
		SPUSH( (long) pCur );
    }

    baseMethodEntry rcListMembers[] =
    {
        METHOD(     "new",                  rcListNew ),
        METHOD(     "delete",               rcListDeleteMethod ),
        METHOD(     "head",                 rcListHeadMethod ),
        METHOD(     "tail",                 rcListTailMethod ),
        METHOD(     "addHead",              rcListAddHeadMethod ),
        METHOD(     "addTail",              rcListAddTailMethod ),
        METHOD(     "removeHead",           rcListRemoveHeadMethod ),
        METHOD(     "removeTail",           rcListRemoveTailMethod ),
        METHOD(     "headIter",             rcListHeadIterMethod ),
        METHOD(     "tailIter",             rcListTailIterMethod ),
        METHOD(     "count",                rcListCountMethod ),
        METHOD(     "find",                 rcListFindMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 rcListIter
    //

    FORTHOP( rcListIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a rcListIter object" );
    }

    FORTHOP( rcListIterDeleteMethod )
    {
        GET_THIS( rcListIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		FREE_ITER( pIter );
        METHOD_RETURN;
    }

    FORTHOP( rcListIterSeekNextMethod )
    {
        GET_THIS( rcListIterStruct, pIter );
		if ( pIter->cursor != NULL )
		{
			pIter->cursor = pIter->cursor->next;
		}
        METHOD_RETURN;
    }

    FORTHOP( rcListIterSeekPrevMethod )
    {
        GET_THIS( rcListIterStruct, pIter );
		if ( pIter->cursor != NULL )
		{
			pIter->cursor = pIter->cursor->prev;
		}
		else
		{
			pIter->cursor = reinterpret_cast<rcListStruct *>(pIter->parent.pData)->tail;
		}
        METHOD_RETURN;
    }

    FORTHOP( rcListIterSeekHeadMethod )
    {
        GET_THIS( rcListIterStruct, pIter );
		pIter->cursor = reinterpret_cast<rcListStruct *>(pIter->parent.pData)->head;
        METHOD_RETURN;
    }

    FORTHOP( rcListIterSeekTailMethod )
    {
        GET_THIS( rcListIterStruct, pIter );
		pIter->cursor = NULL;
        METHOD_RETURN;
    }

    FORTHOP( rcListIterNextMethod )
    {
        GET_THIS( rcListIterStruct, pIter );
		if ( pIter->cursor == NULL )
		{
			SPUSH( 0 );
		}
		else
		{
			PUSH_OBJECT( pIter->cursor->obj );
			pIter->cursor = pIter->cursor->next;
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcListIterPrevMethod )
    {
        GET_THIS( rcListIterStruct, pIter );
		// special case: NULL cursor means tail of list
		if ( pIter->cursor == NULL )
		{
			pIter->cursor = reinterpret_cast<rcListStruct *>(pIter->parent.pData)->tail;
		}
		else
		{
			pIter->cursor = pIter->cursor->prev;
		}
		if ( pIter->cursor == NULL )
		{
			SPUSH( 0 );
		}
		else
		{
			PUSH_OBJECT( pIter->cursor->obj );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcListIterCurrentMethod )
    {
        GET_THIS( rcListIterStruct, pIter );
		if ( pIter->cursor == NULL )
		{
			SPUSH( 0 );
		}
		else
		{
			PUSH_OBJECT( pIter->cursor->obj );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    baseMethodEntry rcListIterMembers[] =
    {
        METHOD(     "new",                  rcListIterNew ),
        METHOD(     "delete",               rcListIterDeleteMethod ),
        METHOD(     "seekNext",             rcListIterSeekNextMethod ),
        METHOD(     "seekPrev",             rcListIterSeekPrevMethod ),
        METHOD(     "seekHead",             rcListIterSeekHeadMethod ),
        METHOD(     "seekTail",             rcListIterSeekTailMethod ),
        METHOD(     "next",					rcListIterNextMethod ),
        METHOD(     "prev",                 rcListIterPrevMethod ),
        METHOD(     "current",				rcListIterCurrentMethod ),
        // following must be last in table
        END_MEMBERS
    };


	//////////////////////////////////////////////////////////////////////
    ///
    //                 rcMap
    //

    typedef std::map<long, ForthObject> rcMap;
    struct rcMapStruct
    {
        ulong       refCount;
        rcMap*		elements;
    };

    struct rcMapIterStruct
    {
        ulong				refCount;
		ForthObject			parent;
		rcMap::iterator		cursor;
    };
	static ForthClassVocabulary* gpMapIterClassVocab = NULL;



    FORTHOP( rcMapNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( rcMapStruct, pMap );
        pMap->refCount = 1;
        pMap->elements = new rcMap;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pMap );
    }

    FORTHOP( rcMapDeleteMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( rcMapStruct, pMap );
        rcMap::iterator iter;
        rcMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE( o );
        }
        delete pMap->elements;
        FREE_OBJECT( pMap );
        METHOD_RETURN;
    }

    FORTHOP( rcMapClearMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( rcMapStruct, pMap );
        rcMap::iterator iter;
        rcMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE( o );
        }
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP( rcMapCountMethod )
    {
        GET_THIS( rcMapStruct, pMap );
        SPUSH( (long) (pMap->elements->size()) );
        METHOD_RETURN;
    }

    FORTHOP( rcMapGetMethod )
    {
        GET_THIS( rcMapStruct, pMap );
        rcMap& a = *(pMap->elements);
        long key = SPOP;
        rcMap::iterator iter = a.find( key );
		if ( iter != a.end() )
        {
            ForthObject fobj = iter->second;
            PUSH_OBJECT( fobj );
        }
        else
        {
			PUSH_PAIR( 0, 0 );
        }
        METHOD_RETURN;
    }

    FORTHOP( rcMapSetMethod )
    {
        GET_THIS( rcMapStruct, pMap );
        rcMap& a = *(pMap->elements);
        long key = SPOP;
        ForthObject newObj;
        POP_OBJECT( newObj );
        rcMap::iterator iter = a.find( key );
		if ( newObj.pMethodOps != NULL )
		{
            SAFE_KEEP( newObj );
			if ( iter != a.end() )
			{
				ForthObject oldObj = iter->second;
				SAFE_RELEASE( oldObj );
			}
			a[key] = newObj;
		}
		else
		{
			// remove element associated with key from map
			if ( iter != a.end() )
			{
				ForthObject& oldObj = iter->second;
                SAFE_RELEASE( oldObj );
				a.erase( iter );
			}
		}
        METHOD_RETURN;
    }

    FORTHOP( rcMapFindIndexMethod )
    {
        GET_THIS( rcMapStruct, pMap );
        long retVal = -1;
		long found = 0;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
        rcMap::iterator iter;
        rcMap& a = *(pMap->elements);
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = iter->second;
            if ( (o.pMethodOps == soughtObj.pMethodOps) && (o.pData == soughtObj.pData) )
            {
				found = 1;
                retVal = iter->first;
            }
        }
        SPUSH( retVal );
        SPUSH( found );
        METHOD_RETURN;
    }

    FORTHOP( rcMapRemoveMethod )
    {
        GET_THIS( rcMapStruct, pMap );
        rcMap& a = *(pMap->elements);
        long key = SPOP;
        rcMap::iterator iter = a.find( key );
		if ( iter != a.end() )
		{
			ForthObject& oldObj = iter->second;
            SAFE_RELEASE( oldObj );
			a.erase( iter );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcMapHeadIterMethod )
    {
        GET_THIS( rcMapStruct, pMap );
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		rcMapIterStruct* pIter = new rcMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 1;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
        rcMap::iterator iter = pMap->elements->begin();
		pIter->cursor = iter;
		//pIter->cursor = pMap->elements->begin();
        ForthInterface* pPrimaryInterface = gpMapIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( rcMapTailIterMethod )
    {
        GET_THIS( rcMapStruct, pMap );
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		rcMapIterStruct* pIter = new rcMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 1;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = pMap->elements->end();
        ForthInterface* pPrimaryInterface = gpMapIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }


    baseMethodEntry rcMapMembers[] =
    {
        METHOD(     "new",                  rcMapNew ),
        METHOD(     "delete",               rcMapDeleteMethod ),
        METHOD(     "clear",                rcMapClearMethod ),
        METHOD(     "count",                rcMapCountMethod ),
        METHOD(     "get",                  rcMapGetMethod ),
        METHOD(     "set",                  rcMapSetMethod ),
        METHOD(     "findIndex",            rcMapFindIndexMethod ),
        METHOD(     "remove",               rcMapRemoveMethod ),
        METHOD(     "headIter",             rcMapHeadIterMethod ),
        METHOD(     "tailIter",             rcMapTailIterMethod ),
        //METHOD(     "insert",               rcMapInsertMethod ),
        // following must be last in table
        END_MEMBERS
    };


	//////////////////////////////////////////////////////////////////////
    ///
    //                 rcMapIter
    //

    FORTHOP( rcMapIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a rcMapIter object" );
    }

    FORTHOP( rcMapIterDeleteMethod )
    {
        GET_THIS( rcMapIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		delete pIter;
		TRACK_ITER_DELETE;
        METHOD_RETURN;
    }

    FORTHOP( rcMapIterSeekNextMethod )
    {
        GET_THIS( rcMapIterStruct, pIter );
		pIter->cursor++;
        METHOD_RETURN;
    }

    FORTHOP( rcMapIterSeekPrevMethod )
    {
        GET_THIS( rcMapIterStruct, pIter );
		pIter->cursor--;
        METHOD_RETURN;
    }

    FORTHOP( rcMapIterSeekHeadMethod )
    {
        GET_THIS( rcMapIterStruct, pIter );
		rcMapStruct* pMap = reinterpret_cast<rcMapStruct *>( pIter->parent.pData );
		pIter->cursor = pMap->elements->begin();
        METHOD_RETURN;
    }

    FORTHOP( rcMapIterSeekTailMethod )
    {
        GET_THIS( rcMapIterStruct, pIter );
		rcMapStruct* pMap = reinterpret_cast<rcMapStruct *>( pIter->parent.pData );
		pIter->cursor = pMap->elements->end();
        METHOD_RETURN;
    }

    FORTHOP( rcMapIterNextMethod )
    {
        GET_THIS( rcMapIterStruct, pIter );
		rcMapStruct* pMap = reinterpret_cast<rcMapStruct *>( pIter->parent.pData );
		if ( pIter->cursor == pMap->elements->end() )
		{
			SPUSH( 0 );
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT( o );
			pIter->cursor++;
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcMapIterPrevMethod )
    {
        GET_THIS( rcMapIterStruct, pIter );
		rcMapStruct* pMap = reinterpret_cast<rcMapStruct *>( pIter->parent.pData );
		if ( pIter->cursor == pMap->elements->begin() )
		{
			SPUSH( 0 );
		}
		else
		{
			pIter->cursor--;
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT( o );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcMapIterCurrentMethod )
    {
        GET_THIS( rcMapIterStruct, pIter );
		rcMapStruct* pMap = reinterpret_cast<rcMapStruct *>( pIter->parent.pData );
		if ( pIter->cursor == pMap->elements->end() )
		{
			SPUSH( 0 );
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT( o );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcMapIterNextPairMethod )
    {
        GET_THIS( rcMapIterStruct, pIter );
		rcMapStruct* pMap = reinterpret_cast<rcMapStruct *>( pIter->parent.pData );
		if ( pIter->cursor == pMap->elements->end() )
		{
			SPUSH( 0 );
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT( o );
			SPUSH( pIter->cursor->first );
			pIter->cursor++;
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( rcMapIterPrevPairMethod )
    {
        GET_THIS( rcMapIterStruct, pIter );
		rcMapStruct* pMap = reinterpret_cast<rcMapStruct *>( pIter->parent.pData );
		if ( pIter->cursor == pMap->elements->begin() )
		{
			SPUSH( 0 );
		}
		else
		{
			pIter->cursor--;
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT( o );
			SPUSH( pIter->cursor->first );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    baseMethodEntry rcMapIterMembers[] =
    {
        METHOD(     "new",                  rcMapIterNew ),
        METHOD(     "delete",               rcMapIterDeleteMethod ),
        METHOD(     "seekNext",             rcMapIterSeekNextMethod ),
        METHOD(     "seekPrev",             rcMapIterSeekPrevMethod ),
        METHOD(     "seekHead",             rcMapIterSeekHeadMethod ),
        METHOD(     "seekTail",             rcMapIterSeekTailMethod ),
        METHOD(     "next",					rcMapIterNextMethod ),
        METHOD(     "prev",                 rcMapIterPrevMethod ),
        METHOD(     "current",				rcMapIterCurrentMethod ),
		METHOD(     "nextPair",				rcMapIterNextPairMethod ),
        METHOD(     "prevPair",             rcMapIterPrevPairMethod ),
        // following must be last in table
        END_MEMBERS
    };

    //////////////////////////////////////////////////////////////////////
    ///
    //                 rcString
    //

    typedef std::vector<ForthObject> rcArray;
#define DEFAULT_STRING_DATA_BYTES 32
	int gDefaultRCStringSize = DEFAULT_STRING_DATA_BYTES - 1;

	struct rcString
	{
		long		maxLen;
		long		curLen;
		char		data[DEFAULT_STRING_DATA_BYTES];
	};

	rcString* CreateRCString( int maxChars )
	{
		int dataBytes = ((maxChars  + 4) & ~3);
        size_t nBytes = sizeof(rcString) + (dataBytes - DEFAULT_STRING_DATA_BYTES);
		rcString* str = (rcString *) malloc( nBytes );
		str->maxLen = dataBytes - 1;
		str->curLen = 0;
		str->data[0] = '\0';
		return str;
	}

    struct rcStringStruct
    {
        ulong       refCount;
        rcString*   str;
    };


    FORTHOP( rcStringNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        MALLOCATE_OBJECT( rcStringStruct, pString );
        pString->refCount = 1;
		pString->str = CreateRCString( gDefaultRCStringSize );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pString );
    }

    FORTHOP( rcStringDeleteMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( rcStringStruct, pString );
		free( pString->str );
        FREE_OBJECT( pString );
        METHOD_RETURN;
    }

    FORTHOP( rcStringShowMethod )
    {
        char buff[128];
        ForthClassObject* pClassObject = (ForthClassObject *)(*((GET_TPM) - 1));
        GET_THIS( rcStringStruct, pString );
        ulong* pRefCount = (ulong*)(GET_TPD);      // member at offset 0 is refcount
        sprintf( buff, "%s  [%s] refCount=%d  METHODS=0x%08x  DATA=0x%08x", pClassObject->pVocab->GetName(), &(pString->str->data[0]), pString->refCount, GET_TPM, GET_TPD );
        CONSOLE_STRING_OUT( buff );
        METHOD_RETURN;
    }

    FORTHOP( rcStringSizeMethod )
    {
        GET_THIS( rcStringStruct, pString );
		SPUSH( pString->str->maxLen );
        METHOD_RETURN;
    }

    FORTHOP( rcStringLengthMethod )
    {
        GET_THIS( rcStringStruct, pString );
		SPUSH( pString->str->curLen );
        METHOD_RETURN;
    }

    FORTHOP( rcStringGetMethod )
    {
        GET_THIS( rcStringStruct, pString );
		SPUSH( (long) &(pString->str->data[0]) );
        METHOD_RETURN;
    }

    FORTHOP( rcStringSetMethod )
    {
        GET_THIS( rcStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
		long len = (long) strlen( srcStr );
		rcString* dst = pString->str;
		if ( len > dst->maxLen )
		{
			// enlarge string
			free( dst );
			dst = CreateRCString( len );
		}
		dst->curLen = len;
		memcpy( &(dst->data[0]), srcStr, len + 1 );
        METHOD_RETURN;
    }

    FORTHOP( rcStringAppendMethod )
    {
        GET_THIS( rcStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
		long len = (long) strlen( srcStr );
		rcString* dst = pString->str;
		long newLen = dst->curLen + len;
		if ( newLen > dst->maxLen )
		{
			// enlarge string
			int dataBytes = ((newLen  + 4) & ~3);
	        size_t nBytes = sizeof(rcString) + (dataBytes - DEFAULT_STRING_DATA_BYTES);
			dst = (rcString *) realloc( dst, nBytes );
		}
		memcpy( &(dst->data[dst->curLen]), srcStr, len + 1 );
		dst->curLen = newLen;
        METHOD_RETURN;
    }


    baseMethodEntry rcStringMembers[] =
    {
        METHOD(     "new",                  rcStringNew ),
        METHOD(     "delete",               rcStringDeleteMethod ),
        METHOD(     "show",					rcStringShowMethod ),
        METHOD(     "size",                 rcStringSizeMethod ),
        METHOD(     "length",               rcStringLengthMethod ),
        METHOD(     "get",                  rcStringGetMethod ),
        METHOD(     "set",                  rcStringSetMethod ),
        METHOD(     "append",               rcStringAppendMethod ),
        // following must be last in table
        END_MEMBERS
    };
}

void
ForthTypesManager::AddBuiltinClasses( ForthEngine* pEngine )
{
    ForthClassVocabulary* pObjectClass = pEngine->AddBuiltinClass( "object", NULL, objectMembers );
    ForthClassVocabulary* pClassClass = pEngine->AddBuiltinClass( "class", pObjectClass, classMembers );
    ForthClassVocabulary* pRCObjectClass = pEngine->AddBuiltinClass( "rcObject", pObjectClass, rcObjectMembers );
    ForthClassVocabulary* pRCIterClass = pEngine->AddBuiltinClass( "rcIter", pRCObjectClass, rcIterMembers );
    ForthClassVocabulary* pRCArrayClass = pEngine->AddBuiltinClass( "rcArray", pRCObjectClass, rcArrayMembers );
    gpArraryIterClassVocab = pEngine->AddBuiltinClass( "rcArrayIter", pRCIterClass, rcArrayIterMembers );
    ForthClassVocabulary* pRCListClass = pEngine->AddBuiltinClass( "rcList", pRCObjectClass, rcListMembers );
    gpListIterClassVocab = pEngine->AddBuiltinClass( "rcListIter", pRCIterClass, rcListIterMembers );
    ForthClassVocabulary* pRCMapClass = pEngine->AddBuiltinClass( "rcMap", pRCObjectClass, rcMapMembers );
    gpMapIterClassVocab = pEngine->AddBuiltinClass( "rcMapIter", pRCIterClass, rcMapIterMembers );
    ForthClassVocabulary* pRCStringClass = pEngine->AddBuiltinClass( "rcString", pRCObjectClass, rcStringMembers );
    mpClassMethods = pClassClass->GetInterface( 0 )->GetMethods();
}

