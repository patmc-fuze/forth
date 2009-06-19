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

enum
{
    kClientCmdDisplayText,
    kClientCmdSendLine,
    kClientCmdStartLoad,
    kClientCmdGetChar,
};

enum
{
    kServerCmdProcessLine,
    kServerCmdProcessChar,
    kServerCmdPopStream         // sent when file is empty
};


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

protected:
    SOCKET  mSocket;
    bool    mDoAutoload;
};


