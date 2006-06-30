//////////////////////////////////////////////////////////////////////
//
// ForthShell.h: interface for the ForthShell class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FORTHSHELL_H__A1EA9AA2_9092_11D4_97DF_00B0D011B654__INCLUDED_)
#define AFX_FORTHSHELL_H__A1EA9AA2_9092_11D4_97DF_00B0D011B654__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"

class ForthInputStack;

//
// the ForthParseInfo class exists to support fast vocabulary searches.
// these searches are sped up by doing symbol comparisons using longwords
// instead of characters
//
#define TOKEN_BUFF_LONGS    (64)
#define TOKEN_BUFF_CHARS    (TOKEN_BUFF_LONGS << 2)

// These are the flags that can be passed to ForthEngine::ProcessToken
#define PARSE_FLAG_QUOTED_STRING        1
#define PARSE_FLAG_QUOTED_CHARACTER     2

class ForthParseInfo
{
public:
    ForthParseInfo( long *pBuffer, int numLongs );
    ~ForthParseInfo();

    // SetToken copies symbol to token buffer (if pSrc not NULL), sets the length byte,
    //   sets mNumLongs and pads end of token buffer with nuls to next longword boundary
    // call with no argument or NULL if token has already been copied to mpToken
    void            SetToken( const char *pSrc = NULL );

    inline int      GetFlags( void ) { return mFlags; };
    inline void     SetAllFlags( int flags ) { mFlags = flags; };
    inline void     SetFlag( int flag ) { mFlags |= flag; };

    inline char *   GetToken( void ) { return ((char *) mpToken) + 1; };
    inline long *   GetTokenAsLong( void ) { return mpToken; };
    inline int      GetTokenLength( void ) { return (int) (* ((char *) mpToken) ); };
    inline int      GetNumLongs( void ) { return mNumLongs; };

private:
    long *      mpToken;         // pointer to token buffer, first byte is strlen(token)
    int         mFlags;          // flags set by ForthShell::ParseToken for ForthEngine::ProcessToken
    int         mNumLongs;       // number of longwords for fast comparison algorithm
    int         mMaxChars;
};

class ForthShell  
{
public:

    // if the creator of a ForthShell passes in non-NULL engine and/or thread params,
    //   that creator is responsible for deleting the engine and/or thread
    ForthShell( ForthEngine *pEngine = NULL, ForthThread *pThread = NULL );
    virtual ~ForthShell();

    void                    PushInputStream( FILE *pInFile );
    bool                    PopInputStream( void );
    int                     Run( ForthInputStream *pStream );
    char *                  GetNextSimpleToken( void );

    void                    SetCommandLine( int argc, const char ** argv );
    void                    SetCommandLine( const char *pCmdLine );

    void                    SetEnvironmentVars( const char ** envp );

    inline ForthEngine *    GetEngine( void ) { return mpEngine; };
    inline ForthThread *    GetThread( void ) { return mpThread; };
    inline ForthInputStack * GetInput( void ) { return mpInput; };

    inline int              GetArgCount( void ) { return mNumArgs; };
    inline char *           GetArg( int argNum ) { return mpArgs[argNum]; };

protected:

    eForthResult            InterpretLine( void );

    // parse next token from input stream into mTokenBuff, padded with 0's up
    // to next longword boundary
    bool                    ParseToken( ForthParseInfo *pInfo );
    void                    ReportError( void );

    void                    DeleteEnvironmentVars();
    void                    DeleteCommandLine();

    ForthInputStack *       mpInput;
    ForthEngine *           mpEngine;
    ForthThread *           mpThread;
    long                    mTokenBuffer[ TOKEN_BUFF_LONGS ];

    bool                    mbCreatedEngine;
    int                     mNumArgs;
    char **                 mpArgs;
    int                     mNumEnvVars;
    char **                 mpEnvVarNames;
    char **                 mpEnvVarValues;
};

#endif // !defined(AFX_FORTHSHELL_H__A1EA9AA2_9092_11D4_97DF_00B0D011B654__INCLUDED_)
