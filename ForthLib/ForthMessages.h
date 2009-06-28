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
    kClientMsgFileGetPosition,
    kClientMsgFileGetLength,
    kClientMsgFileCheckExists,
    kClientMsgFileGetLine,
    kClientMsgLimit,

    // client -> server    
    kServerMsgProcessLine = 20000,
    kServerMsgProcessChar,
    kServerMsgPopStream,         // sent when file is empty
    kServerMsgFileOpResult,
    kServerMsgLimit		
};


