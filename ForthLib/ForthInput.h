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

    virtual char    *GetLine( const char *pPrompt ) = 0;
    virtual char    *GetBufferPointer( void );
    virtual char    *GetBufferBasePointer( void );
    virtual int     GetBufferLength( void );
    virtual void    SetBufferPointer( char *pBuff );
    virtual bool    IsInteractive( void ) = 0;
    virtual int     GetLineNumber( void );
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

    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive( void ) { return false; };
    virtual int     GetLineNumber( void );
protected:
    FILE        *mpInFile;
    int         mLineNumber;
};


class ForthConsoleInputStream : public ForthInputStream
{
public:
    ForthConsoleInputStream( int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ForthConsoleInputStream();

    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive( void ) { return true; };
protected:
};


class ForthBufferInputStream : public ForthInputStream
{
public:
    ForthBufferInputStream( char *pDataBuffer, int dataBufferLen, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ForthBufferInputStream();

    virtual char    *GetLine( const char *pPrompt );
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
    const char              *GetLine( const char *pPrompt );
    inline ForthInputStream *InputStream( void ) { return mpHead; };

    char                    *GetBufferPointer( void );
    char                    *GetBufferBasePointer( void );
    int                     GetBufferLength( void );
    void                    SetBufferPointer( char *pBuff );

protected:
    ForthInputStream        *mpHead;
};

#endif
