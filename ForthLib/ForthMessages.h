#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthMessages.h: Message types sent between the Forth client and server
//
//////////////////////////////////////////////////////////////////////

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
    kClientMsgLimit,

    // client -> server    
    kServerMsgProcessLine = 20000,
    kServerMsgProcessChar,
    kServerMsgPopStream,         // sent when file is empty
    kServerMsgFileOpResult,
    kServerMsgFileReadResult,
    kServerMsgFileGetStringResult,
    kServerMsgStartLoadResult,      // sent in response to kClientMsgStartLoad
    kServerMsgLimit
};


