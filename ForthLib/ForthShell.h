// ForthShell.h: interface for the ForthShell class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FORTHSHELL_H__A1EA9AA2_9092_11D4_97DF_00B0D011B654__INCLUDED_)
#define AFX_FORTHSHELL_H__A1EA9AA2_9092_11D4_97DF_00B0D011B654__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"

class ForthShell  
{
public:

	ForthShell( ForthEngine *pEngine, ForthThread *pThread );
	virtual ~ForthShell();

    void            PushInputStream( FILE *pInFile );
    bool            PopInputStream( void );
    int             Run( char *pFileName );
    char *          GetNextSimpleToken( void );
protected:

    eForthResult    InterpretLine( int *pExitCode );
    bool            ParseToken( char *pTokenBuffer, int *pParseFlags );
    void            ReportError( void );

    ForthInputStack     *mpInStream;
    ForthEngine         *mpEngine;
    ForthThread         *mpThread;
    bool                mbCreatedEngine;
    bool                mbCreatedThread;
};

#endif // !defined(AFX_FORTHSHELL_H__A1EA9AA2_9092_11D4_97DF_00B0D011B654__INCLUDED_)
