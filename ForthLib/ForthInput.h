#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthInput.h: interface for the ForthInputStack class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

#ifndef DEFAULT_INPUT_BUFFER_LEN
#define DEFAULT_INPUT_BUFFER_LEN MAX_STRING_SIZE
#endif

class ForthInputStack;

class ForthInputStream
{
public:
    ForthInputStream( int bufferLen );
    virtual ~ForthInputStream();

    virtual char    *GetLine( const char *pPrompt ) = 0;
    virtual char    *GetBufferPointer( void );
    virtual char    *GetBufferBasePointer( void );
    virtual const char    *GetReportedBufferBasePointer( void );
    virtual int     *GetReadOffsetPointer( void );
    virtual int     GetReadOffset( void );
    virtual void    SetReadOffset( int offset );
    virtual int     GetWriteOffset( void );
    virtual void    SetWriteOffset( int offset );
    
    virtual int     GetBufferLength( void );
    virtual void    SetBufferPointer( char *pBuff );
    virtual bool    IsInteractive( void ) = 0;
    virtual int     GetLineNumber( void );
	virtual const char* GetType( void );

    friend class ForthInputStack;

protected:
    ForthInputStream    *mpNext;
    int                 mReadOffset;
    int                 mWriteOffset;
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
	virtual const char* GetType( void );
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
	virtual const char* GetType( void );
protected:
};


class ForthBufferInputStream : public ForthInputStream
{
public:
    ForthBufferInputStream( const char *pDataBuffer, int dataBufferLen, bool isInteractive = true, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ForthBufferInputStream();

    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive( void ) { return mIsInteractive; };
	virtual const char* GetType( void );
    virtual const char* GetReportedBufferBasePointer( void );
protected:
    const char      *mpSourceBuffer;
    char			*mpDataBuffer;
    char			*mpDataBufferBase;
    char			*mpDataBufferLimit;
	bool			mIsInteractive;
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
    int                     *GetReadOffsetPointer( void );
    int                     GetBufferLength( void );
    void                    SetBufferPointer( char *pBuff );
    int                     GetReadOffset( void );
    void                    SetReadOffset( int );
    int                     GetWriteOffset( void );
    void                    SetWriteOffset( int offset );

protected:
    ForthInputStream        *mpHead;
};

