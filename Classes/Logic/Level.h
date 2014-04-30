/* 
 * File:   Level.h
 * Author: thunderliu
 *
 * Created on 2013��12��8��, ����10:31
 */

#ifndef __LEVEL_H__
#define	__LEVEL_H__


class CLevelExp;

class CLevelUpdate
{
public:
    virtual void updateExpRange(CLevelExp* pLevel) = 0; // @override
    virtual void onChangeLevel(CLevelExp* pLevel, int iChanged) = 0; // @override
};

// �ȼ�����ֵ���������ȼ�����ֵ����
// ��Ҫ���� updateExpRange���ṩ�ȼ��仯ʱ�������ֵ�����ʽ
// �ȼ��仯�󴥷� onChangeLevel
class CLevelExp
{
public:
    CLevelExp();
    virtual ~CLevelExp();
    
    virtual void updateExpRange(); // @override
    virtual void onChangeLevel(int iChanged); // @override
    void addExp(int iExp);
    void addLevel(int iLvl);
    
    M_SYNTHESIZE_READONLY(int, m_iLvl, Level);
    M_SYNTHESIZE_READONLY(int, m_iMaxLvl, MaxLevel);
    M_SYNTHESIZE(int, m_iExp, Exp);
    M_SYNTHESIZE(int, m_iBaseExp, BaseExp);
    M_SYNTHESIZE(int, m_iMaxExp, MaxExp);
    M_SYNTHESIZE_READONLY(CLevelUpdate*, m_pUpdate, Update);
    
    void setLevel(int iLvl);
    void setMaxLevel(int iMaxLvl);
    void setLevelUpdate(CLevelUpdate* pUpdate);
    bool canIncreaseExp() const;
};


#endif	/* __LEVEL_H__ */

