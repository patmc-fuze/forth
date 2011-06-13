//////////////////////////////////////////////////////////////////////
//
// ForthServer.cpp: Forth server support
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ForthServer.h"
#include "ForthPipe.h"
#include "ForthMessages.h"
#include "ForthExtension.h"

#ifdef DEBUG_WITH_NDS_EMULATOR
#include <nds.h>
#endif

namespace
{
    void consoleOutToClient( ForthCoreState   *pCore,
                             const char       *pMessage )
    {
    #ifdef DEBUG_WITH_NDS_EMULATOR
	    iprintf( "%s", pMessage );
    #else
        ForthServerShell* pShell = (ForthServerShell *) (((ForthEngine *)(pCore->pEngine))->GetShell());
        pShell->SendTextToClient( pMessage );
    #endif
    }

    FILE* fileOpen( const char* pPath, const char* pAccess )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileOpen( pPath, pAccess );
    }

    int fileClose( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileClose( pFile );
    }

    size_t fileRead( void* data, size_t itemSize, size_t numItems, FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileRead( pFile, data, itemSize, numItems );
    }

    size_t fileWrite( const void* data, size_t itemSize, size_t numItems, FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileWrite( pFile, data, itemSize, numItems );
    }

    int fileGetChar( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileGetChar( pFile );
    }

    int filePutChar( int val, FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FilePutChar( pFile, val );
    }

    int fileAtEnd( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileAtEOF( pFile );
    }

    int fileExists( const char* pPath )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileCheckExists( pPath );
    }

    int fileSeek( FILE* pFile, long offset, int ctrl )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileSeek( pFile, offset, ctrl );
    }

    long fileTell( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileGetPosition( pFile );
    }

    int fileGetLength( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileGetLength( pFile );
    }

    char* fileGetString( char* buffer, int bufferLength, FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileGetString( pFile, buffer, bufferLength );
    }

    int filePutString( const char* buffer, FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FilePutString( pFile, buffer );
    }


};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ForthServerInputStream::ForthServerInputStream( ForthPipe* pPipe, bool isFile, int bufferLen )
:   ForthInputStream( bufferLen )
,   mpMsgPipe( pPipe )
,   mIsFile( isFile )
{
}

ForthServerInputStream::~ForthServerInputStream()
{
}

char* ForthServerInputStream::GetLine( const char *pPrompt )
{
    int msgType, msgLen;
    char* result = NULL;

    mpBuffer = mpBufferBase;

    mpMsgPipe->StartMessage( kClientMsgSendLine );
    mpMsgPipe->WriteString( mIsFile ? NULL : pPrompt );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    switch ( msgType )
    {
    case kServerMsgProcessLine:
        {
            const char* pSrcLine;
            int srcLen = 0;
            if ( mpMsgPipe->ReadCountedData( pSrcLine, srcLen ) )
            {
                if ( srcLen != 0 )
                {
#ifdef PIPE_SPEW
                    printf( "line = '%s'\n", pSrcLine );
#endif
                    memcpy( mpBuffer, pSrcLine, srcLen );
                    result = mpBuffer;
                }
            }
            else
            {
                // TBD: report error
            }
        }
        break;

    case kServerMsgPopStream:
        break;

    default:
        // TBD: report unexpected message type error
        printf( "ForthServerShell::GetLine unexpected message type %d\n", msgType );
        break;
    }

    return result;
}

bool ForthServerInputStream::IsInteractive( void )
{
    return !mIsFile;
}

ForthPipe* ForthServerInputStream::GetPipe()
{
    return mpMsgPipe;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ForthServerShell::ForthServerShell( bool doAutoload, ForthEngine *pEngine, ForthExtension *pExtension, ForthThread *pThread, int shellStackLongs )
:   ForthShell( pEngine, pExtension, pThread, shellStackLongs )
,   mDoAutoload( doAutoload )
,   mpMsgPipe( NULL )
{
    mFileInterface.fileOpen = fileOpen;
    mFileInterface.fileClose = fileClose;
    mFileInterface.fileRead = fileRead;
    mFileInterface.fileWrite = fileWrite;
    mFileInterface.fileGetChar = fileGetChar;
    mFileInterface.filePutChar = filePutChar;
    mFileInterface.fileAtEnd = fileAtEnd;
    mFileInterface.fileExists = fileExists;
    mFileInterface.fileSeek = fileSeek;
    mFileInterface.fileTell = fileTell;
    mFileInterface.fileGetLength = fileGetLength;
    mFileInterface.fileGetString = fileGetString;
    mFileInterface.filePutString = filePutString;
}

ForthServerShell::~ForthServerShell()
{
}

int ForthServerShell::Run( ForthInputStream *pInputStream )
{
    ForthServerInputStream* pStream = (ForthServerInputStream *) pInputStream;
    mpMsgPipe = pStream->GetPipe();

    const char *pBuffer;
    int retVal = 0;
    bool bQuit = false;
    eForthResult result = kResultOk;
    bool bInteractiveMode = pStream->IsInteractive();

    mpEngine->SetConsoleOut( consoleOutToClient, NULL );
	mpEngine->ResetConsoleOut( mpEngine->GetCoreState() );
    mpInput->PushInputStream( pStream );

    if ( mDoAutoload )
    {
        mpEngine->PushInputFile( "forth_autoload.txt" );
    }

    while ( !bQuit )
    {

        // try to fetch a line from current stream
        pBuffer = mpInput->GetLine( mpEngine->GetFastMode() ? "turbo>" : "ok>" );
        if ( pBuffer == NULL )
        {
            bQuit = PopInputStream();
        }

        if ( !bQuit )
        {

            result = InterpretLine();

            switch( result )
            {

            case kResultExitShell:
                // users has typed "bye", exit the shell
                bQuit = true;
                retVal = 0;
                mpMsgPipe->StartMessage( kClientMsgGoAway );
                mpMsgPipe->SendMessage();
                break;

            case kResultError:
                // an error has occured, empty input stream stack
                // TBD
                if ( !bInteractiveMode )
                {
                    bQuit = true;
                }
                else
                {
                    // TBD: dump all but outermost input stream
                }
                retVal = 0;
                break;

            case kResultFatalError:
                // a fatal error has occured, exit the shell
                bQuit = true;
                retVal = 1;
                break;

            default:
                break;
            }

        }
    }

    return retVal;
}

bool ForthServerShell::PushInputFile( const char *pFileName )
{
    int result = 0;
    int msgType, msgLen;

    mpMsgPipe->StartMessage( kClientMsgStartLoad );
    mpMsgPipe->WriteString( pFileName );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgStartLoadResult )
    {
        mpMsgPipe->ReadInt( result );
        if ( result )
        {
            mpInput->PushInputStream( new ForthServerInputStream( mpMsgPipe, true ) );
        }
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::PushInputFile unexpected message type %d\n", msgType );
    }

    return (result != 0);
}

bool
ForthServerShell::PopInputStream( void )
{
    mpMsgPipe->StartMessage( kClientMsgPopStream );
    mpMsgPipe->SendMessage();

    return mpInput->PopInputStream();
}


void ForthServerShell::SendTextToClient( const char *pMessage )
{
    mpMsgPipe->StartMessage( kClientMsgDisplayText );
    mpMsgPipe->WriteString( pMessage );
    mpMsgPipe->SendMessage();
}

char ForthServerShell::GetChar()
{
    int c;
    int msgType, msgLen;

    mpMsgPipe->StartMessage( kClientMsgGetChar );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgProcessChar )
    {
        mpMsgPipe->ReadInt( c );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::GetChar unexpected message type %d\n", msgType );
    }
    return (int) c;
}

FILE*
ForthServerShell::FileOpen( const char* filePath, const char* openMode )
{
    FILE* pFile = NULL;
    int c;
    int msgType, msgLen;

    mpMsgPipe->StartMessage( kClientMsgFileOpen );
    mpMsgPipe->WriteString( filePath );
    mpMsgPipe->WriteString( openMode );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( c );
        pFile = (FILE *) c;
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FileOpen unexpected message type %d\n", msgType );
    }
    return pFile;
}

int
ForthServerShell::FileClose( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileClose );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FileClose unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileSeek( FILE* pFile, int offset, int control )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileSetPosition );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->WriteInt( offset );
    mpMsgPipe->WriteInt( control );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FileSeek unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileRead( FILE* pFile, void* pDst, int itemSize, int numItems )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileRead );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->WriteInt( itemSize );
    mpMsgPipe->WriteInt( numItems );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileReadResult )
    {
        mpMsgPipe->ReadInt( result );
        if ( result > 0 )
        {
            int numBytes;
            const char* pData;
            mpMsgPipe->ReadCountedData( pData, numBytes );
            if ( numBytes == (result * itemSize) )
            {
                memcpy( pDst, pData, numBytes );
            }
            else
            {
                // TBD: report error
                printf( "ForthServerShell::FileRead unexpected data size %d != itemsRead %d * itemSize %d\n",
                            numBytes, result, itemSize );
            }
        }
        else
        {
            // TBD: report error
            printf( "ForthServerShell::FileRead returned %d \n", result );
        }
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FileRead unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileWrite( FILE* pFile, const void* pSrc, int itemSize, int numItems ) 
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileWrite );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->WriteInt( itemSize );
    mpMsgPipe->WriteInt( numItems );
    int numBytes = numItems * itemSize;
    mpMsgPipe->WriteCountedData( pSrc, numBytes );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FileWrite unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileGetChar( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileGetChar );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FileGetChar unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FilePutChar( FILE* pFile, int outChar )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileGetChar );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->WriteInt( outChar );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FilePutChar unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileAtEOF( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileCheckEOF );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FileAtEOF unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileGetLength( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileGetLength );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FileGetLength unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileCheckExists( const char* pFilename )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileCheckExists );
    mpMsgPipe->WriteString( pFilename );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FileCheckExists unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileGetPosition( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileGetPosition );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FileGetPosition unexpected message type %d\n", msgType );
    }
    return result;
}

char*
ForthServerShell::FileGetString( FILE* pFile, char* pBuffer, int maxChars )
{
    int msgType, msgLen;
    char* result = NULL;

    if ( maxChars <= 0 )
    {
        return NULL;
    }
    mpMsgPipe->StartMessage( kClientMsgFileGetString );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->WriteInt( maxChars );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileGetStringResult )
    {
        int numBytes;
        const char* pData;
        mpMsgPipe->ReadCountedData( pData, numBytes );
        if ( (numBytes != 0) && (numBytes <= maxChars) )
        {
            memcpy( pBuffer, pData, numBytes );
            result = pBuffer;
        }
        else
        {
            // TBD: report error
            printf( "ForthServerShell::FileGetString unexpected data size %d\n", numBytes );
        }
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FileGetString unexpected message type %d\n", msgType );
    }
    return result;
}


int
ForthServerShell::FilePutString( FILE* pFile, const char* pBuffer )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFilePutString );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->WriteString( pBuffer );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::FilePutString unexpected message type %d\n", msgType );
    }
    return result;
}

