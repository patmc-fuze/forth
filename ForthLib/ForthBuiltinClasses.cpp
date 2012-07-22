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

#ifdef TRACK_OBJECT_ALLOCATIONS
long gStatNews = 0;
long gStatDeletes = 0;
long gStatLinkNews = 0;
long gStatLinkDeletes = 0;
long gStatIterNews = 0;
long gStatIterDeletes = 0;
long gStatKeeps = 0;
long gStatReleases = 0;
#endif

namespace
{

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
    FORTHOP( objectNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        long nBytes = pClassVocab->GetSize();
        long* pData = (long *) malloc( nBytes );
		// clear the entire object area - this handles both its refcount and any object pointers it might contain
		memset( pData, 0, nBytes );
		TRACK_NEW;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pData );
    }

    FORTHOP( objectDeleteMethod )
    {
        FREE_OBJECT( GET_TPD );
        METHOD_RETURN;
    }

    FORTHOP( objectShowMethod )
    {
        char buff[512];
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

    FORTHOP( objectKeepMethod )
    {
        ulong* pRefCount = (ulong*)(GET_TPD);      // member at offset 0 is refcount
        (*pRefCount) += 1;
		TRACK_KEEP;
        METHOD_RETURN;
    }

    FORTHOP( objectReleaseMethod )
    {
        ulong* pRefCount = (ulong*)(GET_TPD);      // member at offset 0 is refcount
        (*pRefCount) -= 1;
		TRACK_RELEASE;
        if ( *pRefCount != 0 )
        {
            METHOD_RETURN;
		}
        ForthEngine *pEngine = ForthEngine::GetInstance();
        ulong deleteOp = GET_TPM[ kMethodDelete ];
        pEngine->ExecuteOneOp( deleteOp );
        // we are effectively chaining to the delete op, its method return will pop TPM & TPD for us
    }

    baseMethodEntry objectMembers[] =
    {
        METHOD(     "new",              objectNew ),
        METHOD(     "delete",           objectDeleteMethod ),
        METHOD(     "show",             objectShowMethod ),
        METHOD_RET( "getClass",         objectClassMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIClass) ),
        METHOD_RET( "compare",          objectCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "keep",				objectKeepMethod ),
        METHOD(     "release",			objectReleaseMethod ),
        MEMBER_VAR( "refCount",             NATIVE_TYPE_TO_CODE(0, kBaseTypeInt) ),
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
    //                 oIter
    //

	// oIter is an abstract iterator class

    baseMethodEntry oIterMembers[] =
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
    //                 oArray
    //

    typedef std::vector<ForthObject> oArray;
    struct oArrayStruct
    {
        ulong       refCount;
        oArray*    elements;
    };

	struct oArrayIterStruct
    {
        ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
    };
	static ForthClassVocabulary* gpArraryIterClassVocab = NULL;

    FORTHOP( oArrayNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oArrayStruct, pArray );
        pArray->refCount = 0;
        pArray->elements = new oArray;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pArray );
    }

    FORTHOP( oArrayDeleteMethod )
    {
        // go through all elements and release any which are not null
		GET_THIS( oArrayStruct, pArray );
        oArray::iterator iter;
        oArray& a = *(pArray->elements);
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

    FORTHOP( oArrayResizeMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS( oArrayStruct, pArray );
        oArray& a = *(pArray->elements);
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

    FORTHOP( oArrayClearMethod )
    {
        // go through all elements and release any which are not null
		GET_THIS( oArrayStruct, pArray );
        oArray::iterator iter;
        oArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = *iter;
            SAFE_RELEASE( o );
        }
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP( oArrayCountMethod )
    {
		GET_THIS( oArrayStruct, pArray );
		SPUSH( (long) (pArray->elements->size()) );
        METHOD_RETURN;
    }

    FORTHOP( oArrayGetMethod )
    {
		GET_THIS( oArrayStruct, pArray );
        oArray& a = *(pArray->elements);
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

    FORTHOP( oArraySetMethod )
    {
		GET_THIS( oArrayStruct, pArray );
        oArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
            ForthObject& oldObj = a[ix];
            ForthObject newObj;
            POP_OBJECT( newObj );
            if ( OBJECTS_DIFFERENT( oldObj, newObj ) )
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

    FORTHOP( oArrayFindIndexMethod )
    {
		GET_THIS( oArrayStruct, pArray );
        long retVal = -1;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
        oArray::iterator iter;
        oArray& a = *(pArray->elements);
        for ( ulong i = 0; i < a.size(); i++ )
        {
            ForthObject& o = a[i];
            if ( OBJECTS_SAME( o, soughtObj ) )
            {
                retVal = i;
                break;
            }
        }
        SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( oArrayPushMethod )
    {
		GET_THIS( oArrayStruct, pArray );
        oArray& a = *(pArray->elements);
        ForthObject fobj;
        POP_OBJECT( fobj );
		SAFE_KEEP( fobj );
        a.push_back( fobj );
        METHOD_RETURN;
    }

    FORTHOP( oArrayPopMethod )
    {
		GET_THIS( oArrayStruct, pArray );
        oArray& a = *(pArray->elements);
        if ( a.size() > 0 )
        {
			ForthObject fobj = a.back();
			a.pop_back();
			SAFE_RELEASE( fobj );
			PUSH_OBJECT( fobj );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " pop of empty oArray" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oArrayHeadIterMethod )
    {
        GET_THIS( oArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oArrayIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = 0;
        ForthInterface* pPrimaryInterface = gpArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oArrayTailIterMethod )
    {
        GET_THIS( oArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oArrayIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = pArray->elements->size();
        ForthInterface* pPrimaryInterface = gpArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oArrayCloneMethod )
    {
		GET_THIS( oArrayStruct, pArray );
        oArray::iterator iter;
        oArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        // bump reference counts of all valid elements in this array
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = *iter;
            SAFE_KEEP( o );
        }
        // create clone array and set is size to match this array
		MALLOCATE_OBJECT( oArrayStruct, pCloneArray );
        pCloneArray->refCount = 0;
        pCloneArray->elements = new oArray;
        pCloneArray->elements->resize( a.size() );
        // copy this array contents to clone array
        memcpy( &(pCloneArray->elements[0]), &(a[0]), a.size() << 3 );
        // push cloned array on TOS
        PUSH_PAIR( GET_TPM, pCloneArray );
        METHOD_RETURN;
    }

    FORTHOP( oArrayRemoveMethod )
    {
        METHOD_RETURN;
    }

    baseMethodEntry oArrayMembers[] =
    {
        METHOD(     "new",                  oArrayNew ),
        METHOD(     "delete",               oArrayDeleteMethod ),
        METHOD(     "clear",                oArrayClearMethod ),
        METHOD(     "resize",               oArrayResizeMethod ),
        METHOD(     "count",                oArrayCountMethod ),
        METHOD(     "get",                  oArrayGetMethod ),
        METHOD(     "set",                  oArraySetMethod ),
        METHOD(     "findIndex",            oArrayFindIndexMethod ),
        METHOD(     "push",                 oArrayPushMethod ),
        METHOD(     "pop",                  oArrayPopMethod ),
        METHOD(     "headIter",             oArrayHeadIterMethod ),
        METHOD(     "tailIter",             oArrayTailIterMethod ),
        METHOD(     "clone",                oArrayCloneMethod ),
        //METHOD(     "remove",               oArrayRemoveMethod ),
        //METHOD(     "insert",               oArrayInsertMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oArrayIter
    //

    FORTHOP( oArrayIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a oArrayIter object" );
    }

    FORTHOP( oArrayIterDeleteMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		FREE_ITER( pIter );
        METHOD_RETURN;
    }

    FORTHOP( oArrayIterSeekNextMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>( pIter->parent.pData );
		if ( pIter->cursor < pArray->elements->size() )
		{
			pIter->cursor++;
		}
        METHOD_RETURN;
    }

    FORTHOP( oArrayIterSeekPrevMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
		if ( pIter->cursor > 0 )
		{
			pIter->cursor--;
		}
        METHOD_RETURN;
    }

    FORTHOP( oArrayIterSeekHeadMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
		pIter->cursor = 0;
        METHOD_RETURN;
    }

    FORTHOP( oArrayIterSeekTailMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>( pIter->parent.pData );
		pIter->cursor = pArray->elements->size();
        METHOD_RETURN;
    }

    FORTHOP( oArrayIterNextMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>( pIter->parent.pData );
        oArray& a = *(pArray->elements);
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

    FORTHOP( oArrayIterPrevMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>( pIter->parent.pData );
        oArray& a = *(pArray->elements);
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

    FORTHOP( oArrayIterCurrentMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>( pIter->parent.pData );
        oArray& a = *(pArray->elements);
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

    baseMethodEntry oArrayIterMembers[] =
    {
        METHOD(     "new",                  oArrayIterNew ),
        METHOD(     "delete",               oArrayIterDeleteMethod ),
        METHOD(     "seekNext",             oArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             oArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             oArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             oArrayIterSeekTailMethod ),
        METHOD(     "next",					oArrayIterNextMethod ),
        METHOD(     "prev",                 oArrayIterPrevMethod ),
        METHOD(     "current",				oArrayIterCurrentMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oList
    //


	struct oListElement
	{
		oListElement*	prev;
		oListElement*	next;
		ForthObject		obj;
	};

    struct oListStruct
    {
        ulong			refCount;
		oListElement*	head;
		oListElement*	tail;
    };

	struct oListIterStruct
    {
        ulong			refCount;
		ForthObject		parent;
		oListElement*	cursor;
    };
	static ForthClassVocabulary* gpListIterClassVocab = NULL;


    FORTHOP( oListNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oListStruct, pList );
        pList->refCount = 0;
		pList->head = NULL;
		pList->tail = NULL;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pList );
    }

    FORTHOP( oListDeleteMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( oListStruct, pList );
        oListElement* pCur = pList->head;
		while ( pCur != NULL )
		{
			oListElement* pNext = pCur->next;
			SAFE_RELEASE( pCur->obj );
			FREE_LINK( pCur );
			pCur = pNext;
		}
        FREE_OBJECT( pList );
        METHOD_RETURN;
    }

    FORTHOP( oListHeadMethod )
    {
        GET_THIS( oListStruct, pList );
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

    FORTHOP( oListTailMethod )
    {
        GET_THIS( oListStruct, pList );
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

    FORTHOP( oListAddHeadMethod )
    {
        GET_THIS( oListStruct, pList );
		MALLOCATE_LINK( oListElement, newElem );
        POP_OBJECT( newElem->obj );
		SAFE_KEEP( newElem->obj );
		newElem->prev = NULL;
		oListElement* oldHead = pList->head;
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

    FORTHOP( oListAddTailMethod )
    {
        GET_THIS( oListStruct, pList );
		MALLOCATE_LINK( oListElement, newElem );
        POP_OBJECT( newElem->obj );
		SAFE_KEEP( newElem->obj );
		newElem->next = NULL;
		oListElement* oldTail = pList->tail;
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

    FORTHOP( oListRemoveHeadMethod )
    {
        GET_THIS( oListStruct, pList );
		oListElement* oldHead = pList->head;
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
				oListElement* newHead = oldHead->next;
				ASSERT( newHead != NULL );
				newHead->prev = NULL;
				pList->head = newHead;
			}
			PUSH_OBJECT( obj );
			FREE_LINK( oldHead );
		}
        METHOD_RETURN;
    }

    FORTHOP( oListRemoveTailMethod )
    {
        GET_THIS( oListStruct, pList );
		oListElement* oldTail = pList->tail;
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
				oListElement* newTail = oldTail->prev;
				ASSERT( newTail != NULL );
				newTail->next = NULL;
				pList->tail = newTail;
			}
			PUSH_OBJECT( obj );
			FREE_LINK( oldTail );
		}
        METHOD_RETURN;
    }

    FORTHOP( oListHeadIterMethod )
    {
        GET_THIS( oListStruct, pList );
		pList->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oListIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pList);
		pIter->cursor = pList->head;
        ForthInterface* pPrimaryInterface = gpListIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oListTailIterMethod )
    {
        GET_THIS( oListStruct, pList );
		pList->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oListIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pList);
		pIter->cursor = NULL;
        ForthInterface* pPrimaryInterface = gpListIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oListCountMethod )
    {
        GET_THIS( oListStruct, pList );
		long count = 0;
        oListElement* pCur = pList->head;
		while ( pCur != NULL )
		{
			count++;
			pCur = pCur->next;
		}
        SPUSH( count );
        METHOD_RETURN;
    }

	/*


	Cut( oListIter start, oListIter end )
	  remove a sublist from this list
	  end is one past last element to delete, end==NULL deletes to end of list
	Splice( oList soList, listElement* insertBefore
	  insert soList into this list before position specified by insertBefore
	  if insertBefore is null, insert at tail of list
	  after splice, soList is empty

	? replace an elements object
	? isEmpty
	*/

    FORTHOP( oListFindMethod )
    {
        GET_THIS( oListStruct, pList );
        oListElement* pCur = pList->head;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
		while ( pCur != NULL )
		{
			ForthObject& o = pCur->obj;
			if ( OBJECTS_SAME( o, soughtObj ) )
			{
				break;
			}
			pCur = pCur->next;
		}
		SPUSH( (long) pCur );
    }

    FORTHOP( oListCloneMethod )
    {
		// create an empty list
		MALLOCATE_OBJECT( oListStruct, pCloneList );
        pCloneList->refCount = 0;
		pCloneList->head = NULL;
		pCloneList->tail = NULL;
        // go through all elements and add a reference to any which are not null and add to clone list
        GET_THIS( oListStruct, pList );
        oListElement* pCur = pList->head;
		oListElement* oldTail = NULL;
		while ( pCur != NULL )
		{
			oListElement* pNext = pCur->next;
			MALLOCATE_LINK( oListElement, newElem );
			newElem->obj = pCur->obj;
		    SAFE_KEEP( newElem->obj );
		    if ( oldTail == NULL )
		    {
			    pCloneList->head = newElem;
		    }
		    else
		    {
			    oldTail->next = newElem;
		    }
		    newElem->prev = oldTail;
		    oldTail = newElem;
			pCur = pNext;
		}
		if ( oldTail != NULL )
		{
			oldTail->next = NULL;
		}
        PUSH_PAIR( GET_TPM, pCloneList );
        METHOD_RETURN;
    }

    baseMethodEntry oListMembers[] =
    {
        METHOD(     "new",                  oListNew ),
        METHOD(     "delete",               oListDeleteMethod ),
        METHOD(     "head",                 oListHeadMethod ),
        METHOD(     "tail",                 oListTailMethod ),
        METHOD(     "addHead",              oListAddHeadMethod ),
        METHOD(     "addTail",              oListAddTailMethod ),
        METHOD(     "removeHead",           oListRemoveHeadMethod ),
        METHOD(     "removeTail",           oListRemoveTailMethod ),
        METHOD(     "headIter",             oListHeadIterMethod ),
        METHOD(     "tailIter",             oListTailIterMethod ),
        METHOD(     "count",                oListCountMethod ),
        METHOD(     "find",                 oListFindMethod ),
        METHOD(     "clone",                oListCloneMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oListIter
    //

    FORTHOP( oListIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a oListIter object" );
    }

    FORTHOP( oListIterDeleteMethod )
    {
        GET_THIS( oListIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		FREE_ITER( pIter );
        METHOD_RETURN;
    }

    FORTHOP( oListIterSeekNextMethod )
    {
        GET_THIS( oListIterStruct, pIter );
		if ( pIter->cursor != NULL )
		{
			pIter->cursor = pIter->cursor->next;
		}
        METHOD_RETURN;
    }

    FORTHOP( oListIterSeekPrevMethod )
    {
        GET_THIS( oListIterStruct, pIter );
		if ( pIter->cursor != NULL )
		{
			pIter->cursor = pIter->cursor->prev;
		}
		else
		{
			pIter->cursor = reinterpret_cast<oListStruct *>(pIter->parent.pData)->tail;
		}
        METHOD_RETURN;
    }

    FORTHOP( oListIterSeekHeadMethod )
    {
        GET_THIS( oListIterStruct, pIter );
		pIter->cursor = reinterpret_cast<oListStruct *>(pIter->parent.pData)->head;
        METHOD_RETURN;
    }

    FORTHOP( oListIterSeekTailMethod )
    {
        GET_THIS( oListIterStruct, pIter );
		pIter->cursor = NULL;
        METHOD_RETURN;
    }

    FORTHOP( oListIterNextMethod )
    {
        GET_THIS( oListIterStruct, pIter );
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

    FORTHOP( oListIterPrevMethod )
    {
        GET_THIS( oListIterStruct, pIter );
		// special case: NULL cursor means tail of list
		if ( pIter->cursor == NULL )
		{
			pIter->cursor = reinterpret_cast<oListStruct *>(pIter->parent.pData)->tail;
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

    FORTHOP( oListIterCurrentMethod )
    {
        GET_THIS( oListIterStruct, pIter );
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

    baseMethodEntry oListIterMembers[] =
    {
        METHOD(     "new",                  oListIterNew ),
        METHOD(     "delete",               oListIterDeleteMethod ),
        METHOD(     "seekNext",             oListIterSeekNextMethod ),
        METHOD(     "seekPrev",             oListIterSeekPrevMethod ),
        METHOD(     "seekHead",             oListIterSeekHeadMethod ),
        METHOD(     "seekTail",             oListIterSeekTailMethod ),
        METHOD(     "next",					oListIterNextMethod ),
        METHOD(     "prev",                 oListIterPrevMethod ),
        METHOD(     "current",				oListIterCurrentMethod ),
        // following must be last in table
        END_MEMBERS
    };


	//////////////////////////////////////////////////////////////////////
    ///
    //                 oMap
    //

    typedef std::map<long, ForthObject> oMap;
    struct oMapStruct
    {
        ulong       refCount;
        oMap*		elements;
    };

    struct oMapIterStruct
    {
        ulong				refCount;
		ForthObject			parent;
		oMap::iterator		cursor;
    };
	static ForthClassVocabulary* gpMapIterClassVocab = NULL;



    FORTHOP( oMapNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oMapStruct, pMap );
        pMap->refCount = 0;
        pMap->elements = new oMap;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pMap );
    }

    FORTHOP( oMapDeleteMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( oMapStruct, pMap );
        oMap::iterator iter;
        oMap& a = *(pMap->elements);
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

    FORTHOP( oMapClearMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( oMapStruct, pMap );
        oMap::iterator iter;
        oMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE( o );
        }
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP( oMapCountMethod )
    {
        GET_THIS( oMapStruct, pMap );
        SPUSH( (long) (pMap->elements->size()) );
        METHOD_RETURN;
    }

    FORTHOP( oMapGetMethod )
    {
        GET_THIS( oMapStruct, pMap );
        oMap& a = *(pMap->elements);
        long key = SPOP;
        oMap::iterator iter = a.find( key );
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

    FORTHOP( oMapSetMethod )
    {
        GET_THIS( oMapStruct, pMap );
        oMap& a = *(pMap->elements);
        long key = SPOP;
        ForthObject newObj;
        POP_OBJECT( newObj );
        oMap::iterator iter = a.find( key );
		if ( newObj.pMethodOps != NULL )
		{
			if ( iter != a.end() )
			{
				ForthObject oldObj = iter->second;
				if ( OBJECTS_DIFFERENT( oldObj, newObj ) )
				{
					SAFE_KEEP( newObj );
					SAFE_RELEASE( oldObj );
				}
			}
			else
			{
				SAFE_KEEP( newObj );
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

    FORTHOP( oMapFindIndexMethod )
    {
        GET_THIS( oMapStruct, pMap );
        long retVal = -1;
		long found = 0;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
        oMap::iterator iter;
        oMap& a = *(pMap->elements);
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = iter->second;
			if ( OBJECTS_SAME( o, soughtObj ) )
            {
				found = 1;
                retVal = iter->first;
                break;
            }
        }
        SPUSH( retVal );
        SPUSH( found );
        METHOD_RETURN;
    }

    FORTHOP( oMapRemoveMethod )
    {
        GET_THIS( oMapStruct, pMap );
        oMap& a = *(pMap->elements);
        long key = SPOP;
        oMap::iterator iter = a.find( key );
		if ( iter != a.end() )
		{
			ForthObject& oldObj = iter->second;
            SAFE_RELEASE( oldObj );
			a.erase( iter );
		}
        METHOD_RETURN;
    }

    FORTHOP( oMapHeadIterMethod )
    {
        GET_THIS( oMapStruct, pMap );
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oMapIterStruct* pIter = new oMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
        oMap::iterator iter = pMap->elements->begin();
		pIter->cursor = iter;
		//pIter->cursor = pMap->elements->begin();
        ForthInterface* pPrimaryInterface = gpMapIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oMapTailIterMethod )
    {
        GET_THIS( oMapStruct, pMap );
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oMapIterStruct* pIter = new oMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = pMap->elements->end();
        ForthInterface* pPrimaryInterface = gpMapIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }


    baseMethodEntry oMapMembers[] =
    {
        METHOD(     "new",                  oMapNew ),
        METHOD(     "delete",               oMapDeleteMethod ),
        METHOD(     "clear",                oMapClearMethod ),
        METHOD(     "count",                oMapCountMethod ),
        METHOD(     "get",                  oMapGetMethod ),
        METHOD(     "set",                  oMapSetMethod ),
        METHOD(     "findIndex",            oMapFindIndexMethod ),
        METHOD(     "remove",               oMapRemoveMethod ),
        METHOD(     "headIter",             oMapHeadIterMethod ),
        METHOD(     "tailIter",             oMapTailIterMethod ),
        //METHOD(     "insert",               oMapInsertMethod ),
        // following must be last in table
        END_MEMBERS
    };


	//////////////////////////////////////////////////////////////////////
    ///
    //                 oMapIter
    //

    FORTHOP( oMapIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a oMapIter object" );
    }

    FORTHOP( oMapIterDeleteMethod )
    {
        GET_THIS( oMapIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		delete pIter;
		TRACK_ITER_DELETE;
        METHOD_RETURN;
    }

    FORTHOP( oMapIterSeekNextMethod )
    {
        GET_THIS( oMapIterStruct, pIter );
		pIter->cursor++;
        METHOD_RETURN;
    }

    FORTHOP( oMapIterSeekPrevMethod )
    {
        GET_THIS( oMapIterStruct, pIter );
		pIter->cursor--;
        METHOD_RETURN;
    }

    FORTHOP( oMapIterSeekHeadMethod )
    {
        GET_THIS( oMapIterStruct, pIter );
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>( pIter->parent.pData );
		pIter->cursor = pMap->elements->begin();
        METHOD_RETURN;
    }

    FORTHOP( oMapIterSeekTailMethod )
    {
        GET_THIS( oMapIterStruct, pIter );
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>( pIter->parent.pData );
		pIter->cursor = pMap->elements->end();
        METHOD_RETURN;
    }

    FORTHOP( oMapIterNextMethod )
    {
        GET_THIS( oMapIterStruct, pIter );
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>( pIter->parent.pData );
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

    FORTHOP( oMapIterPrevMethod )
    {
        GET_THIS( oMapIterStruct, pIter );
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>( pIter->parent.pData );
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

    FORTHOP( oMapIterCurrentMethod )
    {
        GET_THIS( oMapIterStruct, pIter );
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>( pIter->parent.pData );
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

    FORTHOP( oMapIterNextPairMethod )
    {
        GET_THIS( oMapIterStruct, pIter );
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>( pIter->parent.pData );
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

    FORTHOP( oMapIterPrevPairMethod )
    {
        GET_THIS( oMapIterStruct, pIter );
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>( pIter->parent.pData );
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

    baseMethodEntry oMapIterMembers[] =
    {
        METHOD(     "new",                  oMapIterNew ),
        METHOD(     "delete",               oMapIterDeleteMethod ),
        METHOD(     "seekNext",             oMapIterSeekNextMethod ),
        METHOD(     "seekPrev",             oMapIterSeekPrevMethod ),
        METHOD(     "seekHead",             oMapIterSeekHeadMethod ),
        METHOD(     "seekTail",             oMapIterSeekTailMethod ),
        METHOD(     "next",					oMapIterNextMethod ),
        METHOD(     "prev",                 oMapIterPrevMethod ),
        METHOD(     "current",				oMapIterCurrentMethod ),
		METHOD(     "nextPair",				oMapIterNextPairMethod ),
        METHOD(     "prevPair",             oMapIterPrevPairMethod ),
        // following must be last in table
        END_MEMBERS
    };

    //////////////////////////////////////////////////////////////////////
    ///
    //                 oString
    //

    typedef std::vector<ForthObject> oArray;
//#define DEFAULT_STRING_DATA_BYTES 32
#define DEFAULT_STRING_DATA_BYTES 256
	int gDefaultRCStringSize = DEFAULT_STRING_DATA_BYTES - 1;

	struct oString
	{
		long		maxLen;
		long		curLen;
		char		data[DEFAULT_STRING_DATA_BYTES];
	};

// temp hackaround for a heap corruption when expanding a string
#define RCSTRING_SLOP 16
	oString* CreateRCString( int maxChars )
	{
		int dataBytes = ((maxChars  + 4) & ~3);
        size_t nBytes = sizeof(oString) + (dataBytes - DEFAULT_STRING_DATA_BYTES);
		oString* str = (oString *) malloc( nBytes +  RCSTRING_SLOP );
		str->maxLen = dataBytes - 1;
		str->curLen = 0;
		str->data[0] = '\0';
		return str;
	}

    struct oStringStruct
    {
        ulong       refCount;
        oString*   str;
    };


    FORTHOP( oStringNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        MALLOCATE_OBJECT( oStringStruct, pString );
        pString->refCount = 0;
		pString->str = CreateRCString( gDefaultRCStringSize );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pString );
    }

    FORTHOP( oStringDeleteMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( oStringStruct, pString );
		free( pString->str );
        FREE_OBJECT( pString );
        METHOD_RETURN;
    }

    FORTHOP( oStringShowMethod )
    {
        char buff[512];
        ForthClassObject* pClassObject = (ForthClassObject *)(*((GET_TPM) - 1));
        GET_THIS( oStringStruct, pString );
        ulong* pRefCount = (ulong*)(GET_TPD);      // member at offset 0 is refcount
        sprintf( buff, "%s  [%s] refCount=%d  METHODS=0x%08x  DATA=0x%08x", pClassObject->pVocab->GetName(), &(pString->str->data[0]), pString->refCount, GET_TPM, GET_TPD );
        CONSOLE_STRING_OUT( buff );
        METHOD_RETURN;
    }

    FORTHOP( oStringSizeMethod )
    {
        GET_THIS( oStringStruct, pString );
		SPUSH( pString->str->maxLen );
        METHOD_RETURN;
    }

    FORTHOP( oStringLengthMethod )
    {
        GET_THIS( oStringStruct, pString );
		SPUSH( pString->str->curLen );
        METHOD_RETURN;
    }

    FORTHOP( oStringGetMethod )
    {
        GET_THIS( oStringStruct, pString );
		SPUSH( (long) &(pString->str->data[0]) );
        METHOD_RETURN;
    }

    FORTHOP( oStringSetMethod )
    {
        GET_THIS( oStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
		long len = (long) strlen( srcStr );
		oString* dst = pString->str;
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

    FORTHOP( oStringAppendMethod )
    {
        GET_THIS( oStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
		long len = (long) strlen( srcStr );
		oString* dst = pString->str;
		long newLen = dst->curLen + len;
		if ( newLen > dst->maxLen )
		{
			// enlarge string
			int dataBytes = ((newLen  + 4) & ~3);
	        size_t nBytes = sizeof(oString) + (dataBytes - DEFAULT_STRING_DATA_BYTES);
			dst = (oString *) realloc( dst, nBytes );
		}
		memcpy( &(dst->data[dst->curLen]), srcStr, len + 1 );
		dst->curLen = newLen;
        METHOD_RETURN;
    }

    FORTHOP( oStringStartsWithMethod )
    {
        GET_THIS( oStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
		long result = 0;
		if ( srcStr != NULL )
		{
			long len = (long) strlen( srcStr );
			if ( (len <= pString->str->curLen)
				&& (strncmp( pString->str->data, srcStr, len ) == 0 ) )
			{
				result = ~0;
			}
		}
		SPUSH( result );
        METHOD_RETURN;
    }

    FORTHOP( oStringEndsWithMethod )
    {
        GET_THIS( oStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
		long result = 0;
		if ( srcStr != NULL )
		{
			long len = (long) strlen( srcStr );
			if ( len <= pString->str->curLen )
			{
				const char* strEnd = pString->str->data + (pString->str->curLen - len);
				if ( strcmp( strEnd, srcStr ) == 0 )
				{
					result = ~0;
				}
			}
		}
		SPUSH( result );
        METHOD_RETURN;
    }

    FORTHOP( oStringContainsMethod )
    {
        GET_THIS( oStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
		long result = 0;
		if ( (srcStr != NULL)
			&& ( (long) strlen( srcStr ) <= pString->str->curLen )
			&& (strstr( pString->str->data, srcStr ) != NULL ) )
		{
			result = ~0;
		}
		SPUSH( result );
        METHOD_RETURN;
    }


    baseMethodEntry oStringMembers[] =
    {
        METHOD(     "new",                  oStringNew ),
        METHOD(     "delete",               oStringDeleteMethod ),
        METHOD(     "show",					oStringShowMethod ),
        METHOD(     "size",                 oStringSizeMethod ),
        METHOD(     "length",               oStringLengthMethod ),
        METHOD(     "get",                  oStringGetMethod ),
        METHOD(     "set",                  oStringSetMethod ),
        METHOD(     "append",               oStringAppendMethod ),
        METHOD(     "startsWith",           oStringStartsWithMethod ),
        METHOD(     "endsWith",             oStringEndsWithMethod ),
        METHOD(     "contains",             oStringContainsMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oPair
    //

    struct oPairStruct
    {
        ulong       refCount;
		ForthObject	a;
		ForthObject	b;
    };


    FORTHOP( oPairNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        MALLOCATE_OBJECT( oPairStruct, pPair );
        pPair->refCount = 0;
		pPair->a.pData = NULL;
		pPair->a.pMethodOps = NULL;
		pPair->b.pData = NULL;
		pPair->b.pMethodOps = NULL;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pPair );
    }

    FORTHOP( oPairDeleteMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( oPairStruct, pPair );
        ForthObject& oa = pPair->a;
        SAFE_RELEASE( oa );
        ForthObject& ob = pPair->b;
        SAFE_RELEASE( ob );
        FREE_OBJECT( pPair );
        METHOD_RETURN;
    }

    FORTHOP( oPairGetAMethod )
    {
        GET_THIS( oPairStruct, pPair );
		PUSH_OBJECT( pPair->a );
        METHOD_RETURN;
    }

    FORTHOP( oPairSetAMethod )
    {
        GET_THIS( oPairStruct, pPair );
        ForthObject newObj;
        POP_OBJECT( newObj );
		ForthObject& oldObj = pPair->a;
        if ( OBJECTS_DIFFERENT( oldObj, newObj ) )
        {
            SAFE_RELEASE( oldObj );
			pPair->a = newObj;
        }
        METHOD_RETURN;
    }

    FORTHOP( oPairGetBMethod )
    {
        GET_THIS( oPairStruct, pPair );
		PUSH_OBJECT( pPair->b );
        METHOD_RETURN;
    }

    FORTHOP( oPairSetBMethod )
    {
        GET_THIS( oPairStruct, pPair );
        ForthObject newObj;
        POP_OBJECT( newObj );
		ForthObject& oldObj = pPair->b;
        if ( OBJECTS_DIFFERENT( oldObj, newObj ) )
        {
            SAFE_RELEASE( oldObj );
			pPair->b = newObj;
        }
        METHOD_RETURN;
    }

    baseMethodEntry oPairMembers[] =
    {
        METHOD(     "new",                  oPairNew ),
        METHOD(     "delete",               oPairDeleteMethod ),
        METHOD(     "setA",                 oPairSetAMethod ),
        METHOD(     "getA",                 oPairGetAMethod ),
        METHOD(     "setB",                 oPairSetBMethod ),
        METHOD(     "getB",                 oPairGetBMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oTriple
    //

    struct oTripleStruct
    {
        ulong       refCount;
		ForthObject	a;
		ForthObject	b;
		ForthObject	c;
    };


    FORTHOP( oTripleNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        MALLOCATE_OBJECT( oTripleStruct, pTriple );
        pTriple->refCount = 0;
		pTriple->a.pData = NULL;
		pTriple->a.pMethodOps = NULL;
		pTriple->b.pData = NULL;
		pTriple->b.pMethodOps = NULL;
		pTriple->c.pData = NULL;
		pTriple->c.pMethodOps = NULL;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pTriple );
    }

    FORTHOP( oTripleDeleteMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( oTripleStruct, pTriple );
        ForthObject& oa = pTriple->a;
        SAFE_RELEASE( oa );
        ForthObject& ob = pTriple->b;
        SAFE_RELEASE( ob );
        ForthObject& oc = pTriple->c;
        SAFE_RELEASE( oc );
        FREE_OBJECT( pTriple );
        METHOD_RETURN;
    }

    FORTHOP( oTripleGetAMethod )
    {
        GET_THIS( oTripleStruct, pTriple );
		PUSH_OBJECT( pTriple->a );
        METHOD_RETURN;
    }

    FORTHOP( oTripleSetAMethod )
    {
        GET_THIS( oTripleStruct, pTriple );
        ForthObject newObj;
        POP_OBJECT( newObj );
		ForthObject& oldObj = pTriple->a;
        if ( OBJECTS_DIFFERENT( oldObj, newObj ) )
        {
            SAFE_RELEASE( oldObj );
			pTriple->a = newObj;
        }
        METHOD_RETURN;
    }

    FORTHOP( oTripleGetBMethod )
    {
        GET_THIS( oTripleStruct, pTriple );
		PUSH_OBJECT( pTriple->b );
        METHOD_RETURN;
    }

    FORTHOP( oTripleSetBMethod )
    {
        GET_THIS( oTripleStruct, pTriple );
        ForthObject newObj;
        POP_OBJECT( newObj );
		ForthObject& oldObj = pTriple->b;
        if ( OBJECTS_DIFFERENT( oldObj, newObj ) )
        {
            SAFE_RELEASE( oldObj );
			pTriple->b = newObj;
        }
        METHOD_RETURN;
    }

    FORTHOP( oTripleGetCMethod )
    {
        GET_THIS( oTripleStruct, pTriple );
		PUSH_OBJECT( pTriple->c );
        METHOD_RETURN;
    }

    FORTHOP( oTripleSetCMethod )
    {
        GET_THIS( oTripleStruct, pTriple );
        ForthObject newObj;
        POP_OBJECT( newObj );
		ForthObject& oldObj = pTriple->b;
        if ( OBJECTS_DIFFERENT( oldObj, newObj ) )
        {
            SAFE_RELEASE( oldObj );
			pTriple->c = newObj;
        }
        METHOD_RETURN;
    }

    baseMethodEntry oTripleMembers[] =
    {
        METHOD(     "new",                  oTripleNew ),
        METHOD(     "delete",               oTripleDeleteMethod ),
        METHOD(     "setA",                 oTripleSetAMethod ),
        METHOD(     "getA",                 oTripleGetAMethod ),
        METHOD(     "setB",                 oTripleSetBMethod ),
        METHOD(     "getB",                 oTripleGetBMethod ),
        METHOD(     "setC",                 oTripleSetCMethod ),
        METHOD(     "getC",                 oTripleGetCMethod ),
        // following must be last in table
        END_MEMBERS
    };

    
    //////////////////////////////////////////////////////////////////////
    ///
    //                 oByteArray
    //

    typedef std::vector<char> oByteArray;
    struct oByteArrayStruct
    {
        ulong       refCount;
        oByteArray*    elements;
    };

	struct oByteArrayIterStruct
    {
        ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
    };
	static ForthClassVocabulary* gpByteArraryIterClassVocab = NULL;

    FORTHOP( oByteArrayNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oByteArrayStruct, pArray );
        pArray->refCount = 0;
        pArray->elements = new oByteArray;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pArray );
    }

    FORTHOP( oByteArrayDeleteMethod )
    {
		GET_THIS( oByteArrayStruct, pArray );
        ForthEngine *pEngine = ForthEngine::GetInstance();
        delete pArray->elements;
        FREE_OBJECT( pArray );
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayResizeMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS( oByteArrayStruct, pArray );
        oByteArray& a = *(pArray->elements);
        ulong newSize = SPOP;
        ulong oldSize = a.size();
        a.resize( newSize );
        if ( oldSize < newSize )
        {
            // growing - add null bytes to end of array
            char* pElement = &(a[oldSize]);
            memset( pElement, 0, newSize - oldSize );
        }
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayClearMethod )
    {
        // go through all elements and release any which are not null
		GET_THIS( oByteArrayStruct, pArray );
        oByteArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        char* pElement = &(a[0]);
        memset( pElement, 0, a.size() );
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayCountMethod )
    {
		GET_THIS( oByteArrayStruct, pArray );
		SPUSH( (long) (pArray->elements->size()) );
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayGetMethod )
    {
		GET_THIS( oByteArrayStruct, pArray );
        oByteArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
	        SPUSH( (long) a[ix] );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oByteArraySetMethod )
    {
		GET_THIS( oByteArrayStruct, pArray );
        oByteArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
            a[ix] = (char) SPOP;
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayFindIndexMethod )
    {
		GET_THIS( oByteArrayStruct, pArray );
        long retVal = -1;
        char val = (char) SPOP;
        oByteArray& a = *(pArray->elements);
        char* pElement = &(a[0]);
        char* pFound = (char *) memchr( pElement, val, a.size() );
        if ( pFound )
        {
	        retVal = pFound - pElement;
        }
        SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayPushMethod )
    {
		GET_THIS( oByteArrayStruct, pArray );
        oByteArray& a = *(pArray->elements);
        char val = (char) SPOP;
        a.push_back( val );
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayPopMethod )
    {
		GET_THIS( oByteArrayStruct, pArray );
        oByteArray& a = *(pArray->elements);
        if ( a.size() > 0 )
        {
			char val = a.back();
			a.pop_back();
			SPUSH( (long) val );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " pop of empty oByteArray" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayHeadIterMethod )
    {
        GET_THIS( oByteArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oByteArrayIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = 0;
        ForthInterface* pPrimaryInterface = gpByteArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayTailIterMethod )
    {
        GET_THIS( oByteArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oByteArrayIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = pArray->elements->size();
        ForthInterface* pPrimaryInterface = gpByteArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayCloneMethod )
    {
		GET_THIS( oByteArrayStruct, pArray );
        oByteArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        // create clone array and set is size to match this array
		MALLOCATE_OBJECT( oByteArrayStruct, pCloneArray );
        pCloneArray->refCount = 0;
        pCloneArray->elements = new oByteArray;
        pCloneArray->elements->resize( a.size() );
        // copy this array contents to clone array
        memcpy( &(pCloneArray->elements[0]), &(a[0]), a.size() );
        // push cloned array on TOS
        PUSH_PAIR( GET_TPM, pCloneArray );
        METHOD_RETURN;
    }

    baseMethodEntry oByteArrayMembers[] =
    {
        METHOD(     "new",                  oByteArrayNew ),
        METHOD(     "delete",               oByteArrayDeleteMethod ),
        METHOD(     "clear",                oByteArrayClearMethod ),
        METHOD(     "resize",               oByteArrayResizeMethod ),
        METHOD(     "count",                oByteArrayCountMethod ),
        METHOD(     "get",                  oByteArrayGetMethod ),
        METHOD(     "set",                  oByteArraySetMethod ),
        METHOD(     "findIndex",            oByteArrayFindIndexMethod ),
        METHOD(     "push",                 oByteArrayPushMethod ),
        METHOD(     "pop",                  oByteArrayPopMethod ),
        METHOD(     "headIter",             oByteArrayHeadIterMethod ),
        METHOD(     "tailIter",             oByteArrayTailIterMethod ),
        METHOD(     "clone",                oByteArrayCloneMethod ),
        //METHOD(     "remove",               oByteArrayRemoveMethod ),
        //METHOD(     "insert",               oByteArrayInsertMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oByteArrayIter
    //

    FORTHOP( oByteArrayIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a oByteArrayIter object" );
    }

    FORTHOP( oByteArrayIterDeleteMethod )
    {
        GET_THIS( oByteArrayIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		FREE_ITER( pIter );
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayIterSeekNextMethod )
    {
        GET_THIS( oByteArrayIterStruct, pIter );
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>( pIter->parent.pData );
		if ( pIter->cursor < pArray->elements->size() )
		{
			pIter->cursor++;
		}
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayIterSeekPrevMethod )
    {
        GET_THIS( oByteArrayIterStruct, pIter );
		if ( pIter->cursor > 0 )
		{
			pIter->cursor--;
		}
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayIterSeekHeadMethod )
    {
        GET_THIS( oByteArrayIterStruct, pIter );
		pIter->cursor = 0;
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayIterSeekTailMethod )
    {
        GET_THIS( oByteArrayIterStruct, pIter );
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>( pIter->parent.pData );
		pIter->cursor = pArray->elements->size();
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayIterNextMethod )
    {
        GET_THIS( oByteArrayIterStruct, pIter );
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>( pIter->parent.pData );
        oByteArray& a = *(pArray->elements);
		if ( pIter->cursor >= a.size() )
		{
			SPUSH( 0 );
		}
		else
		{
			char val = a[ pIter->cursor ];
			SPUSH( (long) val );
			pIter->cursor++;
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayIterPrevMethod )
    {
        GET_THIS( oByteArrayIterStruct, pIter );
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>( pIter->parent.pData );
        oByteArray& a = *(pArray->elements);
		if ( pIter->cursor == 0 )
		{
			SPUSH( 0 );
		}
		else
		{
			pIter->cursor--;
			char val = a[ pIter->cursor ];
			SPUSH( (long) val );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayIterCurrentMethod )
    {
        GET_THIS( oByteArrayIterStruct, pIter );
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>( pIter->parent.pData );
        oByteArray& a = *(pArray->elements);
		if ( pIter->cursor >= a.size() )
		{
			SPUSH( 0 );
		}
		else
		{
			char ch = a[ pIter->cursor ];
			SPUSH( (long) ch );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    baseMethodEntry oByteArrayIterMembers[] =
    {
        METHOD(     "new",                  oByteArrayIterNew ),
        METHOD(     "delete",               oByteArrayIterDeleteMethod ),
        METHOD(     "seekNext",             oByteArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             oByteArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             oByteArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             oByteArrayIterSeekTailMethod ),
        METHOD(     "next",					oByteArrayIterNextMethod ),
        METHOD(     "prev",                 oByteArrayIterPrevMethod ),
        METHOD(     "current",				oByteArrayIterCurrentMethod ),
        // following must be last in table
        END_MEMBERS
    };



    //////////////////////////////////////////////////////////////////////
    ///
    //                 oShortArray
    //

    typedef std::vector<short> oShortArray;
    struct oShortArrayStruct
    {
        ulong       refCount;
        oShortArray*    elements;
    };

	struct oShortArrayIterStruct
    {
        ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
    };
	static ForthClassVocabulary* gpShortArraryIterClassVocab = NULL;

    FORTHOP( oShortArrayNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oShortArrayStruct, pArray );
        pArray->refCount = 0;
        pArray->elements = new oShortArray;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pArray );
    }

    FORTHOP( oShortArrayDeleteMethod )
    {
		GET_THIS( oShortArrayStruct, pArray );
        ForthEngine *pEngine = ForthEngine::GetInstance();
        delete pArray->elements;
        FREE_OBJECT( pArray );
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayResizeMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS( oShortArrayStruct, pArray );
        oShortArray& a = *(pArray->elements);
        ulong newSize = SPOP;
        ulong oldSize = a.size();
        a.resize( newSize );
        if ( oldSize < newSize )
        {
            // growing - add zeros to end of array
            short* pElement = &(a[oldSize]);
            memset( pElement, 0, ((newSize - oldSize) << 1) );
        }
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayClearMethod )
    {
        // go through all elements and release any which are not null
		GET_THIS( oShortArrayStruct, pArray );
        oShortArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        short* pElement = &(a[0]);
        memset( pElement, 0, (a.size() << 1) );
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayCountMethod )
    {
		GET_THIS( oShortArrayStruct, pArray );
		SPUSH( (long) (pArray->elements->size()) );
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayGetMethod )
    {
		GET_THIS( oShortArrayStruct, pArray );
        oShortArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
	        SPUSH( (long) a[ix] );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oShortArraySetMethod )
    {
		GET_THIS( oShortArrayStruct, pArray );
        oShortArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
            a[ix] = (short) SPOP;
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayFindIndexMethod )
    {
		GET_THIS( oShortArrayStruct, pArray );
        long retVal = -1;
        short val = (short) SPOP;
        oShortArray& a = *(pArray->elements);
        for ( ulong i = 0; i < a.size(); i++ )
        {
	        if ( val == a[i] )
	        {
		        retVal = i;
		        break;
	        }
        }
        SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayPushMethod )
    {
		GET_THIS( oShortArrayStruct, pArray );
        oShortArray& a = *(pArray->elements);
        short val = (short) SPOP;
        a.push_back( val );
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayPopMethod )
    {
		GET_THIS( oShortArrayStruct, pArray );
        oShortArray& a = *(pArray->elements);
        if ( a.size() > 0 )
        {
			short val = a.back();
			a.pop_back();
			SPUSH( (long) val );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " pop of empty oShortArray" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayHeadIterMethod )
    {
        GET_THIS( oShortArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oShortArrayIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = 0;
        ForthInterface* pPrimaryInterface = gpShortArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayTailIterMethod )
    {
        GET_THIS( oShortArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oShortArrayIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = pArray->elements->size();
        ForthInterface* pPrimaryInterface = gpShortArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayCloneMethod )
    {
		GET_THIS( oShortArrayStruct, pArray );
        oShortArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        // create clone array and set is size to match this array
		MALLOCATE_OBJECT( oShortArrayStruct, pCloneArray );
        pCloneArray->refCount = 0;
        pCloneArray->elements = new oShortArray;
        pCloneArray->elements->resize( a.size() );
        // copy this array contents to clone array
        memcpy( &(pCloneArray->elements[0]), &(a[0]), a.size() << 1 );
        // push cloned array on TOS
        PUSH_PAIR( GET_TPM, pCloneArray );
        METHOD_RETURN;
    }
    
    baseMethodEntry oShortArrayMembers[] =
    {
        METHOD(     "new",                  oShortArrayNew ),
        METHOD(     "delete",               oShortArrayDeleteMethod ),
        METHOD(     "clear",                oShortArrayClearMethod ),
        METHOD(     "resize",               oShortArrayResizeMethod ),
        METHOD(     "count",                oShortArrayCountMethod ),
        METHOD(     "get",                  oShortArrayGetMethod ),
        METHOD(     "set",                  oShortArraySetMethod ),
        METHOD(     "findIndex",            oShortArrayFindIndexMethod ),
        METHOD(     "push",                 oShortArrayPushMethod ),
        METHOD(     "pop",                  oShortArrayPopMethod ),
        METHOD(     "headIter",             oShortArrayHeadIterMethod ),
        METHOD(     "tailIter",             oShortArrayTailIterMethod ),
        METHOD(     "clone",                oShortArrayCloneMethod ),
        //METHOD(     "remove",               oShortArrayRemoveMethod ),
        //METHOD(     "insert",               oShortArrayInsertMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oShortArrayIter
    //

    FORTHOP( oShortArrayIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a oShortArrayIter object" );
    }

    FORTHOP( oShortArrayIterDeleteMethod )
    {
        GET_THIS( oShortArrayIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		FREE_ITER( pIter );
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayIterSeekNextMethod )
    {
        GET_THIS( oShortArrayIterStruct, pIter );
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>( pIter->parent.pData );
		if ( pIter->cursor < pArray->elements->size() )
		{
			pIter->cursor++;
		}
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayIterSeekPrevMethod )
    {
        GET_THIS( oShortArrayIterStruct, pIter );
		if ( pIter->cursor > 0 )
		{
			pIter->cursor--;
		}
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayIterSeekHeadMethod )
    {
        GET_THIS( oShortArrayIterStruct, pIter );
		pIter->cursor = 0;
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayIterSeekTailMethod )
    {
        GET_THIS( oShortArrayIterStruct, pIter );
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>( pIter->parent.pData );
		pIter->cursor = pArray->elements->size();
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayIterNextMethod )
    {
        GET_THIS( oShortArrayIterStruct, pIter );
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>( pIter->parent.pData );
        oShortArray& a = *(pArray->elements);
		if ( pIter->cursor >= a.size() )
		{
			SPUSH( 0 );
		}
		else
		{
			short val = a[ pIter->cursor ];
			SPUSH( (long) val );
			pIter->cursor++;
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayIterPrevMethod )
    {
        GET_THIS( oShortArrayIterStruct, pIter );
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>( pIter->parent.pData );
        oShortArray& a = *(pArray->elements);
		if ( pIter->cursor == 0 )
		{
			SPUSH( 0 );
		}
		else
		{
			pIter->cursor--;
			short val = a[ pIter->cursor ];
			SPUSH( (long) val );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayIterCurrentMethod )
    {
        GET_THIS( oShortArrayIterStruct, pIter );
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>( pIter->parent.pData );
        oShortArray& a = *(pArray->elements);
		if ( pIter->cursor >= a.size() )
		{
			SPUSH( 0 );
		}
		else
		{
			short val = a[ pIter->cursor ];
			SPUSH( (long) val );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    baseMethodEntry oShortArrayIterMembers[] =
    {
        METHOD(     "new",                  oShortArrayIterNew ),
        METHOD(     "delete",               oShortArrayIterDeleteMethod ),
        METHOD(     "seekNext",             oShortArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             oShortArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             oShortArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             oShortArrayIterSeekTailMethod ),
        METHOD(     "next",					oShortArrayIterNextMethod ),
        METHOD(     "prev",                 oShortArrayIterPrevMethod ),
        METHOD(     "current",				oShortArrayIterCurrentMethod ),
        // following must be last in table
        END_MEMBERS
    };

    //////////////////////////////////////////////////////////////////////
    ///
    //                 oIntArray
    //

    typedef std::vector<int> oIntArray;
    struct oIntArrayStruct
    {
        ulong       refCount;
        oIntArray*    elements;
    };

	struct oIntArrayIterStruct
    {
        ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
    };
	static ForthClassVocabulary* gpIntArraryIterClassVocab = NULL;

    FORTHOP( oIntArrayNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oIntArrayStruct, pArray );
        pArray->refCount = 0;
        pArray->elements = new oIntArray;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pArray );
    }

    FORTHOP( oIntArrayDeleteMethod )
    {
		GET_THIS( oIntArrayStruct, pArray );
        ForthEngine *pEngine = ForthEngine::GetInstance();
        delete pArray->elements;
        FREE_OBJECT( pArray );
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayResizeMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS( oIntArrayStruct, pArray );
        oIntArray& a = *(pArray->elements);
        ulong newSize = SPOP;
        ulong oldSize = a.size();
        a.resize( newSize );
        if ( oldSize < newSize )
        {
            // growing - add zeros to end of array
            int* pElement = &(a[oldSize]);
            memset( pElement, 0, ((newSize - oldSize) << 2) );
        }
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayClearMethod )
    {
        // go through all elements and release any which are not null
		GET_THIS( oIntArrayStruct, pArray );
        oIntArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        int* pElement = &(a[0]);
        memset( pElement, 0, (a.size() << 2) );
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayCountMethod )
    {
		GET_THIS( oIntArrayStruct, pArray );
		SPUSH( (long) (pArray->elements->size()) );
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayGetMethod )
    {
		GET_THIS( oIntArrayStruct, pArray );
        oIntArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
	        SPUSH( a[ix] );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oIntArraySetMethod )
    {
		GET_THIS( oIntArrayStruct, pArray );
        oIntArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
            a[ix] = SPOP;
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayFindIndexMethod )
    {
		GET_THIS( oIntArrayStruct, pArray );
        long retVal = -1;
        int val = SPOP;
        oIntArray& a = *(pArray->elements);
        for ( ulong i = 0; i < a.size(); i++ )
        {
	        if ( val == a[i] )
	        {
		        retVal = i;
		        break;
	        }
        }
        SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayPushMethod )
    {
		GET_THIS( oIntArrayStruct, pArray );
        oIntArray& a = *(pArray->elements);
        int val = SPOP;
        a.push_back( val );
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayPopMethod )
    {
		GET_THIS( oIntArrayStruct, pArray );
        oIntArray& a = *(pArray->elements);
        if ( a.size() > 0 )
        {
			long val = a.back();
			a.pop_back();
			SPUSH( val );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " pop of empty oIntArray" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayHeadIterMethod )
    {
        GET_THIS( oIntArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oIntArrayIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = 0;
        ForthInterface* pPrimaryInterface = gpIntArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayTailIterMethod )
    {
        GET_THIS( oIntArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oIntArrayIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = pArray->elements->size();
        ForthInterface* pPrimaryInterface = gpIntArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayCloneMethod )
    {
		GET_THIS( oIntArrayStruct, pArray );
        oIntArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        // create clone array and set is size to match this array
		MALLOCATE_OBJECT( oIntArrayStruct, pCloneArray );
        pCloneArray->refCount = 0;
        pCloneArray->elements = new oIntArray;
        pCloneArray->elements->resize( a.size() );
        // copy this array contents to clone array
        memcpy( &(pCloneArray->elements[0]), &(a[0]), a.size() << 2 );
        // push cloned array on TOS
        PUSH_PAIR( GET_TPM, pCloneArray );
        METHOD_RETURN;
    }
    
    baseMethodEntry oIntArrayMembers[] =
    {
        METHOD(     "new",                  oIntArrayNew ),
        METHOD(     "delete",               oIntArrayDeleteMethod ),
        METHOD(     "clear",                oIntArrayClearMethod ),
        METHOD(     "resize",               oIntArrayResizeMethod ),
        METHOD(     "count",                oIntArrayCountMethod ),
        METHOD(     "get",                  oIntArrayGetMethod ),
        METHOD(     "set",                  oIntArraySetMethod ),
        METHOD(     "findIndex",            oIntArrayFindIndexMethod ),
        METHOD(     "push",                 oIntArrayPushMethod ),
        METHOD(     "pop",                  oIntArrayPopMethod ),
        METHOD(     "headIter",             oIntArrayHeadIterMethod ),
        METHOD(     "tailIter",             oIntArrayTailIterMethod ),
        METHOD(     "clone",                oIntArrayCloneMethod ),
        //METHOD(     "remove",               oIntArrayRemoveMethod ),
        //METHOD(     "insert",               oIntArrayInsertMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oIntArrayIter
    //

    FORTHOP( oIntArrayIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a oIntArrayIter object" );
    }

    FORTHOP( oIntArrayIterDeleteMethod )
    {
        GET_THIS( oIntArrayIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		FREE_ITER( pIter );
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayIterSeekNextMethod )
    {
        GET_THIS( oIntArrayIterStruct, pIter );
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>( pIter->parent.pData );
		if ( pIter->cursor < pArray->elements->size() )
		{
			pIter->cursor++;
		}
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayIterSeekPrevMethod )
    {
        GET_THIS( oIntArrayIterStruct, pIter );
		if ( pIter->cursor > 0 )
		{
			pIter->cursor--;
		}
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayIterSeekHeadMethod )
    {
        GET_THIS( oIntArrayIterStruct, pIter );
		pIter->cursor = 0;
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayIterSeekTailMethod )
    {
        GET_THIS( oIntArrayIterStruct, pIter );
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>( pIter->parent.pData );
		pIter->cursor = pArray->elements->size();
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayIterNextMethod )
    {
        GET_THIS( oIntArrayIterStruct, pIter );
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>( pIter->parent.pData );
        oIntArray& a = *(pArray->elements);
		if ( pIter->cursor >= a.size() )
		{
			SPUSH( 0 );
		}
		else
		{
			SPUSH( a[ pIter->cursor ] );
			pIter->cursor++;
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayIterPrevMethod )
    {
        GET_THIS( oIntArrayIterStruct, pIter );
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>( pIter->parent.pData );
        oIntArray& a = *(pArray->elements);
		if ( pIter->cursor == 0 )
		{
			SPUSH( 0 );
		}
		else
		{
			pIter->cursor--;
			SPUSH( a[ pIter->cursor ] );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayIterCurrentMethod )
    {
        GET_THIS( oIntArrayIterStruct, pIter );
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>( pIter->parent.pData );
        oIntArray& a = *(pArray->elements);
		if ( pIter->cursor >= a.size() )
		{
			SPUSH( 0 );
		}
		else
		{
			SPUSH( a[ pIter->cursor ] );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    baseMethodEntry oIntArrayIterMembers[] =
    {
        METHOD(     "new",                  oIntArrayIterNew ),
        METHOD(     "delete",               oIntArrayIterDeleteMethod ),
        METHOD(     "seekNext",             oIntArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             oIntArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             oIntArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             oIntArrayIterSeekTailMethod ),
        METHOD(     "next",					oIntArrayIterNextMethod ),
        METHOD(     "prev",                 oIntArrayIterPrevMethod ),
        METHOD(     "current",				oIntArrayIterCurrentMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oLongArray
    //

    typedef std::vector<long long> oLongArray;
    struct oLongArrayStruct
    {
        ulong       refCount;
        oLongArray*    elements;
    };

	struct oLongArrayIterStruct
    {
        ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
    };
	static ForthClassVocabulary* gpLongArraryIterClassVocab = NULL;

    FORTHOP( oLongArrayNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oLongArrayStruct, pArray );
        pArray->refCount = 0;
        pArray->elements = new oLongArray;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pArray );
    }

    FORTHOP( oLongArrayDeleteMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
        ForthEngine *pEngine = ForthEngine::GetInstance();
        delete pArray->elements;
        FREE_OBJECT( pArray );
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayResizeMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS( oLongArrayStruct, pArray );
        oLongArray& a = *(pArray->elements);
        ulong newSize = SPOP;
        ulong oldSize = a.size();
        a.resize( newSize );
        if ( oldSize < newSize )
        {
            // growing - add zeros to end of array
            long long* pElement = &(a[oldSize]);
            memset( pElement, 0, ((newSize - oldSize) << 3) );
        }
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayClearMethod )
    {
        // go through all elements and release any which are not null
		GET_THIS( oLongArrayStruct, pArray );
        oLongArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        long long* pElement = &(a[0]);
        memset( pElement, 0, (a.size() << 3) );
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayCountMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
		SPUSH( (long) (pArray->elements->size()) );
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayGetMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
        oLongArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
	        LPUSH( a[ix] );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oLongArraySetMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
        oLongArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
            a[ix] = LPOP;
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayFindIndexMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
        long retVal = -1;
        long long val = LPOP;
        oLongArray& a = *(pArray->elements);
        for ( ulong i = 0; i < a.size(); i++ )
        {
	        if ( val == a[i] )
	        {
		        retVal = i;
		        break;
	        }
        }
        SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayPushMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
        oLongArray& a = *(pArray->elements);
        long long val = LPOP;
        a.push_back( val );
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayPopMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
        oLongArray& a = *(pArray->elements);
        if ( a.size() > 0 )
        {
			long long val = a.back();
			a.pop_back();
			LPUSH( val );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " pop of empty oLongArray" );
        }
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayHeadIterMethod )
    {
        GET_THIS( oLongArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oLongArrayIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = 0;
        ForthInterface* pPrimaryInterface = gpLongArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayTailIterMethod )
    {
        GET_THIS( oLongArrayStruct, pArray );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oLongArrayIterStruct, pIter );
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pArray);
		pIter->cursor = pArray->elements->size();
        ForthInterface* pPrimaryInterface = gpLongArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayCloneMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
        oLongArray& a = *(pArray->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        // create clone array and set is size to match this array
		MALLOCATE_OBJECT( oLongArrayStruct, pCloneArray );
        pCloneArray->refCount = 0;
        pCloneArray->elements = new oLongArray;
        pCloneArray->elements->resize( a.size() );
        // copy this array contents to clone array
        memcpy( &(pCloneArray->elements[0]), &(a[0]), a.size() << 3 );
        // push cloned array on TOS
        PUSH_PAIR( GET_TPM, pCloneArray );
        METHOD_RETURN;
    }
    
    baseMethodEntry oLongArrayMembers[] =
    {
        METHOD(     "new",                  oLongArrayNew ),
        METHOD(     "delete",               oLongArrayDeleteMethod ),
        METHOD(     "clear",                oLongArrayClearMethod ),
        METHOD(     "resize",               oLongArrayResizeMethod ),
        METHOD(     "count",                oLongArrayCountMethod ),
        METHOD(     "get",                  oLongArrayGetMethod ),
        METHOD(     "set",                  oLongArraySetMethod ),
        METHOD(     "findIndex",            oLongArrayFindIndexMethod ),
        METHOD(     "push",                 oLongArrayPushMethod ),
        METHOD(     "pop",                  oLongArrayPopMethod ),
        METHOD(     "headIter",             oLongArrayHeadIterMethod ),
        METHOD(     "tailIter",             oLongArrayTailIterMethod ),
        METHOD(     "clone",                oLongArrayCloneMethod ),
        //METHOD(     "remove",               oLongArrayRemoveMethod ),
        //METHOD(     "insert",               oLongArrayInsertMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oLongArrayIter
    //

    FORTHOP( oLongArrayIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a oLongArrayIter object" );
    }

    FORTHOP( oLongArrayIterDeleteMethod )
    {
        GET_THIS( oLongArrayIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		FREE_ITER( pIter );
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayIterSeekNextMethod )
    {
        GET_THIS( oLongArrayIterStruct, pIter );
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>( pIter->parent.pData );
		if ( pIter->cursor < pArray->elements->size() )
		{
			pIter->cursor++;
		}
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayIterSeekPrevMethod )
    {
        GET_THIS( oLongArrayIterStruct, pIter );
		if ( pIter->cursor > 0 )
		{
			pIter->cursor--;
		}
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayIterSeekHeadMethod )
    {
        GET_THIS( oLongArrayIterStruct, pIter );
		pIter->cursor = 0;
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayIterSeekTailMethod )
    {
        GET_THIS( oLongArrayIterStruct, pIter );
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>( pIter->parent.pData );
		pIter->cursor = pArray->elements->size();
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayIterNextMethod )
    {
        GET_THIS( oLongArrayIterStruct, pIter );
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>( pIter->parent.pData );
        oLongArray& a = *(pArray->elements);
		if ( pIter->cursor >= a.size() )
		{
			SPUSH( 0 );
		}
		else
		{
			long long val = a[ pIter->cursor ];
			LPUSH( val );
			pIter->cursor++;
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayIterPrevMethod )
    {
        GET_THIS( oLongArrayIterStruct, pIter );
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>( pIter->parent.pData );
        oLongArray& a = *(pArray->elements);
		if ( pIter->cursor == 0 )
		{
			SPUSH( 0 );
		}
		else
		{
			pIter->cursor--;
			long long val = a[ pIter->cursor ];
			LPUSH( val );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayIterCurrentMethod )
    {
        GET_THIS( oLongArrayIterStruct, pIter );
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>( pIter->parent.pData );
        oLongArray& a = *(pArray->elements);
		if ( pIter->cursor >= a.size() )
		{
			SPUSH( 0 );
		}
		else
		{
			long long val = a[ pIter->cursor ];
			LPUSH( val );
		    SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    baseMethodEntry oLongArrayIterMembers[] =
    {
        METHOD(     "new",                  oLongArrayIterNew ),
        METHOD(     "delete",               oLongArrayIterDeleteMethod ),
        METHOD(     "seekNext",             oLongArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             oLongArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             oLongArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             oLongArrayIterSeekTailMethod ),
        METHOD(     "next",					oLongArrayIterNextMethod ),
        METHOD(     "prev",                 oLongArrayIterPrevMethod ),
        METHOD(     "current",				oLongArrayIterCurrentMethod ),
        // following must be last in table
        END_MEMBERS
    };

}

void
ForthTypesManager::AddBuiltinClasses( ForthEngine* pEngine )
{
    ForthClassVocabulary* pObjectClass = pEngine->AddBuiltinClass( "object", NULL, objectMembers );
    ForthClassVocabulary* pClassClass = pEngine->AddBuiltinClass( "class", pObjectClass, classMembers );

	ForthClassVocabulary* pOIterClass = pEngine->AddBuiltinClass( "oIter", pObjectClass, oIterMembers );

    ForthClassVocabulary* pOArrayClass = pEngine->AddBuiltinClass( "oArray", pObjectClass, oArrayMembers );
    gpArraryIterClassVocab = pEngine->AddBuiltinClass( "oArrayIter", pOIterClass, oArrayIterMembers );

    ForthClassVocabulary* pOListClass = pEngine->AddBuiltinClass( "oList", pObjectClass, oListMembers );
    gpListIterClassVocab = pEngine->AddBuiltinClass( "oListIter", pOIterClass, oListIterMembers );

    ForthClassVocabulary* pOMapClass = pEngine->AddBuiltinClass( "oMap", pObjectClass, oMapMembers );
    gpMapIterClassVocab = pEngine->AddBuiltinClass( "oMapIter", pOIterClass, oMapIterMembers );

    ForthClassVocabulary* pOStringClass = pEngine->AddBuiltinClass( "oString", pObjectClass, oStringMembers );

    ForthClassVocabulary* pOPairClass = pEngine->AddBuiltinClass( "oPair", pObjectClass, oPairMembers );
    ForthClassVocabulary* pOTripleClass = pEngine->AddBuiltinClass( "oTriple", pObjectClass, oTripleMembers );

    ForthClassVocabulary* pOByteArrayClass = pEngine->AddBuiltinClass( "oByteArray", pObjectClass, oByteArrayMembers );
    gpByteArraryIterClassVocab = pEngine->AddBuiltinClass( "oByteArrayIter", pOIterClass, oByteArrayIterMembers );

    ForthClassVocabulary* pOShortArrayClass = pEngine->AddBuiltinClass( "oShortArray", pObjectClass, oShortArrayMembers );
    gpShortArraryIterClassVocab = pEngine->AddBuiltinClass( "oShortArrayIter", pOIterClass, oShortArrayIterMembers );

    ForthClassVocabulary* pOIntArrayClass = pEngine->AddBuiltinClass( "oIntArray", pObjectClass, oIntArrayMembers );
    gpIntArraryIterClassVocab = pEngine->AddBuiltinClass( "oIntArrayIter", pOIterClass, oIntArrayIterMembers );

    ForthClassVocabulary* pOLongArrayClass = pEngine->AddBuiltinClass( "oLongArray", pObjectClass, oLongArrayMembers );
    gpLongArraryIterClassVocab = pEngine->AddBuiltinClass( "oLongArrayIter", pOIterClass, oLongArrayIterMembers );

    mpClassMethods = pClassClass->GetInterface( 0 )->GetMethods();
}

