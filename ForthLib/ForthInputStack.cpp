// ForthInputStack.cpp: implementation of the ForthInputStack class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthInputStack.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ForthInputStack::ForthInputStack()
{
    mpHead = NULL;
}

ForthInputStack::~ForthInputStack()
{
    // TBD: should we be closing file here?
    while ( mpHead != NULL ) {
        ForthInputStream *pNextStream = mpHead->pNext;
        delete [] mpHead->pBufferBase;
        if ( mpHead->pInFile != NULL ) {
            fclose( mpHead->pInFile );
        }
        delete mpHead;
        mpHead = pNextStream;
    }

}

void
ForthInputStack::PushInputStream( FILE *pInFile )
{
    ForthInputStream *pOldStream;
    
    pOldStream = mpHead;
    mpHead = new ForthInputStream;
    mpHead->pInFile = pInFile;
    mpHead->pNext = pOldStream;
    mpHead->pBufferBase = new char [ DEFAULT_INPUT_BUFFER_LEN ];
    mpHead->pBuffer = mpHead->pBufferBase;
    mpHead->bufferLen = DEFAULT_INPUT_BUFFER_LEN;
}


bool
ForthInputStack::PopInputStream( void )
{
    ForthInputStream *pNext;

    if ( mpHead->pInFile != NULL ) {
        fclose( mpHead->pInFile );
        mpHead->pInFile = NULL;
    }
    if ( mpHead->pNext == NULL ) {
        // all done!
        return true;
    } else {
        // this stream is done, pop out to previous input stream
        pNext = mpHead->pNext;
        delete [] mpHead->pBufferBase;
        delete mpHead;
        mpHead = pNext;
    }
    return false;
}


char *
ForthInputStack::GetLine( char *pPrompt )
{
    char *pBuffer, *pEndLine;

    if ( mpHead->pInFile != NULL ) {
        pBuffer = fgets( mpHead->pBufferBase, mpHead->bufferLen, mpHead->pInFile );
    } else {
        printf( "\nok> " );
        pBuffer = gets( mpHead->pBufferBase );
    }
    mpHead->pBuffer = mpHead->pBufferBase;

    // get rid of the trailing linefeed (if any)
    pEndLine = strchr( mpHead->pBuffer, '\n' );
    if ( pEndLine ) {
        *pEndLine = '\0';
    }

    return pBuffer;
}


char *
ForthInputStack::GetBufferPointer( void )
{
    return mpHead->pBuffer;
}


void
ForthInputStack::SetBufferPointer( char *pBuff )
{
    mpHead->pBuffer = pBuff;
}


void
ForthInputStack::Reset( void )
{
    // dump all nested input stream
    while ( mpHead->pNext != NULL ) {
        PopInputStream();
    }
}


