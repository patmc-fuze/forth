//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.cpp: builtin classes
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
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

extern "C" {
	unsigned long SuperFastHash (const char * data, int len, unsigned long hash);
};

extern void unimplementedMethodOp( ForthCoreState *pCore );

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
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			ulong deleteOp = GET_TPM[ kMethodDelete ];
			pEngine->ExecuteOneOp( deleteOp );
			// we are effectively chaining to the delete op, its method return will pop TPM & TPD for us
		}
    }

    baseMethodEntry objectMembers[] =
    {
        METHOD(     "_%new%_",          objectNew ),
        METHOD(     "delete",           objectDeleteMethod ),
        METHOD(     "show",             objectShowMethod ),
        METHOD_RET( "getClass",         objectClassMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIClass) ),
        METHOD_RET( "compare",          objectCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "keep",				objectKeepMethod ),
        METHOD(     "release",			objectReleaseMethod ),
        MEMBER_VAR( "refCount",         NATIVE_TYPE_TO_CODE(0, kBaseTypeInt) ),
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
        METHOD(     "seekNext",             unimplementedMethodOp ),
        METHOD(     "seekPrev",             unimplementedMethodOp ),
        METHOD(     "seekHead",             unimplementedMethodOp ),
        METHOD(     "seekTail",             unimplementedMethodOp ),
        METHOD_RET( "next",					unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				unimplementedMethodOp ),
        METHOD_RET( "findNext",				unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "clone",                unimplementedMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIter) ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oIterable
    //

	// oIterable is an abstract iterable class, containers should be derived from oIterable

    baseMethodEntry oIterableMembers[] =
    {
        METHOD_RET( "headIter",             unimplementedMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIter) ),
        METHOD_RET( "tailIter",             unimplementedMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIter) ),
        METHOD_RET( "find",                 unimplementedMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIter) ),
        METHOD_RET( "clone",                unimplementedMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIterable) ),
        METHOD_RET( "count",                unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
		METHOD( "clear",                    unimplementedMethodOp ),
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

    FORTHOP( oArrayRefMethod )
    {
		GET_THIS( oArrayStruct, pArray );
        oArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
	        SPUSH( (long) &(a[ix]) );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
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

    FORTHOP( oArrayFindMethod )
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
		if ( retVal < 0 )
		{
	        SPUSH( 0 );
		}
		else
		{
			pArray->refCount++;
			TRACK_KEEP;
			MALLOCATE_ITER( oArrayIterStruct, pIter );
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pArray);
			pIter->cursor = retVal;
			ForthInterface* pPrimaryInterface = gpArraryIterClassVocab->GetInterface( 0 );
			PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
	        SPUSH( ~0 );
		}
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

    baseMethodEntry oArrayMembers[] =
    {
        METHOD(     "_%new%_",              oArrayNew ),
        METHOD(     "delete",               oArrayDeleteMethod ),

		METHOD_RET( "headIter",             oArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter) ),
        METHOD_RET( "tailIter",             oArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter) ),
        METHOD_RET( "find",                 oArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter) ),
        METHOD_RET( "clone",                oArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArray) ),
        METHOD_RET( "count",                oArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oArrayClearMethod ),

        METHOD(     "resize",               oArrayResizeMethod ),
        METHOD_RET( "ref",                  oArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject | kDTIsPtr) ),
        METHOD_RET( "get",                  oArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD(     "set",                  oArraySetMethod ),
        METHOD_RET( "findIndex",            oArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "push",                 oArrayPushMethod ),
        METHOD_RET( "pop",                  oArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
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

    FORTHOP( oArrayIterRemoveMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>( pIter->parent.pData );
        oArray& a = *(pArray->elements);
		if ( pIter->cursor < a.size() )
		{
			// TBD!
			ForthObject o = a[ pIter->cursor ];
            SAFE_RELEASE( o );
			pArray->elements->erase( pArray->elements->begin() + pIter->cursor );
		}
        METHOD_RETURN;
    }

    FORTHOP( oArrayIterFindNextMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
        long retVal = 0;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>( pIter->parent.pData );
        oArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while ( i < a.size() )
		{
            ForthObject& o = a[i];
            if ( OBJECTS_SAME( o, soughtObj ) )
            {
                retVal = ~0;
				pIter->cursor = i;
                break;
            }
		}
		SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( oArrayIterCloneMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
		MALLOCATE_ITER( oArrayIterStruct, pNewIter );
		pNewIter->refCount = 0;
		pNewIter->parent.pMethodOps = pIter->parent.pMethodOps;
		pNewIter->parent.pData = pIter->parent.pData;
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>( pIter->parent.pData );
		pArray->refCount++;
		TRACK_KEEP;
		pNewIter->cursor = pIter->cursor;
        PUSH_PAIR( GET_TPM, pNewIter );
        METHOD_RETURN;
    }

    baseMethodEntry oArrayIterMembers[] =
    {
        METHOD(     "_%new%_",              oArrayIterNew ),
        METHOD(     "delete",               oArrayIterDeleteMethod ),

        METHOD(     "seekNext",             oArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             oArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             oArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             oArrayIterSeekTailMethod ),
        METHOD_RET( "next",					oArrayIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oArrayIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oArrayIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				oArrayIterRemoveMethod ),
        METHOD_RET( "findNext",				oArrayIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "clone",                oArrayIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter) ),
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
		if ( pCur == NULL )
		{
	        SPUSH( 0 );
		}
		else
		{
			pList->refCount++;
			TRACK_KEEP;
			MALLOCATE_ITER( oListIterStruct, pIter );
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pList);
			pIter->cursor = pCur;
			ForthInterface* pPrimaryInterface = gpListIterClassVocab->GetInterface( 0 );
			PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
	        SPUSH( ~0 );
		}
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

    FORTHOP( oListClearMethod )
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
		pList->head = NULL;
		pList->tail = NULL;
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
        METHOD(     "_%new%_",              oListNew ),
        METHOD(     "delete",               oListDeleteMethod ),

        METHOD_RET( "headIter",             oListHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIListIter) ),
        METHOD_RET( "tailIter",             oListTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIListIter) ),
        METHOD_RET( "find",                 oListFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIListIter) ),
        METHOD_RET( "clone",                oListCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIList) ),
        METHOD_RET( "count",                oListCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oListClearMethod ),

		METHOD_RET( "head",                 oListHeadMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD_RET( "tail",                 oListTailMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD(     "addHead",              oListAddHeadMethod ),
        METHOD(     "addTail",              oListAddTailMethod ),
        METHOD(     "removeHead",           oListRemoveHeadMethod ),	// TBD
        METHOD(     "removeTail",           oListRemoveTailMethod ),	// TBD
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

    FORTHOP( oListIterSwapNextMethod )
    {
        GET_THIS( oListIterStruct, pIter );
		oListElement* pCursor = pIter->cursor;
		if ( pCursor != NULL )
		{
			oListElement* pNext = pCursor->next;
			if ( pNext != NULL )
			{
				ForthObject nextObj = pNext->obj;
				ForthObject obj = pCursor->obj;
				pCursor->obj = nextObj;
				pNext->obj = obj;
			}
		}
        METHOD_RETURN;
    }

    FORTHOP( oListIterSwapPrevMethod )
    {
        GET_THIS( oListIterStruct, pIter );
		oListElement* pCursor = pIter->cursor;
		if ( pCursor != NULL )
		{
			oListElement* pPrev = pCursor->prev;
			if ( pPrev != NULL )
			{
				ForthObject prevObj = pPrev->obj;
				ForthObject obj = pCursor->obj;
				pCursor->obj = prevObj;
				pPrev->obj = obj;
			}
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

    FORTHOP( oListIterRemoveMethod )
    {
        GET_THIS( oListIterStruct, pIter );
        oListElement* pCur = pIter->cursor;
		if ( pCur != NULL )
		{
			oListStruct* pList = reinterpret_cast<oListStruct *>(pIter->parent.pData);
			oListElement* pPrev = pCur->prev;
			oListElement* pNext = pCur->next;
			if ( pCur == pList->head )
			{
				pList->head = pNext;
			}
			if ( pCur == pList->tail )
			{
				pList->tail = pPrev;
			}
			if ( pNext != NULL )
			{
				pNext->prev = pPrev;
			}
			if ( pPrev != NULL )
			{
				pPrev->next = pNext;
			}
			pIter->cursor = pNext;
			SAFE_RELEASE( pCur->obj );
			FREE_LINK( pCur );
		}
        METHOD_RETURN;
	}

    FORTHOP( oListIterFindNextMethod )
    {
        GET_THIS( oListIterStruct, pIter );
		oListStruct* pList = reinterpret_cast<oListStruct *>( pIter->parent.pData );
		oListElement* pCur = pIter->cursor;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
		if ( pCur != NULL )
		{
			pCur = pCur->next;
		}
		while ( pCur != NULL )
		{
			ForthObject& o = pCur->obj;
			if ( OBJECTS_SAME( o, soughtObj ) )
			{
				break;
			}
			pCur = pCur->next;
		}
		if ( pCur == NULL )
		{
	        SPUSH( 0 );
		}
		else
		{
			pIter->cursor = pCur;
	        SPUSH( ~0 );
		}
        METHOD_RETURN;
    }

    FORTHOP( oListIterSplitMethod )
    {
        GET_THIS( oListIterStruct, pIter );

		// create an empty list
		MALLOCATE_OBJECT( oListStruct, pNewList );
        pNewList->refCount = 0;
		pNewList->head = NULL;
		pNewList->tail = NULL;

		oListElement* pCursor = pIter->cursor;
		oListStruct* pOldList = reinterpret_cast<oListStruct *>(pIter->parent.pData);
		// if pCursor is NULL, iter cursor is past tail, new list is just empty list, leave old list alone
		if ( pCursor != NULL )
		{
			if ( pCursor == pOldList->head )
			{
				// iter cursor is start of list, make old list empty, new list is entire old list
				pNewList->head = pOldList->head;
				pNewList->tail = pOldList->tail;
				pOldList->head = NULL;
				pOldList->tail = NULL;
			}
			else
			{
				// head of new list is iter cursor
				pNewList->head = pCursor;
				pNewList->tail = pOldList->tail;
				// fix old list tail
				pOldList->tail = pNewList->head->prev;
				pOldList->tail->next = NULL;
			}
		}

		// split leaves iter cursor past tail
		pIter->cursor = NULL;

		PUSH_PAIR( pIter->parent.pMethodOps, pNewList );
        METHOD_RETURN;
    }

    baseMethodEntry oListIterMembers[] =
    {
        METHOD(     "_%new%_",              oListIterNew ),
        METHOD(     "delete",               oListIterDeleteMethod ),

		METHOD(     "seekNext",             oListIterSeekNextMethod ),
        METHOD(     "seekPrev",             oListIterSeekPrevMethod ),
        METHOD(     "seekHead",             oListIterSeekHeadMethod ),
        METHOD(     "seekTail",             oListIterSeekTailMethod ),
        METHOD_RET( "next",					oListIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oListIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oListIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				oListIterRemoveMethod ),
        METHOD_RET( "findNext",				oListIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        //METHOD_RET( "clone",                oListIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIListIter) ),

		METHOD(     "swapNext",             oListIterSwapNextMethod ),
        METHOD(     "swapPrev",             oListIterSwapPrevMethod ),
        METHOD(     "split",                oListIterSplitMethod ),
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

    FORTHOP( oMapFindKeyMethod )
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

    FORTHOP( oMapFindMethod )
    {
        GET_THIS( oMapStruct, pMap );
		bool found = false;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
        oMap::iterator iter;
        oMap& a = *(pMap->elements);
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = iter->second;
			if ( OBJECTS_SAME( o, soughtObj ) )
            {
				found = true;
                break;
            }
        }
		if ( found )
		{
			pMap->refCount++;
			TRACK_KEEP;
			// needed to use new instead of malloc otherwise the iterator isn't setup right and
			//   a crash happens when you assign to it
			oMapIterStruct* pIter = new oMapIterStruct;
			TRACK_ITER_NEW;
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pMap);
			pIter->cursor = iter;
			ForthInterface* pPrimaryInterface = gpMapIterClassVocab->GetInterface( 0 );
			PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
	        SPUSH( ~0 );
		}
		else
		{
	        SPUSH( 0 );
		}
        METHOD_RETURN;
    }


    baseMethodEntry oMapMembers[] =
    {
        METHOD(     "_%new%_",              oMapNew ),
        METHOD(     "delete",               oMapDeleteMethod ),

        METHOD_RET( "headIter",             oMapHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),
        METHOD_RET( "tailIter",             oMapTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),
        METHOD_RET( "find",                 oMapFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),
        //METHOD_RET( "clone",                oMapCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMap) ),
        METHOD_RET( "count",                oMapCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oMapClearMethod ),

        METHOD_RET( "get",                  oMapGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD(     "set",                  oMapSetMethod ),
        METHOD_RET( "findKey",              oMapFindKeyMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",               oMapRemoveMethod ),
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

    FORTHOP( oMapIterRemoveMethod )
    {
        GET_THIS( oMapIterStruct, pIter );
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>( pIter->parent.pData );
		if ( pIter->cursor != pMap->elements->end() )
		{
            ForthObject& o = pIter->cursor->second;
            SAFE_RELEASE( o );
			pMap->elements->erase( pIter->cursor );
			pIter->cursor++;
		}
        METHOD_RETURN;
    }

    FORTHOP( oMapIterFindNextMethod )
    {
	    SPUSH( 0 );
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
        METHOD(     "_%new%_",              oMapIterNew ),
        METHOD(     "delete",               oMapIterDeleteMethod ),

		METHOD(     "seekNext",             oMapIterSeekNextMethod ),
        METHOD(     "seekPrev",             oMapIterSeekPrevMethod ),
        METHOD(     "seekHead",             oMapIterSeekHeadMethod ),
        METHOD(     "seekTail",             oMapIterSeekTailMethod ),
        METHOD_RET( "next",					oMapIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oMapIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oMapIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				oMapIterRemoveMethod ),
        METHOD_RET( "findNext",				oMapIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        //METHOD_RET( "clone",                oMapIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),

		METHOD_RET( "nextPair",				oMapIterNextPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prevPair",             oMapIterPrevPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
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
//#define RCSTRING_SLOP 16
#define RCSTRING_SLOP 0
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
        ulong		refCount;
		ulong		hash;
        oString*	str;
    };


    FORTHOP( oStringNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        MALLOCATE_OBJECT( oStringStruct, pString );
        pString->refCount = 0;
        pString->hash = 0;
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
			pString->str = dst;
		}
		dst->curLen = len;
		memcpy( &(dst->data[0]), srcStr, len + 1 );
		pString->hash = 0;
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
			//int dataBytes = ((((newLen * 6) >> 2)  + 4) & ~3);
			int dataBytes = ((newLen + 4) & ~3);
	        size_t nBytes = sizeof(oString) + (dataBytes - DEFAULT_STRING_DATA_BYTES);
			dst = (oString *) realloc( dst, nBytes );
			dst->maxLen = dataBytes - 1;
			pString->str = dst;
		}
		memcpy( &(dst->data[dst->curLen]), srcStr, len + 1 );
		dst->curLen = newLen;
		pString->hash = 0;
        METHOD_RETURN;
    }

    FORTHOP( oStringResizeMethod )
    {
        GET_THIS( oStringStruct, pString );
		long newLen = SPOP;
		oString* dst = pString->str;
		int dataBytes = ((newLen + 4) & ~3);
        size_t nBytes = sizeof(oString) + (dataBytes - DEFAULT_STRING_DATA_BYTES);
		dst = (oString *) realloc( dst, nBytes );
		dst->maxLen = dataBytes - 1;
		pString->str = dst;
		if ( dst->curLen > dst->maxLen )
		{
			dst->curLen = dst->maxLen;
			pString->hash = 0;
		}
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

    FORTHOP( oStringClearMethod )
    {
        GET_THIS( oStringStruct, pString );
		pString->hash = 0;
		oString* dst = pString->str;
		dst->curLen = 0;
		dst->data[0] = '\0';
        METHOD_RETURN;
    }

    FORTHOP( oStringHashMethod )
    {
        GET_THIS( oStringStruct, pString );
		pString->hash = 0;
		oString* dst = pString->str;
		if ( dst->curLen != 0 )
		{
			pString->hash = SuperFastHash( &(dst->data[0]), dst->curLen, 0 );
		}
		SPUSH( (long)(pString->hash) );
        METHOD_RETURN;
    }

    FORTHOP( oStringAppendCharMethod )
    {
        GET_THIS( oStringStruct, pString );
		char c = (char) SPOP;
		oString* dst = pString->str;
		long newLen = dst->curLen + 1;
		if ( newLen > dst->maxLen )
		{
			// enlarge string
			//int dataBytes = ((((newLen * 6) >> 2)  + 4) & ~3);
			int dataBytes = ((newLen + 4) & ~3);
	        size_t nBytes = sizeof(oString) + (dataBytes - DEFAULT_STRING_DATA_BYTES);
			dst = (oString *) realloc( dst, nBytes );
			dst->maxLen = dataBytes - 1;
			pString->str = dst;
		}
		char* pDst = &(dst->data[dst->curLen]);
		*pDst++ = c;
		*pDst = '\0';
		dst->curLen = newLen;
		pString->hash = 0;
        METHOD_RETURN;
    }


    baseMethodEntry oStringMembers[] =
    {
        METHOD(     "_%new%_",              oStringNew ),
        METHOD(     "delete",               oStringDeleteMethod ),
        METHOD(     "show",					oStringShowMethod ),
        METHOD(     "size",                 oStringSizeMethod ),
        METHOD(     "length",               oStringLengthMethod ),
        METHOD(     "get",                  oStringGetMethod ),
        METHOD(     "set",                  oStringSetMethod ),
        METHOD(     "append",               oStringAppendMethod ),
        METHOD(     "resize",               oStringResizeMethod ),
        METHOD(     "startsWith",           oStringStartsWithMethod ),
        METHOD(     "endsWith",             oStringEndsWithMethod ),
        METHOD(     "contains",             oStringContainsMethod ),
        METHOD(     "clear",                oStringClearMethod ),
        METHOD(     "hash",                 oStringHashMethod ),
        METHOD(     "appendChar",           oStringAppendCharMethod ),
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

	struct oPairIterStruct
    {
        ulong				refCount;
		ForthObject			parent;
		int					cursor;
    };
	static ForthClassVocabulary* gpPairIterClassVocab = NULL;


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

    FORTHOP( oPairHeadIterMethod )
    {
        GET_THIS( oPairStruct, pPair );
		pPair->refCount++;
		TRACK_KEEP;
		oPairIterStruct* pIter = new oPairIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pPair);
		pIter->cursor = 0;
        ForthInterface* pPrimaryInterface = gpPairIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oPairTailIterMethod )
    {
        GET_THIS( oPairStruct, pPair );
		pPair->refCount++;
		TRACK_KEEP;
		oPairIterStruct* pIter = new oPairIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pPair);
		pIter->cursor = 2;
        ForthInterface* pPrimaryInterface = gpPairIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }


    baseMethodEntry oPairMembers[] =
    {
        METHOD(     "_%new%_",              oPairNew ),
        METHOD(     "delete",               oPairDeleteMethod ),

        METHOD_RET( "headIter",             oPairHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIPairIter) ),
        METHOD_RET( "tailIter",             oPairTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIPairIter) ),
        //METHOD_RET( "find",                 oPairFindMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIPairIter) ),
        //METHOD_RET( "clone",                oPairCloneMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIPair) ),
        //METHOD_RET( "count",                oPairCountMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        //METHOD(     "clear",                oPairClearMethod ),

        METHOD(     "setA",                 oPairSetAMethod ),
        METHOD_RET( "getA",                 oPairGetAMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject) ),
        METHOD(     "setB",                 oPairSetBMethod ),
        METHOD_RET( "getB",                 oPairGetBMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject) ),
        // following must be last in table
        END_MEMBERS
    };


	//////////////////////////////////////////////////////////////////////
    ///
    //                 oPairIter
    //

    FORTHOP( oPairIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a oPairIter object" );
    }

    FORTHOP( oPairIterDeleteMethod )
    {
        GET_THIS( oPairIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		delete pIter;
		TRACK_ITER_DELETE;
        METHOD_RETURN;
    }

    FORTHOP( oPairIterSeekNextMethod )
    {
        GET_THIS( oPairIterStruct, pIter );
		if ( pIter->cursor < 2 )
		{
			pIter->cursor++;
		}
        METHOD_RETURN;
    }

    FORTHOP( oPairIterSeekPrevMethod )
    {
        GET_THIS( oPairIterStruct, pIter );
		if ( pIter->cursor > 0 )
		{
			pIter->cursor--;
		}
        METHOD_RETURN;
    }

    FORTHOP( oPairIterSeekHeadMethod )
    {
        GET_THIS( oPairIterStruct, pIter );
		pIter->cursor = 0;
        METHOD_RETURN;
    }

    FORTHOP( oPairIterSeekTailMethod )
    {
        GET_THIS( oPairIterStruct, pIter );
		pIter->cursor = 2;
        METHOD_RETURN;
    }

    FORTHOP( oPairIterNextMethod )
    {
        GET_THIS( oPairIterStruct, pIter );
		oPairStruct* pPair = reinterpret_cast<oPairStruct *>( &(pIter->parent) );
		switch ( pIter->cursor )
		{
		case 0:
			pIter->cursor++;
			PUSH_OBJECT( pPair->a );
		    SPUSH( ~0 );
			break;
		case 1:
			pIter->cursor++;
			PUSH_OBJECT( pPair->b );
		    SPUSH( ~0 );
			break;
		default:
			SPUSH( 0 );
			break;
		}
        METHOD_RETURN;
    }

    FORTHOP( oPairIterPrevMethod )
    {
        GET_THIS( oPairIterStruct, pIter );
		oPairStruct* pPair = reinterpret_cast<oPairStruct *>( &(pIter->parent) );
		switch ( pIter->cursor )
		{
		case 1:
			pIter->cursor--;
			PUSH_OBJECT( pPair->a );
		    SPUSH( ~0 );
			break;
		case 2:
			pIter->cursor--;
			PUSH_OBJECT( pPair->b );
		    SPUSH( ~0 );
			break;
		default:
			SPUSH( 0 );
			break;
		}
        METHOD_RETURN;
    }

    FORTHOP( oPairIterCurrentMethod )
    {
        GET_THIS( oPairIterStruct, pIter );
		oPairStruct* pPair = reinterpret_cast<oPairStruct *>( &(pIter->parent) );
		switch ( pIter->cursor )
		{
		case 0:
			PUSH_OBJECT( pPair->a );
		    SPUSH( ~0 );
			break;
		case 1:
			PUSH_OBJECT( pPair->b );
		    SPUSH( ~0 );
			break;
		default:
			SPUSH( 0 );
			break;
		}
        METHOD_RETURN;
    }

    baseMethodEntry oPairIterMembers[] =
    {
        METHOD(     "_%new%_",              oPairIterNew ),
        METHOD(     "delete",               oPairIterDeleteMethod ),

        METHOD(     "seekNext",             oPairIterSeekNextMethod ),
        METHOD(     "seekPrev",             oPairIterSeekPrevMethod ),
        METHOD(     "seekHead",             oPairIterSeekHeadMethod ),
        METHOD(     "seekTail",             oPairIterSeekTailMethod ),
        METHOD_RET( "next",					oPairIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oPairIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oPairIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        //METHOD(     "remove",				oPairIterRemoveMethod ),
        //METHOD_RET( "findNext",				oPairIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        //METHOD_RET( "clone",                oPairIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIPairIter) ),
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

	struct oTripleIterStruct
    {
        ulong				refCount;
		ForthObject			parent;
		int					cursor;
    };
	static ForthClassVocabulary* gpTripleIterClassVocab = NULL;


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

    FORTHOP( oTripleHeadIterMethod )
    {
        GET_THIS( oTripleStruct, pTriple );
		pTriple->refCount++;
		TRACK_KEEP;
		oTripleIterStruct* pIter = new oTripleIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pTriple);
		pIter->cursor = 0;
        ForthInterface* pPrimaryInterface = gpTripleIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oTripleTailIterMethod )
    {
        GET_THIS( oTripleStruct, pTriple );
		pTriple->refCount++;
		TRACK_KEEP;
		oTripleIterStruct* pIter = new oTripleIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pTriple);
		pIter->cursor = 3;
        ForthInterface* pPrimaryInterface = gpTripleIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    baseMethodEntry oTripleMembers[] =
    {
        METHOD(     "_%new%_",              oTripleNew ),
        METHOD(     "delete",               oTripleDeleteMethod ),

        METHOD_RET( "headIter",             oTripleHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCITripleIter) ),
        METHOD_RET( "tailIter",             oTripleTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCITripleIter) ),
        //METHOD_RET( "find",                 oTripleFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCITripleIter) ),
        //METHOD_RET( "clone",                oTripleCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCITriple) ),
        //METHOD_RET( "count",                oTripleCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        //METHOD(     "clear",                oTripleClearMethod ),

        METHOD(     "setA",                 oTripleSetAMethod ),
        METHOD_RET( "getA",                 oTripleGetAMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject) ),
        METHOD(     "setB",                 oTripleSetBMethod ),
        METHOD_RET( "getB",                 oTripleGetBMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject) ),
        METHOD(     "setC",                 oTripleSetCMethod ),
        METHOD_RET( "getC",                 oTripleGetCMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject) ),
        // following must be last in table
        END_MEMBERS
    };

    
	//////////////////////////////////////////////////////////////////////
    ///
    //                 oTripleIter
    //

    FORTHOP( oTripleIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a oTripleIter object" );
    }

    FORTHOP( oTripleIterDeleteMethod )
    {
        GET_THIS( oTripleIterStruct, pIter );
		SAFE_RELEASE( pIter->parent );
		delete pIter;
		TRACK_ITER_DELETE;
        METHOD_RETURN;
    }

    FORTHOP( oTripleIterSeekNextMethod )
    {
        GET_THIS( oTripleIterStruct, pIter );
		if ( pIter->cursor < 3 )
		{
			pIter->cursor++;
		}
        METHOD_RETURN;
    }

    FORTHOP( oTripleIterSeekPrevMethod )
    {
        GET_THIS( oTripleIterStruct, pIter );
		if ( pIter->cursor > 0 )
		{
			pIter->cursor--;
		}
        METHOD_RETURN;
    }

    FORTHOP( oTripleIterSeekHeadMethod )
    {
        GET_THIS( oTripleIterStruct, pIter );
		pIter->cursor = 0;
        METHOD_RETURN;
    }

    FORTHOP( oTripleIterSeekTailMethod )
    {
        GET_THIS( oTripleIterStruct, pIter );
		pIter->cursor = 3;
        METHOD_RETURN;
    }

    FORTHOP( oTripleIterNextMethod )
    {
        GET_THIS( oTripleIterStruct, pIter );
		oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>( &(pIter->parent) );
		switch ( pIter->cursor )
		{
		case 0:
			pIter->cursor++;
			PUSH_OBJECT( pTriple->a );
		    SPUSH( ~0 );
			break;
		case 1:
			pIter->cursor++;
			PUSH_OBJECT( pTriple->b );
		    SPUSH( ~0 );
			break;
		case 2:
			pIter->cursor++;
			PUSH_OBJECT( pTriple->c );
		    SPUSH( ~0 );
			break;
		default:
			SPUSH( 0 );
			break;
		}
        METHOD_RETURN;
    }

    FORTHOP( oTripleIterPrevMethod )
    {
        GET_THIS( oTripleIterStruct, pIter );
		oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>( &(pIter->parent) );
		switch ( pIter->cursor )
		{
		case 1:
			pIter->cursor--;
			PUSH_OBJECT( pTriple->a );
		    SPUSH( ~0 );
			break;
		case 2:
			pIter->cursor--;
			PUSH_OBJECT( pTriple->b );
		    SPUSH( ~0 );
			break;
		case 3:
			pIter->cursor--;
			PUSH_OBJECT( pTriple->c );
		    SPUSH( ~0 );
			break;
		default:
			SPUSH( 0 );
			break;
		}
        METHOD_RETURN;
    }

    FORTHOP( oTripleIterCurrentMethod )
    {
        GET_THIS( oTripleIterStruct, pIter );
		oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>( &(pIter->parent) );
		switch ( pIter->cursor )
		{
		case 0:
			PUSH_OBJECT( pTriple->a );
		    SPUSH( ~0 );
			break;
		case 1:
			PUSH_OBJECT( pTriple->b );
		    SPUSH( ~0 );
			break;
		case 2:
			PUSH_OBJECT( pTriple->c );
		    SPUSH( ~0 );
			break;
		default:
			SPUSH( 0 );
			break;
		}
        METHOD_RETURN;
    }

    baseMethodEntry oTripleIterMembers[] =
    {
        METHOD(     "_%new%_",              oTripleIterNew ),
        METHOD(     "delete",               oTripleIterDeleteMethod ),

        METHOD(     "seekNext",             oTripleIterSeekNextMethod ),
        METHOD(     "seekPrev",             oTripleIterSeekPrevMethod ),
        METHOD(     "seekHead",             oTripleIterSeekHeadMethod ),
        METHOD(     "seekTail",             oTripleIterSeekTailMethod ),
        METHOD_RET( "next",					oTripleIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oTripleIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oTripleIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        //METHOD(     "remove",				oTripleIterRemoveMethod ),
        //METHOD_RET( "findNext",				oTripleIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        //METHOD_RET( "clone",                oTripleIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCITripleIter) ),
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

    FORTHOP( oByteArrayRefMethod )
    {
		GET_THIS( oByteArrayStruct, pArray );
        oByteArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
	        SPUSH( (long) &(a[ix]) );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
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

    FORTHOP( oByteArrayFindMethod )
    {
		GET_THIS( oByteArrayStruct, pArray );
        long retVal = -1;
		char soughtByte = (char)(SPOP);
        oByteArray::iterator iter;
        oByteArray& a = *(pArray->elements);
        for ( unsigned int i = 0; i < a.size(); i++ )
        {
			if ( soughtByte == a[i] )
			{
                retVal = i;
                break;
            }
        }
		if ( retVal < 0 )
		{
	        SPUSH( 0 );
		}
		else
		{
			pArray->refCount++;
			TRACK_KEEP;
			MALLOCATE_ITER( oByteArrayIterStruct, pIter );
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pArray);
			pIter->cursor = retVal;
			ForthInterface* pPrimaryInterface = gpByteArraryIterClassVocab->GetInterface( 0 );
			PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
	        SPUSH( ~0 );
		}
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

    FORTHOP( oByteArrayBaseMethod )
    {
		GET_THIS( oByteArrayStruct, pArray );
        oByteArray& a = *(pArray->elements);
        SPUSH( (long) &(a[0]) );
        METHOD_RETURN;
	}

    baseMethodEntry oByteArrayMembers[] =
    {
        METHOD(     "_%new%_",              oByteArrayNew ),
        METHOD(     "delete",               oByteArrayDeleteMethod ),

        METHOD_RET( "headIter",             oByteArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArrayIter) ),
        METHOD_RET( "tailIter",             oByteArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArrayIter) ),
        METHOD_RET( "find",                 oByteArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArrayIter) ),
        METHOD_RET( "clone",                oByteArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArray) ),
        METHOD_RET( "count",                oByteArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
		METHOD(     "clear",                oByteArrayClearMethod ),

        METHOD(     "resize",               oByteArrayResizeMethod ),
        METHOD_RET( "ref",                  oByteArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte|kDTIsPtr) ),
        METHOD_RET( "get",                  oByteArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte) ),
        METHOD(     "set",                  oByteArraySetMethod ),
        METHOD_RET( "findIndex",            oByteArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "push",                 oByteArrayPushMethod ),
        METHOD_RET( "pop",                  oByteArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte) ),
        METHOD_RET( "base",                 oByteArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte|kDTIsPtr) ),
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

    FORTHOP( oByteArrayIterRemoveMethod )
    {
        GET_THIS( oByteArrayIterStruct, pIter );
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>( pIter->parent.pData );
        oByteArray& a = *(pArray->elements);
		if ( pIter->cursor < a.size() )
		{
			pArray->elements->erase( pArray->elements->begin() + pIter->cursor );
		}
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayIterFindNextMethod )
    {
        GET_THIS( oByteArrayIterStruct, pIter );
        long retVal = 0;
		char soughtByte = (char)(SPOP);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>( pIter->parent.pData );
        oByteArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while ( i < a.size() )
		{
			if ( a[i] == soughtByte )
            {
                retVal = ~0;
				pIter->cursor = i;
                break;
            }
		}
		SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( oByteArrayIterCloneMethod )
    {
        GET_THIS( oByteArrayIterStruct, pIter );
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>( pIter->parent.pData );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oByteArrayIterStruct, pNewIter );
		pNewIter->refCount = 0;
		pNewIter->parent.pMethodOps = pIter->parent.pMethodOps ;
		pNewIter->parent.pData = reinterpret_cast<long *>(pArray);
		pNewIter->cursor = pIter->cursor;
        ForthInterface* pPrimaryInterface = gpByteArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( GET_TPM, pNewIter );
        METHOD_RETURN;
    }

    baseMethodEntry oByteArrayIterMembers[] =
    {
        METHOD(     "_%new%_",              oByteArrayIterNew ),
        METHOD(     "delete",               oByteArrayIterDeleteMethod ),

        METHOD(     "seekNext",             oByteArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             oByteArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             oByteArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             oByteArrayIterSeekTailMethod ),
        METHOD_RET( "next",					oByteArrayIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oByteArrayIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oByteArrayIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				oByteArrayIterRemoveMethod ),
        METHOD_RET( "findNext",				oByteArrayIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "clone",                oByteArrayIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArrayIter) ),
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

    FORTHOP( oShortArrayRefMethod )
    {
		GET_THIS( oShortArrayStruct, pArray );
        oShortArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
	        SPUSH( (long) &(a[ix]) );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
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

    FORTHOP( oShortArrayFindMethod )
    {
		GET_THIS( oShortArrayStruct, pArray );
        long retVal = -1;
		short soughtShort = (short)(SPOP);
        oShortArray::iterator iter;
        oShortArray& a = *(pArray->elements);
        for ( unsigned int i = 0; i < a.size(); i++ )
        {
			if ( soughtShort == a[i] )
			{
                retVal = i;
                break;
            }
        }
		if ( retVal < 0 )
		{
	        SPUSH( 0 );
		}
		else
		{
			pArray->refCount++;
			TRACK_KEEP;
			MALLOCATE_ITER( oShortArrayIterStruct, pIter );
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pArray);
			pIter->cursor = retVal;
			ForthInterface* pPrimaryInterface = gpShortArraryIterClassVocab->GetInterface( 0 );
			PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
	        SPUSH( ~0 );
		}
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
    
    FORTHOP( oShortArrayBaseMethod )
    {
		GET_THIS( oShortArrayStruct, pArray );
        oShortArray& a = *(pArray->elements);
        SPUSH( (long) &(a[0]) );
        METHOD_RETURN;
	}

    baseMethodEntry oShortArrayMembers[] =
    {
        METHOD(     "_%new%_",              oShortArrayNew ),
        METHOD(     "delete",               oShortArrayDeleteMethod ),

        METHOD_RET( "headIter",             oShortArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArrayIter) ),
        METHOD_RET( "tailIter",             oShortArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArrayIter) ),
        METHOD_RET( "find",                 oShortArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArrayIter) ),
        METHOD_RET( "clone",                oShortArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArray) ),
        METHOD_RET( "count",                oShortArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oShortArrayClearMethod ),

        METHOD(     "resize",               oShortArrayResizeMethod ),
        METHOD_RET( "ref",                  oShortArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort|kDTIsPtr) ),
        METHOD_RET( "get",                  oShortArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort) ),
        METHOD(     "set",                  oShortArraySetMethod ),
        METHOD_RET( "findIndex",            oShortArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "push",                 oShortArrayPushMethod ),
        METHOD_RET( "pop",                  oShortArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort) ),
        METHOD_RET( "base",                 oShortArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort|kDTIsPtr) ),
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

    FORTHOP( oShortArrayIterRemoveMethod )
    {
        GET_THIS( oShortArrayIterStruct, pIter );
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>( pIter->parent.pData );
        oShortArray& a = *(pArray->elements);
		if ( pIter->cursor < a.size() )
		{
			pArray->elements->erase( pArray->elements->begin() + pIter->cursor );
		}
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayIterFindNextMethod )
    {
        GET_THIS( oShortArrayIterStruct, pIter );
        long retVal = 0;
		char soughtShort = (char)(SPOP);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>( pIter->parent.pData );
        oShortArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while ( i < a.size() )
		{
			if ( a[i] == soughtShort )
            {
                retVal = ~0;
				pIter->cursor = i;
                break;
            }
		}
		SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( oShortArrayIterCloneMethod )
    {
        GET_THIS( oShortArrayIterStruct, pIter );
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>( pIter->parent.pData );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oShortArrayIterStruct, pNewIter );
		pNewIter->refCount = 0;
		pNewIter->parent.pMethodOps = pIter->parent.pMethodOps ;
		pNewIter->parent.pData = reinterpret_cast<long *>(pArray);
		pNewIter->cursor = pIter->cursor;
        ForthInterface* pPrimaryInterface = gpShortArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( GET_TPM, pNewIter );
        METHOD_RETURN;
    }

    baseMethodEntry oShortArrayIterMembers[] =
    {
        METHOD(     "_%new%_",              oShortArrayIterNew ),
        METHOD(     "delete",               oShortArrayIterDeleteMethod ),
        METHOD(     "seekNext",             oShortArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             oShortArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             oShortArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             oShortArrayIterSeekTailMethod ),
        METHOD_RET( "next",					oShortArrayIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oShortArrayIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oShortArrayIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				oShortArrayIterRemoveMethod ),
        METHOD_RET( "findNext",				oShortArrayIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "clone",                oShortArrayIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArrayIter) ),
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

    FORTHOP( oIntArrayRefMethod )
    {
		GET_THIS( oIntArrayStruct, pArray );
        oIntArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
	        SPUSH( (long) &(a[ix]) );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
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

    FORTHOP( oIntArrayFindMethod )
    {
		GET_THIS( oIntArrayStruct, pArray );
        long retVal = -1;
		int soughtInt = SPOP;
        oIntArray::iterator iter;
        oIntArray& a = *(pArray->elements);
        for ( unsigned int i = 0; i < a.size(); i++ )
        {
			if ( soughtInt == a[i] )
			{
                retVal = i;
                break;
            }
        }
		if ( retVal < 0 )
		{
	        SPUSH( 0 );
		}
		else
		{
			pArray->refCount++;
			TRACK_KEEP;
			MALLOCATE_ITER( oIntArrayIterStruct, pIter );
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pArray);
			pIter->cursor = retVal;
			ForthInterface* pPrimaryInterface = gpIntArraryIterClassVocab->GetInterface( 0 );
			PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
	        SPUSH( ~0 );
		}
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

    FORTHOP( oIntArrayBaseMethod )
    {
		GET_THIS( oIntArrayStruct, pArray );
        oIntArray& a = *(pArray->elements);
        SPUSH( (long) &(a[0]) );
        METHOD_RETURN;
	}
    
    baseMethodEntry oIntArrayMembers[] =
    {
        METHOD(     "_%new%_",              oIntArrayNew ),
        METHOD(     "delete",               oIntArrayDeleteMethod ),

        METHOD_RET( "headIter",             oIntArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArrayIter) ),
        METHOD_RET( "tailIter",             oIntArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArrayIter) ),
        METHOD_RET( "find",                 oIntArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArrayIter) ),
        METHOD_RET( "clone",                oIntArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArray) ),
        METHOD_RET( "count",                oIntArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oIntArrayClearMethod ),

        METHOD(     "resize",               oIntArrayResizeMethod ),
        METHOD_RET( "ref",                  oIntArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt|kDTIsPtr) ),
        METHOD_RET( "get",                  oIntArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "set",                  oIntArraySetMethod ),
        METHOD_RET( "findIndex",            oIntArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "push",                 oIntArrayPushMethod ),
        METHOD_RET( "pop",                  oIntArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "base",                 oIntArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt|kDTIsPtr) ),
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

    FORTHOP( oIntArrayIterRemoveMethod )
    {
        GET_THIS( oIntArrayIterStruct, pIter );
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>( pIter->parent.pData );
        oIntArray& a = *(pArray->elements);
		if ( pIter->cursor < a.size() )
		{
			pArray->elements->erase( pArray->elements->begin() + pIter->cursor );
		}
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayIterFindNextMethod )
    {
        GET_THIS( oIntArrayIterStruct, pIter );
        long retVal = 0;
		char soughtInt = (char)(SPOP);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>( pIter->parent.pData );
        oIntArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while ( i < a.size() )
		{
			if ( a[i] == soughtInt )
            {
                retVal = ~0;
				pIter->cursor = i;
                break;
            }
		}
		SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( oIntArrayIterCloneMethod )
    {
        GET_THIS( oIntArrayIterStruct, pIter );
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>( pIter->parent.pData );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oIntArrayIterStruct, pNewIter );
		pNewIter->refCount = 0;
		pNewIter->parent.pMethodOps = pIter->parent.pMethodOps ;
		pNewIter->parent.pData = reinterpret_cast<long *>(pArray);
		pNewIter->cursor = pIter->cursor;
        ForthInterface* pPrimaryInterface = gpIntArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( GET_TPM, pNewIter );
        METHOD_RETURN;
    }

    baseMethodEntry oIntArrayIterMembers[] =
    {
        METHOD(     "_%new%_",              oIntArrayIterNew ),
        METHOD(     "delete",               oIntArrayIterDeleteMethod ),
        METHOD(     "seekNext",             oIntArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             oIntArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             oIntArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             oIntArrayIterSeekTailMethod ),
        METHOD_RET( "next",					oIntArrayIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oIntArrayIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oIntArrayIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				oIntArrayIterRemoveMethod ),
        METHOD_RET( "findNext",				oIntArrayIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "clone",                oIntArrayIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArrayIter) ),
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

    FORTHOP( oLongArrayRefMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
        oLongArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
	        SPUSH( (long) &(a[ix]) );
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
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

    FORTHOP( oLongArrayFindMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
        long retVal = -1;
		long long soughtLong = LPOP;
        oLongArray::iterator iter;
        oLongArray& a = *(pArray->elements);
        for ( unsigned int i = 0; i < a.size(); i++ )
        {
			if ( soughtLong == a[i] )
			{
                retVal = i;
                break;
            }
        }
		if ( retVal < 0 )
		{
	        SPUSH( 0 );
		}
		else
		{
			pArray->refCount++;
			TRACK_KEEP;
			MALLOCATE_ITER( oLongArrayIterStruct, pIter );
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pArray);
			pIter->cursor = retVal;
			ForthInterface* pPrimaryInterface = gpLongArraryIterClassVocab->GetInterface( 0 );
			PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
	        SPUSH( ~0 );
		}
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

	FORTHOP( oLongArrayBaseMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
        oLongArray& a = *(pArray->elements);
        SPUSH( (long) &(a[0]) );
        METHOD_RETURN;
	}
    
    baseMethodEntry oLongArrayMembers[] =
    {
        METHOD(     "_%new%_",              oLongArrayNew ),
        METHOD(     "delete",               oLongArrayDeleteMethod ),

        METHOD_RET( "headIter",             oLongArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArrayIter) ),
        METHOD_RET( "tailIter",             oLongArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArrayIter) ),
        METHOD_RET( "find",                 oLongArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArrayIter) ),
        METHOD_RET( "clone",                oLongArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArray) ),
        METHOD_RET( "count",                oLongArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oLongArrayClearMethod ),

        METHOD(     "resize",               oLongArrayResizeMethod ),
        METHOD_RET( "ref",                  oLongArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong|kDTIsPtr) ),
        METHOD_RET( "get",                  oLongArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong) ),
        METHOD(     "set",                  oLongArraySetMethod ),
        METHOD_RET( "findIndex",            oLongArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "push",                 oLongArrayPushMethod ),
        METHOD_RET( "pop",                  oLongArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong) ),
        METHOD_RET( "base",                 oLongArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong|kDTIsPtr) ),
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

    FORTHOP( oLongArrayIterRemoveMethod )
    {
        GET_THIS( oLongArrayIterStruct, pIter );
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>( pIter->parent.pData );
        oLongArray& a = *(pArray->elements);
		if ( pIter->cursor < a.size() )
		{
			pArray->elements->erase( pArray->elements->begin() + pIter->cursor );
		}
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayIterFindNextMethod )
    {
        GET_THIS( oLongArrayIterStruct, pIter );
        long retVal = 0;
		long long soughtLong = LPOP;
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>( pIter->parent.pData );
        oLongArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while ( i < a.size() )
		{
			if ( a[i] == soughtLong )
            {
                retVal = ~0;
				pIter->cursor = i;
                break;
            }
		}
		SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayIterCloneMethod )
    {
        GET_THIS( oLongArrayIterStruct, pIter );
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>( pIter->parent.pData );
		pArray->refCount++;
		TRACK_KEEP;
		MALLOCATE_ITER( oLongArrayIterStruct, pNewIter );
		pNewIter->refCount = 0;
		pNewIter->parent.pMethodOps = pIter->parent.pMethodOps ;
		pNewIter->parent.pData = reinterpret_cast<long *>(pArray);
		pNewIter->cursor = pIter->cursor;
        ForthInterface* pPrimaryInterface = gpLongArraryIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( GET_TPM, pNewIter );
        METHOD_RETURN;
    }

    baseMethodEntry oLongArrayIterMembers[] =
    {
        METHOD(     "_%new%_",              oLongArrayIterNew ),
        METHOD(     "delete",               oLongArrayIterDeleteMethod ),

        METHOD(     "seekNext",             oLongArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             oLongArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             oLongArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             oLongArrayIterSeekTailMethod ),
        METHOD_RET( "next",					oLongArrayIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oLongArrayIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oLongArrayIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				oLongArrayIterRemoveMethod ),
        METHOD_RET( "findNext",				oLongArrayIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "clone",                oLongArrayIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArrayIter) ),
        // following must be last in table
        END_MEMBERS
    };

    //////////////////////////////////////////////////////////////////////
    ///
    //                 oThread
    //

    typedef std::vector<ForthObject> oArray;
    struct oThreadStruct
    {
        ulong			refCount;
        ForthThread		*pThread;
    };

	// TBD: add tracking of run state of thread - this should be done inside ForthThread, not here
    FORTHOP( oThreadNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oThreadStruct, pThreadStruct );
        pThreadStruct->refCount = 0;
		pThreadStruct->pThread = NULL;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pThreadStruct );
    }

    FORTHOP( oThreadDeleteMethod )
    {
		GET_THIS( oThreadStruct, pThreadStruct );
		if ( pThreadStruct->pThread != NULL )
		{
			GET_ENGINE->DestroyThread( pThreadStruct->pThread );
			pThreadStruct->pThread = NULL;
		}
        FREE_OBJECT( pThreadStruct );
        METHOD_RETURN;
    }

	FORTHOP( oThreadInitMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		ForthEngine* pEngine = GET_ENGINE;
		if ( pThreadStruct->pThread != NULL )
		{
			pEngine->DestroyThread( pThreadStruct->pThread );
		}
		long threadOp  = SPOP;
		int returnStackSize = (int)(SPOP);
		int paramStackSize = (int)(SPOP);
		pThreadStruct->pThread = GET_ENGINE->CreateThread( threadOp, paramStackSize, returnStackSize );
		pThreadStruct->pThread->Reset();
        METHOD_RETURN;
	}

	FORTHOP( oThreadStartMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		long result = pThreadStruct->pThread->Start();
		SPUSH( result );
        METHOD_RETURN;
	}

	FORTHOP( oThreadExitMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		pThreadStruct->pThread->Exit();
        METHOD_RETURN;
	}

	FORTHOP( oThreadPushMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		pThreadStruct->pThread->Push( SPOP );
        METHOD_RETURN;
	}

	FORTHOP( oThreadPopMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		long val = pThreadStruct->pThread->Pop();
		SPUSH( val );
        METHOD_RETURN;
	}

	FORTHOP( oThreadRPushMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		pThreadStruct->pThread->RPush( SPOP );
        METHOD_RETURN;
	}

	FORTHOP( oThreadRPopMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		long val = pThreadStruct->pThread->RPop();
		SPUSH( val );
        METHOD_RETURN;
	}

	FORTHOP( oThreadGetStateMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		SPUSH( (long) (pThreadStruct->pThread->GetCoreState()) );
        METHOD_RETURN;
	}

	FORTHOP( oThreadStepMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		ForthThread* pThread = pThreadStruct->pThread;
		ForthCoreState* pThreadCore = pThread->GetCoreState();
		long op = *(pThreadCore->IP)++;
		long result;
		ForthEngine *pEngine = GET_ENGINE;
		if ( pEngine->GetFastMode() )
		{
			result = (long) InterpretOneOpFast( pThreadCore, op );
		}
		else
		{
			result = (long) InterpretOneOp( pThreadCore, op );
		}
		SPUSH( result );
        METHOD_RETURN;
	}

	FORTHOP( oThreadResetMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		pThreadStruct->pThread->Reset();
        METHOD_RETURN;
	}

    baseMethodEntry oThreadMembers[] =
    {
        METHOD(     "_%new%_",              oThreadNew ),
        METHOD(     "delete",               oThreadDeleteMethod ),
        METHOD(     "init",                 oThreadInitMethod ),
        METHOD_RET( "start",                oThreadStartMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "exit",                 oThreadExitMethod ),
        METHOD(     "push",                 oThreadPushMethod ),
        METHOD_RET( "pop",                  oThreadPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "rpush",                oThreadRPushMethod ),
        METHOD_RET( "rpop",                 oThreadRPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "getState",             oThreadGetStateMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "step",                 oThreadStepMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "reset",                oThreadResetMethod ),
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
	ForthClassVocabulary* pOIterableClass = pEngine->AddBuiltinClass( "oIterable", pObjectClass, oIterableMembers );

    ForthClassVocabulary* pOArrayClass = pEngine->AddBuiltinClass( "oArray", pOIterableClass, oArrayMembers );
    gpArraryIterClassVocab = pEngine->AddBuiltinClass( "oArrayIter", pOIterClass, oArrayIterMembers );

    ForthClassVocabulary* pOListClass = pEngine->AddBuiltinClass( "oList", pOIterableClass, oListMembers );
    gpListIterClassVocab = pEngine->AddBuiltinClass( "oListIter", pOIterClass, oListIterMembers );

    ForthClassVocabulary* pOMapClass = pEngine->AddBuiltinClass( "oMap", pOIterableClass, oMapMembers );
    gpMapIterClassVocab = pEngine->AddBuiltinClass( "oMapIter", pOIterClass, oMapIterMembers );

    ForthClassVocabulary* pOStringClass = pEngine->AddBuiltinClass( "oString", pObjectClass, oStringMembers );

    ForthClassVocabulary* pOPairClass = pEngine->AddBuiltinClass( "oPair", pOIterableClass, oPairMembers );
    gpPairIterClassVocab = pEngine->AddBuiltinClass( "oPairIter", pOIterClass, oPairIterMembers );

	ForthClassVocabulary* pOTripleClass = pEngine->AddBuiltinClass( "oTriple", pOIterableClass, oTripleMembers );
    gpTripleIterClassVocab = pEngine->AddBuiltinClass( "oTripleIter", pOIterClass, oTripleIterMembers );

    ForthClassVocabulary* pOByteArrayClass = pEngine->AddBuiltinClass( "oByteArray", pOIterableClass, oByteArrayMembers );
    gpByteArraryIterClassVocab = pEngine->AddBuiltinClass( "oByteArrayIter", pOIterClass, oByteArrayIterMembers );

    ForthClassVocabulary* pOShortArrayClass = pEngine->AddBuiltinClass( "oShortArray", pOIterableClass, oShortArrayMembers );
    gpShortArraryIterClassVocab = pEngine->AddBuiltinClass( "oShortArrayIter", pOIterClass, oShortArrayIterMembers );

    ForthClassVocabulary* pOIntArrayClass = pEngine->AddBuiltinClass( "oIntArray", pOIterableClass, oIntArrayMembers );
    gpIntArraryIterClassVocab = pEngine->AddBuiltinClass( "oIntArrayIter", pOIterClass, oIntArrayIterMembers );

    ForthClassVocabulary* pOLongArrayClass = pEngine->AddBuiltinClass( "oLongArray", pOIterableClass, oLongArrayMembers );
    gpLongArraryIterClassVocab = pEngine->AddBuiltinClass( "oLongArrayIter", pOIterClass, oLongArrayIterMembers );

    ForthClassVocabulary* pOThreadClass = pEngine->AddBuiltinClass( "oThread", pObjectClass, oThreadMembers );

    mpClassMethods = pClassClass->GetInterface( 0 )->GetMethods();
}

