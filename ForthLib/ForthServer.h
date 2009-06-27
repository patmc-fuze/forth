#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthServer.h: Forth server support
//
//////////////////////////////////////////////////////////////////////
#include "Forth.h"
#include "ForthEngine.h"
#include "ForthShell.h"
#include "ForthInput.h"
#include <string.h>
#ifdef _WINDOWS
#include <winsock2.h>
#else
#include <sys/socket.h>
#define SOCKET  int
#endif

// commands sent to the client from the server
enum
{
    kClientMsgDisplayText,
    kClientMsgSendLine,
    kClientMsgStartLoad,
    kClientMsgGetChar,
    kClientMsgFileOpen,
    kClientMsgFileClose,
    kClientMsgFileSetPosition,
    kClientMsgFileRead,
    kClientMsgFileWrite,
    kClientMsgFileGetChar,
    kClientMsgFilePutChar,
    kClientMsgFileCheckEOF,
    kClientMsgFileGetPosition,
    kClientMsgFileGetLength,
    kClientMsgFileCheckExists,
    kClientMsgFileGetLine,
    kNumClientMsgs
};


// commands sent to the server from the client
enum
{
    kServerMsgProcessLine,
    kServerMsgProcessChar,
    kServerMsgPopStream,         // sent when file is empty
    kServerMsgFileOpResult,
    kNumServerMsgs
};

typedef struct
{
    int     messageType;
    int     numDataBytes;
} messageHeader;

extern int      readSocketData( SOCKET s, char *buf, int len );
extern void     writeSocketString( SOCKET s, int command, const char* str );
extern bool     readSocketResponse( SOCKET s, int* command, char* buffer, int bufferLen );

class ForthServerInputStream : public ForthInputStream
{
public:
    ForthServerInputStream( SOCKET inSocket, bool isFile = false, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ForthServerInputStream();

    virtual char    *GetLine( const char *pPrompt );

    bool            GetResponse( int* command );

    void            SendCommandString( int command, const char* str );

    virtual bool    IsInteractive( void );

    SOCKET          GetSocket();

protected:
    SOCKET      mSocket;
    bool        mIsFile;
};



class ForthServerShell : public ForthShell
{
public:
    ForthServerShell( bool doAutoload = true, ForthEngine *pEngine = NULL, ForthThread *pThread = NULL, int shellStackLongs = 1024 );
    virtual ~ForthServerShell();

    virtual int             Run( ForthInputStream *pInputStream );

    virtual bool            PushInputFile( const char *pFileName );

    void                    SendTextToClient( const char *pMessage );

    virtual char            GetChar();

    virtual FILE*           FileOpen( const char* filePath, const char* openMode );
    virtual int             FileClose( FILE* pFile );
    virtual int             FileSeek( FILE* pFile, int offset, int control );
    virtual int             FileRead( FILE* pFile, void* pDst, int numItems, int itemSize );
    virtual int             FileWrite( FILE* pFile, void* pDst, int numItems, int itemSize );
    virtual int             FileGetChar( FILE* pFile );
    virtual int             FilePutChar( FILE* pFile, int outChar );
    virtual int             FileAtEOF( FILE* pFile );
    virtual int             FileGetLength( FILE* pFile );
    virtual int             FileGetPosition( FILE* pFile );
    virtual char*           FileGetString( FILE* pFile, char* dstBuffer, int maxChars );
    virtual int             FilePutString( FILE* pFile, const char* pBuffer );

protected:
    SOCKET  mSocket;
    bool    mDoAutoload;
};


