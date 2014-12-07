//////////////////////////////////////////////////////////////////////
//
// ForthInput.cpp: implementation of the ForthInputStack class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ForthInput.h"
#include "ForthEngine.h"
#include "ForthBlockFileManager.h"

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
    // TODO: should we be closing file here?
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

    *(ForthEngine::GetInstance()->GetBlockPtr()) = mpHead->GetBlockNumber();

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

    *(ForthEngine::GetInstance()->GetBlockPtr()) = mpHead->GetBlockNumber();

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
    // dump all nested input streams
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


const char *
ForthInputStream::GetReportedBufferBasePointer( void )
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

const char*
ForthInputStream::GetName( void )
{
    return "mysteriousStream";
}

void
ForthInputStream::SeekToLineEnd()
{
    mReadOffset = mWriteOffset;
}

long
ForthInputStream::GetBlockNumber()
{
    return 0;
}


//////////////////////////////////////////////////////////////////////
////
///
//                     ForthFileInputStream
// 

ForthFileInputStream::ForthFileInputStream( FILE *pInFile, const char *pFilename, int bufferLen )
: ForthInputStream(bufferLen)
, mpInFile( pInFile )
, mLineNumber( 0 )
, mLineStartOffset( 0 )
{
    mpName = new char[strlen(pFilename) + 1];
    strcpy( mpName, pFilename );
}

ForthFileInputStream::~ForthFileInputStream()
{
    // TODO: should we be closing file here?
    if ( mpInFile != NULL )
    {
        fclose( mpInFile );
    }
    delete [] mpName;
}

const char*
ForthFileInputStream::GetName( void )
{
    return mpName;
}


char *
ForthFileInputStream::GetLine( const char *pPrompt )
{
    char *pBuffer;

    mLineStartOffset = ftell( mpInFile );

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

int
ForthFileInputStream::GetSourceID()
{
    return (int) mpInFile;
}

long*
ForthFileInputStream::GetInputState()
{
    // save-input items:
    //  0   5
    //  1   this pointer
    //  2   lineNumber
    //  3   readOffset
    //  4   writeOffset (count of valid bytes in buffer)
    //  5   lineStartOffset

    long* pState = &(mState[0]);
    pState[0] = 5;
    pState[1] = (int)this;
    pState[2] = mLineNumber;
    pState[3] = mReadOffset;
    pState[4] = mWriteOffset;
    pState[5] = mLineStartOffset;
    
    return pState;
}

bool
ForthFileInputStream::SetInputState( long* pState )
{
    if ( pState[0] != 5 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (long)this )
    {
        // TODO: report restore-input error - input object mismatch
        return false;
    }
    if ( fseek( mpInFile, pState[5], SEEK_SET )  != 0 )
    {
        // TODO: report restore-input error - error seeking to beginning of line
        return false;
    }
    GetLine(NULL);
    if ( mWriteOffset != pState[4] )
    {
        // TODO: report restore-input error - line length doesn't match save-input value
        return false;
    }
    mLineNumber = pState[2];
    mReadOffset = pState[3];
    return true;
}

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthConsoleInputStream
// 

ForthConsoleInputStream::ForthConsoleInputStream( int bufferLen )
: ForthInputStream(bufferLen)
, mLineNumber(0)
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
    mLineNumber++;
    return pBuffer;
}


const char*
ForthConsoleInputStream::GetType( void )
{
    return "Console";
}

int
ForthConsoleInputStream::GetSourceID()
{
    return 0;
}

long*
ForthConsoleInputStream::GetInputState()
{
    // save-input items:
    //  0   4
    //  1   this pointer
    //  2   lineNumber
    //  3   readOffset
    //  4   writeOffset (count of valid bytes in buffer)

    long* pState = &(mState[0]);
    pState[0] = 4;
    pState[1] = (int)this;
    pState[2] = mLineNumber;
    pState[3] = mReadOffset;
    pState[4] = mWriteOffset;
    
    return &(mState[0]);
}

bool
ForthConsoleInputStream::SetInputState( long* pState )
{
    if ( pState[0] != 4 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (long)this )
    {
        // TODO: report restore-input error - input object mismatch
        return false;
    }
    if ( pState[2] != mLineNumber )
    {
        // TODO: report restore-input error - line number mismatch
        return false;
    }
    if ( mWriteOffset != pState[4] )
    {
        // TODO: report restore-input error - line length doesn't match save-input value
        return false;
    }
    mReadOffset = pState[3];
    return true;
}



//////////////////////////////////////////////////////////////////////
////
///
//                     ForthBufferInputStream
// 

// to be compliant with the ANSI Forth standard we have to:
//
// 1) allow the original input buffer to not be null terminated
// 2) return the original input buffer pointer when queried
//
// so we make a copy of the original buffer with a null terminator,
// but we return the original buffer pointer when queried

int ForthBufferInputStream::sInstanceNumber = 0;    // used for checking consistency in restore-input

ForthBufferInputStream::ForthBufferInputStream( const char *pSourceBuffer, int sourceBufferLen, bool isInteractive, int bufferLen )
: ForthInputStream(bufferLen)
, mIsInteractive(isInteractive)
, mpSourceBuffer(pSourceBuffer)
{
	mpDataBufferBase = new char[ sourceBufferLen + 1];
	memcpy( mpDataBufferBase, pSourceBuffer, sourceBufferLen );
    mpDataBufferBase[ sourceBufferLen ] = '\0';
	mpDataBuffer = mpDataBufferBase;
	mpDataBufferLimit = mpDataBuffer + sourceBufferLen;
    mWriteOffset = sourceBufferLen;
    mInstanceNumber = sInstanceNumber++;
}

ForthBufferInputStream::~ForthBufferInputStream()
{
	delete [] mpDataBufferBase;
}

int
ForthBufferInputStream::GetSourceID()
{
    return -1;
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


const char *
ForthBufferInputStream::GetReportedBufferBasePointer( void )
{
    return mpSourceBuffer;
}

long*
ForthBufferInputStream::GetInputState()
{
    // save-input items:
    //  0   4
    //  1   this pointer
    //  2   mInstanceNumber
    //  3   readOffset
    //  4   writeOffset (count of valid bytes in buffer)

    long* pState = &(mState[0]);
    pState[0] = 4;
    pState[1] = (int)this;
    pState[2] = mInstanceNumber;
    pState[3] = mReadOffset;
    pState[4] = mWriteOffset;
    
    return pState;
}

bool
ForthBufferInputStream::SetInputState( long* pState )
{
    if ( pState[0] != 4 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (long)this )
    {
        // TODO: report restore-input error - input object mismatch
        return false;
    }
    if ( pState[2] != mInstanceNumber )
    {
        // TODO: report restore-input error - instance number mismatch
        return false;
    }
    if ( mWriteOffset != pState[4] )
    {
        // TODO: report restore-input error - line length doesn't match save-input value
        return false;
    }
    mReadOffset = pState[3];
    return true;
}

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthBlockInputStream
// 

ForthBlockInputStream::ForthBlockInputStream( unsigned int firstBlock, unsigned int lastBlock )
:   ForthInputStream( BYTES_PER_BLOCK + 1 )
,   mCurrentBlock( firstBlock )
,   mLastBlock( lastBlock )
{
    mReadOffset = 0;
    mWriteOffset = BYTES_PER_BLOCK;
    ReadBlock();
}

ForthBlockInputStream::~ForthBlockInputStream()
{
}

int
ForthBlockInputStream::GetSourceID()
{
    return -1;
}


char *
ForthBlockInputStream::GetLine( const char *pPrompt )
{
    // TODO!
    char* pBuffer = NULL;
    if ( mCurrentBlock < mLastBlock )
    {
        mCurrentBlock++;
        if ( ReadBlock() )
        {
            pBuffer = mpBufferBase;
        }
    }
        
    return pBuffer;
}


const char*
ForthBlockInputStream::GetType( void )
{
    return "Block";
}


void
ForthBlockInputStream::SeekToLineEnd()
{
    // TODO! this 
    mReadOffset = (mReadOffset + 64) & 0xFFFFFFC0;
    if ( mReadOffset > BYTES_PER_BLOCK )
    {
        mReadOffset = BYTES_PER_BLOCK;
    }
}


long*
ForthBlockInputStream::GetInputState()
{
    // save-input items:
    //  0   3
    //  1   this pointer
    //  2   blockNumber
    //  3   readOffset

    long* pState = &(mState[0]);
    pState[0] = 3;
    pState[1] = (int)this;
    pState[2] = mCurrentBlock;
    pState[3] = mReadOffset;
    
    return pState;
}

bool
ForthBlockInputStream::SetInputState( long* pState )
{
    if ( pState[0] != 4 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (long)this )
    {
        // TODO: report restore-input error - input object mismatch
        return false;
    }
    if ( pState[2] != mCurrentBlock )
    {
        // TODO: report restore-input error - wrong block
        return false;
    }
    mReadOffset = pState[3];
    return true;
}

long
ForthBlockInputStream::GetBlockNumber()
{
    return mCurrentBlock;
}

bool
ForthBlockInputStream::ReadBlock()
{
    bool success = true;
    ForthEngine* pEngine = ForthEngine::GetInstance();
    ForthBlockFileManager* pBlockManager = pEngine->GetBlockFileManager();
    FILE * pInFile = pBlockManager->OpenBlockFile( false );
    if ( pInFile == NULL )
    {
        pEngine->SetError( kForthErrorIO, "BlockInputStream - failed to open block file" );
        success = false;
    }
    else
    {
        fseek( pInFile, BYTES_PER_BLOCK * mCurrentBlock, SEEK_SET );
        int numRead = fread( mpBufferBase, BYTES_PER_BLOCK, 1, pInFile );
        if ( numRead != 1 )
        {
            pEngine->SetError( kForthErrorIO, "BlockInputStream - failed to read block file" );
            success = false;
        }
        fclose( pInFile );
    }
    return success;
}

