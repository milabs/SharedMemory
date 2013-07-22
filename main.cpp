#include "CSmAfx.h"

#include "conio.h"

#define TEST_CLIENT_TO_SERVER   1
#define TEST_SERVER_TO_CLIENT   2

#define TEST_VALUE              TEST_SERVER_TO_CLIENT

tstring timestamp()
{
    tstring str;
    tstringstream stream;

    SYSTEMTIME time;
    GetLocalTime( &time );

    stream << time.wHour << _T(":") << time.wMinute << _T(":") << time.wSecond << _T(".") << time.wMilliseconds;

    return tstring(stream.str());
}

long getfilesize(FILE * file)
{
    long filesize;
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    rewind(file);
    return filesize;
}

bool file_to_socket(const TCHAR * ptrFileName, CSmSocket * ptrSocket, TCHAR * ptrToken)
{
    DWORD id = \
        GetCurrentThreadId();

    FILE * file = NULL;
    
    if (_wfopen_s(&file, ptrFileName, _T("rb")) != 0) {
        _tprintf(_T("(%d) Can't open %s file for read!\n"), id, ptrFileName);
        return false;
    }

    void * data = \
        malloc(SHMEM_PIPE_SIZE/2);

    if (data == NULL) {
        _tprintf(_T("(%d) Can't allocate memory!\n"), id);
        fclose(file);
        return false;
    }

    int fsize = 0, total = 0, count = 0;

    fsize = getfilesize(file);

    bool result = true;

    while ( !feof(file) ) {
        count = fread(data, 1, SHMEM_PIPE_SIZE/2, file);
        if (ferror(file)) {
            _tprintf(_T("(%d) Can't read %s file!\n"), id, ptrFileName);
            result = false;
            break;
        }

        if (ptrSocket->SocketWrite(data, count, &count, TRUE) == FALSE) {
            _tprintf(_T("(%d) Can't send socket %s data!\n"), id, ptrSocket->GetSocketName().data());
            result = false;
            break;
        }

        total = total + count;

        if (ptrToken) {
            _tprintf(_T("\r(%d) %s: processed %d / %d bytes...\r"), id, ptrToken, total, fsize);
        }
    }

    if (ptrToken) {
        _tprintf(_T("\n"));
    }

    fclose(file);
    free(data);

    return result;
}

bool socket_to_file(CSmSocket * ptrSocket, const TCHAR * ptrFileName, TCHAR * ptrToken)
{
    DWORD id = \
        GetCurrentThreadId();

    FILE * file = NULL;
    
    if (_wfopen_s(&file, ptrFileName, _T("wb")) != 0) {
        _tprintf(_T("(%d) Can't open %s file for write!\n"), id, ptrFileName);
        return false;
    }

    void * data = \
        malloc(SHMEM_PIPE_SIZE/2);

    if (data == NULL) {
        _tprintf(_T("(%d) Can't allocate memory!\n"), id);
        fclose(file);
        return false;
    }

    int count = 0, total = 0;

    bool result = true;

    while (ptrSocket->SocketRead(data, SHMEM_PIPE_SIZE/2, &count, FALSE) == TRUE) {
        fwrite(data, 1, count, file);
        if (ferror(file)) {
            _tprintf(_T("(%d) Can't write %s file!\n"), id, ptrFileName);
            result = false;
            break;
        }

        total = total + count;
        
        if (ptrToken) {
            _tprintf(_T("\r(%d) %s: processed %d bytes...\r"), id, ptrToken, total);
        }
    }

    if (ptrToken) {
        _tprintf(_T("\n"));
    }

    fclose(file);
    free(data);

    return result;
}

/*
 *
 * SERVER's side
 *
 */

TCHAR * g_strServerFileName = NULL;

DWORD WINAPI ServerThreadClient(CSmSocket * ptrSocket)
{
    bool result;

#if (TEST_VALUE == TEST_CLIENT_TO_SERVER)
    result = socket_to_file(ptrSocket, ptrSocket->GetSocketName().data(), NULL);
#elif (TEST_VALUE == TEST_SERVER_TO_CLIENT)
    result = file_to_socket(g_strServerFileName, ptrSocket, NULL);
#endif

    _tprintf(_T("(%d) Client's thread completed for %s, status: %s...\n"), \
        GetCurrentThreadId(), ptrSocket->GetSocketName().data(), result == true ? _T("success") : _T("failure"));

    delete ptrSocket;

    return 0;
}

DWORD WINAPI ServerThread(CSmSocket * ptrSocket)
{
    CSmSocket * ptrNewSocket;

    while (ptrSocket->SocketAccept(&ptrNewSocket)) {
        DWORD id;
        HANDLE handleThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerThreadClient, ptrNewSocket, 0, &id);
        if (handleThread != NULL) {
            _tprintf(_T("Connection accepted for %s @ %s, thread %d\n"), \
                ptrNewSocket->GetSocketName().data(), timestamp().data(), id);
            CloseHandle(handleThread);
        } else {
            _tprintf(_T("Can't create client's thread for %s!\n"), ptrNewSocket->GetSocketName().data());
            delete ptrNewSocket;
        }
    }

    return 0;
}

int run_as_server(TCHAR * strSocketName, TCHAR * strFileName)
{
    HANDLE hThread;
    CSmSocket objSocket;

    g_strServerFileName = strFileName;

    if (objSocket.SocketCreate(strSocketName) == FALSE) {
        _tprintf(_T("Can't create socket object with name %s\n"), strSocketName);
        return -1;
    }

    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerThread, &objSocket, 0, NULL);
    if (hThread == NULL) {
        _tprintf(_T("Can't create server's thread!\n"));
        return -1;
    }

    _tprintf(_T("## Server started @ %s, press ENTER to terminate...\n"), timestamp().data());
    
    while (WaitForSingleObject(hThread, 100) != WAIT_OBJECT_0) {
        if (_kbhit() && _getch() == 13) {
            _tprintf(_T("## Stopping the server, please wait...\n"));
            objSocket.SocketAcceptCancel();
        }
    }

    objSocket.SocketClose();

    _tprintf(_T("## Server stopped @ %s\n"), timestamp().data());

    return 0;
}

/*
 *
 * CLIENT's side
 *
 */

TCHAR * g_strClientFileName = NULL;

DWORD WINAPI ClientThread(CSmSocket * ptrSocket)
{
    bool result;

#if (TEST_VALUE == TEST_CLIENT_TO_SERVER)
    result = file_to_socket(g_strClientFileName, ptrSocket, _T("SEND"));
#elif (TEST_VALUE == TEST_SERVER_TO_CLIENT)
    result = socket_to_file(ptrSocket, g_strClientFileName, _T("RECV"));
#endif

    _tprintf(_T("(%d) Client's thread completed for %s, status: %s...\n"), \
        GetCurrentThreadId(), ptrSocket->GetSocketName().data(), result == true ? _T("success") : _T("failure"));

    return 0;
}

int run_as_client(TCHAR * strSocketName, TCHAR * strTargetSocketName, TCHAR * strFileName)
{
    HANDLE hThread;
    CSmSocket objSocket;

    g_strClientFileName = strFileName;

    if (objSocket.SocketCreate(strSocketName) == FALSE) {
        _tprintf(_T("Can't create socket object with name %s!\n"), strSocketName);
        return -1;
    }

    if (objSocket.SocketConnect(strTargetSocketName, 10*1000) == FALSE) {
        _tprintf(_T("Can't connect socket object with name %s to %s!\n"), strSocketName, strTargetSocketName);
        return -1;
    }

    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ClientThread, &objSocket, 0, NULL);
    if (hThread == NULL) {
        _tprintf(_T("Can't create client's thread!\n"));
        return -1;
    }

    _tprintf(_T("## Client started @ %s, please wait...\n"), timestamp().data());

    WaitForSingleObject(hThread, INFINITE);

    _tprintf(_T("## Client completed @ %s\n"), timestamp().data());

    objSocket.SocketClose();

    return 0;
}

/*
 *
 * MAIN, program's entry point
 *
 */

int _tmain(int argc, TCHAR * argv[])
{
    if (argc >= 3) {
        if (_tcscmp(argv[1], _T("server")) == 0) {
#if (TEST_VALUE == TEST_CLIENT_TO_SERVER)
            if (argc == 3) {
#else
            if (argc == 4) {
#endif
                return run_as_server(argv[2], argv[3]);
            }
        }
        
        if (_tcscmp(argv[1], _T("client")) == 0) {
            if (argc == 5) {
                return run_as_client(argv[2], argv[3], argv[4]);
            }
        }
    }

    while ( TCHAR * ptrFileName = _tcsrchr(argv[0], _T('\\')) ) {
        argv[0] = ptrFileName + 1;
    }

#if (TEST_VALUE == TEST_CLIENT_TO_SERVER)
    _tprintf(_T("Usage: %s server <name>\n"), argv[0]);
    _tprintf(_T("       %s client <name> <target> filename\n"), argv[0]);
#elif (TEST_VALUE == TEST_SERVER_TO_CLIENT)
    _tprintf(_T("Usage: %s server <name> filename\n"), argv[0]);
    _tprintf(_T("       %s client <name> <target> filename\n"), argv[0]);
#else
    #error TEST_VALUE is incorrect!
#endif

    return -1;
}

