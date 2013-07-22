#include "CSmAfx.h"

BOOL CSmShared::SharedMemoryOpen(tstring strMemoryName)
{
    if (SharedMemoryGetBase()) {
        return TRUE;
    }

    m_intMemorySize = 0;
    m_strMemoryName = strMemoryName.empty() ? \
        NULL : _T("shmem$") + strMemoryName;

    m_handleSharedMemory = OpenFileMapping(FILE_MAP_WRITE, \
        FALSE, m_strMemoryName.data());

    if (m_handleSharedMemory) {
        m_ptrSharedMemory = MapViewOfFile(m_handleSharedMemory, FILE_MAP_WRITE, 0, 0, 0);
        if (m_ptrSharedMemory) {
            MEMORY_BASIC_INFORMATION meminfo;

            if (VirtualQuery(m_ptrSharedMemory, &meminfo, sizeof(meminfo))) {
                m_intMemorySize = meminfo.RegionSize;
            }

            return TRUE;
        }
    }

    m_strMemoryName.clear();

    return FALSE;
}

BOOL CSmShared::SharedMemoryCreate(tstring strMemoryName, INT intMemorySize)
{
    if (SharedMemoryGetBase()) {
        return TRUE;
    }

    m_intMemorySize = intMemorySize;
    m_strMemoryName = strMemoryName.empty() ? \
        NULL : _T("shmem$") + strMemoryName;

    m_handleSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, \
        NULL, PAGE_READWRITE, 0, (DWORD)intMemorySize, m_strMemoryName.data());

    if (m_handleSharedMemory) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            m_ptrSharedMemory = MapViewOfFile(m_handleSharedMemory, FILE_MAP_WRITE, 0, 0, 0);
            if (m_ptrSharedMemory) {
                return TRUE;
            }
        }
        CloseHandle(m_handleSharedMemory);
    }

    m_intMemorySize = 0;
    m_strMemoryName.clear();

    return FALSE;
}

VOID CSmShared::SharedMemoryClose()
{
    if (SharedMemoryGetBase()) {
        m_intMemorySize = 0;
        m_strMemoryName.clear();

        CloseHandle(m_handleSharedMemory);
        m_handleSharedMemory = NULL;

        UnmapViewOfFile(m_ptrSharedMemory);
        m_ptrSharedMemory = NULL;
    }
}

BOOL CSmShared::SharedMemoryRead(LPVOID ptrData, INT intSize, INT * ptrResultSize)
{
    intSize = (intSize > m_intMemorySize) ? m_intMemorySize : intSize;
    RtlCopyMemory(ptrData, m_ptrSharedMemory, intSize);
    ptrResultSize[0] = intSize;

    return TRUE;
}

BOOL CSmShared::SharedMemoryWrite(LPVOID ptrData, INT intSize, INT * ptrResultSize)
{
    intSize = (intSize > m_intMemorySize) ? m_intMemorySize : intSize;
    RtlCopyMemory(m_ptrSharedMemory, ptrData, intSize);
    ptrResultSize[0] = intSize;

    return TRUE;
}

BOOL CSmShared::SharedMemoryAcquireLock()
{
    return TRUE;
}

BOOL CSmShared::SharedMemoryReleaseLock()
{
    return TRUE;
}
