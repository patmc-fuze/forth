// ForthInputStack.h: interface for the ForthInputStack class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FORTHINPUTSTACK_H__AB37354E_9BC7_11D4_97E6_00B0D011B654__INCLUDED_)
#define AFX_FORTHINPUTSTACK_H__AB37354E_9BC7_11D4_97E6_00B0D011B654__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class ForthInputStack  
{
public:
	ForthInputStack();
	virtual ~ForthInputStack();
    void            PushInputStream( FILE *pInFile );
    bool            PopInputStream();
    char *          GetLine( char *pPrompt );
    char *          GetBufferPointer( void );
    void            SetBufferPointer( char *pBuff );
    inline bool     IsAFile( void ) { return mpHead->pInFile != NULL; };
    void            Reset( void );

protected:
    typedef struct _ForthInputStream {
        struct _ForthInputStream    *pNext;
        char                        *pBuffer;
        char                        *pBufferBase;
        int                         bufferLen;
        FILE                        *pInFile;
    } ForthInputStream;

    ForthInputStream *mpHead;
};

#endif // !defined(AFX_FORTHINPUTSTACK_H__AB37354E_9BC7_11D4_97E6_00B0D011B654__INCLUDED_)
