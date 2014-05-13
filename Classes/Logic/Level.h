/* 
 * File:   Level.h
 * Author: thunderliu
 *
 * Created on 2013��12��8��, ����10:31
 */

#ifndef __LEVEL_H__
#define	__LEVEL_H__

#include "MultiRefObject.h"


class CLevelExp;

class CLevelUpdate : public CMultiRefObject
{
public:
    CLevelUpdate();
    virtual ~CLevelUpdate();
    virtual CLevelUpdate* copy() override;

    M_SYNTHESIZE(int, m_iScriptHandler, ScriptHandler);

    virtual void updateExpRange(CLevelExp* pLevel);
    virtual void onChangeLevel(CLevelExp* pLevel, int iChanged);
    virtual int calcExp(int iLevel);
};

// �ȼ�����ֵ���������ȼ�����ֵ����
// ��Ҫ���� updateExpRange���ṩ�ȼ��仯ʱ�������ֵ�����ʽ
// �ȼ��仯�󴥷� onChangeLevel
class CLevelExp : public CMultiRefObject
{
public:
    CLevelExp();
    virtual ~CLevelExp();
    virtual CLevelExp* copy() override;
    
    virtual void updateExpRange(); // @override
    virtual void onChangeLevel(int iChanged); // @override
    void addLevel(int iLvl);
    void addExp(int iExp);
    
    M_SYNTHESIZE_READONLY(int, m_iLvl, Level);
    M_SYNTHESIZE_READONLY(int, m_iMaxLvl, MaxLevel);
    M_SYNTHESIZE(int, m_iExp, Exp);
    M_SYNTHESIZE(int, m_iBaseExp, BaseExp);
    M_SYNTHESIZE(int, m_iMaxExp, MaxExp);
    M_SYNTHESIZE_READONLY(CLevelUpdate*, m_pUpdate, Update);
    void setExpRange(int iFrom, int iTo);
    
    void setLevel(int iLvl);
    void setMaxLevel(int iMaxLvl);
    void setLevelUpdate(CLevelUpdate* pUpdate);
    bool canIncreaseExp() const;
};


#endif	/* __LEVEL_H__ */

