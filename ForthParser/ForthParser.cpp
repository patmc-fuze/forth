// ForthParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ForthParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////


void
traceAction( char   *pToken,
             int    parseFlags )
{
    TRACE( "token [%s]\n", pToken );
}


static char *
ParseQuoted( char       *pBuffer )
{
    char *pToken, *pDst;

    pToken = pBuffer + 1;
    pDst = pToken;
    while ( *pToken != '\0' ) {
        switch ( *pToken ) {

        case '"':
            *pDst = 0;
            return pToken;

        case '\\':
            pToken++;
            switch( *pToken ) {
            case 'a':
                *pDst++ = '\a';
                break;
            case 'b':
                *pDst++ = '\b';
                break;
            case 'f':
                *pDst++ = '\f';
                break;
            case 'n':
                *pDst++ = '\n';
                break;
            case 'r':
                *pDst++ = '\r';
                break;
            case 't':
                *pDst++ = '\t';
                break;
            case 'v':
                *pDst++ = '\v';
                break;
            case '\\':
                *pDst++ = '\\';
                break;
            case '\?':
                *pDst++ = '\?';
                break;
            case '\'':
                *pDst++ = '\'';
                break;
            case '\"':
                *pDst++ = '\"';
                break;
            // TBD: support hex constants

            default:
                *pDst++ = '\\';
                break;
            }
            break;
        default:
            *pDst++ = *pToken;
            break;

        }
        pToken++;
    }
    return pToken;
}


void
ForthParseLine( char            *pBuffer,
                ParseAction     pAction )
{
    char *pToken, *pEndToken;
    int parseFlags;

    // get rid of the linefeed (if any)
    pToken = strchr( pBuffer, '\n' );
    if ( pToken ) {
        *pToken = '\0';
    }

    pToken = pBuffer;
    while ( *pToken != '\0' ) {
        parseFlags = 0;
        // eat any leading white space
        while ( (*pToken == ' ') || (*pToken == '\t') ) {
            pToken++;
        }
        // parse symbol up till next white space
        pEndToken = pToken;
        if ( *pToken == '\"' ) {
            // support quoted strings...
            parseFlags |= PARSE_FLAG_QUOTED;
            pEndToken = ParseQuoted( pToken );
            pToken++;
        } else {
            while ( (*pEndToken != ' ') && (*pEndToken != '\t')
                    && (*pEndToken != '\0') ) {
                pEndToken++;
            }
        }
        // if we aren't already at end of line, tack a terminator
        //  onto the token, and advance to char after the terminator
        if ( *pEndToken != '\0' ) {
            *pEndToken = '\0';
            pEndToken++;
        }
        if ( *pToken != '\0' ) {

            // support C++ end-of-line style comments
            if ( (*pToken == '/') && (pToken[1] == '/') ) {
                return;
            }

            // process the token
            pAction( pToken, parseFlags );
        }
        pToken = pEndToken;
    }
}

