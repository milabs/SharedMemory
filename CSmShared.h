#ifndef __SHMEM_CSmShared_H__
#define __SHMEM_CSmShared_H__

class CSmShared
{
    public:
        CSmShared() : m_intMemorySize(0)
            {
                m_ptrSharedMemory = NULL,
                m_handleSharedMemory = NULL;
            };
       ~CSmShared()
            {
                SharedMemoryClose();
            };

    public:
        BOOL SharedMemoryOpen(tstring strMemoryName);
        BOOL SharedMemoryCreate(tstring strMemoryName, INT intMemorySize);

        virtual BOOL SharedMemoryRead(LPVOID ptrData, INT intSize, INT * ptrResultSize);
        virtual BOOL SharedMemoryWrite(LPVOID ptrData, INT intSize, INT * ptrResultSize);
        virtual BOOL SharedMemoryAcquireLock();
        virtual BOOL SharedMemoryReleaseLock();

        INT SharedMemoryGetSize() { return m_intMemorySize; };
        LPVOID SharedMemoryGetBase() { return m_ptrSharedMemory; };
        tstring & SharedMemoryGetName() { return m_strMemoryName; };

        VOID SharedMemoryClose();

    private:
        INT m_intMemorySize;
        LPVOID m_ptrSharedMemory;
        HANDLE m_handleSharedMemory;
        tstring m_strMemoryName;
};

#endif // __SHMEM_CSmShared_H__