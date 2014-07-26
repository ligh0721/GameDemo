/* 
 * File:   Network.h
 * Author: thunderliu
 *
 * Created on 2014��7��24��, ����0:45
 */

#ifndef __NETWORK_H__
#define	__NETWORK_H__


template <typename TYPE>
class CSyncQueue : public queue<TYPE>
{
public:
    bool syncPop(TYPE& val);
    void syncPush(const TYPE& val);

protected:
    mutex m_mtx;
    condition_variable m_cv;
};

class CCommandNode
{
public:
    enum CMD_CODE
    {
        kEmpty
    };

public:
    CCommandNode();

    void clear();

    int cmdCode;
    string cmdData;
};

// ÿ��Unit�Լ�World������һ���ö��󣬸ö��󻺴���յ��������World����֡��ʱ��ִ��
class CCommandCache
{
public:
    // �������������������
    enum MTX_CMD_INDEX
    {
        kUnitPosUpdate,  // ��λλ�ø���
        kUnitOprCmd
    };

    typedef vector<CCommandNode> VEC_MTX_CMDS;
    typedef queue<CCommandNode> QUE_SEQ_CMDS;

public:
    CCommandCache();

    CCommandNode* getMutexCommandNodeToWrite(int cmdIndex);
    CCommandNode* getSequenceCommandToWrite();

protected:
    //M_SYNTHESIZE(uint32_t, m_dwMtxCmdFlags, MutexCommandFlags);
    M_SYNTHESIZE_READONLY_PASS_BY_REF(VEC_MTX_CMDS, m_mtxCmds, MutexCommands);
    M_SYNTHESIZE_READONLY_PASS_BY_REF(QUE_SEQ_CMDS, m_seqCmds, SequenceCommands);

};









//////////////////////// Inline /////////////////////////////////

// CSyncQueue
template <typename TYPE>
inline bool CSyncQueue<TYPE>::syncPop(TYPE& val)
{
    unique_lock<mutex> g(m_mtx);

    while (this->empty())
    {
        m_cv.wait(g);
    }

    val = this->front();
    this->pop();

    return true;
}

template <typename TYPE>
inline void CSyncQueue<TYPE>::syncPush(const TYPE& val)
{
    unique_lock<mutex> g(m_mtx);

    this->push(val);

    m_cv.notify_one();
}




#endif	/* __NETWORK_H__ */

