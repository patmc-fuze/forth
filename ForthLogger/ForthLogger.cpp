// ForthLogger.cpp : Defines the entry point for the console application.
//

#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

//
// this program takes log stream output from the named pipe ForthLogPipe and prints it
//

int _tmain(int argc, _TCHAR* argv[])
{
    HANDLE hPipe;
    char buffer[8192];
    DWORD dwRead;

    hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\ForthLogPipe"),
        PIPE_ACCESS_DUPLEX | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
        PIPE_WAIT,
        1,
        1024 * 16,
        1024 * 16,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        _tprintf(TEXT("Could not open pipe. Last error code=%d\n"), GetLastError());
    }

    while (hPipe != INVALID_HANDLE_VALUE)
    {
        printf("========================== NEW CLIENT =============================\n");
        if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait for someone to connect to the pipe
        {
            while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
            {
                buffer[dwRead] = '\0';

                int offset = 0;
                while (offset < dwRead)
                {
                    char *pLine = &(buffer[offset]);
                    int lineLen = strlen(pLine);
                    printf("%s", pLine);
                    offset += (lineLen + 1);

                }
            }
        }

        DisconnectNamedPipe(hPipe);
    }
    return 0;
}

