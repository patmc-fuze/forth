//////////////////////////////////////////////////////////////////////
//
// OStream.cpp: builtin stream related classes
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <stdio.h>
#include <string.h>
#include <map>

#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthObject.h"
#include "ForthBuiltinClasses.h"
#include "ForthShowContext.h"

#include "OStream.h"
#include "OString.h"

extern "C" {
	extern void unimplementedMethodOp(ForthCoreState *pCore);
	extern void illegalMethodOp(ForthCoreState *pCore);
};

namespace OStream
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 oInStream
	//

	// oInStream is an abstract input stream class

	FORTHOP(oInStreamIterCharMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthObject obj;
		obj.pData = pCore->TPD;
		obj.pMethodOps = pCore->TPM;
		pEngine->ExecuteOneMethod(pCore, obj, kInStreamGetCharMethod);
		if (*(pCore->SP) == -1)
		{
			*(pCore->SP) = 0;
		}
		else
		{
			SPUSH(-1);
		}
		METHOD_RETURN;
	}

	FORTHOP(oInStreamGetBytesMethod)
	{
		int numBytes = SPOP;
		int outBytes = 0;
		char* pBuffer = reinterpret_cast<char *>(SPOP);
		char* pDst = pBuffer;

		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthObject obj;
		obj.pData = pCore->TPD;
		obj.pMethodOps = pCore->TPM;
		for (int i = 0; i < numBytes; i++)
		{
			pEngine->ExecuteOneMethod(pCore, obj, kInStreamGetCharMethod);
			int ch = SPOP;
			if (ch == -1)
			{
				break;
			}
			*pDst++ = (char)ch;
			outBytes++;
		}

		SPUSH(outBytes);
		METHOD_RETURN;
	}

	FORTHOP(oInStreamIterBytesMethod)
	{
		int numBytes = SPOP;
		int outBytes = 0;
		char* pBuffer = reinterpret_cast<char *>(SPOP);
		char* pDst = pBuffer;

		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthObject obj;
		obj.pData = pCore->TPD;
		obj.pMethodOps = pCore->TPM;
		pEngine->ExecuteOneMethod(pCore, obj, kInStreamGetBytesMethod);
		if (*(pCore->SP) == 0)
		{
			SPOP;
			*(pCore->SP) = 0;
		}
		else
		{
			SPUSH(-1);
		}

		METHOD_RETURN;
	}

	FORTHOP(oInStreamGetLineMethod)
	{
		GET_THIS(oInStreamStruct, pInStream);
		int maxBytes = SPOP;
		char* pBuffer = reinterpret_cast<char *>(SPOP);

		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthObject obj;
		obj.pData = pCore->TPD;
		obj.pMethodOps = pCore->TPM;
		char* pDst = pBuffer;

		bool atEOF = false;
		bool done = false;
		int previousChar = 0;
		for (int i = 0; i < (maxBytes - 1); i++)
		{
			pEngine->ExecuteOneMethod(pCore, obj, kInStreamGetCharMethod);
			int ch = SPOP;
			switch (ch)
			{
			case -1:
				atEOF = true;
				break;

			case '\n':
				if (previousChar == '\r')
				{
					pDst--;
				}
				if (!pInStream->bTrimEOL)
				{
					*pDst++ = '\n';
				}
				done = true;
				break;

			default:
				*pDst++ = (char) ch;
				break;
			}

			if (atEOF || done)
			{
				break;
			}
			previousChar = ch;
		}
		int numWritten = pDst - pBuffer;
		*pDst++ = '\0';
		SPUSH(numWritten);
		METHOD_RETURN;
	}

	FORTHOP(oInStreamIterLineMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthObject obj;
		obj.pData = pCore->TPD;
		obj.pMethodOps = pCore->TPM;
		pEngine->ExecuteOneMethod(pCore, obj, kInStreamGetLineMethod);
		if (*(pCore->SP) == 0)
		{
			// getLine returned 0 chars - is it an empty line or end of file?
			pEngine->ExecuteOneMethod(pCore, obj, kInStreamAtEOFMethod);
			int atEOF = SPOP;
			if (!atEOF)
			{
				// just an empty line
				SPUSH(-1);
			}
		}
		else
		{
			SPUSH(-1);
		}

		METHOD_RETURN;
	}

	FORTHOP(oInStreamSetTrimEOLMethod)
	{
		GET_THIS(oInStreamStruct, pInStreamStruct);
		pInStreamStruct->bTrimEOL = (SPOP != 0);
		METHOD_RETURN;
	}

	baseMethodEntry oInStreamMembers[] =
	{
		// getChar, getBytes, getLine and atEOF must be first 4 methods and in this order
		METHOD("getChar", unimplementedMethodOp),			// derived classes must define getChar
		METHOD("getBytes", oInStreamGetBytesMethod),
		METHOD("getLine", unimplementedMethodOp),			// derived classes must define getLine (for now)
		METHOD("atEOF", unimplementedMethodOp),				// derived classes must define atEOF
		METHOD("iterChar", oInStreamIterCharMethod),
		METHOD("iterBytes", oInStreamIterBytesMethod),
		METHOD("iterLine", oInStreamIterLineMethod),
		METHOD("setTrimEOL", oInStreamSetTrimEOLMethod),

		MEMBER_VAR("userData", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("trimEOL", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oFileInStream
	//

	struct oFileInStreamStruct
	{
		oInStreamStruct istream;
	};

	FORTHOP(oFileInStreamNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oFileInStreamStruct, pFileInStreamStruct);
		pFileInStreamStruct->istream.refCount = 0;
		pFileInStreamStruct->istream.pUserData = NULL;
		pFileInStreamStruct->istream.bTrimEOL = true;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pFileInStreamStruct);
	}

	FORTHOP(oFileInStreamDeleteMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		if (pFileInStreamStruct->istream.pUserData != NULL)
		{
			GET_ENGINE->GetShell()->GetFileInterface()->fileClose((FILE *)(pFileInStreamStruct->istream.pUserData));
			pFileInStreamStruct->istream.pUserData = NULL;
		}
		METHOD_RETURN;
	}

	FORTHOP(oFileInStreamSetFileMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		pFileInStreamStruct->istream.pUserData = reinterpret_cast<FILE *>(SPOP);
		METHOD_RETURN;
	}

	FORTHOP(oFileInStreamGetFileMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		SPUSH(reinterpret_cast<long>(pFileInStreamStruct->istream.pUserData));
		METHOD_RETURN;
	}

	FORTHOP(oFileInStreamOpenMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		if (pFileInStreamStruct->istream.pUserData != NULL)
		{
			GET_ENGINE->GetShell()->GetFileInterface()->fileClose((FILE *)(pFileInStreamStruct->istream.pUserData));
			pFileInStreamStruct->istream.pUserData = NULL;
		}
		const char* access = (const char*)(SPOP);
		const char* path = (const char*)(SPOP);
		pFileInStreamStruct->istream.pUserData = GET_ENGINE->GetShell()->GetFileInterface()->fileOpen(path, access);
		METHOD_RETURN;
	}

	FORTHOP(oFileInStreamCloseMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		if (pFileInStreamStruct->istream.pUserData != NULL)
		{
			GET_ENGINE->GetShell()->GetFileInterface()->fileClose((FILE *)(pFileInStreamStruct->istream.pUserData));
			pFileInStreamStruct->istream.pUserData = NULL;
		}
		METHOD_RETURN;
	}

	FORTHOP(oFileInStreamGetCharMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		int ch = -1;
		if (pFileInStreamStruct->istream.pUserData != NULL)
		{
			ch = GET_ENGINE->GetShell()->GetFileInterface()->fileGetChar((FILE *)(pFileInStreamStruct->istream.pUserData));
		}
		SPUSH(ch);
		METHOD_RETURN;
	}

	FORTHOP(oFileInStreamIterCharMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		int gotData = 0;
		if (pFileInStreamStruct->istream.pUserData != NULL)
		{
			int ch = GET_ENGINE->GetShell()->GetFileInterface()->fileGetChar((FILE *)(pFileInStreamStruct->istream.pUserData));
			if (ch != -1)
			{
				SPUSH(ch);
				gotData--;
			}
		}
		SPUSH(gotData);
		METHOD_RETURN;
	}

	FORTHOP(oFileInStreamGetBytesMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		int numBytes = SPOP;
		char* pBuffer = reinterpret_cast<char *>(SPOP);
		int numRead = 0;
		if (pFileInStreamStruct->istream.pUserData != NULL)
		{
			numRead = GET_ENGINE->GetShell()->GetFileInterface()->fileRead(pBuffer, 1, numBytes, (FILE *)(pFileInStreamStruct->istream.pUserData));
		}
		SPUSH(numRead);
		METHOD_RETURN;
	}

	FORTHOP(oFileInStreamIterBytesMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		int numBytes = SPOP;
		char* pBuffer = reinterpret_cast<char *>(SPOP);
		int gotData = 0;
		if (pFileInStreamStruct->istream.pUserData != NULL)
		{
			int numRead = GET_ENGINE->GetShell()->GetFileInterface()->fileRead(pBuffer, 1, numBytes, (FILE *)(pFileInStreamStruct->istream.pUserData));
			if (numRead > 0)
			{
				SPUSH(numRead);
				gotData--;
			}
		}
		SPUSH(gotData);
		METHOD_RETURN;
	}

	FORTHOP(oFileInStreamGetLineMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		int maxBytes = SPOP;
		char* pBuffer = reinterpret_cast<char *>(SPOP);
		char* pResult = NULL;
		if (pFileInStreamStruct->istream.pUserData != NULL)
		{
			pResult = GET_ENGINE->GetShell()->GetFileInterface()->fileGetString(pBuffer, maxBytes, (FILE *)(pFileInStreamStruct->istream.pUserData));
		}
		int numRead = 0;
		if (pResult != NULL)
		{
			if (pFileInStreamStruct->istream.bTrimEOL)
			{
				char* pEOL = pResult;
				char ch;
				while ((ch = *pEOL) != '\0')
				{
					if ((ch == '\n') || (ch == '\r'))
					{
						*pEOL = '\0';
						break;
					}
					++pEOL;
				}
			}
			numRead = strlen(pResult);
		}
		SPUSH(numRead);
		METHOD_RETURN;
	}

	FORTHOP(oFileInStreamIterLineMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		int maxBytes = SPOP;
		char* pBuffer = reinterpret_cast<char *>(SPOP);
		char* pResult = NULL;
		int gotData = 0;
		if (pFileInStreamStruct->istream.pUserData != NULL)
		{
			pResult = GET_ENGINE->GetShell()->GetFileInterface()->fileGetString(pBuffer, maxBytes, (FILE *)(pFileInStreamStruct->istream.pUserData));
		}
		if (pResult != NULL)
		{
			if (pFileInStreamStruct->istream.bTrimEOL)
			{
				char* pEOL = pResult;
				while (char ch = *pEOL != '\0')
				{
					if ((ch == '\n') || (ch == '\r'))
					{
						*pEOL = '\0';
						break;
					}
					++pEOL;
				}
			}
			int numRead = strlen(pResult);
			if (numRead > 0)
			{
				SPUSH(numRead);
				gotData++;
			}
		}
		SPUSH(gotData);
		METHOD_RETURN;
	}

	FORTHOP(oFileInStreamSetAtEOFMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		int atEOF = 0;
		if (pFileInStreamStruct->istream.pUserData == NULL)
		{
			atEOF--;
		}
		else
		{
			if (feof((FILE *)(pFileInStreamStruct->istream.pUserData)))
			{
				atEOF--;
			}
		}
		SPUSH(atEOF);
		METHOD_RETURN;
	}

	baseMethodEntry oFileInStreamMembers[] =
	{
		METHOD("__newOp", oFileInStreamNew),
		METHOD("open", oFileInStreamOpenMethod),
		METHOD("close", oFileInStreamCloseMethod),
		METHOD("setFile", oFileInStreamSetFileMethod),
		METHOD("getFile", oFileInStreamGetFileMethod),
		METHOD("delete", oFileInStreamDeleteMethod),
		METHOD("getChar", oFileInStreamGetCharMethod),
		METHOD("iterChar", oFileInStreamIterCharMethod),
		METHOD("getBytes", oFileInStreamGetBytesMethod),
		METHOD("iterBytes", oFileInStreamIterBytesMethod),
		METHOD("getLine", oFileInStreamGetLineMethod),
		METHOD("iterLine", oFileInStreamIterLineMethod),
		METHOD("atEOF", oFileInStreamSetAtEOFMethod),

		MEMBER_VAR("inFile", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oConsoleInStream
	//

	FORTHOP(oConsoleInStreamNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oFileInStreamStruct, pConsoleInStreamStruct);
		pConsoleInStreamStruct->istream.refCount = 0;
		pConsoleInStreamStruct->istream.bTrimEOL = true;
		pConsoleInStreamStruct->istream.pUserData = GET_ENGINE->GetShell()->GetFileInterface()->getStdIn();
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pConsoleInStreamStruct);
	}

	FORTHOP(oConsoleInStreamDeleteMethod)
	{
		GET_THIS(oFileInStreamStruct, pFileInStreamStruct);
		pFileInStreamStruct->istream.pUserData = NULL;
		METHOD_RETURN;
	}

	FORTHOP(oConsoleInStreamSetFileMethod)
	{
		// TODO: report an error?
		SPOP;
		METHOD_RETURN;
	}

	FORTHOP(oConsoleInStreamSetAtEOFMethod)
	{
		SPUSH(0);
		METHOD_RETURN;
	}

	baseMethodEntry oConsoleInStreamMembers[] =
	{
		METHOD("__newOp", oConsoleInStreamNew),
		METHOD("delete", oConsoleInStreamDeleteMethod),
		METHOD("setFile", oConsoleInStreamSetFileMethod),
		METHOD("atEOF", oConsoleInStreamSetAtEOFMethod),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oOutStream
	//

	FORTHOP(oOutStreamNew)
	{
		ForthEngine::GetInstance()->SetError(kForthErrorBadObject, " can't create abstract oOutStream");

		PUSH_PAIR(NULL, NULL);
	}

	void streamCharOut(ForthCoreState* pCore, oOutStreamStruct* pOutStream, char ch)
	{
		if (pOutStream->pOutFuncs->outChar != NULL)
		{
			pOutStream->pOutFuncs->outChar(pCore, pOutStream, ch);
		}
		else if (pOutStream->pOutFuncs->outBytes != NULL)
		{
			pOutStream->pOutFuncs->outBytes(pCore, pOutStream, &ch, 1);
		}
		else if (pOutStream->pOutFuncs->outString != NULL)
		{
			char buff[2];
			buff[0] = ch;
			buff[1] = '\0';
			pOutStream->pOutFuncs->outString(pCore, pOutStream, buff);
		}
		else
		{
			ForthEngine::GetInstance()->SetError(kForthErrorIO, " output stream has no output routines");
		}
	}

	FORTHOP(oOutStreamPutCharMethod)
	{
		GET_THIS(oOutStreamStruct, pOutStream);

		if (pOutStream->pOutFuncs == NULL)
		{
			ForthEngine::GetInstance()->SetError(kForthErrorIO, " output stream has no output routines");
		}
		else
		{
			char ch = static_cast<char>(SPOP);
			streamCharOut(pCore, pOutStream, ch);
		}
		METHOD_RETURN;
	}

	void streamBytesOut(ForthCoreState* pCore, oOutStreamStruct* pOutStream, const char* pBuffer, int numBytes)
	{
		if (pOutStream->pOutFuncs->outBytes != NULL)
		{
			pOutStream->pOutFuncs->outBytes(pCore, pOutStream, pBuffer, numBytes);
		}
		else if (pOutStream->pOutFuncs->outChar != NULL)
		{
			for (int i = 0; i < numBytes; i++)
			{
				pOutStream->pOutFuncs->outChar(pCore, pOutStream, pBuffer[i]);
			}
		}
		else if (pOutStream->pOutFuncs->outString != NULL)
		{
			char buff[2];
			buff[1] = '\0';
			for (int i = 0; i < numBytes; i++)
			{
				buff[0] = pBuffer[i];
				pOutStream->pOutFuncs->outString(pCore, pOutStream, buff);
			}
		}
		else
		{
			ForthEngine::GetInstance()->SetError(kForthErrorIO, " output stream has no output routines");
		}
	}

	FORTHOP(oOutStreamPutBytesMethod)
	{
		GET_THIS(oOutStreamStruct, pOutStream);
		int numBytes = SPOP;
		char* pBuffer = reinterpret_cast<char *>(SPOP);

		if (pOutStream->pOutFuncs == NULL)
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			ForthObject obj;
			obj.pData = pCore->TPD;
			obj.pMethodOps = pCore->TPM;
			for (int i = 0; i < numBytes; i++)
			{
				char ch = *pBuffer++;
				SPUSH(((long)ch));
				pEngine->ExecuteOneMethod(pCore, obj, kOutStreamPutCharMethod);
			}
		}
		else
		{
			streamBytesOut(pCore, pOutStream, pBuffer, numBytes);
		}
		METHOD_RETURN;
	}

	void streamStringOut(ForthCoreState* pCore, oOutStreamStruct* pOutStream, const char* pBuffer)
	{
		if (pOutStream->pOutFuncs->outString != NULL)
		{
			pOutStream->pOutFuncs->outString(pCore, pOutStream, pBuffer);
		}
		else
		{
			int numBytes = strlen(pBuffer);
			if (pOutStream->pOutFuncs->outBytes != NULL)
			{
				pOutStream->pOutFuncs->outBytes(pCore, pOutStream, pBuffer, numBytes);
			}
			else if (pOutStream->pOutFuncs->outChar != NULL)
			{
				for (int i = 0; i < numBytes; i++)
				{
					pOutStream->pOutFuncs->outChar(pCore, pOutStream, pBuffer[i]);
				}
			}
			else
			{
				ForthEngine::GetInstance()->SetError(kForthErrorIO, " output stream has no output routines");
			}
		}
	}

	FORTHOP(oOutStreamPutStringMethod)
	{
		GET_THIS(oOutStreamStruct, pOutStream);
		char* pBuffer = reinterpret_cast<char *>(SPOP);

		if (pOutStream->pOutFuncs == NULL)
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			ForthObject obj;
			obj.pData = pCore->TPD;
			obj.pMethodOps = pCore->TPM;
			int numBytes = strlen(pBuffer);
			for (int i = 0; i < numBytes; i++)
			{
				char ch = *pBuffer++;
				SPUSH(((long)ch));
				pEngine->ExecuteOneMethod(pCore, obj, kOutStreamPutCharMethod);
			}
		}
		else
		{
			streamStringOut(pCore, pOutStream, pBuffer);
		}
		METHOD_RETURN;
	}

	FORTHOP(oOutStreamPutLineMethod)
	{
		GET_THIS(oOutStreamStruct, pOutStream);
		char* pBuffer = reinterpret_cast<char *>(SPOP);

		pOutStream->eolChars[3] = '\0';
		if (pOutStream->pOutFuncs == NULL)
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			ForthObject obj;
			obj.pData = pCore->TPD;
			obj.pMethodOps = pCore->TPM;
			int numBytes = strlen(pBuffer);
			for (int i = 0; i < numBytes; i++)
			{
				char ch = *pBuffer++;
				SPUSH(((long)ch));
				pEngine->ExecuteOneMethod(pCore, obj, kOutStreamPutCharMethod);
			}
			SPUSH((long)'\n');
			pEngine->ExecuteOneMethod(pCore, obj, kOutStreamPutCharMethod);
		}
		else
		{
			streamStringOut(pCore, pOutStream, pBuffer);
			streamCharOut(pCore, pOutStream, '\n');
		}
		METHOD_RETURN;
	}

	baseMethodEntry oOutStreamMembers[] =
	{
		METHOD("__newOp", oOutStreamNew),
		// putChar, putBytes and putString must be first 3 methods and in this order
		METHOD("putChar", oOutStreamPutCharMethod),
		METHOD("putBytes", oOutStreamPutBytesMethod),
		METHOD("putString", oOutStreamPutStringMethod),
		METHOD("putLine", oOutStreamPutLineMethod),
		MEMBER_VAR("userData", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("__outFuncs", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oFileOutStream
	//

	void fileCharOut(ForthCoreState* pCore, void *pData, char ch)
	{
		GET_ENGINE->GetShell()->GetFileInterface()->filePutChar(ch, static_cast<FILE *>(static_cast<oOutStreamStruct*>(pData)->pUserData));
	}

	void fileBytesOut(ForthCoreState* pCore, void *pData, const char *pBuffer, int numChars)
	{
		GET_ENGINE->GetShell()->GetFileInterface()->fileWrite(pBuffer, 1, numChars, static_cast<FILE *>(static_cast<oOutStreamStruct*>(pData)->pUserData));
	}

	void fileStringOut(ForthCoreState* pCore, void *pData, const char *pBuffer)
	{
		GET_ENGINE->GetShell()->GetFileInterface()->filePutString(pBuffer, static_cast<FILE *>(static_cast<oOutStreamStruct*>(pData)->pUserData));
	}

	OutStreamFuncs fileOutFuncs =
	{
		fileCharOut,
		fileBytesOut,
		fileStringOut
	};

	FORTHOP(oFileOutStreamNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oOutStreamStruct, pFileOutStream);
		pFileOutStream->refCount = 0;
		pFileOutStream->pOutFuncs = &fileOutFuncs;
		pFileOutStream->pUserData = NULL;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pFileOutStream);
	}

	FORTHOP(oFileOutStreamDeleteMethod)
	{
		GET_THIS(oOutStreamStruct, pFileOutStream);
		if (pFileOutStream->pUserData != NULL)
		{
			GET_ENGINE->GetShell()->GetFileInterface()->fileClose(static_cast<FILE *>(pFileOutStream->pUserData));
			pFileOutStream->pUserData = NULL;
		}
		FREE_OBJECT(pFileOutStream);
		METHOD_RETURN;
	}

	FORTHOP(oFileOutStreamSetFileMethod)
	{
		GET_THIS(oOutStreamStruct, pFileOutStream);
		pFileOutStream->pUserData = reinterpret_cast<FILE *>(SPOP);
		METHOD_RETURN;
	}

	FORTHOP(oFileOutStreamGetFileMethod)
	{
		GET_THIS(oOutStreamStruct, pFileOutStream);
		SPUSH(reinterpret_cast<long>(pFileOutStream->pUserData));
		METHOD_RETURN;
	}

	FORTHOP(oFileOutStreamOpenMethod)
	{
		GET_THIS(oOutStreamStruct, pFileOutStreamStruct);
		if (pFileOutStreamStruct->pUserData != NULL)
		{
			GET_ENGINE->GetShell()->GetFileInterface()->fileClose((FILE *)(pFileOutStreamStruct->pUserData));
			pFileOutStreamStruct->pUserData = NULL;
		}
		const char* access = (const char*)(SPOP);
		const char* path = (const char*)(SPOP);
		pFileOutStreamStruct->pUserData = GET_ENGINE->GetShell()->GetFileInterface()->fileOpen(path, access);
		METHOD_RETURN;
	}

	FORTHOP(oFileOutStreamCloseMethod)
	{
		GET_THIS(oOutStreamStruct, pFileOutStreamStruct);
		if (pFileOutStreamStruct->pUserData != NULL)
		{
			GET_ENGINE->GetShell()->GetFileInterface()->fileClose((FILE *)(pFileOutStreamStruct->pUserData));
			pFileOutStreamStruct->pUserData = NULL;
		}
		METHOD_RETURN;
	}

	baseMethodEntry oFileOutStreamMembers[] =
	{
		METHOD("__newOp", oFileOutStreamNew),
		METHOD("delete", oFileOutStreamDeleteMethod),

		METHOD("open", oFileOutStreamOpenMethod),
		METHOD("close", oFileOutStreamCloseMethod),
		METHOD("setFile", oFileOutStreamSetFileMethod),
		METHOD("getFile", oFileOutStreamGetFileMethod),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oStringOutStream
	//

	struct oStringOutStreamStruct
	{
		oOutStreamStruct		ostream;
		ForthObject				outString;
	};

	OutStreamFuncs stringOutFuncs =
	{
		OString::stringCharOut,
		OString::stringBlockOut,
		OString::stringStringOut
	};

	FORTHOP(oStringOutStreamNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oStringOutStreamStruct, pStringOutStream);
		pStringOutStream->ostream.refCount = 0;
		pStringOutStream->ostream.pOutFuncs = &stringOutFuncs;
		pStringOutStream->ostream.pUserData = &(pStringOutStream->outString);
		CLEAR_OBJECT(pStringOutStream->outString);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pStringOutStream);
	}

	FORTHOP(oStringOutStreamDeleteMethod)
	{
		GET_THIS(oStringOutStreamStruct, pStringOutStream);
		SAFE_RELEASE(pCore, pStringOutStream->outString);
		FREE_OBJECT(pStringOutStream);
		METHOD_RETURN;
	}

	FORTHOP(oStringOutStreamSetStringMethod)
	{
		GET_THIS(oStringOutStreamStruct, pStringOutStream);
		ForthObject dstString;
		POP_OBJECT(dstString);
		OBJECT_ASSIGN(pCore, pStringOutStream->outString, dstString);
		pStringOutStream->outString = dstString;
		METHOD_RETURN;
	}

	FORTHOP(oStringOutStreamGetStringMethod)
	{
		GET_THIS(oStringOutStreamStruct, pStringOutStream);
		PUSH_OBJECT(pStringOutStream->outString);
		METHOD_RETURN;
	}

	baseMethodEntry oStringOutStreamMembers[] =
	{
		METHOD("__newOp", oStringOutStreamNew),
		METHOD("delete", oStringOutStreamDeleteMethod),
		METHOD("setString", oStringOutStreamSetStringMethod),
		METHOD_RET("getString", oStringOutStreamGetStringMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIString)),

		MEMBER_VAR("outString", OBJECT_TYPE_TO_CODE(0, kBCIString)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oConsoleOutStream
	//

	oOutStreamStruct consoleOutSingleton;

	FORTHOP(oConsoleOutStreamNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		consoleOutSingleton.refCount = 1000;
		consoleOutSingleton.pOutFuncs = &fileOutFuncs;
		consoleOutSingleton.pUserData = GET_ENGINE->GetShell()->GetFileInterface()->getStdOut();
		PUSH_PAIR(pPrimaryInterface->GetMethods(), &consoleOutSingleton);
	}

	FORTHOP(oConsoleOutStreamDeleteMethod)
	{
		// this is an undeletable singleton, make the ref count high to avoid needless delete calls
		consoleOutSingleton.refCount = 1000;
		METHOD_RETURN;
	}

	baseMethodEntry oConsoleOutStreamMembers[] =
	{
		METHOD("__newOp", oConsoleOutStreamNew),
		METHOD("delete", oConsoleOutStreamDeleteMethod),
		METHOD("setFile", illegalMethodOp),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oFunctionOutStream
	//

	struct oFunctionOutStreamStruct
	{
		oOutStreamStruct			ostream;
		OutStreamFuncs			outFuncs;
	};

	FORTHOP(oFunctionOutStreamNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oFunctionOutStreamStruct, pFunctionOutStream);
		pFunctionOutStream->ostream.refCount = 0;
		pFunctionOutStream->ostream.pOutFuncs = &(pFunctionOutStream->outFuncs);
		pFunctionOutStream->ostream.pUserData = NULL;
		pFunctionOutStream->outFuncs.outChar = NULL;
		pFunctionOutStream->outFuncs.outBytes = NULL;
		pFunctionOutStream->outFuncs.outString = NULL;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pFunctionOutStream);
	}

	FORTHOP(oFunctionOutStreamInitMethod)
	{
		GET_THIS(oFunctionOutStreamStruct, pFunctionOutStream);
		pFunctionOutStream->ostream.pUserData = reinterpret_cast<void *>(SPOP);
		pFunctionOutStream->outFuncs.outString = reinterpret_cast<streamStringOutRoutine>(SPOP);
		pFunctionOutStream->outFuncs.outBytes = reinterpret_cast<streamBytesOutRoutine>(SPOP);
		pFunctionOutStream->outFuncs.outChar = reinterpret_cast<streamCharOutRoutine>(SPOP);
		METHOD_RETURN;
	}

	baseMethodEntry oFunctionOutStreamMembers[] =
	{
		METHOD("__newOp", oFunctionOutStreamNew),
		METHOD("init", oFunctionOutStreamInitMethod),

		MEMBER_VAR("__outFuncs", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		pEngine->AddBuiltinClass("OInStream", kBCIInStream, kBCIObject, oInStreamMembers);
		pEngine->AddBuiltinClass("OFileInStream", kBCIFileInStream, kBCIInStream, oFileInStreamMembers);
		pEngine->AddBuiltinClass("OConsoleInStream", kBCIConsoleInStream, kBCIFileInStream, oConsoleInStreamMembers);

		pEngine->AddBuiltinClass("OOutStream", kBCIOutStream, kBCIObject, oOutStreamMembers);
		pEngine->AddBuiltinClass("OFileOutStream", kBCIFileOutStream, kBCIOutStream, oFileOutStreamMembers);
		pEngine->AddBuiltinClass("OStringOutStream", kBCIStringOutStream, kBCIOutStream, oStringOutStreamMembers);
		pEngine->AddBuiltinClass("OConsoleOutStream", kBCIConsoleOutStream, kBCIFileOutStream, oConsoleOutStreamMembers);
		pEngine->AddBuiltinClass("OFunctionOutStream", kBCIFunctionOutStream, kBCIOutStream, oFunctionOutStreamMembers);

	}
} // namespace OStream

void GetForthConsoleOutStream(ForthCoreState* pCore, ForthObject& outObject)
{
	OStream::consoleOutSingleton.refCount = 1000;
	OStream::consoleOutSingleton.pOutFuncs = &OStream::fileOutFuncs;
	OStream::consoleOutSingleton.pUserData = GET_ENGINE->GetShell()->GetFileInterface()->getStdOut();

	ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIOutStream, 0);
	outObject.pMethodOps = pPrimaryInterface->GetMethods();
	outObject.pData = reinterpret_cast<long *>(&OStream::consoleOutSingleton);
}

void CreateForthFileOutStream(ForthCoreState* pCore, ForthObject& outObject, FILE* pOutFile)
{
	MALLOCATE_OBJECT(oOutStreamStruct, pFileOutStream);
	pFileOutStream->refCount = 1;
	pFileOutStream->pOutFuncs = &OStream::fileOutFuncs;
	pFileOutStream->pUserData = pOutFile;
	ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIFileOutStream, 0);
	outObject.pMethodOps = pPrimaryInterface->GetMethods();
	outObject.pData = reinterpret_cast<long *>(pFileOutStream);
}

void CreateForthStringOutStream(ForthCoreState* pCore, ForthObject& outObject)
{
	MALLOCATE_OBJECT(OStream::oStringOutStreamStruct, pStringOutStream);
	pStringOutStream->ostream.refCount = 1;
	pStringOutStream->ostream.pOutFuncs = &OStream::stringOutFuncs;
	pStringOutStream->ostream.pUserData = &(pStringOutStream->outString);
	ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIStringOutStream, 0);
	outObject.pMethodOps = pPrimaryInterface->GetMethods();
	outObject.pData = reinterpret_cast<long *>(pStringOutStream);

	// create the internal string object
	MALLOCATE_OBJECT(OString::oStringStruct, pString);
	pString->refCount = 0;
	pString->hash = 0;
	pString->str = OString::createOString(OString::gDefaultOStringSize);
	ForthInterface* pStringInterface = GET_BUILTIN_INTERFACE(kBCIString, 0);
	pStringOutStream->outString.pData = reinterpret_cast<long *>(pString);
	pStringOutStream->outString.pMethodOps = pPrimaryInterface->GetMethods();
}

const char* GetForthStringOutStreamData(ForthCoreState* pCore, ForthObject& streamObject)
{
	OStream::oStringOutStreamStruct* pStream = reinterpret_cast<OStream::oStringOutStreamStruct *>(streamObject.pData);
	OString::oStringStruct* pString = reinterpret_cast<OString::oStringStruct *>(pStream->outString.pData);
	return pString->str->data;
}

void CreateForthFunctionOutStream(ForthCoreState* pCore, ForthObject& outObject, streamCharOutRoutine outChar,
	streamBytesOutRoutine outBytes, streamStringOutRoutine outString, void* pUserData)
{
	MALLOCATE_OBJECT(OStream::oFunctionOutStreamStruct, pFunctionOutStream);
	pFunctionOutStream->ostream.refCount = 1;
	pFunctionOutStream->ostream.pOutFuncs = &(pFunctionOutStream->outFuncs);
	pFunctionOutStream->ostream.pUserData = pUserData;
	pFunctionOutStream->outFuncs.outChar = outChar;
	pFunctionOutStream->outFuncs.outBytes = outBytes;
	pFunctionOutStream->outFuncs.outString = outString;
	ForthInterface* pPrimaryInterface = GET_BUILTIN_INTERFACE(kBCIFunctionOutStream, 0);
	outObject.pMethodOps = pPrimaryInterface->GetMethods();
	outObject.pData = reinterpret_cast<long *>(pFunctionOutStream);
}

void ReleaseForthObject(ForthCoreState* pCore, ForthObject& inObject)
{
	oOutStreamStruct* pObjData = reinterpret_cast<oOutStreamStruct *>(inObject.pData);
	if (pObjData->refCount > 1)
	{
		--pObjData->refCount;
	}
	else
	{
		FREE_OBJECT(pObjData);
		inObject.pData = NULL;
		inObject.pMethodOps = NULL;
	}
}

// ForthConsoleCharOut etc. exist so that stuff outside this module can do output
//   without having to know about object innards
// TODO: remove hard coded method numbers
void ForthConsoleCharOut(ForthCoreState* pCore, char ch)
{
	ForthEngine *pEngine = GET_ENGINE;
	oOutStreamStruct* pOutStream = reinterpret_cast<oOutStreamStruct*>(pCore->consoleOutStream.pData);
	if (pOutStream->pOutFuncs != NULL)
	{
		OStream::streamCharOut(pCore, pOutStream, ch);
	}
	else
	{
		long* pMethods = pCore->consoleOutStream.pMethodOps;
		SPUSH(((long)ch));
		pEngine->ExecuteOneMethod(pCore, pCore->consoleOutStream, kOutStreamPutCharMethod);
	}
}

void ForthConsoleBytesOut(ForthCoreState* pCore, const char* pBuffer, int numChars)
{
	ForthEngine *pEngine = GET_ENGINE;
	oOutStreamStruct* pOutStream = reinterpret_cast<oOutStreamStruct*>(pCore->consoleOutStream.pData);
	if (pOutStream->pOutFuncs != NULL)
	{
		OStream::streamBytesOut(pCore, pOutStream, pBuffer, numChars);
	}
	else
	{
		SPUSH(((long)pBuffer));
		SPUSH(numChars);
		pEngine->ExecuteOneMethod(pCore, pCore->consoleOutStream, kOutStreamPutBytesMethod);
	}
}

void ForthConsoleStringOut(ForthCoreState* pCore, const char* pBuffer)
{
	ForthEngine *pEngine = GET_ENGINE;
	oOutStreamStruct* pOutStream = reinterpret_cast<oOutStreamStruct*>(pCore->consoleOutStream.pData);
	if (pOutStream->pOutFuncs != NULL)
	{
		OStream::streamStringOut(pCore, pOutStream, pBuffer);
	}
	else
	{
		SPUSH(((long)pBuffer));
		pEngine->ExecuteOneMethod(pCore, pCore->consoleOutStream, kOutStreamPutStringMethod);
	}
}



