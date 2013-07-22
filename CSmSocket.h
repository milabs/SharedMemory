#ifndef __SHMEM_CSmSocket_H__
#define __SHMEM_CSmSocket_H__

#define SHMEM_MSG_STATUS_ISOK(x)    ((LONG)x >= 0)
#define SHMEM_MSG_STATUS_SUCCESS    0
#define SHMEM_MSG_STATUS_FAILURE   -1

#define SHMEM_MSG_NOP               0
#define SHMEM_MSG_CONNECT           1
#define SHMEM_MSG_DISCONNECT        2

typedef struct tagSHMEM_MSG
{
    union {
        DWORD Reply;
        DWORD Request;
    } Message;
    DWORD Status;
} SHMEM_MSG, * PSHMEM_MSG;

// Socket's state machine
enum SmSocketState
{
    StateNew = 0,
    StateCreated,
    StateConnecting,
    StateConnected,
    StateListening,
    StateAccepted,
};

class CSmSocket : public CSmSharedMsg, public CSmSharedPipe
{
    public:
        CSmSocket() : m_intSpawnedCounter(0)
            {
                InitializeCriticalSection(&m_objCriticalSection);
                SetSocketState(StateNew);
            };
       ~CSmSocket()
            {
                SocketClose();
            };

    public:
        BOOL SocketCreate(tstring strName);
        BOOL SocketConnect(tstring strTargetName, DWORD dwordTimeout);
        BOOL SocketAccept(CSmSocket ** ptrNewObject);
        VOID SocketAcceptCancel();
        BOOL SocketRead(LPVOID ptrData, INT intSize, INT * ptrRealSize, BOOL boolBatchMode);
        BOOL SocketWrite(LPVOID ptrData, INT intSize, INT * ptrRealSize, BOOL boolBatchMode);
        VOID SocketClose();

        const tstring & GetSocketName()
            {
                return m_strObjectName;
            };
    private:
        // Внутреннее состояние и счётчик порождений
        INT m_intState;
        INT m_intSpawnedCounter;

        // Критическая секция
        CRITICAL_SECTION m_objCriticalSection;

        // Имя объекта
        tstring m_strObjectName;

        // Таймеры
        HANDLE m_handleAcceptTimeoutTimer;
        HANDLE m_handleConnectTimeoutTimer;

    protected:
        CSmSocket * SocketSpawnSocket(DWORD dwordId);

        // Object's crtical section lock/unlock methods
        VOID SocketInternalLock()
            {
                EnterCriticalSection(&m_objCriticalSection);
            };
        VOID SocketInternalUnlock()
            {
                LeaveCriticalSection(&m_objCriticalSection);
            };

        // Object's state get/set methods
        INT GetSocketState()
            {
                return m_intState;
            };
        INT SetSocketState(enum SmSocketState intNewState)
            {
                return (m_intState = intNewState);
            };

        static VOID CALLBACK SocketAcceptWakeupApc(LPVOID ptrThis, DWORD, DWORD);
        static VOID CALLBACK SocketConnectCancelApc(LPVOID ptrThis, DWORD, DWORD);
};

#endif // __SHMEM_CSmSocket_H__