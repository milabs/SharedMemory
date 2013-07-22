#ifndef __SHMEM_CSmSharedPipe_H__
#define __SHMEM_CSmSharedPipe_H__

#pragma pack(1)
typedef struct tagSHMEM_FIFO
{
    LONG Module;
    LONG TimeStamp;

    volatile LONG Size;
    volatile LONG PtrRead;
    volatile LONG PtrWrite;
} SHMEM_FIFO, * PSHMEM_FIFO;
#pragma pack()

#define SHMEM_FIFO_MEM(x)       (LPBYTE)((ULONG)x + sizeof(SHMEM_FIFO))
#define SHMEM_FIFO_MEM_READ(x)  (LPBYTE)(SHMEM_FIFO_MEM(x) + x->PtrRead)
#define SHMEM_FIFO_MEM_WRITE(x)  (LPBYTE)(SHMEM_FIFO_MEM(x) + x->PtrWrite)

class CSmSharedPipe : private CSmShared
{
    public:
        CSmSharedPipe()
            {
                m_ptrPipeReadEnd = NULL,
                m_ptrPipeWriteEnd = NULL;
            };
       ~CSmSharedPipe()
            {
                SharedPipeClose();
            };

    public:
        BOOL SharedPipeOpen(tstring ptrPipeName);
        BOOL SharedPipeCreate(tstring ptrPipeName);

        BOOL SharedPipeRead(LPVOID ptrData, INT intSize, INT * intRealSize);
        BOOL SharedPipeWrite(LPVOID ptrData, INT intSize, INT * intRealSize);

        VOID SharedPipeClose();
    private:
        PSHMEM_FIFO m_ptrPipeReadEnd;
        PSHMEM_FIFO m_ptrPipeWriteEnd;

    protected:
        BOOL SharedPipeKeepalive();
};

#endif // __SHMEM_CSmSharedPipe_H__