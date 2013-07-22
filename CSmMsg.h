#ifndef __SHMEM_CSmMsg_H__
#define __SHMEM_CSmMsg_H__

class CSmMsg
{
    public:
        CSmMsg() : m_intSize(0)
            {
                m_ptrData = NULL;
            };
       ~CSmMsg()
            {
                delete m_ptrData;
            };
        INT GetSize()
            {
                return m_intSize;
            };
        LPVOID SetData(LPVOID ptrData, INT intSize)
            {
                delete m_ptrData;
                m_intSize = intSize;
                m_ptrData = operator new (intSize);
                RtlCopyMemory(m_ptrData, ptrData, intSize);
                return m_ptrData;
            };
        LPVOID GetData()
            {
                return m_ptrData;
            };
    private:
        INT m_intSize;
        LPVOID m_ptrData;
};

#endif // __SHMEM_CSmMsg_H__