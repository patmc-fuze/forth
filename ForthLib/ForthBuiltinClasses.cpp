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

#define SAFE_RELEASE( _obj )    if ( (_obj).pMethodOps != NULL ) { INVOKE_METHOD( (_obj), kMethodRelease ); }
#define SAFE_KEEP( _obj )       if ( (_obj).pMethodOps != NULL ) { INVOKE_METHOD( (_obj), kMethodKeep ); }

namespace
{
    //////////////////////////////////////////////////////////////////////
    ///
    //                 object
    //

    FORTHOP( objectDeleteMethod )
    {
        free( GET_TPD );
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
        *pData = 1;     // initialize refCount to 1
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pData );
    }

    FORTHOP( rcObjectKeepMethod )
    {
        ulong* pRefCount = (ulong*)(GET_TPD);      // member at offset 0 is refcount
        (*pRefCount) += 1;
        METHOD_RETURN;
    }

    FORTHOP( rcObjectReleaseMethod )
    {
        ulong* pRefCount = (ulong*)(GET_TPD);      // member at offset 0 is refcount
        (*pRefCount) -= 1;
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
        METHOD(     "keep",                 rcObjectKeepMethod ),
        METHOD(     "release",              rcObjectReleaseMethod ),
        MEMBER_VAR( "refCount",             NATIVE_TYPE_TO_CODE(0, kBaseTypeInt) ),
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


    FORTHOP( rcArrayNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE( rcArrayStruct, pArray );
        pArray->refCount = 1;
        pArray->elements = new rcArray;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pArray );
    }

    FORTHOP( rcArrayDeleteMethod )
    {
        // go through all elements and release any which are not null
        rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct*>(GET_TPD);
        rcArray::iterator iter;
        rcArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = *iter;
            SAFE_RELEASE( o );
        }
        delete pArray->elements;
        free( pArray );
        METHOD_RETURN;
    }

    FORTHOP( rcArrayResizeMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct*>(GET_TPD);
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
        rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct*>(GET_TPD);
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
        rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct*>(GET_TPD);
        SPUSH( (long) (pArray->elements->size()) );
        METHOD_RETURN;
    }

    FORTHOP( rcArrayGetMethod )
    {
        rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct*>(GET_TPD);
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
        rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct*>(GET_TPD);
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
        long retVal = -1;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
        rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct*>(GET_TPD);
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
        rcArrayStruct* pArray = reinterpret_cast<rcArrayStruct*>(GET_TPD);
        rcArray& a = *(pArray->elements);
        ForthObject fobj;
        POP_OBJECT( fobj );
        SAFE_KEEP( fobj );
        a.push_back( fobj );
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
        METHOD(     "remove",               rcArrayRemoveMethod ),
        //METHOD(     "insert",               rcArrayInsertMethod ),
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


    FORTHOP( rcListNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE( rcListStruct, pList );
        pList->refCount = 1;
		pList->head = NULL;
		pList->tail = NULL;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pList );
    }

    FORTHOP( rcListDeleteMethod )
    {
        // go through all elements and release any which are not null
        rcListStruct* pList = reinterpret_cast<rcListStruct*>(GET_TPD);
        rcListElement* pCur = pList->head;
		while ( pCur != NULL )
		{
			rcListElement* pNext = pCur->next;
			SAFE_RELEASE( pCur->obj );
			free( pCur );
			pCur = pNext;
		}
        free( pList );
        METHOD_RETURN;
    }

    FORTHOP( rcListHeadMethod )
    {
        rcListStruct* pList = reinterpret_cast<rcListStruct*>(GET_TPD);
		SPUSH( (long) pList->head );
        METHOD_RETURN;
    }

    FORTHOP( rcListTailMethod )
    {
        rcListStruct* pList = reinterpret_cast<rcListStruct*>(GET_TPD);
		SPUSH( (long) pList->tail );
        METHOD_RETURN;
    }

    FORTHOP( rcListAddHeadMethod )
    {
        rcListStruct* pList = reinterpret_cast<rcListStruct*>(GET_TPD);
		MALLOCATE( rcListElement, newElem );
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
        rcListStruct* pList = reinterpret_cast<rcListStruct*>(GET_TPD);
		MALLOCATE( rcListElement, newElem );
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
        rcListStruct* pList = reinterpret_cast<rcListStruct*>(GET_TPD);
		if ( pList->head == NULL )
		{
			// TBD: error or return NULL object?
			ASSERT( pList->tail == NULL );
			PUSH_PAIR( NULL, NULL );
		}
		else
		{
			rcListElement* oldHead = pList->head;
			ForthObject& obj = oldHead->obj;
			if ( pList->head == pList->tail )
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
		}
        METHOD_RETURN;
    }

    FORTHOP( rcListRemoveTailMethod )
    {
        rcListStruct* pList = reinterpret_cast<rcListStruct*>(GET_TPD);
		if ( pList->tail == NULL )
		{
			// TBD: error or return NULL object?
			ASSERT( pList->head == NULL );
			PUSH_PAIR( NULL, NULL );
		}
		else
		{
			rcListElement* oldTail = pList->tail;
			ForthObject& obj = oldTail->obj;
			if ( pList->head == pList->tail )
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
		}
        METHOD_RETURN;
    }

    FORTHOP( rcListCountMethod )
    {
        rcListStruct* pList = reinterpret_cast<rcListStruct*>(GET_TPD);
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
	  
	Splice( rcList srcList, listElement* insertBefore
	  insert srcList into this list before position specified by insertBefore
	  if insertBefore is null, insert at tail of list
	  after splice, srcList is empty


    remove a range (start, end)
	 end is one past last element to delete, end==NULL deletes to end of list

	? replace an elements object
	? isEmpty
	*/

    FORTHOP( rcListFindMethod )
    {
        rcListStruct* pList = reinterpret_cast<rcListStruct*>(GET_TPD);
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
        METHOD(     "count",                rcListCountMethod ),
        METHOD(     "find",                 rcListFindMethod ),
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


    FORTHOP( rcMapNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE( rcMapStruct, pMap );
        pMap->refCount = 1;
        pMap->elements = new rcMap;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pMap );
    }

    FORTHOP( rcMapDeleteMethod )
    {
        // go through all elements and release any which are not null
        rcMapStruct* pMap = reinterpret_cast<rcMapStruct*>(GET_TPD);
        rcMap::iterator iter;
        rcMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE( o );
        }
        delete pMap->elements;
        free( pMap );
        METHOD_RETURN;
    }

    FORTHOP( rcMapClearMethod )
    {
        // go through all elements and release any which are not null
        rcMapStruct* pMap = reinterpret_cast<rcMapStruct*>(GET_TPD);
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
        rcMapStruct* pMap = reinterpret_cast<rcMapStruct*>(GET_TPD);
        SPUSH( (long) (pMap->elements->size()) );
        METHOD_RETURN;
    }

    FORTHOP( rcMapGetMethod )
    {
        rcMapStruct* pMap = reinterpret_cast<rcMapStruct*>(GET_TPD);
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
        rcMapStruct* pMap = reinterpret_cast<rcMapStruct*>(GET_TPD);
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
        long retVal = -1;
		long found = 0;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
        rcMapStruct* pMap = reinterpret_cast<rcMapStruct*>(GET_TPD);
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
        rcMapStruct* pMap = reinterpret_cast<rcMapStruct*>(GET_TPD);
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
        //METHOD(     "insert",               rcMapInsertMethod ),
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
        MALLOCATE( rcStringStruct, pString );
        pString->refCount = 1;
		pString->str = CreateRCString( gDefaultRCStringSize );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pString );
    }

    FORTHOP( rcStringDeleteMethod )
    {
        // go through all elements and release any which are not null
        rcStringStruct* pString = reinterpret_cast<rcStringStruct*>(GET_TPD);
		free( pString->str );
        free( pString );
        METHOD_RETURN;
    }

    FORTHOP( rcStringSizeMethod )
    {
        rcStringStruct* pString = reinterpret_cast<rcStringStruct*>(GET_TPD);
		SPUSH( pString->str->maxLen );
        METHOD_RETURN;
    }

    FORTHOP( rcStringLengthMethod )
    {
        rcStringStruct* pString = reinterpret_cast<rcStringStruct*>(GET_TPD);
		SPUSH( pString->str->curLen );
        METHOD_RETURN;
    }

    FORTHOP( rcStringGetMethod )
    {
        rcStringStruct* pString = reinterpret_cast<rcStringStruct*>(GET_TPD);
		SPUSH( (long) &(pString->str->data[0]) );
        METHOD_RETURN;
    }

    FORTHOP( rcStringSetMethod )
    {
        rcStringStruct* pString = reinterpret_cast<rcStringStruct*>(GET_TPD);
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
        rcStringStruct* pString = reinterpret_cast<rcStringStruct*>(GET_TPD);
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
    ForthClassVocabulary* pRCArrayClass = pEngine->AddBuiltinClass( "rcArray", pRCObjectClass, rcArrayMembers );
    ForthClassVocabulary* pRCListClass = pEngine->AddBuiltinClass( "rcList", pRCObjectClass, rcListMembers );
    ForthClassVocabulary* pRCMapClass = pEngine->AddBuiltinClass( "rcMap", pRCObjectClass, rcMapMembers );
    ForthClassVocabulary* pRCStringClass = pEngine->AddBuiltinClass( "rcString", pRCObjectClass, rcStringMembers );
    mpClassMethods = pClassClass->GetInterface( 0 )->GetMethods();
}

