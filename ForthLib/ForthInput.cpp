//////////////////////////////////////////////////////////////////////
//
// ForthInput.cpp: implementation of the ForthInputStack class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
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

	TRACE( "PushInputStream %s\n", pNewStream->GetType() );
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

	TRACE( "PopInputStream %s\n", (mpHead == NULL) ? "NULL" : mpHead->GetType() );

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
#if defined(LINUX)
        pEndLine = strchr( pBuffer, '\r' );
        if ( pEndLine )
        {
            *pEndLine = '\0';
        }
 #endif
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


int *
ForthInputStack::GetReadOffsetPointer( void )
{
    return (mpHead == NULL) ? NULL : mpHead->GetReadOffsetPointer();
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


int
ForthInputStack::GetReadOffset( void )
{
    return (mpHead == NULL) ? 0 : mpHead->GetReadOffset();
}


void
ForthInputStack::SetReadOffset( int offset )
{
    if (mpHead == NULL)
    {
        mpHead->SetReadOffset( offset );
    }
}


int
ForthInputStack::GetWriteOffset( void )
{
    return (mpHead == NULL) ? 0 : mpHead->GetWriteOffset();
}


void
ForthInputStack::SetWriteOffset( int offset )
{
    if (mpHead == NULL)
    {
        mpHead->SetWriteOffset( offset );
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
, mReadOffset(0)
, mWriteOffset(0)
{
    mpBufferBase = new char[bufferLen];
    mpBufferBase[0] = '\0';
    mpBufferBase[bufferLen - 1] = '\0';
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
    return mpBufferBase + mReadOffset;
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
    int offset = pBuff - mpBufferBase;
    if ( (offset < 0) || (offset >= mBufferLen) )
    {
        // TODO: report error!
    }
    else
    {
        mReadOffset = offset;
    }
}

int*
ForthInputStream::GetReadOffsetPointer( void )
{
    return &mReadOffset;
}


int
ForthInputStream::GetReadOffset( void )
{
    return mReadOffset;
}


void
ForthInputStream::SetReadOffset( int offset )
{
    if ( (offset < 0) || (offset >= mBufferLen) )
    {
        // TODO: report error!
    }
    else
    {
        mReadOffset = offset;
    }
    mReadOffset = offset;
}


int
ForthInputStream::GetWriteOffset( void )
{
    return mWriteOffset;
}


void
ForthInputStream::SetWriteOffset( int offset )
{
    if ( (offset < 0) || (offset >= mBufferLen) )
    {
        // TODO: report error!
    }
    else
    {
        mWriteOffset = offset;
    }
    mWriteOffset = offset;
}


int
ForthInputStream::GetLineNumber( void )
{
    return -1;
}

const char*
ForthInputStream::GetType( void )
{
    return "Base";
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


    mReadOffset = 0;
    mpBufferBase[ mBufferLen - 1 ] = '\0';
    mWriteOffset = strlen( mpBufferBase );
    mLineNumber++;
    return pBuffer;
}


int
ForthFileInputStream::GetLineNumber( void )
{
    return mLineNumber;
}


const char*
ForthFileInputStream::GetType( void )
{
    return "File";
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

    mReadOffset = 0;
    const char* pEnd = (const char*) memchr( pBuffer, '\0', mBufferLen );
    mWriteOffset = (pEnd == NULL) ? (mBufferLen - 1) : (pEnd - pBuffer);
    return pBuffer;
}


const char*
ForthConsoleInputStream::GetType( void )
{
    return "Console";
}


//////////////////////////////////////////////////////////////////////
////
///
//                     ForthBufferInputStream
// 

ForthBufferInputStream::ForthBufferInputStream( const char *pDataBuffer, int dataBufferLen, bool isInteractive, int bufferLen )
: ForthInputStream(bufferLen)
, mIsInteractive(isInteractive)
{
	mpDataBufferBase = new char[ dataBufferLen ];
	memcpy( mpDataBufferBase, pDataBuffer, dataBufferLen );
	mpDataBuffer = mpDataBufferBase;
	mpDataBufferLimit = mpDataBuffer + dataBufferLen;
}

ForthBufferInputStream::~ForthBufferInputStream()
{
	delete [] mpDataBufferBase;
}


char *
ForthBufferInputStream::GetLine( const char *pPrompt )
{
    char *pBuffer = NULL;
    char *pDst, c;

    if ( mpDataBuffer < mpDataBufferLimit )
    {
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
		*pDst = '\0';

        mReadOffset = 0;
        mWriteOffset = (pDst - mpBufferBase);
		pBuffer = mpBufferBase;
    }

    return pBuffer;
}


const char*
ForthBufferInputStream::GetType( void )
{
    return "Buffer";
}


