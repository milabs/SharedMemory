#include "CSmAfx.h"

BOOL CSmSharedPipe::SharedPipeOpen(tstring ptrPipeName)
{
    if (SharedMemoryOpen(ptrPipeName + _T("$pipe")) == FALSE) {
        return FALSE;
    }

    m_ptrPipeReadEnd = (PSHMEM_FIFO)((ULONG)SharedMemoryGetBase() + (SHMEM_PIPE_SIZE / 2));
    m_ptrPipeWriteEnd = (PSHMEM_FIFO)SharedMemoryGetBase();

    if (SharedPipeKeepalive()) {
        return TRUE;
    }

    SharedPipeClose();

    return FALSE;
}

BOOL CSmSharedPipe::SharedPipeCreate(tstring ptrPipeName)
{
    if (SharedMemoryCreate(ptrPipeName + _T("$pipe"), SHMEM_PIPE_SIZE) == FALSE) {
        return FALSE;
    }

    m_ptrPipeReadEnd = (PSHMEM_FIFO)SharedMemoryGetBase();
    m_ptrPipeReadEnd->Size = 0;
    m_ptrPipeReadEnd->Module = (SHMEM_PIPE_SIZE / 2) - sizeof(SHMEM_FIFO);
    m_ptrPipeReadEnd->PtrRead = 0;
    m_ptrPipeReadEnd->PtrWrite = 0;
    m_ptrPipeReadEnd->TimeStamp = GetTickCount();

    m_ptrPipeWriteEnd = (PSHMEM_FIFO)((ULONG)SharedMemoryGetBase() + (SHMEM_PIPE_SIZE / 2));
    m_ptrPipeWriteEnd->Size = 0;
    m_ptrPipeWriteEnd->Module = (SHMEM_PIPE_SIZE / 2) - sizeof(SHMEM_FIFO);
    m_ptrPipeWriteEnd->PtrRead = 0;
    m_ptrPipeWriteEnd->PtrWrite = 0;
    m_ptrPipeWriteEnd->TimeStamp = GetTickCount();

    return TRUE;
}

VOID CSmSharedPipe::SharedPipeClose()
{
    if (SharedMemoryGetBase()) {
        m_ptrPipeWriteEnd->TimeStamp = 0;
        SharedMemoryClose();
    }
}

BOOL CSmSharedPipe::SharedPipeRead(LPVOID ptrData, INT intSize, INT * intRealSize)
{
    intRealSize[0] = 0;

    if (SharedPipeKeepalive() == FALSE) {
        if (m_ptrPipeReadEnd->Size == 0) {
            return FALSE;
        }
    }

    if (intSize == 0) {
        return TRUE;
    }

    INT intPipeDataSize = \
        m_ptrPipeReadEnd->Size;

    if (intPipeDataSize == 0) {
        return TRUE;
    }

    intSize = intSize > intPipeDataSize ? \
        intPipeDataSize : intSize;

    INT intBytesToRead = \
        m_ptrPipeReadEnd->Module - m_ptrPipeReadEnd->PtrRead;

    if (intBytesToRead < intSize) {
        RtlCopyMemory(ptrData, SHMEM_FIFO_MEM_READ(m_ptrPipeReadEnd), intBytesToRead);

        ptrData = (PBYTE)ptrData + intBytesToRead;
        intBytesToRead = intSize - intBytesToRead;

        RtlCopyMemory(ptrData, SHMEM_FIFO_MEM(m_ptrPipeReadEnd), intBytesToRead);
    } else {
        RtlCopyMemory(ptrData, SHMEM_FIFO_MEM_READ(m_ptrPipeReadEnd), intSize); 
    }

    InterlockedExchange(&m_ptrPipeReadEnd->PtrRead, \
        (m_ptrPipeReadEnd->PtrRead + intSize) % m_ptrPipeReadEnd->Module);

    // Now it's really updated
    InterlockedExchangeAdd(&m_ptrPipeReadEnd->Size, -(LONG)intSize);

    intRealSize[0] = intSize;

    return TRUE;
}

BOOL CSmSharedPipe::SharedPipeWrite(LPVOID ptrData, INT intSize, INT * intRealSize)
{
    intRealSize[0] = 0;

    if (SharedPipeKeepalive() == FALSE) {
        return FALSE;
    }

    if (intSize == 0) {
        return TRUE;
    }

    INT intPipeDataSize = \
        m_ptrPipeWriteEnd->Size;

    if (intPipeDataSize == m_ptrPipeWriteEnd->Module) {
        return TRUE;
    }

    intSize = intSize > (m_ptrPipeWriteEnd->Module - intPipeDataSize) ? \
        m_ptrPipeWriteEnd->Module - intPipeDataSize : intSize;

    INT intBytesToWrite = \
        (m_ptrPipeWriteEnd->Module - m_ptrPipeWriteEnd->PtrWrite);

    if (intBytesToWrite < intSize) {
        RtlCopyMemory(SHMEM_FIFO_MEM_WRITE(m_ptrPipeWriteEnd), ptrData, intBytesToWrite);

        ptrData = (PBYTE)ptrData + intBytesToWrite;
        intBytesToWrite = intSize - intBytesToWrite;

        RtlCopyMemory(SHMEM_FIFO_MEM(m_ptrPipeWriteEnd), ptrData, intBytesToWrite);
    } else {
        RtlCopyMemory(SHMEM_FIFO_MEM_WRITE(m_ptrPipeWriteEnd), ptrData, intSize);
    }

    InterlockedExchange(&m_ptrPipeWriteEnd->PtrWrite, \
        (m_ptrPipeWriteEnd->PtrWrite + intSize) % m_ptrPipeWriteEnd->Module);

    // Now it's really updated
    InterlockedExchangeAdd(&m_ptrPipeWriteEnd->Size, (LONG)intSize);

    intRealSize[0] = intSize;

    return TRUE;
}

BOOL CSmSharedPipe::SharedPipeKeepalive()
{
    LONG deltaValue;

    m_ptrPipeWriteEnd->TimeStamp = GetTickCount() & 0x7FFFFFFF;

    if (m_ptrPipeWriteEnd->TimeStamp > m_ptrPipeReadEnd->TimeStamp) {
        deltaValue = m_ptrPipeWriteEnd->TimeStamp - m_ptrPipeReadEnd->TimeStamp;
    } else {
        deltaValue = m_ptrPipeReadEnd->TimeStamp - m_ptrPipeWriteEnd->TimeStamp;
    }

    // Timeout?..
    if (deltaValue > SHMEM_PIPE_TIMEOUT) {
        return FALSE;
    }

    return TRUE;
}
