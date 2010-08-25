//////////////////////////////////////////////////////////////////////
//
// ForthInput.cpp: implementation of the ForthInputStack class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthInput.h"

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthInputStack
// 

ForthInputStack::ForthInputStack()
{
    mpHead = NULL;
}

ForthInputStack::~ForthInputStack()
{
    // TBD: should we be closing file here?
    while ( mpHead != NULL )
    {
        ForthInputStream *pNextStream = mpHead->mpNext;
        delete mpHead;
        mpHead = pNextStream;
    }

}

void
ForthInputStack::PushInputStream( ForthInputStream *pNewStream )
{
    ForthInputStream *pOldStream;
    
    pOldStream = mpHead;
    mpHead = pNewStream;
    mpHead->mpNext = pOldStream;
}


bool
ForthInputStack::PopInputStream( void )
{
    ForthInputStream *pNext;

    if ( (mpHead == NULL) || (mpHead->mpNext == NULL) )
    {
        // all done!
        return true;
    }

    pNext = mpHead->mpNext;
    delete mpHead;
    mpHead = pNext;

    return false;
}


const char *
ForthInputStack::GetLine( const char *pPrompt )
{
    char *pBuffer, *pEndLine;

    if ( mpHead == NULL )
    {
        return NULL;
    }

    pBuffer = mpHead->GetLine( pPrompt );

    if ( pBuffer != NULL )
    {
        // get rid of the trailing linefeed (if any)
        pEndLine = strchr( pBuffer, '\n' );
        if ( pEndLine )
        {
            *pEndLine = '\0';
        }
    }

    return pBuffer;
}


char *
ForthInputStack::GetBufferPointer( void )
{
    return (mpHead == NULL) ? NULL : mpHead->GetBufferPointer();
}


char *
ForthInputStack::GetBufferBasePointer( void )
{
    return (mpHead == NULL) ? NULL : mpHead->GetBufferBasePointer();
}


int
ForthInputStack::GetBufferLength( void )
{
    return (mpHead == NULL) ? 0 : mpHead->GetBufferLength();
}


void
ForthInputStack::SetBufferPointer( char *pBuff )
{
    if ( mpHead != NULL )
    {
        mpHead->SetBufferPointer( pBuff );
    }
}


void
ForthInputStack::Reset( void )
{
    // dump all nested input stream
    if ( mpHead != NULL )
    {
        while ( mpHead->mpNext != NULL )
        {
            PopInputStream();
        }
    }
}


//////////////////////////////////////////////////////////////////////
////
///
//                     ForthInputStream
// 

ForthInputStream::ForthInputStream( int bufferLen )
: mpNext(NULL)
, mBufferLen(bufferLen)
{
    mpBufferBase = new char[bufferLen];
    mpBuffer = mpBufferBase;
    mpBuffer[0] = '\0';
}


ForthInputStream::~ForthInputStream()
{
    if ( mpBufferBase != NULL )
    {
        delete [] mpBufferBase;
    }
}


char *
ForthInputStream::GetBufferPointer( void )
{
    return mpBuffer;
}


char *
ForthInputStream::GetBufferBasePointer( void )
{
    return mpBufferBase;
}


int
ForthInputStream::GetBufferLength( void )
{
    return mBufferLen;
}


void
ForthInputStream::SetBufferPointer( char *pBuff )
{
    mpBuffer = pBuff;
}

int
ForthInputStream::GetLineNumber( void )
{
    return -1;
}

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthFileInputStream
// 

ForthFileInputStream::ForthFileInputStream( FILE *pInFile, int bufferLen )
: ForthInputStream(bufferLen)
, mpInFile( pInFile )
, mLineNumber( 0 )
{
}

ForthFileInputStream::~ForthFileInputStream()
{
    // TBD: should we be closing file here?
    if ( mpInFile != NULL )
    {
        fclose( mpInFile );
    }
}


char *
ForthFileInputStream::GetLine( const char *pPrompt )
{
    char *pBuffer;

    pBuffer = fgets( mpBufferBase, mBufferLen, mpInFile );

    mpBuffer = mpBufferBase;
    mLineNumber++;
    return pBuffer;
}


int
ForthFileInputStream::GetLineNumber( void )
{
    return mLineNumber;
}


//////////////////////////////////////////////////////////////////////
////
///
//                     ForthConsoleInputStream
// 

ForthConsoleInputStream::ForthConsoleInputStream( int bufferLen )
: ForthInputStream(bufferLen)
{
}

ForthConsoleInputStream::~ForthConsoleInputStream()
{
}


char *
ForthConsoleInputStream::GetLine( const char *pPrompt )
{
    char *pBuffer;

    printf( "\n%s ", pPrompt );
    pBuffer = gets( mpBufferBase );

    mpBuffer = mpBufferBase;
    return pBuffer;
}


//////////////////////////////////////////////////////////////////////
////
///
//                     ForthBufferInputStream
// 

ForthBufferInputStream::ForthBufferInputStream( const char *pDataBuffer, int dataBufferLen, int bufferLen )
: ForthInputStream(bufferLen)
, mpDataBufferLimit( pDataBuffer + dataBufferLen )
, mpDataBuffer( pDataBuffer )
{
}

ForthBufferInputStream::~ForthBufferInputStream()
{
}


char *
ForthBufferInputStream::GetLine( const char *pPrompt )
{
    const char *pBuffer;
    char *pDst, c;

    if ( mpDataBuffer >= mpDataBufferLimit )
    {
        return NULL;
    }

    pBuffer = mpDataBuffer;
    pDst = mpBufferBase;
    while ( mpDataBuffer < mpDataBufferLimit )
    {
        c = *mpDataBuffer++;
        if ( (c == '\0') || (c == '\n') || (c == '\r') )
        {
            break;
        } 
        else
        {
            *pDst++ = c;
        }
    }
    *pDst++ = '\0';

    mpBuffer = mpBufferBase;
    return mpBuffer;
}

