//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.cpp: builtin classes
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthBuiltinClasses.h"

#define METHOD_RETURN \
    SET_TPM( (long *) RPOP ); \
    SET_TPD( (long *) RPOP )

#define METHOD( NAME, VALUE  )          { NAME, (ulong) VALUE, NATIVE_TYPE_TO_CODE( kDTIsMethod, kBaseTypeVoid ) }
#define METHOD_RET( NAME, VAL, RVAL )   { NAME, (ulong) VAL, RVAL }
#define MEMBER_VAR( NAME, TYPE )        { NAME, 0, (ulong) TYPE }
#define MEMBER_ARRAY( NAME, TYPE, NUM ) { NAME, NUM, (ulong) (TYPE | kDTIsArray) }

#define END_MEMBERS { NULL, 0, 0 }
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
        long* pClassObject = (GET_TPM) - 1;
        SPUSH( (long) pClassObject );
        SPUSH( (long) ForthTypesManager::GetInstance()->GetClassMethods() );
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
    FORTHOP( classNewMethod )
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
        SPUSH( (long) pClassVocab->BaseVocabulary() );
        SPUSH( (long) (GET_TPM) );
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
        long foundInterface = NULL;
	    long numInterfaces = pClassVocab->GetNumInterfaces();
	    for ( int i = 0; i < numInterfaces; i++ )
	    {
            ForthInterface* pInterface = pClassVocab->GetInterface( i );
            if ( strcmp( pInterface->GetDefiningClass()->GetName(), pName ) == 0 )
            {
                foundInterface = (long) pInterface;
                break;
		    }
	    }
        SPUSH( (long)(GET_TPD) );
        SPUSH( foundInterface );
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
        METHOD_RET( "new",              classNewMethod,             OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIObject) ),
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


    FORTHOP( rcObjectAddrefMethod )
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
        METHOD(     "addref",               rcObjectAddrefMethod ),
        METHOD(     "release",              rcObjectReleaseMethod ),
        MEMBER_VAR( "refCount",             NATIVE_TYPE_TO_CODE(0, kBaseTypeInt) ),
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
    mpClassMethods = pClassClass->GetInterface( 0 )->GetMethods();
}

