
#if !defined(AFX_PARSER_H__9FA5DC9F_754B_11D4_97D2_00B0D011B654__INCLUDED_)
#define AFX_PARSER_H__9FA5DC9F_754B_11D4_97D2_00B0D011B654__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define PARSE_FLAG_QUOTED 1
// action routine that the parser calls on a symbol by symbol basis
typedef void (*ParseAction)( char *pToken, int parseFlags );

extern void
ForthParseLine( char            *pBuffer,
                ParseAction     pAction );


#endif // !defined(AFX_PARSER_H__9FA5DC9F_754B_11D4_97D2_00B0D011B654__INCLUDED_)
