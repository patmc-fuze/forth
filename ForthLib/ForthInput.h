// ForthInput.h: interface for the ForthInputStack class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_FORTHINPUT_H_INCLUDED_)
#define _FORTHINPUT_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"

class ForthInputStack;

class ForthInputStream
{
public:
    ForthInputStream( int bufferLen );
    virtual ~ForthInputStream();

    virtual char    *GetLine( char *pPrompt ) = 0;
    virtual char    *GetBufferPointer( void );
    virtual void    SetBufferPointer( char *pBuff );
    virtual bool    IsInteractive( void ) = 0;

    friend ForthInputStack;

protected:
    ForthInputStream    *mpNext;
    char                *mpBuffer;
    char                *mpBufferBase;
    int                 mBufferLen;
};


class ForthFileInputStream : public ForthInputStream
{
public:
    ForthFileInputStream( FILE *pInFile, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ForthFileInputStream();

    virtual char    *GetLine( char *pPrompt );
    virtual bool    IsInteractive( void ) { return false; };
protected:
    FILE        *mpInFile;
};


class ForthConsoleInputStream : public ForthInputStream
{
public:
    ForthConsoleInputStream( int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ForthConsoleInputStream();

    virtual char    *GetLine( char *pPrompt );
    virtual bool    IsInteractive( void ) { return true; };
protected:
};


class ForthBufferInputStream : public ForthInputStream
{
public:
    ForthBufferInputStream( char *pDataBuffer, int dataBufferLen, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ForthBufferInputStream();

    virtual char    *GetLine( char *pPrompt );
    virtual bool    IsInteractive( void ) { return false; };
protected:
    char    *mpDataBuffer;
    char    *mpDataBufferLimit;
};


class ForthInputStack  
{
public:
    ForthInputStack();
    virtual ~ForthInputStack();

    void                    PushInputStream( ForthInputStream *pStream );
    bool                    PopInputStream();
    void                    Reset( void );
    char                    *GetLine( char *pPrompt );
    inline ForthInputStream *InputStream( void ) { return mpHead; };

    char                    *GetBufferPointer( void );
    void                    SetBufferPointer( char *pBuff );

protected:
    ForthInputStream        *mpHead;
};

#endif
