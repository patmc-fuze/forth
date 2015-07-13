//////////////////////////////////////////////////////////////////////
//
// ForthParseInfo.cpp: implementation of the ForthParseInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ForthEngine.h"
#include "ForthParseInfo.h"
#include "ForthShell.h"
#include "ForthInput.h"
#include "ForthVocabulary.h"
#include "ForthExtension.h"

ForthParseInfo::ForthParseInfo(long *pBuffer, int numLongs)
	: mpToken(pBuffer)
	, mMaxChars((numLongs << 2) - 2)
	, mFlags(0)
	, mNumLongs(0)
{
	ASSERT(numLongs > 0);

	// zero length byte, and first char
	*mpToken = 0;
}


ForthParseInfo::~ForthParseInfo()
{
	// NOTE: don't delete mpToken, it doesn't belong to us
}


// copy string to mpToken buffer, set length, and pad with nulls to a longword boundary
// if pSrc is null, just set length and do padding
void
ForthParseInfo::SetToken(const char *pSrc)
{
	int symLen, padChars;
	char *pDst;

	if (pSrc != NULL)
	{

		symLen = strlen(pSrc);
		pDst = (char *)mpToken;

		if (symLen > mMaxChars)
		{
			symLen = mMaxChars;
		}
		// set length byte
		*pDst++ = symLen;

		// make copy of symbol
		memcpy(pDst, pSrc, symLen);
		pDst += symLen;
		*pDst++ = '\0';

	}
	else
	{

		// token has already been copied to mpToken, just set length byte
		symLen = strlen(((char *)mpToken) + 1);
		*((char *)mpToken) = symLen;
		pDst = ((char *)mpToken) + symLen + 2;
	}

	// in diagram, first char is length byte, 'a' are symbol chars, '0' is terminator, '#' is padding
	//
	//            symLen     padding nLongs
	// 1a0#|        1           1       1
	// 2aa0|        2           0       1
	// 3aaa|0       3           0       1
	// 4aaa|a0##    4           2       2
	// 5aaa|aa0#    5           1       2
	//
	mNumLongs = (symLen + 4) >> 2;


	padChars = (symLen + 2) & 3;
	if (padChars > 1)
	{
		padChars = 4 - padChars;
		while (padChars > 0)
		{
			*pDst++ = '\0';
			padChars--;
		}
	}
}


char
ForthParseInfo::BackslashChar(char c)
{
	char cResult;

	switch (c)
	{

	case 'a':        cResult = '\a';        break;
	case 'b':        cResult = '\b';        break;
	case 'f':        cResult = '\f';        break;
	case 'n':        cResult = '\n';        break;
	case 'r':        cResult = '\r';        break;
	case 't':        cResult = '\t';        break;
	case 'v':        cResult = '\v';        break;
	case '0':        cResult = '\0';        break;
	case '?':        cResult = '\?';        break;
	case '\\':       cResult = '\\';        break;
	case '\'':       cResult = '\'';        break;
	case '\"':       cResult = '\"';        break;

	default:         cResult = c;           break;

	}
	return cResult;
}


const char *
ForthParseInfo::ParseSingleQuote(const char *pSrcIn, const char *pSrcLimit, ForthEngine *pEngine, bool keepBackslashes)
{
	char cc[9];
	bool isQuotedChar = false;

	// in order to not break the standard forth word tick ('), don't allow character constants
	//  to contain space or tab characters
	// to have a space or tab in a char constant use a backslash
	if ((pSrcIn[1] != 0) && (pSrcIn[2] != 0))      // there must be at least 2 more chars on line
	{
		const char *pSrc = pSrcIn + 1;
		int iDst = 0;
		int maxChars = pEngine->CheckFeature(kFFMultiCharacterLiterals) ? 8 : 1;
		while ((iDst < maxChars) && (pSrc < pSrcLimit))
		{
			char ch = *pSrc++;
			if ((ch == '\0') || (ch == ' ') || (ch == '\t'))
			{
				break;
			}
			else if (ch == '\\')
			{
				if (keepBackslashes)
				{
					cc[iDst++] = ch;
				}
				ch = *pSrc++;
				if (ch == '\0')
				{
					break;
				}
				cc[iDst++] = BackslashChar(ch);
			}
			else if (ch == '\'')
			{
				cc[iDst++] = '\0';
				isQuotedChar = true;
				break;
			}
			else
			{
				cc[iDst++] = ch;
			}
		}
		if (iDst == maxChars)
		{
			if (*pSrc == '\'')
			{
				pSrc++;
				cc[iDst++] = '\0';
				isQuotedChar = true;
			}
		}
		if (isQuotedChar)
		{
			SetFlag(PARSE_FLAG_QUOTED_CHARACTER);
			SetToken(cc);
			pSrcIn = pSrc;
		}
	}
	return pSrcIn;
}


const char *
ForthParseInfo::ParseDoubleQuote(const char *pSrc, const char *pSrcLimit, bool keepBackslashes)
{
	char  *pDst = GetToken();

	SetFlag(PARSE_FLAG_QUOTED_STRING);

	pSrc++;  // skip first double-quote
	while ((*pSrc != '\0') && (pSrc < pSrcLimit))
	{

		switch (*pSrc)
		{

		case '"':
			*pDst = 0;
			// set token length byte
			SetToken();
			return pSrc + 1;

		case '\\':
			if (keepBackslashes)
			{
				*pDst++ = '\\';
			}
			pSrc++;
			*pDst++ = BackslashChar(*pSrc);
			break;

		default:
			*pDst++ = *pSrc;
			break;

		}

		pSrc++;
	}
	*pDst = 0;
	SetToken();
	return pSrc;
}


