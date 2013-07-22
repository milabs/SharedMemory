#ifndef __SHMEM_CSmSharedMsg_H__
#define __SHMEM_CSmSharedMsg_H__

typedef struct tagCLIENT_ID
{
    DWORD UniqueThread;
    DWORD UniqueProcess;
} CLIENT_ID, *PCLIENT_ID;

typedef struct tagSHMEM_MESSAGE_HEADER
{
    INT DataSize;
    CLIENT_ID ClientId;
} SHMEM_MESSAGE_HEADER, *PSHMEM_MESSAGE_HEADER;

class CSmSharedMsg : private CSmShared
{
    public:
        CSmSharedMsg()
            {
                m_handleMutex = NULL,
                m_handleEvent0 = NULL, 
                m_handleEvent1 = NULL;
            };
       ~CSmSharedMsg()
            {
                SharedMsgClose();
            };

    public:
        BOOL SharedMsgOpen(tstring strName);
        BOOL SharedMsgCreate(tstring strName);

        BOOL SharedMsgRecvMessage(CSmMsg * ptrMsg);
        BOOL SharedMsgSendMessage(CSmMsg * ptrMsg);

        VOID SharedMsgClose();

    private:
        HANDLE m_handleMutex;
        HANDLE m_handleEvent0;
        HANDLE m_handleEvent1;

    protected:
        BOOL SharedMsgAcquireLock();
        BOOL SharedMsgReleaseLock();
        BOOL SharedMsgSendRecvMessage(CSmMsg * ptrMsg);
};







#if 0
class CSmSharedMsg : public CSmShared
{
    public:
        CSmSharedMsg()
            {
                m_handleEventCS = NULL,
                m_handleEventSC = NULL;
            };
       ~CSmSharedMsg()
            {
                Destroy();
            };

        BOOL Create(tstring strName);
        VOID Destroy();

        BOOL MsgGetMessage(CSmMsg * ptrMsg);
        BOOL MsgSendMessage(CSmMsg * ptrMsg);

    private:
        HANDLE m_handleEventCS;
        HANDLE m_handleEventSC;

    protected:
        virtual BOOL MsgGetMessageHandler(LPVOID ptrMessage);
        virtual BOOL MsgSendMessageHandler(LPVOID ptrMessage);
};
#endif

#endif // __SHMEM_CSmSharedMsg_H__