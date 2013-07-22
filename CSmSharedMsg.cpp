#include "CSmAfx.h"

//
// public interface
//

BOOL CSmSharedMsg::SharedMsgCreate(tstring strName)
{
    if (SharedMemoryCreate(strName, SHMEM_MSG_SIZE) == FALSE) {
        return FALSE;
    }

    m_handleMutex = CreateMutex(NULL, \
        FALSE, (SharedMemoryGetName() + _T("$mutex")).data());

    m_handleEvent0 = CreateEvent(NULL, \
        FALSE, FALSE, (SharedMemoryGetName() + _T("$event0")).data());

    m_handleEvent1 = CreateEvent(NULL, \
        FALSE, FALSE, (SharedMemoryGetName() + _T("$event1")).data());

    if (m_handleMutex && (m_handleEvent0 && m_handleEvent1)) {
        return TRUE;
    }
    
    SharedMsgClose();

    return FALSE;
}

BOOL CSmSharedMsg::SharedMsgOpen(tstring strName)
{
    if (SharedMemoryOpen(strName) == FALSE) {
        return FALSE;
    }

    m_handleMutex = OpenMutex(MUTEX_ALL_ACCESS, \
        FALSE, (SharedMemoryGetName() + _T("$mutex")).data());

    m_handleEvent0 = OpenEvent(EVENT_ALL_ACCESS, \
        FALSE, (SharedMemoryGetName() + _T("$event0")).data());

    m_handleEvent1 = OpenEvent(EVENT_ALL_ACCESS, \
        FALSE, (SharedMemoryGetName() + _T("$event1")).data());

    if (m_handleMutex && (m_handleEvent0 && m_handleEvent1)) {
        return TRUE;
    }

    SharedMsgClose();

    return FALSE;
}

VOID CSmSharedMsg::SharedMsgClose()
{
    SharedMemoryClose();

    CloseHandle(m_handleEvent0);
    m_handleEvent0 = NULL;

    CloseHandle(m_handleEvent1);
    m_handleEvent1 = NULL;

    SharedMsgReleaseLock();

    CloseHandle(m_handleMutex);
    m_handleMutex = NULL;
}

// Must be called under lock
BOOL CSmSharedMsg::SharedMsgSendMessage(CSmMsg * ptrMsg)
{
    PSHMEM_MESSAGE_HEADER ptrMsgHeader = \
        static_cast<PSHMEM_MESSAGE_HEADER>(SharedMemoryGetBase());

    ptrMsgHeader->ClientId.UniqueThread = GetCurrentThreadId();
    ptrMsgHeader->ClientId.UniqueProcess = GetCurrentProcessId();

    ptrMsgHeader->DataSize = ptrMsg->GetSize();
    RtlCopyMemory(ptrMsgHeader + 1, ptrMsg->GetData(), ptrMsg->GetSize());

    return (SignalObjectAndWait(m_handleEvent0, m_handleEvent1, INFINITE, TRUE) == WAIT_OBJECT_0);
}

BOOL CSmSharedMsg::SharedMsgRecvMessage(CSmMsg * ptrMsg)
{
    PSHMEM_MESSAGE_HEADER ptrMsgHeader = \
        static_cast<PSHMEM_MESSAGE_HEADER>(SharedMemoryGetBase());

    if (ptrMsgHeader == NULL) {
        return FALSE;
    }

    if (WaitForSingleObjectEx(m_handleEvent0, INFINITE, TRUE) != WAIT_OBJECT_0) {
        return FALSE;
    }

    ptrMsg->SetData(ptrMsgHeader + 1, ptrMsgHeader->DataSize);

    return SetEvent(m_handleEvent1);
}

BOOL CSmSharedMsg::SharedMsgSendRecvMessage(CSmMsg * ptrMsg)
{
    return SharedMsgSendMessage(ptrMsg) && SharedMsgRecvMessage(ptrMsg);
}

//
// protected interface
//

BOOL CSmSharedMsg::SharedMsgAcquireLock()
{
    DWORD dwordResult = \
        WaitForSingleObjectEx(m_handleMutex, INFINITE, TRUE);

    if (dwordResult == WAIT_OBJECT_0 || dwordResult == WAIT_ABANDONED_0) {
        return TRUE;
    }
    
    return FALSE;
}
    
BOOL CSmSharedMsg::SharedMsgReleaseLock()
{
    if (ReleaseMutex(m_handleMutex)) {
        return TRUE;
    }

    return FALSE;
}
