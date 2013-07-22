#include "CSmAfx.h"

//
// Вспомогательные функции
//

tstring IntToString(int intValue)
{
    tstringstream stream;
    stream << intValue;
    return tstring(stream.str());
}

//
// Public methods
//

BOOL CSmSocket::SocketCreate(tstring strName)
{
    if (GetSocketState() != StateNew) {
        return FALSE;
    }

    SocketInternalLock();

    m_strObjectName = strName;

    m_handleAcceptTimeoutTimer = CreateWaitableTimer(NULL, FALSE, NULL);
    m_handleConnectTimeoutTimer = CreateWaitableTimer(NULL, FALSE, NULL);

    if (m_handleAcceptTimeoutTimer && m_handleConnectTimeoutTimer) {
        SetSocketState(StateCreated);
    } else {
        CloseHandle(m_handleAcceptTimeoutTimer);
        m_handleAcceptTimeoutTimer = NULL;
        CloseHandle(m_handleConnectTimeoutTimer);
        m_handleConnectTimeoutTimer = NULL;

        m_strObjectName.clear();
    }

    SocketInternalUnlock();

    return (GetSocketState() == StateCreated);
}

BOOL CSmSocket::SocketConnect(tstring strTargetName, DWORD dwordTimeout)
{
    if (GetSocketState() != StateCreated) {
        return FALSE;
    }

    SocketInternalLock();

    if (dwordTimeout != INFINITE) {
        LARGE_INTEGER largeDueTime = { 0 };

        largeDueTime.QuadPart = dwordTimeout * 10000;
        largeDueTime.QuadPart = -largeDueTime.QuadPart;
            
        if (SetWaitableTimer(m_handleConnectTimeoutTimer, \
            &largeDueTime, 0, &CSmSocket::SocketConnectCancelApc, this, FALSE) == FALSE)
        {
            return FALSE;
        }
    }

    SetSocketState(StateConnecting);

    while (GetSocketState() == StateConnecting) {
        if (SharedMsgOpen(strTargetName)) {
            CSmMsg objMsg;

            if (SharedMsgAcquireLock()) {
                SHMEM_MSG message = { SHMEM_MSG_CONNECT, SHMEM_MSG_STATUS_SUCCESS };
                objMsg.SetData(&message, sizeof(message));

                // TODO: check for security
                //       check for message result

                if (SharedMsgSendRecvMessage(&objMsg)) {
                    PSHMEM_MSG ptrMsg = \
                        static_cast<PSHMEM_MSG>(objMsg.GetData());

                    tstring string = strTargetName + \
                        _T("-") + IntToString(ptrMsg->Message.Reply);                

                    if (SharedPipeOpen(string)) {
                        SetSocketState(StateConnected);
                    }
                }
                SharedMsgReleaseLock();
            }
            SharedMsgClose();
        }

        // TimerAPC may be here..
        SleepEx(0, TRUE);
    }

    if (dwordTimeout == INFINITE) {
        CancelWaitableTimer(m_handleConnectTimeoutTimer);
    }

    SocketInternalUnlock();

    return (GetSocketState() == StateConnected);
}

VOID CSmSocket::SocketClose()
{
    if (GetSocketState() == StateNew) {
        return;
    }

    SocketInternalLock();

    m_strObjectName.clear();

    SharedMsgClose();
    SharedPipeClose();

    SetSocketState(StateNew);

    SocketInternalUnlock();
}

BOOL CSmSocket::SocketAccept(CSmSocket ** ptrNewObject)
{
    ptrNewObject[0] = NULL;

    if (! (GetSocketState() == StateCreated || GetSocketState() == StateAccepted)) {
        return FALSE;
    }

    SocketInternalLock();

    if (GetSocketState() == StateCreated) {
        if (SharedMsgCreate(m_strObjectName) == FALSE) {
            SocketInternalUnlock();
            return FALSE;
        }
    }

    LONG longPeriod = 5*1000;
    DWORD dwordTimeout = 5*1000;
    LARGE_INTEGER largeDueTime = { 0 };

    largeDueTime.QuadPart = dwordTimeout * 10000;
    largeDueTime.QuadPart = -largeDueTime.QuadPart;

    if (SetWaitableTimer(m_handleAcceptTimeoutTimer, \
        &largeDueTime, longPeriod, &CSmSocket::SocketAcceptWakeupApc, this, FALSE) == FALSE)
    {
        SharedMsgClose();
        SocketInternalUnlock();
        return FALSE;
    }

    CSmMsg objMsg;

    SetSocketState(StateListening);
    
    while (GetSocketState() == StateListening) {
        if (SharedMsgRecvMessage(&objMsg)) {
            PSHMEM_MSG ptrMsg = \
                static_cast<PSHMEM_MSG>(objMsg.GetData());

            if (SHMEM_MSG_STATUS_ISOK(ptrMsg->Status) && ptrMsg->Message.Request == SHMEM_MSG_CONNECT) {
                CSmSocket * ptrObject = \
                    SocketSpawnSocket(m_intSpawnedCounter);

                if (ptrObject) {
                    SHMEM_MSG message = { m_intSpawnedCounter, SHMEM_MSG_STATUS_SUCCESS };

                    objMsg.SetData(&message, sizeof(message));
                    if (SharedMsgSendMessage(&objMsg)) {
                        m_intSpawnedCounter++;
                        ptrNewObject[0] = ptrObject;
                        SetSocketState(StateAccepted);
                    } else {
                        delete ptrObject;
                    }
                }
            }
        }
    }

    CancelWaitableTimer(m_handleConnectTimeoutTimer);

    SocketInternalUnlock();
    
    return (GetSocketState() == StateAccepted);
}

VOID CSmSocket::SocketAcceptCancel()
{
    SetSocketState(StateCreated);

    SocketInternalLock();

    SharedMsgClose();

    SocketInternalUnlock();
}

BOOL CSmSocket::SocketRead(LPVOID ptrData, INT intSize, INT * ptrRealSize, BOOL boolBatchMode)
{
    BOOL boolResult = FALSE;

    ptrRealSize[0] = 0;

    if (GetSocketState() != StateConnected) {
        return boolResult;
    }

    SocketInternalLock();

    if (boolBatchMode == FALSE) {
        boolResult = SharedPipeRead(ptrData, intSize, ptrRealSize);
    } else {
        INT intBytesRead = 0;
        INT intBytesToRead = intSize;

        while (boolResult = SharedPipeRead(ptrData, intBytesToRead, &intBytesRead)) {
            ptrData = (PCHAR)ptrData + intBytesRead;
            intBytesToRead = intBytesToRead - intBytesRead;
            if (intBytesToRead == 0) {
                ptrRealSize[0] = intSize;
                break;
            }
            SleepEx(0, TRUE);
        }
    }

    SocketInternalUnlock();

    return boolResult;
}

BOOL CSmSocket::SocketWrite(LPVOID ptrData, INT intSize, INT * ptrRealSize, BOOL boolBatchMode)
{
    BOOL boolResult = FALSE;

    ptrRealSize[0] = 0;

    if (GetSocketState() != StateConnected) {
        return boolResult;
    }

    SocketInternalLock();

    if (boolBatchMode == FALSE) {
        boolResult = SharedPipeWrite(ptrData, intSize, ptrRealSize);
    } else {
        INT intBytesWritten = 0;
        INT intBytesToWrite = intSize;

        while (boolResult = SharedPipeWrite(ptrData, intBytesToWrite, &intBytesWritten)) {
            ptrData = (PCHAR)ptrData + intBytesWritten;
            intBytesToWrite = intBytesToWrite - intBytesWritten;
            if (intBytesToWrite == 0) {
                ptrRealSize[0] = intSize;
                break;
            }
            SleepEx(0, TRUE);
        }
    }

    SocketInternalUnlock();

    return boolResult;
}

//
// Private methods
//

CSmSocket * CSmSocket::SocketSpawnSocket(DWORD dwordId)
{
    CSmSocket * ptrObject = new CSmSocket;

    tstring strObjectName = m_strObjectName + \
        _T("-") + IntToString(dwordId);

    if (ptrObject->SocketCreate(strObjectName)) {
        if (ptrObject->SharedPipeCreate(strObjectName)) {
            ptrObject->SetSocketState(StateConnected);
            return ptrObject;
        }
    }

    delete ptrObject;

    return NULL;
}

// NOTE: This is an APC
VOID CALLBACK CSmSocket::SocketAcceptWakeupApc(LPVOID ptrThis, DWORD, DWORD)
{
    CSmSocket * ptrObject = static_cast<CSmSocket *>(ptrThis);

    // State?..
}

// NOTE: This is an APC
VOID CALLBACK CSmSocket::SocketConnectCancelApc(LPVOID ptrThis, DWORD, DWORD)
{
    CSmSocket * ptrObject = static_cast<CSmSocket *>(ptrThis);

    ptrObject->SetSocketState(StateCreated);
}
