#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthMessages.h: Message types sent between the Forth client and server
//
//////////////////////////////////////////////////////////////////////

#define FORTH_SERVER_PORT 1742
//#define FORTH_SERVER_PORT 4
//#define FORTH_SERVER_PORT 27015

enum
{
	// server -> client
    kClientMsgDisplayText = 10000,
    kClientMsgSendLine,
    kClientMsgStartLoad,
    kClientMsgPopStream,    // sent when "loaddone" is executed
    kClientMsgGetChar,
    kClientMsgGoAway,
    kClientMsgFileOpen,
    kClientMsgFileClose,
    kClientMsgFileSetPosition,
    kClientMsgFileRead,
    kClientMsgFileWrite,
    kClientMsgFileGetChar,
    kClientMsgFilePutChar,
    kClientMsgFileCheckEOF,
    kClientMsgFileGetLength,
    kClientMsgFileCheckExists,
    kClientMsgFileGetPosition,
    kClientMsgFileGetString,
    kClientMsgFilePutString,
	kClientMsgRemoveFile,
	kClientMsgDupHandle,
	kClientMsgDupHandle2,
	kClientMsgFileToHandle,
	kClientMsgFileFlush,
	kClientMsgGetTempFilename,
	kClientMsgRenameFile,
	kClientMsgRunSystem,
	kClientMsgChangeDir,
	kClientMsgMakeDir,
	kClientMsgRemoveDir,
	kClientMsgGetStdIn,
	kClientMsgGetStdOut,
	kClientMsgGetStdErr,

    kClientMsgLimit,

    // client -> server    
    kServerMsgProcessLine = 20000,
    kServerMsgProcessChar,
    kServerMsgPopStream,         // sent when file is empty
    kServerMsgFileOpResult,
    kServerMsgFileReadResult,
    kServerMsgFileGetStringResult,
    kServerMsgStartLoadResult,      // sent in response to kClientMsgStartLoad
	kServerMsgGetTmpnamResult,
    kServerMsgLimit
};


