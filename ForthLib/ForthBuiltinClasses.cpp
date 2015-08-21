//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.cpp: builtin classes
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <map>

#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthObject.h"
#include "ForthBuiltinClasses.h"
#include "ForthShowContext.h"

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
	extern void unimplementedMethodOp( ForthCoreState *pCore );
	extern void illegalMethodOp( ForthCoreState *pCore );
};


void* ForthAllocateBlock(size_t numBytes)
{
	void* pData = malloc(numBytes);
	ForthEngine::GetInstance()->TraceOut("malloc %d bytes @ %p\n", numBytes, pData);
	return pData;
}

void ForthFreeBlock(void* pBlock)
{
	free(pBlock);
	ForthEngine::GetInstance()->TraceOut("free @ %p\n", pBlock);
}

namespace
{

#define SHOW_HEADER(_TYPENAME)  pShowContext->ShowHeader(pCore, _TYPENAME, GET_TPD)


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

	FORTHOP(objectShowMethod)
	{
		ForthObject obj;
		obj.pData = GET_TPD;
		obj.pMethodOps = GET_TPM;
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		ForthClassObject* pClassObject = (ForthClassObject *)(*((GET_TPM)-1));
		ForthEngine *pEngine = ForthEngine::GetInstance();

		if (pShowContext->AddObject(obj))
		{
			pEngine->ConsoleOut("'@");
			pShowContext->ShowID(pClassObject->pVocab->GetName(), obj.pData);
			pEngine->ConsoleOut("'");
		}
		else
		{
			pClassObject->pVocab->ShowData(GET_TPD, pCore);
			if (pShowContext->GetDepth() == 0)
			{
				pEngine->ConsoleOut("\n");
			}
		}
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
        METHOD(     "__newOp",          objectNew ),
        METHOD(     "delete",           objectDeleteMethod ),
        METHOD(     "show",             objectShowMethod ),
        METHOD_RET( "getClass",         objectClassMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIClass) ),
        METHOD_RET( "compare",          objectCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "keep",				objectKeepMethod ),
        METHOD(     "release",			objectReleaseMethod ),
        MEMBER_VAR( "__refCount",       NATIVE_TYPE_TO_CODE(0, kBaseTypeInt) ),
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
        METHOD_RET( "unref",				unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
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

	struct oArrayIterStruct
    {
        ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
    };
	static ForthClassVocabulary* gpArrayClassVocab = NULL;
	static ForthClassVocabulary* gpListClassVocab = NULL;
	static ForthClassVocabulary* gpArrayIterClassVocab = NULL;

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
            SAFE_RELEASE( pCore, o );
        }
        delete pArray->elements;
        FREE_OBJECT( pArray );
        METHOD_RETURN;
    }

	FORTHOP(oArrayShowMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_HEADER("oArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
				if (iter != a.begin())
				{
					pShowContext->EndElement(",");
				}
				ForthObject& o = *iter;
				pShowContext->ShowIndent();
				ForthShowObject(o, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
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
                    SAFE_RELEASE( pCore, a[i] );
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

    FORTHOP( oArrayLoadMethod )
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
                    SAFE_RELEASE( pCore, a[i] );
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
		if ( newSize > 0 )
		{
			for ( int i = newSize - 1; i >= 0; i-- )
			{
				ForthObject& oldObj = a[i];
				ForthObject newObj;
				POP_OBJECT( newObj );
				if ( OBJECTS_DIFFERENT( oldObj, newObj ) )
				{
					SAFE_KEEP( newObj );
					SAFE_RELEASE( pCore, oldObj );
				}
				a[i] = newObj;
			}
		}
        METHOD_RETURN;
    }

	FORTHOP(oArrayToListMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();

		MALLOCATE_OBJECT(oListStruct, pList);
		pList->refCount = 0;
		pList->head = NULL;
		pList->tail = NULL;
		oListElement* oldTail = NULL;

		// bump reference counts of all valid elements in this array
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = *iter;
			SAFE_KEEP(o);
			MALLOCATE_LINK(oListElement, newElem);
			newElem->obj = o;
			if (oldTail == NULL)
			{
				ASSERT(pList->head == NULL);
				pList->head = newElem;
			}
			else
			{
				ASSERT(oldTail->next == NULL);
				oldTail->next = newElem;
			}
			newElem->prev = oldTail;
			newElem->next = NULL;
			pList->tail = newElem;
			oldTail = newElem;
		}

		// push list on TOS
		ForthInterface* pPrimaryInterface = gpListClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pList);
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
            SAFE_RELEASE( pCore, o );
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

	inline void unrefObject( ForthObject& fobj )
	{
        ForthEngine *pEngine = ForthEngine::GetInstance();
		if ( fobj.pData != NULL )
		{
			ulong refCount = *((ulong *) fobj.pData);
			if ( refCount == 0 )
			{
				pEngine->SetError( kForthErrorBadReferenceCount, " unref with refcount already zero" );
			}
			else
			{
				*fobj.pData -= 1;
			}
		}
	}

    FORTHOP( oArrayUnrefMethod )
    {
		GET_THIS( oArrayStruct, pArray );
        oArray& a = *(pArray->elements);
        ulong ix = (ulong) SPOP;
        if ( a.size() > ix )
        {
            ForthObject fobj = a[ix];
			unrefObject( fobj );
            PUSH_OBJECT( fobj );
			a[ix].pData = NULL;
			a[ix].pMethodOps = NULL;
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
            OBJECT_ASSIGN( pCore, oldObj, newObj );
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

    FORTHOP( oArrayPopUnrefMethod )
    {
		GET_THIS( oArrayStruct, pArray );
        oArray& a = *(pArray->elements);
        if ( a.size() > 0 )
        {
			ForthObject fobj = a.back();
			a.pop_back();
			unrefObject( fobj );
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
        ForthInterface* pPrimaryInterface = gpArrayIterClassVocab->GetInterface( 0 );
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
        ForthInterface* pPrimaryInterface = gpArrayIterClassVocab->GetInterface( 0 );
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
			ForthInterface* pPrimaryInterface = gpArrayIterClassVocab->GetInterface( 0 );
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

    FORTHOP( oArrayInsertMethod )
    {
		GET_THIS( oArrayStruct, pArray );

		oArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
        ulong ix = (ulong) SPOP;
		ulong oldSize = a.size();
		if (oldSize >= ix)
        {
            ForthObject newObj;
            POP_OBJECT( newObj );
			// add dummy object to end of array
			a.resize(oldSize + 1);
			if ((oldSize > 0) && (ix < oldSize))
			{
				// move old entries up by size of ForthObject
				memmove(&(a[ix + 1]), &(a[ix]), sizeof(ForthObject) * (oldSize - ix));
			}
			a[ix] = newObj;
			SAFE_KEEP(newObj);
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    baseMethodEntry oArrayMembers[] =
    {
        METHOD(     "__newOp",              oArrayNew ),
        METHOD(     "delete",               oArrayDeleteMethod ),
        METHOD(     "show",                 oArrayShowMethod ),

		METHOD_RET( "headIter",             oArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter) ),
        METHOD_RET( "tailIter",             oArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter) ),
        METHOD_RET( "find",                 oArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter) ),
        METHOD_RET( "clone",                oArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArray) ),
        METHOD_RET( "count",                oArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oArrayClearMethod ),
        METHOD(     "resize",               oArrayResizeMethod ),
        METHOD(     "load",                 oArrayLoadMethod ),
        METHOD_RET( "toList",               oArrayToListMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIList) ),
        METHOD_RET( "ref",                  oArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject | kDTIsPtr) ),
        METHOD_RET( "unref",                oArrayUnrefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD_RET( "get",                  oArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD(     "set",                  oArraySetMethod ),
        METHOD_RET( "findIndex",            oArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "push",                 oArrayPushMethod ),
        METHOD_RET( "popUnref",             oArrayPopUnrefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD(     "insert",               oArrayInsertMethod ),
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
		SAFE_RELEASE( pCore, pIter->parent );
		FREE_ITER( pIter );
        METHOD_RETURN;
    }

	FORTHOP(oArrayIterShowMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		oArray& a = *(pArray->elements);
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_HEADER("oArrayIter");
		pShowContext->ShowIndent("'cursor : ");
		ForthShowObject(a[pIter->cursor], pCore);
		pShowContext->EndElement(",");
		pShowContext->ShowIndent("'parent' : '@");
		long* pParentData = pIter->parent.pData;
		long* pParentMethods = pIter->parent.pMethodOps;
		ForthClassObject* pClassObject = (ForthClassObject *)(*((pParentMethods)-1));
		pShowContext->ShowID(pClassObject->pVocab->GetName(), pParentData);
		pShowContext->EndElement("'");
		pShowContext->EndIndent();
		pEngine->ConsoleOut("}");
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterSeekNextMethod)
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
			// TODO!
			ForthObject o = a[ pIter->cursor ];
            SAFE_RELEASE( pCore, o );
			pArray->elements->erase( pArray->elements->begin() + pIter->cursor );
		}
        METHOD_RETURN;
    }

    FORTHOP( oArrayIterUnrefMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>( pIter->parent.pData );
        oArray& a = *(pArray->elements);
		ForthObject fobj;
		fobj.pData = NULL;
		fobj.pMethodOps = NULL;
		if ( pIter->cursor < a.size() )
		{
            ForthObject fobj = a[ pIter->cursor ];
			unrefObject( fobj );
			a[ pIter->cursor ].pData = NULL;
			a[ pIter->cursor ].pMethodOps = NULL;
		}
        PUSH_OBJECT( fobj );
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

    FORTHOP( oArrayIterInsertMethod )
    {
        GET_THIS( oArrayIterStruct, pIter );

		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent.pData);
		oArray& a = *(pArray->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
        ulong ix = pIter->cursor;
		ulong oldSize = a.size();
		if (oldSize >= ix)
        {
            ForthObject newObj;
            POP_OBJECT( newObj );
			// add dummy object to end of array
			a.resize(oldSize + 1);
			if ((oldSize > 0) && (ix < oldSize))
			{
				// move old entries up by size of ForthObject
				memmove(&(a[ix + 1]), &(a[ix]), sizeof(ForthObject) * (oldSize - ix));
			}
			a[ix] = newObj;
			SAFE_KEEP(newObj);
        }
        else
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError( kForthErrorBadParameter, " array index out of range" );
        }
        METHOD_RETURN;
    }

    baseMethodEntry oArrayIterMembers[] =
    {
        METHOD(     "__newOp",              oArrayIterNew ),
        METHOD(     "delete",               oArrayIterDeleteMethod ),
        METHOD(     "show",                 oArrayIterShowMethod ),

        METHOD(     "seekNext",             oArrayIterSeekNextMethod ),
        METHOD(     "seekPrev",             oArrayIterSeekPrevMethod ),
        METHOD(     "seekHead",             oArrayIterSeekHeadMethod ),
        METHOD(     "seekTail",             oArrayIterSeekTailMethod ),
        METHOD_RET( "next",					oArrayIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oArrayIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oArrayIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				oArrayIterRemoveMethod ),
        METHOD_RET( "unref",				oArrayIterUnrefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD_RET( "findNext",				oArrayIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "clone",                oArrayIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArrayIter) ),
        METHOD(     "insert",               oArrayIterInsertMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oList
    //


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
			SAFE_RELEASE( pCore, pCur->obj );
			FREE_LINK( pCur );
			pCur = pNext;
		}
        FREE_OBJECT( pList );
        METHOD_RETURN;
    }

	FORTHOP(oListShowMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_HEADER("oList");
		pShowContext->ShowIndent("'elements' : [");
		if (pCur != NULL)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			while (pCur != NULL)
			{
				oListElement* pNext = pCur->next;
				if (pCur != pList->head)
				{
					pShowContext->EndElement(",");
				}
				pShowContext->ShowIndent();
				ForthShowObject(pCur->obj, pCore);
				pCur = pNext;
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
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
			SAFE_RELEASE( pCore, obj );
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
			SAFE_RELEASE( pCore, obj );
			FREE_LINK( oldTail );
		}
        METHOD_RETURN;
    }

    FORTHOP( oListUnrefHeadMethod )
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
			unrefObject( obj );
			PUSH_OBJECT( obj );
			FREE_LINK( oldHead );
		}
        METHOD_RETURN;
    }

    FORTHOP( oListUnrefTailMethod )
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
			unrefObject( obj );
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
			SAFE_RELEASE( pCore, pCur->obj );
			FREE_LINK( pCur );
			pCur = pNext;
		}
		pList->head = NULL;
		pList->tail = NULL;
        METHOD_RETURN;
    }

    FORTHOP( oListLoadMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( oListStruct, pList );
        oListElement* pCur = pList->head;
		while ( pCur != NULL )
		{
			oListElement* pNext = pCur->next;
			SAFE_RELEASE( pCore, pCur->obj );
			FREE_LINK( pCur );
			pCur = pNext;
		}
		pList->head = NULL;
		pList->tail = NULL;

		int n = SPOP;
		for ( int i = 0; i < n; i++ )
		{
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
		}
        METHOD_RETURN;
    }

	FORTHOP(oListToArrayMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		MALLOCATE_OBJECT(oArrayStruct, pArray);
		pArray->refCount = 0;
		pArray->elements = new oArray;

		while (pCur != NULL)
		{
			ForthObject& o = pCur->obj;
			SAFE_KEEP(o);
			pArray->elements->push_back(o);
			pCur = pCur->next;
		}

		// push array on TOS
		ForthInterface* pPrimaryInterface = gpArrayClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pArray);
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

	FORTHOP(oListRemoveMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		while (pCur != NULL)
		{
			ForthObject& o = pCur->obj;
			if (OBJECTS_SAME(o, soughtObj))
			{
				break;
			}
			pCur = pCur->next;
		}
		if (pCur != NULL)
		{
			oListElement* pPrev = pCur->prev;
			oListElement* pNext = pCur->next;
			if (pCur == pList->head)
			{
				pList->head = pNext;
			}
			if (pCur == pList->tail)
			{
				pList->tail = pPrev;
			}
			if (pNext != NULL)
			{
				pNext->prev = pPrev;
			}
			if (pPrev != NULL)
			{
				pPrev->next = pNext;
			}
			SAFE_RELEASE(pCore, pCur->obj);
			FREE_LINK(pCur);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oListMembers[] =
    {
        METHOD(     "__newOp",              oListNew ),
        METHOD(     "delete",               oListDeleteMethod ),
        METHOD(     "show",                 oListShowMethod ),

        METHOD_RET( "headIter",             oListHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIListIter) ),
        METHOD_RET( "tailIter",             oListTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIListIter) ),
        METHOD_RET( "find",                 oListFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIListIter) ),
        METHOD_RET( "clone",                oListCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIList) ),
        METHOD_RET( "count",                oListCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oListClearMethod ),
        METHOD(     "load",                 oListLoadMethod ),
        METHOD_RET( "toArray",              oListToArrayMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIArray) ),

		METHOD_RET( "head",                 oListHeadMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD_RET( "tail",                 oListTailMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD(     "addHead",              oListAddHeadMethod ),
        METHOD(     "addTail",              oListAddTailMethod ),
        METHOD(     "removeHead",           oListRemoveHeadMethod ),
        METHOD(     "removeTail",           oListRemoveTailMethod ),
        METHOD(     "unrefHead",            oListUnrefHeadMethod ),
        METHOD(     "unrefTail",            oListUnrefTailMethod ),
		METHOD(     "remove",               oListRemoveMethod),
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
		SAFE_RELEASE( pCore, pIter->parent );
		FREE_ITER( pIter );
        METHOD_RETURN;
    }

	FORTHOP(oListIterShowMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_HEADER("oListIter");
		pShowContext->ShowIndent("'cursor : ");
		ForthShowObject(pIter->cursor->obj, pCore);
		pShowContext->EndElement(",");
		pShowContext->ShowIndent("'parent' : '@");
		long* pParentData = pIter->parent.pData;
		long* pParentMethods = pIter->parent.pMethodOps;
		ForthClassObject* pClassObject = (ForthClassObject *)(*((pParentMethods)-1));
		pShowContext->ShowID(pClassObject->pVocab->GetName(), pParentData);
		pShowContext->EndElement("'");
		pShowContext->EndIndent();
		pEngine->ConsoleOut("}");
		METHOD_RETURN;
	}

	FORTHOP(oListIterSeekNextMethod)
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
			SAFE_RELEASE( pCore, pCur->obj );
			FREE_LINK( pCur );
		}
        METHOD_RETURN;
	}

    FORTHOP( oListIterUnrefMethod )
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
			PUSH_OBJECT( pCur->obj );
			unrefObject( pCur->obj );
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
        METHOD(     "__newOp",              oListIterNew ),
        METHOD(     "delete",               oListIterDeleteMethod ),

		METHOD(     "seekNext",             oListIterSeekNextMethod ),
        METHOD(     "seekPrev",             oListIterSeekPrevMethod ),
        METHOD(     "seekHead",             oListIterSeekHeadMethod ),
        METHOD(     "seekTail",             oListIterSeekTailMethod ),
        METHOD_RET( "next",					oListIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oListIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oListIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				oListIterRemoveMethod ),
        METHOD_RET( "unref",				oListIterUnrefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
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

	typedef std::map<long long, ForthObject> oMap;
	struct oMapStruct
	{
		ulong       refCount;
		oMap*	elements;
	};

	struct oMapIterStruct
	{
		ulong				refCount;
		ForthObject			parent;
		oMap::iterator	cursor;
	};
	static ForthClassVocabulary* gpMapIterClassVocab = NULL;



	FORTHOP(oMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oMapStruct, pMap);
		pMap->refCount = 0;
		pMap->elements = new oMap;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pMap);
	}

	FORTHOP(oMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			stackInt64 key;
			key.s64 = iter->first;
			SAFE_RELEASE(pCore, key.obj);
			ForthObject& val = iter->second;
			SAFE_RELEASE(pCore, val);
		}
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oMapShowMethod)
	{
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_HEADER("oMap");
		pShowContext->ShowIndent("'map' : {");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
				stackInt64 key;
				key.s64 = iter->first;
				if (iter != a.begin())
				{
					pShowContext->EndElement(",");
				}
				pShowContext->ShowIndent();
				ForthShowObject(key.obj, pCore);
				pEngine->ConsoleOut(" : ");
				ForthShowObject(iter->second, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			stackInt64 key;
			key.s64 = iter->first;
			SAFE_RELEASE(pCore, key.obj);
			ForthObject& val = iter->second;
			SAFE_RELEASE(pCore, val);
		}
		a.clear();
		METHOD_RETURN;
	}



	FORTHOP(oMapLoadMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			stackInt64 key;
			key.s64 = iter->first;
			SAFE_RELEASE(pCore, key.obj);
			ForthObject& val = iter->second;
			SAFE_RELEASE(pCore, val);
		}
		a.clear();
		int n = SPOP;
		for (int i = 0; i < n; i++)
		{
			stackInt64 key;
			LPOP(key);
			ForthObject newObj;
			POP_OBJECT(newObj);
			if (newObj.pMethodOps != NULL)
			{
				SAFE_KEEP(newObj);
			}
			a[key.s64] = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapCountMethod)
	{
		GET_THIS(oMapStruct, pMap);
		SPUSH((long)(pMap->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oMapGetMethod)
	{
		GET_THIS(oMapStruct, pMap);
		oMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			ForthObject fobj = iter->second;
			PUSH_OBJECT(fobj);
		}
		else
		{
			PUSH_PAIR(0, 0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapSetMethod)
	{
		GET_THIS(oMapStruct, pMap);
		oMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		ForthObject newObj;
		POP_OBJECT(newObj);
		oMap::iterator iter = a.find(key.s64);
		if (newObj.pMethodOps != NULL)
		{
			if (iter != a.end())
			{
				ForthObject oldObj = iter->second;
				if (OBJECTS_DIFFERENT(oldObj, newObj))
				{
					SAFE_KEEP(newObj);
					SAFE_RELEASE(pCore, oldObj);
				}
			}
			else
			{
				SAFE_KEEP(newObj);
			}
			a[key.s64] = newObj;
		}
		else
		{
			// remove element associated with key from map
			if (iter != a.end())
			{
				stackInt64 key;
				key.s64 = iter->first;
				SAFE_RELEASE(pCore, key.obj);
				ForthObject& val = iter->second;
				SAFE_RELEASE(pCore, val);
				a.erase(iter);
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapFindKeyMethod)
	{
		GET_THIS(oMapStruct, pMap);
		stackInt64 retVal;
		retVal.s64 = 0L;
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = 1;
				retVal.s64 = iter->first;
				break;
			}
		}
		LPUSH(retVal);
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oMapRemoveMethod)
	{
		GET_THIS(oMapStruct, pMap);
		oMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			stackInt64 key;
			key.s64 = iter->first;
			SAFE_RELEASE(pCore, key.obj);
			ForthObject& val = iter->second;
			SAFE_RELEASE(pCore, val);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oMapStruct, pMap);
		oMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			stackInt64 key;
			key.s64 = iter->first;
			unrefObject(key.obj);
			ForthObject& val = iter->second;
			unrefObject(val);
			PUSH_OBJECT(val);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapHeadIterMethod)
	{
		GET_THIS(oMapStruct, pMap);
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
		ForthInterface* pPrimaryInterface = gpMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oMapTailIterMethod)
	{
		GET_THIS(oMapStruct, pMap);
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
		ForthInterface* pPrimaryInterface = gpMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oMapFindMethod)
	{
		GET_THIS(oMapStruct, pMap);
		bool found = false;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = true;
				break;
			}
		}
		if (found)
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
			ForthInterface* pPrimaryInterface = gpMapIterClassVocab->GetInterface(0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		else
		{
			SPUSH(0);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oMapMembers[] =
	{
		METHOD("__newOp", oMapNew),
		METHOD("delete", oMapDeleteMethod),
		METHOD("show", oMapShowMethod),

		METHOD_RET("headIter", oMapHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("tailIter", oMapTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("find", oMapFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		//METHOD_RET( "clone",                oMapCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMap) ),
		METHOD_RET("count", oMapCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oMapClearMethod),
		METHOD("load", oMapLoadMethod),

		METHOD_RET("get", oMapGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject)),
		METHOD("set", oMapSetMethod),
		METHOD_RET("findKey", oMapFindKeyMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oMapRemoveMethod),
		METHOD("unref", oMapUnrefMethod),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oMapIter
	//

	FORTHOP(oMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oMapIter object");
	}

	FORTHOP(oMapIterDeleteMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekNextMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		pIter->cursor++;
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekPrevMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		pIter->cursor--;
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekHeadMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		pIter->cursor = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekTailMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		pIter->cursor = pMap->elements->end();
		METHOD_RETURN;
	}

	FORTHOP(oMapIterNextMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterPrevMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterCurrentMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterRemoveMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if (pIter->cursor != pMap->elements->end())
		{
			stackInt64 key;
			key.s64 = pIter->cursor->first;
			SAFE_RELEASE(pCore, key.obj);
			ForthObject& o = pIter->cursor->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase(pIter->cursor);
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterFindNextMethod)
	{
		SPUSH(0);
		METHOD_RETURN;
	}

	FORTHOP(oMapIterNextPairMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			stackInt64 key;
			key.s64 = pIter->cursor->first;
			LPUSH(key);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterPrevPairMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			stackInt64 key;
			key.s64 = pIter->cursor->first;
			LPUSH(key);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oMapIterMembers[] =
	{
		METHOD("__newOp", oMapIterNew),
		METHOD("delete", oMapIterDeleteMethod),

		METHOD("seekNext", oMapIterSeekNextMethod),
		METHOD("seekPrev", oMapIterSeekPrevMethod),
		METHOD("seekHead", oMapIterSeekHeadMethod),
		METHOD("seekTail", oMapIterSeekTailMethod),
		METHOD_RET("next", oMapIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oMapIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oMapIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oMapIterRemoveMethod),
		METHOD_RET("findNext", oMapIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		//METHOD_RET( "clone",                oMapIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),

		METHOD_RET("nextPair", oMapIterNextPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prevPair", oMapIterPrevPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
    ///
    //                 oIntMap
    //

    typedef std::map<long, ForthObject> oIntMap;
    struct oIntMapStruct
    {
        ulong       refCount;
        oIntMap*		elements;
    };

    struct oIntMapIterStruct
    {
        ulong				refCount;
		ForthObject			parent;
		oIntMap::iterator		cursor;
    };
	static ForthClassVocabulary* gpIntMapIterClassVocab = NULL;



    FORTHOP( oIntMapNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oIntMapStruct, pMap );
        pMap->refCount = 0;
        pMap->elements = new oIntMap;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pMap );
    }

    FORTHOP( oIntMapDeleteMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( oIntMapStruct, pMap );
        oIntMap::iterator iter;
        oIntMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE( pCore, o );
        }
        delete pMap->elements;
        FREE_OBJECT( pMap );
        METHOD_RETURN;
    }

	FORTHOP(oIntMapShowMethod)
	{
		char buffer[20];
		GET_THIS(oIntMapStruct, pMap);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_HEADER("oIntMap");
		pShowContext->ShowIndent("'map' : {");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
				if (iter != a.begin())
				{
					pShowContext->EndElement(",");
				}
				sprintf(buffer, "%d : ", iter->first);
				pShowContext->ShowIndent(buffer);
				ForthShowObject(iter->second, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

    FORTHOP( oIntMapClearMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( oIntMapStruct, pMap );
        oIntMap::iterator iter;
        oIntMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE( pCore, o );
        }
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP( oIntMapLoadMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( oIntMapStruct, pMap );
        oIntMap::iterator iter;
        oIntMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for ( iter = a.begin(); iter != a.end(); ++iter )
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE( pCore, o );
        }
        a.clear();
		int n = SPOP;
		for ( int i = 0; i < n; i++ )
		{
			long key = SPOP;
			ForthObject newObj;
			POP_OBJECT( newObj );
			if ( newObj.pMethodOps != NULL )
			{
				SAFE_KEEP( newObj );
			}
			a[key] = newObj;
		}
        METHOD_RETURN;
    }

    FORTHOP( oIntMapCountMethod )
    {
        GET_THIS( oIntMapStruct, pMap );
        SPUSH( (long) (pMap->elements->size()) );
        METHOD_RETURN;
    }

    FORTHOP( oIntMapGetMethod )
    {
        GET_THIS( oIntMapStruct, pMap );
        oIntMap& a = *(pMap->elements);
        long key = SPOP;
        oIntMap::iterator iter = a.find( key );
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

    FORTHOP( oIntMapSetMethod )
    {
        GET_THIS( oIntMapStruct, pMap );
        oIntMap& a = *(pMap->elements);
        long key = SPOP;
        ForthObject newObj;
        POP_OBJECT( newObj );
        oIntMap::iterator iter = a.find( key );
		if ( newObj.pMethodOps != NULL )
		{
			if ( iter != a.end() )
			{
				ForthObject oldObj = iter->second;
				if ( OBJECTS_DIFFERENT( oldObj, newObj ) )
				{
					SAFE_KEEP( newObj );
					SAFE_RELEASE( pCore, oldObj );
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
                SAFE_RELEASE( pCore, oldObj );
				a.erase( iter );
			}
		}
        METHOD_RETURN;
    }

    FORTHOP( oIntMapFindKeyMethod )
    {
        GET_THIS( oIntMapStruct, pMap );
        long retVal = -1;
		long found = 0;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
        oIntMap::iterator iter;
        oIntMap& a = *(pMap->elements);
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

    FORTHOP( oIntMapRemoveMethod )
    {
        GET_THIS( oIntMapStruct, pMap );
        oIntMap& a = *(pMap->elements);
        long key = SPOP;
        oIntMap::iterator iter = a.find( key );
		if ( iter != a.end() )
		{
			ForthObject& oldObj = iter->second;
            SAFE_RELEASE( pCore, oldObj );
			a.erase( iter );
		}
        METHOD_RETURN;
    }

    FORTHOP( oIntMapUnrefMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        GET_THIS( oIntMapStruct, pMap );
        oIntMap& a = *(pMap->elements);
        long key = SPOP;
        oIntMap::iterator iter = a.find( key );
		if ( iter != a.end() )
		{
			ForthObject& fobj = iter->second;
			unrefObject( fobj );
            PUSH_OBJECT( fobj );
			a.erase( iter );
		}
        METHOD_RETURN;
    }

    FORTHOP( oIntMapHeadIterMethod )
    {
        GET_THIS( oIntMapStruct, pMap );
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oIntMapIterStruct* pIter = new oIntMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
        oIntMap::iterator iter = pMap->elements->begin();
		pIter->cursor = iter;
		//pIter->cursor = pMap->elements->begin();
        ForthInterface* pPrimaryInterface = gpIntMapIterClassVocab->GetInterface( 0 );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oIntMapTailIterMethod )
    {
        GET_THIS( oIntMapStruct, pMap );
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oIntMapIterStruct* pIter = new oIntMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = pMap->elements->end();
		ForthInterface* pPrimaryInterface = gpIntMapIterClassVocab->GetInterface(0);
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
        METHOD_RETURN;
    }

    FORTHOP( oIntMapFindMethod )
    {
        GET_THIS( oIntMapStruct, pMap );
		bool found = false;
        ForthObject soughtObj;
        POP_OBJECT( soughtObj );
        oIntMap::iterator iter;
        oIntMap& a = *(pMap->elements);
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
			oIntMapIterStruct* pIter = new oIntMapIterStruct;
			TRACK_ITER_NEW;
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pMap);
			pIter->cursor = iter;
			ForthInterface* pPrimaryInterface = gpIntMapIterClassVocab->GetInterface(0);
			PUSH_PAIR( pPrimaryInterface->GetMethods(), pIter );
	        SPUSH( ~0 );
		}
		else
		{
	        SPUSH( 0 );
		}
        METHOD_RETURN;
    }


    baseMethodEntry oIntMapMembers[] =
    {
        METHOD(     "__newOp",              oIntMapNew ),
        METHOD(     "delete",               oIntMapDeleteMethod ),
        METHOD(     "show",                 oIntMapShowMethod ),

        METHOD_RET( "headIter",             oIntMapHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),
        METHOD_RET( "tailIter",             oIntMapTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),
        METHOD_RET( "find",                 oIntMapFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),
        //METHOD_RET( "clone",                oIntMapCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMap) ),
        METHOD_RET( "count",                oIntMapCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oIntMapClearMethod ),
        METHOD(     "load",                 oIntMapLoadMethod ),

        METHOD_RET( "get",                  oIntMapGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD(     "set",                  oIntMapSetMethod ),
        METHOD_RET( "findKey",              oIntMapFindKeyMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",               oIntMapRemoveMethod ),
        METHOD(     "unref",                oIntMapUnrefMethod ),
        // following must be last in table
        END_MEMBERS
    };


	//////////////////////////////////////////////////////////////////////
    ///
    //                 oIntMapIter
    //

    FORTHOP( oIntMapIterNew )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
        pEngine->SetError( kForthErrorException, " cannot explicitly create a oIntMapIter object" );
    }

    FORTHOP( oIntMapIterDeleteMethod )
    {
        GET_THIS( oIntMapIterStruct, pIter );
		SAFE_RELEASE( pCore, pIter->parent );
		delete pIter;
		TRACK_ITER_DELETE;
        METHOD_RETURN;
    }

    FORTHOP( oIntMapIterSeekNextMethod )
    {
        GET_THIS( oIntMapIterStruct, pIter );
		pIter->cursor++;
        METHOD_RETURN;
    }

    FORTHOP( oIntMapIterSeekPrevMethod )
    {
        GET_THIS( oIntMapIterStruct, pIter );
		pIter->cursor--;
        METHOD_RETURN;
    }

    FORTHOP( oIntMapIterSeekHeadMethod )
    {
        GET_THIS( oIntMapIterStruct, pIter );
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>( pIter->parent.pData );
		pIter->cursor = pMap->elements->begin();
        METHOD_RETURN;
    }

    FORTHOP( oIntMapIterSeekTailMethod )
    {
        GET_THIS( oIntMapIterStruct, pIter );
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>( pIter->parent.pData );
		pIter->cursor = pMap->elements->end();
        METHOD_RETURN;
    }

    FORTHOP( oIntMapIterNextMethod )
    {
        GET_THIS( oIntMapIterStruct, pIter );
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>( pIter->parent.pData );
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

    FORTHOP( oIntMapIterPrevMethod )
    {
        GET_THIS( oIntMapIterStruct, pIter );
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>( pIter->parent.pData );
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

    FORTHOP( oIntMapIterCurrentMethod )
    {
        GET_THIS( oIntMapIterStruct, pIter );
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>( pIter->parent.pData );
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

    FORTHOP( oIntMapIterRemoveMethod )
    {
        GET_THIS( oIntMapIterStruct, pIter );
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>( pIter->parent.pData );
		if ( pIter->cursor != pMap->elements->end() )
		{
            ForthObject& o = pIter->cursor->second;
            SAFE_RELEASE( pCore, o );
			pMap->elements->erase( pIter->cursor );
			pIter->cursor++;
		}
        METHOD_RETURN;
    }

    FORTHOP( oIntMapIterNextPairMethod )
    {
        GET_THIS( oIntMapIterStruct, pIter );
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>( pIter->parent.pData );
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

    FORTHOP( oIntMapIterPrevPairMethod )
    {
        GET_THIS( oIntMapIterStruct, pIter );
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>( pIter->parent.pData );
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

    baseMethodEntry oIntMapIterMembers[] =
    {
        METHOD(     "__newOp",              oIntMapIterNew ),
        METHOD(     "delete",               oIntMapIterDeleteMethod ),

		METHOD(     "seekNext",             oIntMapIterSeekNextMethod ),
        METHOD(     "seekPrev",             oIntMapIterSeekPrevMethod ),
        METHOD(     "seekHead",             oIntMapIterSeekHeadMethod ),
        METHOD(     "seekTail",             oIntMapIterSeekTailMethod ),
        METHOD_RET( "next",					oIntMapIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 oIntMapIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				oIntMapIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				oIntMapIterRemoveMethod ),
        METHOD_RET( "findNext",				oMapIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        //METHOD_RET( "clone",                oIntMapIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),

		METHOD_RET( "nextPair",				oIntMapIterNextPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prevPair",             oIntMapIterPrevPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        // following must be last in table
        END_MEMBERS
    };

	//////////////////////////////////////////////////////////////////////
	///
	//                 oFloatMap
	//
	static ForthClassVocabulary* gpFloatMapIterClassVocab = NULL;

	FORTHOP(oFloatMapShowMethod)
	{
		char buffer[20];
		GET_THIS(oIntMapStruct, pMap);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_HEADER("oFloatMap");
		pShowContext->ShowIndent("'map' : {");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
				if (iter != a.begin())
				{
					pShowContext->EndElement(",");
				}
				float fval = *((float *)(&(iter->first)));
				sprintf(buffer, "%f : ", fval);
				pShowContext->ShowIndent(buffer);
				ForthShowObject(iter->second, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}


    baseMethodEntry oFloatMapMembers[] =
    {
        METHOD(     "__newOp",              oIntMapNew ),
        METHOD(     "show",                 oFloatMapShowMethod ),
        // following must be last in table
        END_MEMBERS
    };


	//////////////////////////////////////////////////////////////////////
	///
	//                 oLongMap
	//

	typedef std::map<long long, ForthObject> oLongMap;
	struct oLongMapStruct
	{
		ulong       refCount;
		oLongMap*	elements;
	};

	struct oLongMapIterStruct
	{
		ulong				refCount;
		ForthObject			parent;
		oLongMap::iterator	cursor;
	};
	static ForthClassVocabulary* gpLongMapIterClassVocab = NULL;



	FORTHOP(oLongMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oLongMapStruct, pMap);
		pMap->refCount = 0;
		pMap->elements = new oLongMap;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pMap);
	}

	FORTHOP(oLongMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oLongMapShowMethod)
	{
		char buffer[20];
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_HEADER("oLongMap");
		pShowContext->ShowIndent("'map' : {");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
				if (iter != a.begin())
				{
					pShowContext->EndElement(",");
				}
				sprintf(buffer, "%lld : ", iter->first);
				pShowContext->ShowIndent(buffer);
				ForthShowObject(iter->second, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	FORTHOP(oLongMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oLongMapLoadMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		int n = SPOP;
		for (int i = 0; i < n; i++)
		{
			stackInt64 key;
			LPOP(key);
			ForthObject newObj;
			POP_OBJECT(newObj);
			if (newObj.pMethodOps != NULL)
			{
				SAFE_KEEP(newObj);
			}
			a[key.s64] = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapCountMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		SPUSH((long)(pMap->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oLongMapGetMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		oLongMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oLongMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			ForthObject fobj = iter->second;
			PUSH_OBJECT(fobj);
		}
		else
		{
			PUSH_PAIR(0, 0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapSetMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		oLongMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		ForthObject newObj;
		POP_OBJECT(newObj);
		oLongMap::iterator iter = a.find(key.s64);
		if (newObj.pMethodOps != NULL)
		{
			if (iter != a.end())
			{
				ForthObject oldObj = iter->second;
				if (OBJECTS_DIFFERENT(oldObj, newObj))
				{
					SAFE_KEEP(newObj);
					SAFE_RELEASE(pCore, oldObj);
				}
			}
			else
			{
				SAFE_KEEP(newObj);
			}
			a[key.s64] = newObj;
		}
		else
		{
			// remove element associated with key from map
			if (iter != a.end())
			{
				ForthObject& oldObj = iter->second;
				SAFE_RELEASE(pCore, oldObj);
				a.erase(iter);
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapFindKeyMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		stackInt64 retVal;
		retVal.s64 = -1L;
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = 1;
				retVal.s64 = iter->first;
				break;
			}
		}
		LPUSH(retVal);
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oLongMapRemoveMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		oLongMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oLongMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			ForthObject& oldObj = iter->second;
			SAFE_RELEASE(pCore, oldObj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oLongMapStruct, pMap);
		oLongMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
		oLongMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			ForthObject& fobj = iter->second;
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapHeadIterMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oLongMapIterStruct* pIter = new oLongMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		oLongMap::iterator iter = pMap->elements->begin();
		pIter->cursor = iter;
		//pIter->cursor = pMap->elements->begin();
		ForthInterface* pPrimaryInterface = gpLongMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oLongMapTailIterMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oLongMapIterStruct* pIter = new oLongMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = pMap->elements->end();
		ForthInterface* pPrimaryInterface = gpLongMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oLongMapFindMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		bool found = false;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = true;
				break;
			}
		}
		if (found)
		{
			pMap->refCount++;
			TRACK_KEEP;
			// needed to use new instead of malloc otherwise the iterator isn't setup right and
			//   a crash happens when you assign to it
			oLongMapIterStruct* pIter = new oLongMapIterStruct;
			TRACK_ITER_NEW;
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pMap);
			pIter->cursor = iter;
			ForthInterface* pPrimaryInterface = gpLongMapIterClassVocab->GetInterface(0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		else
		{
			SPUSH(0);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oLongMapMembers[] =
	{
		METHOD("__newOp", oLongMapNew),
		METHOD("delete", oLongMapDeleteMethod),
		METHOD("show", oLongMapShowMethod),

		METHOD_RET("headIter", oLongMapHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("tailIter", oLongMapTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("find", oLongMapFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		//METHOD_RET( "clone",                oLongMapCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMap) ),
		METHOD_RET("count", oLongMapCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oLongMapClearMethod),
		METHOD("load", oLongMapLoadMethod),

		METHOD_RET("get", oLongMapGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject)),
		METHOD("set", oLongMapSetMethod),
		METHOD_RET("findKey", oLongMapFindKeyMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oLongMapRemoveMethod),
		METHOD("unref", oLongMapUnrefMethod),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oLongMapIter
	//

	FORTHOP(oLongMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oLongMapIter object");
	}

	FORTHOP(oLongMapIterDeleteMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekNextMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		pIter->cursor++;
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekPrevMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		pIter->cursor--;
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekHeadMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		pIter->cursor = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekTailMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		pIter->cursor = pMap->elements->end();
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterNextMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterPrevMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterCurrentMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterRemoveMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if (pIter->cursor != pMap->elements->end())
		{
			ForthObject& o = pIter->cursor->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase(pIter->cursor);
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterNextPairMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			stackInt64 key;
			key.s64 = pIter->cursor->first;
			LPUSH(key);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterPrevPairMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			stackInt64 key;
			key.s64 = pIter->cursor->first;
			LPUSH(key);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oLongMapIterMembers[] =
	{
		METHOD("__newOp", oLongMapIterNew),
		METHOD("delete", oLongMapIterDeleteMethod),

		METHOD("seekNext", oLongMapIterSeekNextMethod),
		METHOD("seekPrev", oLongMapIterSeekPrevMethod),
		METHOD("seekHead", oLongMapIterSeekHeadMethod),
		METHOD("seekTail", oLongMapIterSeekTailMethod),
		METHOD_RET("next", oLongMapIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oLongMapIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oLongMapIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oLongMapIterRemoveMethod),
		METHOD_RET("findNext", oMapIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		//METHOD_RET( "clone",                oLongMapIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),

		METHOD_RET("nextPair", oLongMapIterNextPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prevPair", oLongMapIterPrevPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
    ///
    //                 oDoubleMap
    //
	static ForthClassVocabulary* gpDoubleMapIterClassVocab = NULL;

	FORTHOP(oDoubleMapShowMethod)
	{
		char buffer[32];
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_HEADER("oDoubleMap");
		pShowContext->ShowIndent("'map' : {");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
				if (iter != a.begin())
				{
					pShowContext->EndElement(",");
				}
				double dval = *((double *)(&(iter->first)));
				sprintf(buffer, "%g : ", dval);
				pShowContext->ShowIndent(buffer);
				ForthShowObject(iter->second, pCore);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}


	baseMethodEntry oDoubleMapMembers[] =
	{
		METHOD("__newOp", oLongMapNew),
		METHOD("show", oDoubleMapShowMethod),
		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 oStringMap
	//

	typedef std::map<std::string, ForthObject> oStringMap;
	struct oStringMapStruct
	{
		ulong       refCount;
		oStringMap*	elements;
	};

	struct oStringMapIterStruct
	{
		ulong				refCount;
		ForthObject			parent;
		oStringMap::iterator	cursor;
	};
	static ForthClassVocabulary* gpStringMapIterClassVocab = NULL;



	FORTHOP(oStringMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oStringMapStruct, pMap);
		pMap->refCount = 0;
		pMap->elements = new oStringMap;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pMap);
	}

	FORTHOP(oStringMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringMapStruct, pMap);
		oStringMap::iterator iter;
		oStringMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oStringMapShowMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		oStringMap::iterator iter;
		oStringMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_HEADER("oStringMap");
		pShowContext->ShowIndent("'map' : {");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
				if (iter != a.begin())
				{
					pShowContext->EndElement(",");
				}
				pShowContext->ShowIndent("'");
				pEngine->ConsoleOut(iter->first.c_str());
				pEngine->ConsoleOut("' : ");
				ForthShowObject(iter->second, pCore);
			}
			pShowContext->EndIndent();
			pShowContext->EndElement();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("}");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}


	FORTHOP(oStringMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringMapStruct, pMap);
		oStringMap::iterator iter;
		oStringMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oStringMapLoadMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringMapStruct, pMap);
		oStringMap::iterator iter;
		oStringMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		int n = SPOP;
		for (int i = 0; i < n; i++)
		{
			std::string key;
			key = (const char*)(SPOP);
			ForthObject newObj;
			POP_OBJECT(newObj);
			if (newObj.pMethodOps != NULL)
			{
				SAFE_KEEP(newObj);
			}
			a[key] = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapCountMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		SPUSH((long)(pMap->elements->size()));
		METHOD_RETURN;
	}

	FORTHOP(oStringMapGetMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		oStringMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject fobj = iter->second;
			PUSH_OBJECT(fobj);
		}
		else
		{
			PUSH_PAIR(0, 0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapSetMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		oStringMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		ForthObject newObj;
		POP_OBJECT(newObj);
		oStringMap::iterator iter = a.find(key);
		if (newObj.pMethodOps != NULL)
		{
			if (iter != a.end())
			{
				ForthObject oldObj = iter->second;
				if (OBJECTS_DIFFERENT(oldObj, newObj))
				{
					SAFE_KEEP(newObj);
					SAFE_RELEASE(pCore, oldObj);
				}
			}
			else
			{
				SAFE_KEEP(newObj);
			}
			a[key] = newObj;
		}
		else
		{
			// remove element associated with key from map
			if (iter != a.end())
			{
				ForthObject& oldObj = iter->second;
				SAFE_RELEASE(pCore, oldObj);
				a.erase(iter);
			}
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapFindKeyMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		const char* retVal = NULL;
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oStringMap::iterator iter;
		oStringMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = 1;
				retVal = iter->first.c_str();
				break;
			}
		}
		SPUSH(((long) retVal));
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oStringMapRemoveMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		oStringMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& oldObj = iter->second;
			SAFE_RELEASE(pCore, oldObj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oStringMapStruct, pMap);
		oStringMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& fobj = iter->second;
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapHeadIterMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oStringMapIterStruct* pIter = new oStringMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		oStringMap::iterator iter = pMap->elements->begin();
		pIter->cursor = iter;
		//pIter->cursor = pMap->elements->begin();
		ForthInterface* pPrimaryInterface = gpStringMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oStringMapTailIterMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		pMap->refCount++;
		TRACK_KEEP;
		// needed to use new instead of malloc otherwise the iterator isn't setup right and
		//   a crash happens when you assign to it
		oStringMapIterStruct* pIter = new oStringMapIterStruct;
		TRACK_ITER_NEW;
		pIter->refCount = 0;
		pIter->parent.pMethodOps = GET_TPM;
		pIter->parent.pData = reinterpret_cast<long *>(pMap);
		pIter->cursor = pMap->elements->end();
		ForthInterface* pPrimaryInterface = gpStringMapIterClassVocab->GetInterface(0);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
		METHOD_RETURN;
	}

	FORTHOP(oStringMapFindMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		bool found = false;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oStringMap::iterator iter;
		oStringMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = true;
				break;
			}
		}
		if (found)
		{
			pMap->refCount++;
			TRACK_KEEP;
			// needed to use new instead of malloc otherwise the iterator isn't setup right and
			//   a crash happens when you assign to it
			oStringMapIterStruct* pIter = new oStringMapIterStruct;
			TRACK_ITER_NEW;
			pIter->refCount = 0;
			pIter->parent.pMethodOps = GET_TPM;
			pIter->parent.pData = reinterpret_cast<long *>(pMap);
			pIter->cursor = iter;
			ForthInterface* pPrimaryInterface = gpStringMapIterClassVocab->GetInterface(0);
			PUSH_PAIR(pPrimaryInterface->GetMethods(), pIter);
			SPUSH(~0);
		}
		else
		{
			SPUSH(0);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oStringMapMembers[] =
	{
		METHOD("__newOp", oStringMapNew),
		METHOD("delete", oStringMapDeleteMethod),
		METHOD("show", oStringMapShowMethod),

		METHOD_RET("headIter", oStringMapHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("tailIter", oStringMapTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		METHOD_RET("find", oStringMapFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter)),
		//METHOD_RET( "clone",                oStringMapCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMap) ),
		METHOD_RET("count", oStringMapCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("clear", oStringMapClearMethod),
		METHOD("load", oStringMapLoadMethod),

		METHOD_RET("get", oStringMapGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject)),
		METHOD("set", oStringMapSetMethod),
		METHOD_RET("findKey", oStringMapFindKeyMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oStringMapRemoveMethod),
		METHOD("unref", oStringMapUnrefMethod),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oStringMapIter
	//

	FORTHOP(oStringMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorException, " cannot explicitly create a oStringMapIter object");
	}

	FORTHOP(oStringMapIterDeleteMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterSeekNextMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		pIter->cursor++;
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterSeekPrevMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		pIter->cursor--;
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterSeekHeadMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent.pData);
		pIter->cursor = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterSeekTailMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent.pData);
		pIter->cursor = pMap->elements->end();
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterNextMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterPrevMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterCurrentMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterRemoveMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent.pData);
		if (pIter->cursor != pMap->elements->end())
		{
			ForthObject& o = pIter->cursor->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase(pIter->cursor);
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterNextPairMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			SPUSH((long)(pIter->cursor->first.c_str()));
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterPrevPairMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent.pData);
		if (pIter->cursor == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			ForthObject& o = pIter->cursor->second;
			PUSH_OBJECT(o);
			SPUSH((long)(pIter->cursor->first.c_str()));
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oStringMapIterMembers[] =
	{
		METHOD("__newOp", oStringMapIterNew),
		METHOD("delete", oStringMapIterDeleteMethod),

		METHOD("seekNext", oStringMapIterSeekNextMethod),
		METHOD("seekPrev", oStringMapIterSeekPrevMethod),
		METHOD("seekHead", oStringMapIterSeekHeadMethod),
		METHOD("seekTail", oStringMapIterSeekTailMethod),
		METHOD_RET("next", oStringMapIterNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prev", oStringMapIterPrevMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("current", oStringMapIterCurrentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("remove", oStringMapIterRemoveMethod),
		METHOD_RET("findNext", oMapIterFindNextMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		//METHOD_RET( "clone",                oStringMapIterCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIMapIter) ),

		METHOD_RET("nextPair", oStringMapIterNextPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("prevPair", oStringMapIterPrevPairMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
    ///
    //                 oString
    //

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
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		SHOW_HEADER("oString");
		pShowContext->ShowIndent();
		pEngine->ConsoleOut("'value' : '");
		GET_THIS(oStringStruct, pString);
		pEngine->ConsoleOut(&(pString->str->data[0]));
		pShowContext->EndElement("'");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
    }

    FORTHOP( oStringCompareMethod )
    {
        GET_THIS( oStringStruct, pString );
        ForthObject compObj;
        POP_OBJECT( compObj );
		oStringStruct* pComp = (oStringStruct *) compObj.pData;
		int retVal = strcmp( &(pString->str->data[0]), &(pComp->str->data[0]) );
		SPUSH( retVal );
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

    void oStringAppendBytes( oStringStruct* pString, const char* pSrc, int numNewBytes )
    {
		long len = (long) strlen( pSrc );
		oString* dst = pString->str;
		long newLen = dst->curLen + numNewBytes;
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
		memcpy( &(dst->data[dst->curLen]), pSrc, numNewBytes );
		dst->data[newLen] = '\0';
		dst->curLen = newLen;
		pString->hash = 0;
    }

    FORTHOP( oStringAppendMethod )
    {
        GET_THIS( oStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
		int len = strlen( srcStr );
		oStringAppendBytes( pString, srcStr, len );
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
		dst->data[newLen] = '\0';
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
		oStringAppendBytes( pString, &c, 1 );
        METHOD_RETURN;
    }

	FORTHOP(oStringLoadMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oStringStruct, pString);
		int numStrings = SPOP;
		const char** pStrings = (const char**)(GET_SP);
		for (int i = numStrings - 1; i >= 0; i--)
		{
			const char* srcStr = pStrings[i];
			int len = strlen(srcStr);
			oStringAppendBytes(pString, srcStr, len);
		}
		METHOD_RETURN;
	}


    baseMethodEntry oStringMembers[] =
    {
        METHOD(     "__newOp",              oStringNew ),
        METHOD(     "delete",               oStringDeleteMethod ),
        METHOD(     "show",					oStringShowMethod ),
        METHOD_RET( "compare",              oStringCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
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
        METHOD(     "load",                 oStringLoadMethod ),
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
        SAFE_RELEASE( pCore, oa );
        ForthObject& ob = pPair->b;
        SAFE_RELEASE( pCore, ob );
        FREE_OBJECT( pPair );
        METHOD_RETURN;
    }

	FORTHOP(oPairShowMethod)
	{
		GET_THIS(oPairStruct, pPair);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_HEADER("oPair");
		pShowContext->BeginIndent();
		pShowContext->ShowIndent("'a' : ");
		ForthShowObject(pPair->a, pCore);
		pShowContext->EndElement(",");
		pShowContext->ShowIndent("'b' : ");
		ForthShowObject(pPair->b, pCore);
		pShowContext->EndElement();
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
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
            SAFE_RELEASE( pCore, oldObj );
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
            SAFE_RELEASE( pCore, oldObj );
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
        METHOD(     "__newOp",              oPairNew ),
        METHOD(     "delete",               oPairDeleteMethod ),
        METHOD(     "show",                 oPairShowMethod ),

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
		SAFE_RELEASE( pCore, pIter->parent );
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
        METHOD(     "__newOp",              oPairIterNew ),
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
        SAFE_RELEASE( pCore, oa );
        ForthObject& ob = pTriple->b;
        SAFE_RELEASE( pCore, ob );
        ForthObject& oc = pTriple->c;
        SAFE_RELEASE( pCore, oc );
        FREE_OBJECT( pTriple );
        METHOD_RETURN;
    }

	FORTHOP(oTripleShowMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_HEADER("oTriple");
		pShowContext->BeginIndent();
		pShowContext->ShowIndent("'a' : ");
		ForthShowObject(pTriple->a, pCore);
		pShowContext->EndElement(",");
		pShowContext->ShowIndent("'b' : ");
		ForthShowObject(pTriple->b, pCore);
		pShowContext->EndElement();
		pShowContext->ShowIndent("'c' : ");
		ForthShowObject(pTriple->c, pCore);
		pShowContext->EndElement();
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
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
            SAFE_RELEASE( pCore, oldObj );
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
            SAFE_RELEASE( pCore, oldObj );
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
            SAFE_RELEASE( pCore, oldObj );
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
        METHOD(     "__newOp",              oTripleNew ),
        METHOD(     "delete",               oTripleDeleteMethod ),
        METHOD(     "show",                 oTripleShowMethod ),

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
		SAFE_RELEASE( pCore, pIter->parent );
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
        METHOD(     "__newOp",              oTripleIterNew ),
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

	FORTHOP(oByteArrayShowMethod)
	{
		char buffer[8];
		GET_THIS(oByteArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oByteArray& a = *(pArray->elements);
		SHOW_HEADER("oByteArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				sprintf(buffer, "%u", a[i] & 0xFF);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
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

    FORTHOP( oByteArrayLoadMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS( oByteArrayStruct, pArray );
        oByteArray& a = *(pArray->elements);
        ulong newSize = SPOP;
        a.resize( newSize );
		if ( newSize > 0 )
		{
			for ( int i = newSize - 1; i >= 0; i-- )
			{
				int c = SPOP;
				a[i] = (char) c;
			}
		}
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

	FORTHOP(oByteArrayFromStringMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		const char* pSrc = (const char *)(SPOP);
		int numBytes = ((int)strlen(pSrc)) + 1;
		a.resize(numBytes);
		memcpy(&(a[0]), pSrc, numBytes);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayFromMemoryMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		int offset = SPOP;
		int numBytes = SPOP;
		const char* pSrc = (const char *)(SPOP);
		ulong copyEnd = (ulong)(numBytes + offset);
		if (copyEnd > a.size())
		{
			a.resize(numBytes);
		}
		char* pDst = &(a[0]) + offset;
		memcpy(pDst, pSrc, numBytes);
		METHOD_RETURN;
	}

	baseMethodEntry oByteArrayMembers[] =
    {
        METHOD(     "__newOp",              oByteArrayNew ),
        METHOD(     "delete",               oByteArrayDeleteMethod ),
        METHOD(     "show",                 oByteArrayShowMethod ),

        METHOD_RET( "headIter",             oByteArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArrayIter) ),
        METHOD_RET( "tailIter",             oByteArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArrayIter) ),
        METHOD_RET( "find",                 oByteArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArrayIter) ),
        METHOD_RET( "clone",                oByteArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIByteArray) ),
        METHOD_RET( "count",                oByteArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
		METHOD(     "clear",                oByteArrayClearMethod ),
		METHOD(     "load",                 oByteArrayLoadMethod ),

        METHOD(     "resize",               oByteArrayResizeMethod ),
        METHOD_RET( "ref",                  oByteArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte|kDTIsPtr) ),
        METHOD_RET( "get",                  oByteArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte) ),
        METHOD(     "set",                  oByteArraySetMethod ),
        METHOD_RET( "findIndex",            oByteArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "push",                 oByteArrayPushMethod ),
        METHOD_RET( "pop",                  oByteArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte) ),
        METHOD_RET( "base",                 oByteArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeByte|kDTIsPtr) ),
		METHOD(     "fromString",           oByteArrayFromStringMethod ),
		METHOD(     "fromMemory",           oByteArrayFromMemoryMethod ),
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
		SAFE_RELEASE( pCore, pIter->parent );
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
        METHOD(     "__newOp",              oByteArrayIterNew ),
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

	FORTHOP(oShortArrayShowMethod)
	{
		char buffer[16];
		GET_THIS(oShortArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oShortArray& a = *(pArray->elements);
		SHOW_HEADER("oShortArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				sprintf(buffer, "%d", a[i]);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
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

    FORTHOP( oShortArrayLoadMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS( oShortArrayStruct, pArray );
        oShortArray& a = *(pArray->elements);
        ulong newSize = SPOP;
        a.resize( newSize );
		if ( newSize > 0 )
		{
			for ( int i = newSize - 1; i >= 0; i-- )
			{
				int c = SPOP;
				a[i] = (short) c;
			}
		}
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


	FORTHOP(oShortArrayFromMemoryMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		int offset = SPOP;
		int numShorts = SPOP;
		const short* pSrc = (const short *)(SPOP);
		ulong copyEnd = (ulong)(numShorts + offset);
		if (copyEnd > a.size())
		{
			a.resize(numShorts);
		}
		short* pDst = &(a[0]) + offset;
		memcpy(pDst, pSrc, numShorts << 1);
		METHOD_RETURN;
	}

	baseMethodEntry oShortArrayMembers[] =
    {
        METHOD(     "__newOp",              oShortArrayNew ),
        METHOD(     "delete",               oShortArrayDeleteMethod ),
        METHOD(     "show",                 oShortArrayShowMethod ),

        METHOD_RET( "headIter",             oShortArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArrayIter) ),
        METHOD_RET( "tailIter",             oShortArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArrayIter) ),
        METHOD_RET( "find",                 oShortArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArrayIter) ),
        METHOD_RET( "clone",                oShortArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIShortArray) ),
        METHOD_RET( "count",                oShortArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oShortArrayClearMethod ),
        METHOD(     "load",                 oShortArrayLoadMethod ),

        METHOD(     "resize",               oShortArrayResizeMethod ),
        METHOD_RET( "ref",                  oShortArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort|kDTIsPtr) ),
        METHOD_RET( "get",                  oShortArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort) ),
        METHOD(     "set",                  oShortArraySetMethod ),
        METHOD_RET( "findIndex",            oShortArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "push",                 oShortArrayPushMethod ),
        METHOD_RET( "pop",                  oShortArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort) ),
        METHOD_RET( "base",                 oShortArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeShort|kDTIsPtr) ),
		METHOD(     "fromMemory",           oShortArrayFromMemoryMethod ),
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
		SAFE_RELEASE( pCore, pIter->parent );
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
        METHOD(     "__newOp",              oShortArrayIterNew ),
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

	FORTHOP(oIntArrayShowMethod)
	{
		char buffer[16];
		GET_THIS(oIntArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oIntArray& a = *(pArray->elements);
		SHOW_HEADER("oIntArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				sprintf(buffer, "%d", a[i]);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
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

    FORTHOP( oIntArrayLoadMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS( oIntArrayStruct, pArray );
        oIntArray& a = *(pArray->elements);
        ulong newSize = SPOP;
        a.resize( newSize );
		if ( newSize > 0 )
		{
			for ( int i = newSize - 1; i >= 0; i-- )
			{
				int c = SPOP;
				a[i] = c;
			}
		}
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

	FORTHOP(oIntArrayFromMemoryMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		int offset = SPOP;
		int numInts = SPOP;
		const int* pSrc = (const int *)(SPOP);
		ulong copyEnd = (ulong)(numInts + offset);
		if (copyEnd > a.size())
		{
			a.resize(numInts);
		}
		int* pDst = &(a[0]) + offset;
		memcpy(pDst, pSrc, numInts << 2);
		METHOD_RETURN;
	}

    baseMethodEntry oIntArrayMembers[] =
    {
        METHOD(     "__newOp",              oIntArrayNew ),
        METHOD(     "delete",               oIntArrayDeleteMethod ),
        METHOD(     "show",                 oIntArrayShowMethod ),

        METHOD_RET( "headIter",             oIntArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArrayIter) ),
        METHOD_RET( "tailIter",             oIntArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArrayIter) ),
        METHOD_RET( "find",                 oIntArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArrayIter) ),
        METHOD_RET( "clone",                oIntArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIntArray) ),
        METHOD_RET( "count",                oIntArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oIntArrayClearMethod ),
        METHOD(     "load",                 oIntArrayLoadMethod ),

        METHOD(     "resize",               oIntArrayResizeMethod ),
        METHOD_RET( "ref",                  oIntArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt|kDTIsPtr) ),
        METHOD_RET( "get",                  oIntArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "set",                  oIntArraySetMethod ),
        METHOD_RET( "findIndex",            oIntArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "push",                 oIntArrayPushMethod ),
        METHOD_RET( "pop",                  oIntArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "base",                 oIntArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt|kDTIsPtr) ),
		METHOD(     "fromMemory",           oIntArrayFromMemoryMethod ),
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
		SAFE_RELEASE( pCore, pIter->parent );
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
        METHOD(     "__newOp",              oIntArrayIterNew ),
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
	//                 oFloatArray
	//
	static ForthClassVocabulary* gpFloatArraryIterClassVocab = NULL;

	FORTHOP(oFloatArrayShowMethod)
	{
		char buffer[32];
		GET_THIS(oIntArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oIntArray& a = *(pArray->elements);
		SHOW_HEADER("oFloatArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				float fval = *((float *)(&(a[i])));
				sprintf(buffer, "%f", fval);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

	baseMethodEntry oFloatArrayMembers[] =
	{
		METHOD("__newOp", oIntArrayNew),
		METHOD("show", oFloatArrayShowMethod),

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

	FORTHOP(oLongArrayShowMethod)
	{
		char buffer[32];
		GET_THIS(oLongArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oLongArray& a = *(pArray->elements);
		SHOW_HEADER("oLongArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				sprintf(buffer, "%lld", a[i]);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
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

    FORTHOP( oLongArrayLoadMethod )
    {
        ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS( oLongArrayStruct, pArray );
        oLongArray& a = *(pArray->elements);
        ulong newSize = SPOP;
        a.resize( newSize );
        stackInt64 a64;
		if ( newSize > 0 )
		{
			for ( int i = newSize - 1; i >= 0; i-- )
			{
                LPOP( a64 );
				a[i] = a64.s64;
			}
		}
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
            stackInt64 a64;
            a64.s64 = a[ix];
	        LPUSH( a64 );
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
        stackInt64 a64;
        if ( a.size() > ix )
        {
            LPOP( a64 );
			a[ix] = a64.s64;
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
        stackInt64 a64;
        LPOP( a64 );
        long long val = a64.s64;
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
        stackInt64 a64;
        LPOP( a64 );
        a.push_back( a64.s64 );
        METHOD_RETURN;
    }

    FORTHOP( oLongArrayPopMethod )
    {
		GET_THIS( oLongArrayStruct, pArray );
        oLongArray& a = *(pArray->elements);
        if ( a.size() > 0 )
        {
            stackInt64 a64;
			a64.s64 = a.back();
			a.pop_back();
			LPUSH( a64 );
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
        stackInt64 a64;
        LPOP( a64 );
		long long soughtLong = a64.s64;
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

	FORTHOP(oLongArrayFromMemoryMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		int offset = SPOP;
		int numLongs = SPOP;
		const long long* pSrc = (const long long*)(SPOP);
		ulong copyEnd = (ulong)(numLongs + offset);
		if (copyEnd > a.size())
		{
			a.resize(numLongs);
		}
		long long* pDst = &(a[0]) + offset;
		memcpy(pDst, pSrc, numLongs << 3);
		METHOD_RETURN;
	}

    baseMethodEntry oLongArrayMembers[] =
    {
        METHOD(     "__newOp",              oLongArrayNew ),
        METHOD(     "delete",               oLongArrayDeleteMethod ),
        METHOD(     "show",                 oLongArrayShowMethod ),

        METHOD_RET( "headIter",             oLongArrayHeadIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArrayIter) ),
        METHOD_RET( "tailIter",             oLongArrayTailIterMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArrayIter) ),
        METHOD_RET( "find",                 oLongArrayFindMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArrayIter) ),
        METHOD_RET( "clone",                oLongArrayCloneMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCILongArray) ),
        METHOD_RET( "count",                oLongArrayCountMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "clear",                oLongArrayClearMethod ),
        METHOD(     "long",                 oLongArrayLoadMethod ),

        METHOD(     "resize",               oLongArrayResizeMethod ),
        METHOD_RET( "ref",                  oLongArrayRefMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong|kDTIsPtr) ),
        METHOD_RET( "get",                  oLongArrayGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong) ),
        METHOD(     "set",                  oLongArraySetMethod ),
        METHOD_RET( "findIndex",            oLongArrayFindIndexMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "push",                 oLongArrayPushMethod ),
        METHOD_RET( "pop",                  oLongArrayPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong) ),
        METHOD_RET( "base",                 oLongArrayBaseMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong|kDTIsPtr) ),
		METHOD(     "fromMemory",           oLongArrayFromMemoryMethod ),
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
		SAFE_RELEASE( pCore, pIter->parent );
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
            stackInt64 val;
			val.s64 = a[ pIter->cursor ];
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
            stackInt64 val;
			val.s64 = a[ pIter->cursor ];
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
            stackInt64 val;
			val.s64 = a[ pIter->cursor ];
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
        stackInt64 a64;
        LPOP( a64 );
		long long soughtLong = a64.s64;
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
        METHOD(     "__newOp",              oLongArrayIterNew ),
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
	//                 oDoubleArray
	//
	static ForthClassVocabulary* gpDoubleArrayIterClassVocab = NULL;

	FORTHOP(oDoubleArrayShowMethod)
	{
		char buffer[32];
		GET_THIS(oLongArrayStruct, pArray);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->BeginIndent();
		oLongArray& a = *(pArray->elements);
		SHOW_HEADER("oDoubleArray");
		pShowContext->ShowIndent("'elements' : [");
		if (a.size() > 0)
		{
			pShowContext->EndElement();
			pShowContext->BeginIndent();
			pShowContext->ShowIndent();
			for (unsigned int i = 0; i < a.size(); i++)
			{
				if (i != 0)
				{
					if ((i % 10) == 0)
					{
						pShowContext->EndElement(",");
						pShowContext->ShowIndent();
					}
					else
					{
						pEngine->ConsoleOut(", ");
					}
				}
				double dval = *((double *)(&(a[i])));
				sprintf(buffer, "%g", dval);
				pEngine->ConsoleOut(buffer);
			}
			pShowContext->EndElement();
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
		pShowContext->EndElement("]");
		pShowContext->EndIndent();
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}


    baseMethodEntry oDoubleArrayMembers[] =
    {
		METHOD("__newOp", oLongArrayNew),
		METHOD("show", oDoubleArrayShowMethod),
        // following must be last in table
        END_MEMBERS
    };


	//////////////////////////////////////////////////////////////////////
    ///
    //                 oInt
    //

    struct oIntStruct
    {
        ulong       refCount;
		int			val;
    };


    FORTHOP( oIntNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        MALLOCATE_OBJECT( oIntStruct, pInt );
        pInt->refCount = 0;
		pInt->val = 0;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pInt );
    }

    FORTHOP( oIntGetMethod )
    {
        GET_THIS( oIntStruct, pInt );
		SPUSH( pInt->val );
        METHOD_RETURN;
    }

    FORTHOP( oIntSetMethod )
    {
        GET_THIS( oIntStruct, pInt );
		pInt->val = SPOP;
        METHOD_RETURN;
    }

    FORTHOP( oIntShowMethod )
    {
        char buff[32];
        GET_THIS( oIntStruct, pInt );
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_HEADER("oInt");
		pShowContext->ShowIndent("'value' : ");
		sprintf(buff, "%d", pInt->val);
		pShowContext->EndElement(buff);
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
    }

    FORTHOP( oIntCompareMethod )
    {
        GET_THIS( oIntStruct, pInt );
        ForthObject compObj;
        POP_OBJECT( compObj );
		oIntStruct* pComp = (oIntStruct *) compObj.pData;
		int retVal = 0;
		if ( pInt->val != pComp->val )
		{
			retVal = (pInt->val > pComp->val) ? 1 : -1;
		}
		SPUSH( retVal );
        METHOD_RETURN;
    }

    baseMethodEntry oIntMembers[] =
    {
        METHOD(     "__newOp",              oIntNew ),

        METHOD(     "set",                  oIntSetMethod ),
        METHOD_RET( "get",                  oIntGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "show",                 oIntShowMethod ),
        METHOD_RET( "compare",              oIntCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oLong
    //

    struct oLongStruct
    {
        ulong       refCount;
		long long	val;
    };


    FORTHOP( oLongNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        MALLOCATE_OBJECT( oLongStruct, pLong );
        pLong->refCount = 0;
		pLong->val = 0;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pLong );
    }

    FORTHOP( oLongGetMethod )
    {
        GET_THIS( oLongStruct, pLong );
        stackInt64 val;
        val.s64 = pLong->val;
        LPUSH( val );
        METHOD_RETURN;
    }

    FORTHOP( oLongSetMethod )
    {
        GET_THIS( oLongStruct, pLong );
        stackInt64 a64;
        LPOP( a64 );
		pLong->val = a64.s64;
        METHOD_RETURN;
    }

	FORTHOP(oLongShowMethod)
	{
		char buff[32];
		GET_THIS(oLongStruct, pLong);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_HEADER("oLong");
		pShowContext->ShowIndent("'value' : ");
		sprintf(buff, "%lld", pLong->val);
		pShowContext->EndElement(buff);
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

    FORTHOP( oLongCompareMethod )
    {
        GET_THIS( oLongStruct, pLong );
        ForthObject compObj;
        POP_OBJECT( compObj );
		oLongStruct* pComp = (oLongStruct *) compObj.pData;
		int retVal = 0;
		if ( pLong->val != pComp->val )
		{
			retVal = (pLong->val > pComp->val) ? 1 : -1;
		}
		SPUSH( retVal );
        METHOD_RETURN;
    }

    baseMethodEntry oLongMembers[] =
    {
        METHOD(     "__newOp",              oLongNew ),

        METHOD(     "set",                  oLongSetMethod ),
        METHOD_RET( "get",                  oLongGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeLong) ),
        METHOD(     "show",                 oLongShowMethod ),
        METHOD_RET( "compare",              oLongCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oFloat
    //

    struct oFloatStruct
    {
        ulong       refCount;
		float		val;
    };


    FORTHOP( oFloatNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        MALLOCATE_OBJECT( oFloatStruct, pFloat );
        pFloat->refCount = 0;
		pFloat->val = 0.0f;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pFloat );
    }

    FORTHOP( oFloatGetMethod )
    {
        GET_THIS( oFloatStruct, pFloat );
		FPUSH( pFloat->val );
        METHOD_RETURN;
    }

    FORTHOP( oFloatSetMethod )
    {
        GET_THIS( oFloatStruct, pFloat );
		pFloat->val = FPOP;
        METHOD_RETURN;
    }

	FORTHOP(oFloatShowMethod)
	{
		char buff[32];
		GET_THIS(oFloatStruct, pFloat);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_HEADER("oFloat");
		pShowContext->ShowIndent("'value' : ");
		sprintf(buff, "%f", pFloat->val);
		pShowContext->EndElement(buff);
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

    FORTHOP( oFloatCompareMethod )
    {
        GET_THIS( oFloatStruct, pFloat );
        ForthObject compObj;
        POP_OBJECT( compObj );
		oFloatStruct* pComp = (oFloatStruct *) compObj.pData;
		int retVal = 0;
		if ( pFloat->val != pComp->val )
		{
			retVal = (pFloat->val > pComp->val) ? 1 : -1;
		}
		SPUSH( retVal );
        METHOD_RETURN;
    }

    baseMethodEntry oFloatMembers[] =
    {
        METHOD(     "__newOp",              oFloatNew ),

        METHOD(     "set",                  oFloatSetMethod ),
        METHOD_RET( "get",                  oFloatGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeFloat) ),
        METHOD(     "show",                 oFloatShowMethod ),
        METHOD_RET( "compare",              oFloatCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oDouble
    //

    struct oDoubleStruct
    {
        ulong       refCount;
		double		val;
    };


    FORTHOP( oDoubleNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        MALLOCATE_OBJECT( oDoubleStruct, pDouble );
        pDouble->refCount = 0;
		pDouble->val = 0.0;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pDouble );
    }

    FORTHOP( oDoubleGetMethod )
    {
        GET_THIS( oDoubleStruct, pDouble );
		DPUSH( pDouble->val );
        METHOD_RETURN;
    }

    FORTHOP( oDoubleSetMethod )
    {
        GET_THIS( oDoubleStruct, pDouble );
		pDouble->val = DPOP;
        METHOD_RETURN;
    }

    FORTHOP( oDoubleShowMethod )
    {
        char buff[32];
        GET_THIS( oDoubleStruct, pDouble );
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		SHOW_HEADER("oDouble");
		pShowContext->ShowIndent("'value' : ");
		sprintf(buff, "%f", pDouble->val);
		pShowContext->EndElement(buff);
		pShowContext->ShowIndent("}");
		METHOD_RETURN;
	}

    FORTHOP( oDoubleCompareMethod )
    {
        GET_THIS( oDoubleStruct, pDouble );
		int retVal = 0;
        ForthObject compObj;
        POP_OBJECT( compObj );
		oDoubleStruct* pComp = (oDoubleStruct *) compObj.pData;
		if ( pDouble->val != pComp->val )
		{
			retVal = (pDouble->val > pComp->val) ? 1 : -1;
		}
		SPUSH( retVal );
        METHOD_RETURN;
    }

    baseMethodEntry oDoubleMembers[] =
    {
        METHOD(     "__newOp",              oDoubleNew ),

        METHOD(     "set",                  oDoubleSetMethod ),
        METHOD_RET( "get",                  oDoubleGetMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeDouble) ),
        METHOD(     "show",                 oDoubleShowMethod ),
        METHOD_RET( "compare",              oDoubleCompareMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oThread
    //

    struct oThreadStruct
    {
        ulong			refCount;
        ForthThread		*pThread;
    };

	// TODO: add tracking of run state of thread - this should be done inside ForthThread, not here
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
		SPUSH( (long) (pThreadStruct->pThread->GetCore()) );
        METHOD_RETURN;
	}

	FORTHOP( oThreadStepMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		ForthThread* pThread = pThreadStruct->pThread;
		ForthCoreState* pThreadCore = pThread->GetCore();
		long op = *(pThreadCore->IP)++;
		long result;
		ForthEngine *pEngine = GET_ENGINE;
#ifdef ASM_INNER_INTERPRETER
		if ( pEngine->GetFastMode() )
		{
			result = (long) InterpretOneOpFast( pThreadCore, op );
		}
		else
#endif
		{
			result = (long) InterpretOneOp( pThreadCore, op );
		}
		if ( result == kResultDone )
		{
			pThread->ResetIP();
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

	FORTHOP( oThreadResetIPMethod )
	{
		GET_THIS( oThreadStruct, pThreadStruct );
		pThreadStruct->pThread->ResetIP();
        METHOD_RETURN;
	}

    baseMethodEntry oThreadMembers[] =
    {
        METHOD(     "__newOp",              oThreadNew ),
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
        METHOD(     "resetIP",              oThreadResetIPMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oInStream
    //

	// oInStream is an abstract input stream class

    baseMethodEntry oInStreamMembers[] =
    {
        METHOD(     "getChar",              unimplementedMethodOp ),
        METHOD(     "getBytes",             unimplementedMethodOp ),
        METHOD(     "getLine",              unimplementedMethodOp ),
        METHOD(     "setTrimEOL",           unimplementedMethodOp ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oFileInStream
    //

    struct oFileInStreamStruct
    {
        ulong			refCount;
		FILE*			pInFile;
		bool			trimEOL;
    };

    FORTHOP( oFileInStreamNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oFileInStreamStruct, pFileInStreamStruct );
        pFileInStreamStruct->refCount = 0;
		pFileInStreamStruct->pInFile = NULL;
		pFileInStreamStruct->trimEOL = true;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pFileInStreamStruct );
    }

    FORTHOP( oFileInStreamDeleteMethod )
    {
		GET_THIS( oFileInStreamStruct, pFileInStreamStruct );
		if ( pFileInStreamStruct->pInFile != NULL )
		{
			GET_ENGINE->GetShell()->GetFileInterface()->fileClose( pFileInStreamStruct->pInFile );
			pFileInStreamStruct->pInFile = NULL;
		}
        METHOD_RETURN;
    }

    FORTHOP( oFileInStreamSetFileMethod )
    {
		GET_THIS( oFileInStreamStruct, pFileInStreamStruct );
		pFileInStreamStruct->pInFile = reinterpret_cast<FILE *>(SPOP);
        METHOD_RETURN;
    }

    FORTHOP( oFileInStreamGetFileMethod )
    {
		GET_THIS( oFileInStreamStruct, pFileInStreamStruct );
		SPUSH( reinterpret_cast<long>(pFileInStreamStruct->pInFile) );
        METHOD_RETURN;
    }

    FORTHOP( oFileInStreamGetCharMethod )
    {
		GET_THIS( oFileInStreamStruct, pFileInStreamStruct );
		int ch = -1;
		if ( pFileInStreamStruct->pInFile != NULL )
		{
			ch = GET_ENGINE->GetShell()->GetFileInterface()->fileGetChar(pFileInStreamStruct->pInFile);
		}
		SPUSH( ch );
        METHOD_RETURN;
    }

    FORTHOP( oFileInStreamGetBytesMethod )
    {
		GET_THIS( oFileInStreamStruct, pFileInStreamStruct );
		int numBytes = SPOP;
		char* pBuffer = reinterpret_cast<char *>(SPOP);
		int numRead = 0;
		if ( pFileInStreamStruct->pInFile != NULL )
		{
			numRead = GET_ENGINE->GetShell()->GetFileInterface()->fileRead(pBuffer, 1, numBytes, pFileInStreamStruct->pInFile);
		}
		SPUSH( numRead );
        METHOD_RETURN;
    }

    FORTHOP( oFileInStreamGetLineMethod )
    {
		GET_THIS( oFileInStreamStruct, pFileInStreamStruct );
		int maxBytes = SPOP;
		char* pBuffer = reinterpret_cast<char *>(SPOP);
		char* pResult = NULL;
		if ( pFileInStreamStruct->pInFile != NULL )
		{
			pResult = GET_ENGINE->GetShell()->GetFileInterface()->fileGetString(pBuffer, maxBytes, pFileInStreamStruct->pInFile);
		}
		int numRead = 0;
		if ( pResult != NULL )
		{
			if ( pFileInStreamStruct->trimEOL )
			{
				char* pEOL = pResult;
				while ( char ch = *pEOL != '\0' )
				{
					if ( (ch == '\n') || (ch == '\r') )
					{
						*pEOL = '\0';
						break;
					}
					++pEOL;
				}
			}
			numRead = strlen( pResult );
		}
		SPUSH( numRead );
        METHOD_RETURN;
    }

    FORTHOP( oFileInStreamSetTrimEOLMethod )
    {
		GET_THIS( oFileInStreamStruct, pFileInStreamStruct );
		pFileInStreamStruct->trimEOL = (SPOP != 0);
        METHOD_RETURN;
    }

    baseMethodEntry oFileInStreamMembers[] =
    {
        METHOD(     "__newOp",              oFileInStreamNew ),
        METHOD(     "setFile",              oFileInStreamSetFileMethod ),
        METHOD(     "getFile",              oFileInStreamGetFileMethod ),
        METHOD(     "delete",               oFileInStreamDeleteMethod ),
        METHOD(     "getChar",              oFileInStreamGetCharMethod ),
        METHOD(     "getBytes",             oFileInStreamGetBytesMethod ),
        METHOD(     "getLine",              oFileInStreamGetLineMethod ),
        METHOD(     "setTrimEOL",           oFileInStreamSetTrimEOLMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oConsoleInStream
    //

    FORTHOP( oConsoleInStreamNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oFileInStreamStruct, pConsoleInStreamStruct );
        pConsoleInStreamStruct->refCount = 0;
		pConsoleInStreamStruct->trimEOL = true;
		pConsoleInStreamStruct->pInFile = GET_ENGINE->GetShell()->GetFileInterface()->getStdIn();
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pConsoleInStreamStruct );
    }

    FORTHOP( oConsoleInStreamDeleteMethod )
    {
		GET_THIS( oFileInStreamStruct, pFileInStreamStruct );
		pFileInStreamStruct->pInFile = NULL;
        METHOD_RETURN;
    }

    FORTHOP( oConsoleInStreamSetFileMethod )
	{
		// TODO: report an error?
		SPOP;
        METHOD_RETURN;
    }

    baseMethodEntry oConsoleInStreamMembers[] =
    {
        METHOD(     "__newOp",              oConsoleInStreamNew ),
        METHOD(     "delete",               oConsoleInStreamDeleteMethod ),
        METHOD(     "setFile",              oConsoleInStreamSetFileMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oOutStream
    //

	// oOutStream is an abstract output stream class

    struct OutStreamFuncs
    {
		streamCharOutRoutine		outChar;
		streamBytesOutRoutine		outBytes;
		streamStringOutRoutine		outString;
    };


    struct oOutStreamStruct
    {
        ulong               refCount;
		OutStreamFuncs*     pOutFuncs;
		void*               pUserData;
    };


    FORTHOP( oOutStreamNew )
    {
		ForthEngine::GetInstance()->SetError( kForthErrorBadObject, " can't create abstract oOutStream" );

        PUSH_PAIR( NULL, NULL );
    }

	void streamCharOut( ForthCoreState* pCore, oOutStreamStruct* pOutStream, char ch )
	{
		if ( pOutStream->pOutFuncs->outChar != NULL )
		{
			pOutStream->pOutFuncs->outChar( pCore, pOutStream, ch );
		}
		else if ( pOutStream->pOutFuncs->outBytes != NULL )
		{
			pOutStream->pOutFuncs->outBytes( pCore, pOutStream, &ch, 1 );
		}
		else if ( pOutStream->pOutFuncs->outString != NULL )
		{
			char buff[2];
			buff[0] = ch;
			buff[1] = '\0';
			pOutStream->pOutFuncs->outString( pCore, pOutStream, buff);
		}
		else
		{
			ForthEngine::GetInstance()->SetError( kForthErrorIO, " output stream has no output routines" );
		}
	}

	FORTHOP( oOutStreamPutCharMethod )
    {
		GET_THIS( oOutStreamStruct, pOutStream );

		if ( pOutStream->pOutFuncs == NULL )
		{
			ForthEngine::GetInstance()->SetError( kForthErrorIO, " output stream has no output routines" );
		}
		else
		{
			char ch = static_cast<char>(SPOP);
			streamCharOut( pCore, pOutStream, ch );
		}
        METHOD_RETURN;
    }

	void streamBytesOut( ForthCoreState* pCore, oOutStreamStruct* pOutStream, const char* pBuffer, int numBytes )
	{
		if ( pOutStream->pOutFuncs->outBytes != NULL )
		{
			pOutStream->pOutFuncs->outBytes( pCore, pOutStream, pBuffer, numBytes );
		}
		else if ( pOutStream->pOutFuncs->outChar != NULL )
		{
			for ( int i = 0; i < numBytes; i++ )
			{
				pOutStream->pOutFuncs->outChar( pCore, pOutStream, pBuffer[i] );
			}
		}
		else if ( pOutStream->pOutFuncs->outString != NULL )
		{
			char buff[2];
			buff[1] = '\0';
			for ( int i = 0; i < numBytes; i++ )
			{
				buff[0] = pBuffer[i];
				pOutStream->pOutFuncs->outString( pCore, pOutStream, buff );
			}
		}
		else
		{
			ForthEngine::GetInstance()->SetError( kForthErrorIO, " output stream has no output routines" );
		}
	}

    FORTHOP( oOutStreamPutBytesMethod )
    {
		GET_THIS( oOutStreamStruct, pOutStream );
		int numBytes = SPOP;
		char* pBuffer = reinterpret_cast<char *>(SPOP);

		if ( pOutStream->pOutFuncs == NULL )
		{
			ForthEngine::GetInstance()->SetError( kForthErrorIO, " output stream has no output routines" );
		}
		else
		{
			streamBytesOut( pCore, pOutStream, pBuffer, numBytes );
		}
        METHOD_RETURN;
    }

	void streamStringOut( ForthCoreState* pCore, oOutStreamStruct* pOutStream, const char* pBuffer )
	{
		if ( pOutStream->pOutFuncs->outString != NULL )
		{
			pOutStream->pOutFuncs->outString( pCore, pOutStream, pBuffer );
		}
		else
		{
			int numBytes = strlen( pBuffer );
			if ( pOutStream->pOutFuncs->outBytes != NULL )
			{
				pOutStream->pOutFuncs->outBytes( pCore, pOutStream, pBuffer, numBytes );
			}
			else if ( pOutStream->pOutFuncs->outChar != NULL )
			{
				for ( int i = 0; i < numBytes; i++ )
				{
					pOutStream->pOutFuncs->outChar( pCore, pOutStream, pBuffer[i] );
				}
			}
			else
			{
				ForthEngine::GetInstance()->SetError( kForthErrorIO, " output stream has no output routines" );
			}
		}
	}

    FORTHOP( oOutStreamPutStringMethod )
    {
		GET_THIS( oOutStreamStruct, pOutStream );
		char* pBuffer = reinterpret_cast<char *>(SPOP);

		if ( pOutStream->pOutFuncs == NULL )
		{
			ForthEngine::GetInstance()->SetError( kForthErrorIO, " output stream has no output routines" );
		}
		else
		{
			streamStringOut( pCore, pOutStream, pBuffer );
		}
        METHOD_RETURN;
    }

    FORTHOP( oOutStreamGetDataMethod )
    {
		GET_THIS( oOutStreamStruct, pOutStream );
		SPUSH( reinterpret_cast<long>(pOutStream->pUserData) );
        METHOD_RETURN;
    }

    FORTHOP( oOutStreamSetDataMethod )
    {
		GET_THIS( oOutStreamStruct, pOutStream );
		pOutStream->pUserData = reinterpret_cast<void *>(SPOP);
        METHOD_RETURN;
    }

	enum
	{
		kOutStreamPutCharMethod = kNumBaseMethods,
		kOutStreamPutBytesMethod = kNumBaseMethods + 1,
		kOutStreamPutStringMethod = kNumBaseMethods + 2
	};

    baseMethodEntry oOutStreamMembers[] =
    {
        METHOD(     "__newOp",              oOutStreamNew ),
        METHOD(     "putChar",              oOutStreamPutCharMethod ),
        METHOD(     "putBytes",             oOutStreamPutBytesMethod ),
        METHOD(     "putString",            oOutStreamPutStringMethod ),
        METHOD_RET( "getData",              oOutStreamGetDataMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "setData",              oOutStreamSetDataMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oFileOutStream
    //

	void fileCharOut( ForthCoreState* pCore, void *pData, char ch )
	{
		GET_ENGINE->GetShell()->GetFileInterface()->filePutChar(ch, static_cast<FILE *>(static_cast<oOutStreamStruct*>(pData)->pUserData));
	}

	void fileBytesOut( ForthCoreState* pCore, void *pData, const char *pBuffer, int numChars )
	{
		GET_ENGINE->GetShell()->GetFileInterface()->fileWrite(pBuffer, 1, numChars, static_cast<FILE *>(static_cast<oOutStreamStruct*>(pData)->pUserData));
	}

	void fileStringOut( ForthCoreState* pCore, void *pData, const char *pBuffer )
	{
		GET_ENGINE->GetShell()->GetFileInterface()->filePutString(pBuffer, static_cast<FILE *>(static_cast<oOutStreamStruct*>(pData)->pUserData));
	}

	OutStreamFuncs fileOutFuncs =
	{
		fileCharOut,
		fileBytesOut,
		fileStringOut
	};

    FORTHOP( oFileOutStreamNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oOutStreamStruct, pFileOutStream );
        pFileOutStream->refCount = 0;
		pFileOutStream->pOutFuncs = &fileOutFuncs;
		pFileOutStream->pUserData = NULL;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pFileOutStream );
    }

    FORTHOP( oFileOutStreamDeleteMethod )
    {
		GET_THIS( oOutStreamStruct, pFileOutStream );
		if ( pFileOutStream->pUserData != NULL )
		{
			GET_ENGINE->GetShell()->GetFileInterface()->fileClose( static_cast<FILE *>(pFileOutStream->pUserData) );
			pFileOutStream->pUserData = NULL;
		}
		FREE_OBJECT( pFileOutStream );
        METHOD_RETURN;
    }

    baseMethodEntry oFileOutStreamMembers[] =
    {
        METHOD(     "__newOp",              oFileOutStreamNew ),
        METHOD(     "delete",               oFileOutStreamDeleteMethod ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oStringOutStream
    //

    struct oStringOutStreamStruct
    {
		oOutStreamStruct		ostream;
		ForthObject				outString;
    };

	void stringCharOut( ForthCoreState* pCore, void *pData, char ch )
	{
		oStringStruct* pString = reinterpret_cast<oStringStruct*>( static_cast<ForthObject*>(static_cast<oOutStreamStruct*>(pData)->pUserData)->pData );
		oStringAppendBytes( pString, &ch, 1 );
	}

	void stringBlockOut( ForthCoreState* pCore, void *pData, const char *pBuffer, int numChars )
	{
		oStringStruct* pString = reinterpret_cast<oStringStruct*>( static_cast<ForthObject*>(static_cast<oOutStreamStruct*>(pData)->pUserData)->pData );
		oStringAppendBytes( pString, pBuffer, numChars );
	}

	void stringStringOut( ForthCoreState* pCore, void *pData, const char *pBuffer )
	{
		oStringStruct* pString = reinterpret_cast<oStringStruct*>( static_cast<ForthObject*>(static_cast<oOutStreamStruct*>(pData)->pUserData)->pData );
		int numChars = strlen( pBuffer );
		oStringAppendBytes( pString, pBuffer, numChars );
	}

	OutStreamFuncs stringOutFuncs =
	{
		stringCharOut,
		stringBlockOut,
		stringStringOut
	};

    FORTHOP( oStringOutStreamNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oStringOutStreamStruct, pStringOutStream );
        pStringOutStream->ostream.refCount = 0;
		pStringOutStream->ostream.pOutFuncs = &stringOutFuncs;
		pStringOutStream->ostream.pUserData = &(pStringOutStream->outString);
		CLEAR_OBJECT( pStringOutStream->outString );
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pStringOutStream );
    }

    FORTHOP( oStringOutStreamDeleteMethod )
    {
		GET_THIS( oStringOutStreamStruct, pStringOutStream );
		SAFE_RELEASE( pCore, pStringOutStream->outString );
		FREE_OBJECT( pStringOutStream );
        METHOD_RETURN;
    }

    FORTHOP( oStringOutStreamSetStringMethod )
    {
		GET_THIS( oStringOutStreamStruct, pStringOutStream );
		ForthObject dstString;
		POP_OBJECT( dstString );
		OBJECT_ASSIGN( pCore, pStringOutStream->outString, dstString );
		pStringOutStream->outString = dstString;
        METHOD_RETURN;
    }

    FORTHOP( oStringOutStreamGetStringMethod )
    {
		GET_THIS( oStringOutStreamStruct, pStringOutStream );
		PUSH_OBJECT( pStringOutStream->outString );
        METHOD_RETURN;
    }

    baseMethodEntry oStringOutStreamMembers[] =
    {
        METHOD(     "__newOp",              oStringOutStreamNew ),
        METHOD(     "delete",               oStringOutStreamDeleteMethod ),
        METHOD(     "setString",            oStringOutStreamSetStringMethod ),
        METHOD_RET( "getString",            oStringOutStreamGetStringMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIString) ),
        METHOD(     "setData",              illegalMethodOp ),
		
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oConsoleOutStream
    //

	static oOutStreamStruct consoleOutSingleton;

    FORTHOP( oConsoleOutStreamNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
        consoleOutSingleton.refCount = 1000;
		consoleOutSingleton.pOutFuncs = &fileOutFuncs;
		consoleOutSingleton.pUserData = GET_ENGINE->GetShell()->GetFileInterface()->getStdOut();
        PUSH_PAIR( pPrimaryInterface->GetMethods(), &consoleOutSingleton );
    }

	FORTHOP( oConsoleOutStreamDeleteMethod )
    {
		// this is an undeletable singleton, make the ref count high to avoid needless delete calls
        consoleOutSingleton.refCount = 1000;
        METHOD_RETURN;
    }

    baseMethodEntry oConsoleOutStreamMembers[] =
    {
        METHOD(     "__newOp",              oConsoleOutStreamNew ),
        METHOD(     "delete",               oConsoleOutStreamDeleteMethod ),
        METHOD(     "setFile",              illegalMethodOp ),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 oFunctionOutStream
    //

    struct oFunctionOutStreamStruct
    {
		oOutStreamStruct			ostream;
		OutStreamFuncs			outFuncs;
    };

    FORTHOP( oFunctionOutStreamNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
		MALLOCATE_OBJECT( oFunctionOutStreamStruct, pFunctionOutStream );
        pFunctionOutStream->ostream.refCount = 0;
		pFunctionOutStream->ostream.pOutFuncs = &(pFunctionOutStream->outFuncs);
		pFunctionOutStream->ostream.pUserData = NULL;
		pFunctionOutStream->outFuncs.outChar = NULL;
		pFunctionOutStream->outFuncs.outBytes = NULL;
		pFunctionOutStream->outFuncs.outString = NULL;
        PUSH_PAIR( pPrimaryInterface->GetMethods(), pFunctionOutStream );
    }

    FORTHOP( oFunctionOutStreamInitMethod )
    {
		GET_THIS( oFunctionOutStreamStruct, pFunctionOutStream );
		pFunctionOutStream->ostream.pUserData = reinterpret_cast<void *>(SPOP);
		pFunctionOutStream->outFuncs.outString = reinterpret_cast<streamStringOutRoutine>(SPOP);
		pFunctionOutStream->outFuncs.outBytes = reinterpret_cast<streamBytesOutRoutine>(SPOP);
		pFunctionOutStream->outFuncs.outChar = reinterpret_cast<streamCharOutRoutine>(SPOP);
        METHOD_RETURN;
    }

    baseMethodEntry oFunctionOutStreamMembers[] =
    {
        METHOD(     "__newOp",              oFunctionOutStreamNew ),
        METHOD(     "init",					oFunctionOutStreamInitMethod ),
        // following must be last in table
        END_MEMBERS
    };


    ForthClassVocabulary* gpOStringClass;
    ForthClassVocabulary* gpOFileOutStreamClass;
    ForthClassVocabulary* gpOStringOutStreamClass;
    ForthClassVocabulary* gpOConsoleOutStreamClass;
    ForthClassVocabulary* gpOFunctionOutStreamClass;
}

void GetForthConsoleOutStream( ForthCoreState* pCore, ForthObject& outObject )
{
	ASSERT( gpOConsoleOutStreamClass != NULL );
    consoleOutSingleton.refCount = 1000;
	consoleOutSingleton.pOutFuncs = &fileOutFuncs;
	consoleOutSingleton.pUserData = GET_ENGINE->GetShell()->GetFileInterface()->getStdOut();

    ForthInterface* pPrimaryInterface = gpOConsoleOutStreamClass->GetInterface( 0 );
	outObject.pMethodOps = pPrimaryInterface->GetMethods();
	outObject.pData = reinterpret_cast<long *>(&consoleOutSingleton);
}

void CreateForthFileOutStream( ForthCoreState* pCore, ForthObject& outObject, FILE* pOutFile )
{
	ASSERT( gpOConsoleOutStreamClass != NULL );
	MALLOCATE_OBJECT( oOutStreamStruct, pFileOutStream );
    pFileOutStream->refCount = 1;
	pFileOutStream->pOutFuncs = &fileOutFuncs;
	pFileOutStream->pUserData = pOutFile;
    ForthInterface* pPrimaryInterface = gpOFileOutStreamClass->GetInterface( 0 );
	outObject.pMethodOps = pPrimaryInterface->GetMethods();
	outObject.pData = reinterpret_cast<long *>(pFileOutStream);
}

void CreateForthStringOutStream( ForthCoreState* pCore, ForthObject& outObject )
{
	ASSERT( gpOConsoleOutStreamClass != NULL );
	ASSERT( gpOStringClass != NULL );

	MALLOCATE_OBJECT( oStringOutStreamStruct, pStringOutStream );
    pStringOutStream->ostream.refCount = 1;
	pStringOutStream->ostream.pOutFuncs = &stringOutFuncs;
	pStringOutStream->ostream.pUserData = &(pStringOutStream->outString);
    ForthInterface* pPrimaryInterface = gpOFileOutStreamClass->GetInterface( 0 );
	outObject.pMethodOps = pPrimaryInterface->GetMethods();
	outObject.pData = reinterpret_cast<long *>(pStringOutStream);

	// create the internal string object
    MALLOCATE_OBJECT( oStringStruct, pString );
    pString->refCount = 0;
    pString->hash = 0;
	pString->str = CreateRCString( gDefaultRCStringSize );
    pPrimaryInterface = gpOStringClass->GetInterface( 0 );
	pStringOutStream->outString.pData = reinterpret_cast<long *>(pString);
	pStringOutStream->outString.pMethodOps = pPrimaryInterface->GetMethods();
}

const char* GetForthStringOutStreamData( ForthCoreState* pCore, ForthObject& streamObject )
{
	oStringOutStreamStruct* pStream = reinterpret_cast<oStringOutStreamStruct *>( streamObject.pData );
	oStringStruct* pString = reinterpret_cast<oStringStruct *>(pStream->outString.pData);
	return pString->str->data;
}

void CreateForthFunctionOutStream( ForthCoreState* pCore, ForthObject& outObject, streamCharOutRoutine outChar,
									  streamBytesOutRoutine outBytes, streamStringOutRoutine outString, void* pUserData )
{
	ASSERT( gpOConsoleOutStreamClass != NULL );
	MALLOCATE_OBJECT( oFunctionOutStreamStruct, pFunctionOutStream );
    pFunctionOutStream->ostream.refCount = 1;
	pFunctionOutStream->ostream.pOutFuncs = &(pFunctionOutStream->outFuncs);
	pFunctionOutStream->ostream.pUserData = pUserData;
	pFunctionOutStream->outFuncs.outChar = outChar;
	pFunctionOutStream->outFuncs.outBytes = outBytes;
	pFunctionOutStream->outFuncs.outString = outString;
    ForthInterface* pPrimaryInterface = gpOFunctionOutStreamClass->GetInterface( 0 );
	outObject.pMethodOps = pPrimaryInterface->GetMethods();
	outObject.pData = reinterpret_cast<long *>(pFunctionOutStream);
}

void ReleaseForthObject( ForthCoreState* pCore, ForthObject& inObject )
{
	oOutStreamStruct* pObjData = reinterpret_cast<oOutStreamStruct *>(inObject.pData);
	if ( pObjData->refCount > 1 )
	{
		--pObjData->refCount;
	}
	else
	{
		FREE_OBJECT( pObjData );
		inObject.pData = NULL;
		inObject.pMethodOps = NULL;
	}
}

// ForthConsoleCharOut etc. exist so that stuff outside this module can do output
//   without having to know about object innards
// TODO: remove hard coded method numbers
void ForthConsoleCharOut( ForthCoreState* pCore, char ch )
{
    ForthEngine *pEngine = GET_ENGINE;
	oOutStreamStruct* pOutStream = reinterpret_cast<oOutStreamStruct*>(pCore->consoleOutStream.pData);
	if ( pOutStream->pOutFuncs != NULL )
	{
		streamCharOut( pCore, pOutStream, ch );
	}
	else
	{
		long* pMethods = pCore->consoleOutStream.pMethodOps;
		SPUSH( ((long) ch ) );
		pEngine->ExecuteOneMethod( pCore, pCore->consoleOutStream, kOutStreamPutCharMethod );
	}
}

void ForthConsoleBytesOut( ForthCoreState* pCore, const char* pBuffer, int numChars )
{
    ForthEngine *pEngine = GET_ENGINE;
	oOutStreamStruct* pOutStream = reinterpret_cast<oOutStreamStruct*>(pCore->consoleOutStream.pData);
	if ( pOutStream->pOutFuncs != NULL )
	{
		streamBytesOut( pCore, pOutStream, pBuffer, numChars );
	}
	else
	{
		SPUSH( ((long) pBuffer ) );
		SPUSH( numChars );
		pEngine->ExecuteOneMethod( pCore, pCore->consoleOutStream, kOutStreamPutBytesMethod );
	}
}

void ForthConsoleStringOut( ForthCoreState* pCore, const char* pBuffer )
{
    ForthEngine *pEngine = GET_ENGINE;
	oOutStreamStruct* pOutStream = reinterpret_cast<oOutStreamStruct*>(pCore->consoleOutStream.pData);
	if ( pOutStream->pOutFuncs != NULL )
	{
		streamStringOut( pCore, pOutStream, pBuffer );
	}
	else
	{
		SPUSH( ((long) pBuffer ) );
		pEngine->ExecuteOneMethod( pCore, pCore->consoleOutStream, kOutStreamPutStringMethod );
	}
}

void ForthShowObject(ForthObject& obj, ForthCoreState* pCore)
{
	ForthEngine* pEngine = ForthEngine::GetInstance();
	if (obj.pData != NULL)
	{
		pEngine->ExecuteOneMethod(pCore, obj, kOMShow);
	}
	else
	{
		pEngine->ConsoleOut("'@");
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pShowContext->ShowID("nullObject", NULL);
		pEngine->ConsoleOut("'");
	}
}

//////////////////////////////////////////////////////////////////////
////
///     ForthForgettableGlobalObject - handles forgetting of global forth objects
//
// 
ForthForgettableGlobalObject::ForthForgettableGlobalObject( const char* pName, void* pOpAddress, long op, int numElements )
: ForthForgettable( pOpAddress, op )
,	mNumElements( numElements )
{
    int nameLen = strlen( pName );
    mpName = new char[nameLen + 1];
    strcpy( mpName, pName );
}

ForthForgettableGlobalObject::~ForthForgettableGlobalObject()
{
    delete [] mpName;
}

const char *
ForthForgettableGlobalObject::GetName( void )
{
    return mpName;
}

const char *
ForthForgettableGlobalObject::GetTypeName( void )
{
    return "globalObject";
}

void ForthForgettableGlobalObject::ForgetCleanup( void* pForgetLimit, long op )
{
	// first longword is OP_DO_OBJECT or OP_DO_OBJECT_ARRAY, after that are object elements
	if ((ulong)mpOpAddress > (ulong)pForgetLimit)
	{
		ForthObject* pObject = (ForthObject *)((long *)mpOpAddress + 1);
		ForthCoreState* pCore = ForthEngine::GetInstance()->GetCoreState();
		for (int i = 0; i < mNumElements; i++)
		{
			// TODO: release each 
			SAFE_RELEASE(pCore, *pObject);
			pObject->pData = NULL;
			pObject->pMethodOps = NULL;
			pObject++;
		}
	}
}

const char *
ForthTypesManager::GetName( void )
{
    return GetTypeName();
}

const char *
ForthTypesManager::GetTypeName( void )
{
    return "typesManager";
}

void
ForthTypesManager::AddBuiltinClasses( ForthEngine* pEngine )
{
    ForthClassVocabulary* pObjectClass = pEngine->AddBuiltinClass( "object", NULL, objectMembers );
    ForthClassVocabulary* pClassClass = pEngine->AddBuiltinClass( "class", pObjectClass, classMembers );

	ForthClassVocabulary* pOIterClass = pEngine->AddBuiltinClass( "oIter", pObjectClass, oIterMembers );
	ForthClassVocabulary* pOIterableClass = pEngine->AddBuiltinClass( "oIterable", pObjectClass, oIterableMembers );

	gpArrayClassVocab = pEngine->AddBuiltinClass("oArray", pOIterableClass, oArrayMembers);
    gpArrayIterClassVocab = pEngine->AddBuiltinClass( "oArrayIter", pOIterClass, oArrayIterMembers );

    gpListClassVocab = pEngine->AddBuiltinClass( "oList", pOIterableClass, oListMembers );
    gpListIterClassVocab = pEngine->AddBuiltinClass( "oListIter", pOIterClass, oListIterMembers );

	ForthClassVocabulary* pOMapClass = pEngine->AddBuiltinClass("oMap", pOIterableClass, oMapMembers);
	gpMapIterClassVocab = pEngine->AddBuiltinClass("oMapIter", pOIterClass, oMapIterMembers);

	ForthClassVocabulary* pOIntMapClass = pEngine->AddBuiltinClass("oIntMap", pOIterableClass, oIntMapMembers);
	gpIntMapIterClassVocab = pEngine->AddBuiltinClass("oIntMapIter", pOIterClass, oIntMapIterMembers);

	ForthClassVocabulary* pOFloatMapClass = pEngine->AddBuiltinClass("oFloatMap", pOIntMapClass, oFloatMapMembers);
	gpFloatMapIterClassVocab = pEngine->AddBuiltinClass("oFloatMapIter", pOIterClass, oIntMapIterMembers);

	ForthClassVocabulary* pOLongMapClass = pEngine->AddBuiltinClass("oLongMap", pOIterableClass, oLongMapMembers);
	gpLongMapIterClassVocab = pEngine->AddBuiltinClass("oLongMapIter", pOIterClass, oLongMapIterMembers);

	ForthClassVocabulary* pODoubleMapClass = pEngine->AddBuiltinClass("oDoubleMap", pOLongMapClass, oDoubleMapMembers);
	gpDoubleMapIterClassVocab = pEngine->AddBuiltinClass("oDoubleMapIter", pOIterClass, oLongMapIterMembers);

	ForthClassVocabulary* pOStringMapClass = pEngine->AddBuiltinClass("oStringMap", pOIterableClass, oStringMapMembers);
	gpStringMapIterClassVocab = pEngine->AddBuiltinClass("oStringMapIter", pOIterClass, oStringMapIterMembers);

	gpOStringClass = pEngine->AddBuiltinClass("oString", pObjectClass, oStringMembers);

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

	ForthClassVocabulary* pOFloatArrayClass = pEngine->AddBuiltinClass("oFloatArray", pOIntArrayClass, oFloatArrayMembers);
	gpFloatArraryIterClassVocab = pEngine->AddBuiltinClass("oFloatArrayIter", pOIterClass, oIntArrayIterMembers);

	ForthClassVocabulary* pOLongArrayClass = pEngine->AddBuiltinClass("oLongArray", pOIterableClass, oLongArrayMembers);
	gpLongArraryIterClassVocab = pEngine->AddBuiltinClass("oLongArrayIter", pOIterClass, oLongArrayIterMembers);

	ForthClassVocabulary* pODoubleArrayClass = pEngine->AddBuiltinClass("oDoubleArray", pOLongArrayClass, oDoubleArrayMembers);
	gpDoubleArrayIterClassVocab = pEngine->AddBuiltinClass("oDoubleArrayIter", pOIterClass, oLongArrayIterMembers);

	ForthClassVocabulary* pOIntClass = pEngine->AddBuiltinClass("oInt", pObjectClass, oIntMembers);
    ForthClassVocabulary* pOLongClass = pEngine->AddBuiltinClass( "oLong", pObjectClass, oLongMembers );
    ForthClassVocabulary* pOFloatClass = pEngine->AddBuiltinClass( "oFloat", pObjectClass, oFloatMembers );
    ForthClassVocabulary* pODoubleClass = pEngine->AddBuiltinClass( "oDouble", pObjectClass, oDoubleMembers );

    ForthClassVocabulary* pOThreadClass = pEngine->AddBuiltinClass( "oThread", pObjectClass, oThreadMembers );

    ForthClassVocabulary* pOInStreamClass = pEngine->AddBuiltinClass( "oInStream", pObjectClass, oInStreamMembers );
    ForthClassVocabulary* pOFileInStreamClass = pEngine->AddBuiltinClass( "oFileInStream", pOInStreamClass, oFileInStreamMembers );
    ForthClassVocabulary* pOConsoleInStreamClass = pEngine->AddBuiltinClass( "oConsoleInStream", pOFileInStreamClass, oConsoleInStreamMembers );

    ForthClassVocabulary* pOOutStreamClass = pEngine->AddBuiltinClass( "oOutStream", pObjectClass, oOutStreamMembers );
    gpOFileOutStreamClass = pEngine->AddBuiltinClass( "oFileOutStream", pOOutStreamClass, oFileOutStreamMembers );
    gpOStringOutStreamClass = pEngine->AddBuiltinClass( "oStringOutStream", pOOutStreamClass, oStringOutStreamMembers );
    gpOConsoleOutStreamClass = pEngine->AddBuiltinClass( "oConsoleOutStream", gpOFileOutStreamClass, oConsoleOutStreamMembers );
    gpOFunctionOutStreamClass = pEngine->AddBuiltinClass( "oFunctionOutStream", pOOutStreamClass, oFunctionOutStreamMembers );

    mpClassMethods = pClassClass->GetInterface( 0 )->GetMethods();
}

