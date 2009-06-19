#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthShell.h: interface for the ForthShell class.
//
//////////////////////////////////////////////////////////////////////

#include "ForthEngine.h"
#include "ForthInput.h"

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
#define PARSE_FLAG_HAS_PERIOD           4

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

typedef enum
{
   kShellTagNothing  = 0,
   kShellTagDo       = 1,
   kShellTagBegin    = 2,
   kShellTagWhile    = 3,
   kShellTagCase     = 4,
   kShellTagIf       = 5,
   kShellTagElse     = 6,
   kShellTagParen    = 7,
   kShellTagString   = 8,
   kShellTagColon    = 9,
   // if you add tags, remember to update TagStrings in ForthShell.cpp
   kNumShellTags
} eShellTag;


class ForthShellStack
{
public:
   ForthShellStack( int stackLongs = 1024 );
   virtual ~ForthShellStack();


   inline long *       GetSP( void )           { return mSSP; };
   inline void         SetSP( long *pNewSP )   { mSSP = pNewSP; };
   inline long         GetSize( void )         { return mSSLen; };
   inline long         GetDepth( void )        { return mSST - mSSP; };
   inline void         EmptyStack( void )      { mSSP = mSST; };
   // push tag telling what control structure we are compiling (if/else/for/...)
   void         Push( long tag );
   long         Pop( void );
   long         Peek( void );

   // push a string, this should be followed by a PushTag of a tag which uses this string (such as paren)
   void                PushString( const char *pString );
   // return true IFF item on top of shell stack is a string
   bool                PopString( char *pString );

protected:
   long                *mSSP;       // shell stack pointer
   long                *mSSB;       // shell stack base
   long                *mSST;       // empty shell stack pointer
   ulong               mSSLen;      // size of shell stack in longwords

};

#define SHELL_FLAG_CREATED_ENGINE   1
#define SHELL_FLAG_POP_NEXT_TOKEN   2

class ForthShell  
{
public:

    // if the creator of a ForthShell passes in non-NULL engine and/or thread params,
    //   that creator is responsible for deleting the engine and/or thread
    ForthShell( ForthEngine *pEngine = NULL, ForthThread *pThread = NULL, int shellStackLongs = 1024 );
    virtual ~ForthShell();

    // returns true IFF file opened successfully
    virtual bool            PushInputFile( const char *pInFileName );
    virtual void            PushInputBuffer( char *pDataBuffer, int dataBufferLen );
    virtual bool            PopInputStream( void );
    virtual int             Run( ForthInputStream *pStream );
    char *                  GetNextSimpleToken( void );
    char *                  GetToken( char delim );

    void                    SetCommandLine( int argc, const char ** argv );
    void                    SetCommandLine( const char *pCmdLine );

    void                    SetEnvironmentVars( const char ** envp );

    inline ForthEngine *    GetEngine( void ) { return mpEngine; };
    inline ForthThread *    GetThread( void ) { return mpThread; };
    inline ForthInputStack * GetInput( void ) { return mpInput; };
	inline ForthShellStack * GetShellStack( void ) { return mpStack; };

    inline int              GetArgCount( void ) { return mNumArgs; };
    inline char *           GetArg( int argNum ) { return mpArgs[argNum]; };

    bool                    CheckSyntaxError( const char *pString, long tag, long desiredTag );

    eForthResult            InterpretLine( const char *pSrcLine = NULL );

    virtual char            GetChar();

protected:

    // parse next token from input stream into mTokenBuff, padded with 0's up
    // to next longword boundary
    bool                    ParseToken( ForthParseInfo *pInfo );
    // parse next string from input stream into mTokenBuff, padded with 0's up
    // to next longword boundary
    bool                    ParseString( ForthParseInfo *pInfo );
	void                    ReportError( void );
	void                    ErrorReset( void );

    void                    DeleteEnvironmentVars();
    void                    DeleteCommandLine();

    ForthInputStack *       mpInput;
    ForthEngine *           mpEngine;
    ForthThread *           mpThread;
    ForthShellStack *       mpStack;

    long                    mTokenBuffer[ TOKEN_BUFF_LONGS ];

    int                     mNumArgs;
    char **                 mpArgs;
    int                     mNumEnvVars;
    char **                 mpEnvVarNames;
    char **                 mpEnvVarValues;
    int                     mFlags;
    char                    mErrorString[ 128 ];
};

