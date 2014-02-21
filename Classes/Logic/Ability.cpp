/* 
 * File:   Ability.cpp
 * Author: thunderliu
 * 
 * Created on 2013年12月8日, 下午11:45
 */

#include "CommInc.h"
#include "Unit.h"
#include "Ability.h"
#include "MultiRefObject.h"
#include "Application.h"
#include "Draw.h"


// CAbility
CAbility::CAbility(const char* pRootId, const char* pName, float fCoolDown)
: CONST_ROOT_ID(pRootId)
, m_sName(pName)
, m_pOwner(NULL)
, m_fCoolDown(fCoolDown)
, m_fCoolingDownElapsed(FLT_MAX)
, m_fInterval(0.0f)
, m_fIntervalElapsed(0.0f)
, m_dwTriggerFlags(0)
{
    setDbgClassName("CAbility");
}

CAbility::~CAbility()
{
}

const char* CAbility::getDbgTag() const
{
    return getName();
}

const char* CAbility::getRootId() const
{
    return CONST_ROOT_ID.c_str();
}

bool CAbility::isCoolingDown() const
{
    return m_fCoolingDownElapsed < getCoolDown();
}

void CAbility::resetCD()
{
    m_fCoolingDownElapsed = FLT_MAX;
    getOwner()->updateAbilityCD(getId());
}

void CAbility::coolDown()
{
    setCoolingDownElapsed(0.0f);
    getOwner()->abilityCD(this);
}

void CAbility::onUnitAddAbility()
{
}

void CAbility::setInterval(float fInterval)
{
    if (fInterval <= FLT_EPSILON)
    {
        m_fInterval = 0.0f;
        return;
    }
    
    setTriggerFlags(CUnit::kTickTrigger);
    m_fInterval = fInterval;
}

void CAbility::onUnitDelAbility()
{
}

void CAbility::onUnitAbilityReady()
{
}

void CAbility::onUnitRevive()
{
}

void CAbility::onUnitDie()
{
}

void CAbility::onUnitChangeHp(float fChanged)
{
}

void CAbility::onUnitTick(float dt)
{
}

void CAbility::onUnitInterval()
{
}

CAttackData* CAbility::onUnitAttackTarget(CAttackData* pAttack, CUnit* pTarget)
{
    return pAttack;
}

CAttackData* CAbility::onUnitAttacked(CAttackData* pAttack, CUnit* pSource)
{
    return pAttack;
}

void CAbility::onUnitDamaged(CAttackData* pAttack, CUnit* pSource)
{
}

void CAbility::onUnitDamagedDone(float fDamage, CUnit* pSource)
{
}

void CAbility::onUnitDamageTargetDone(float fDamage, CUnit* pTarget)
{
}

void CAbility::onUnitDestroyProjectile(CProjectile* pProjectile)
{
}

void CAbility::onAddToUnit(CUnit* pOwner)
{
    setOwner(pOwner);
    onUnitAddAbility();
}

void CAbility::onDelFromUnit()
{
    onUnitDelAbility();
    setOwner(NULL);
}

void CAbility::setTriggerFlags(uint32_t dwTriggerFlags)
{
    m_dwTriggerFlags |= dwTriggerFlags;
}

void CAbility::unsetTriggerFlags(uint32_t dwTriggerFlags)
{
    m_dwTriggerFlags &= ~dwTriggerFlags;
}

// CActiveAbility
CActiveAbility::CActiveAbility(const char* pRootId, const char* pName, float fCoolDown, CCommandTarget::TARGET_TYPE eCastType, uint32_t dwEffectiveTypeFlags)
: CAbility(pRootId, pName, fCoolDown)
, m_eCastTargetType(eCastType)
, m_dwEffectiveTypeFlags(dwEffectiveTypeFlags)
, m_fCastMinRange(0.0f)
, m_fCastRange(0.0f)
, m_fCastTargetRadius(0.0f)
, m_iTemplateProjectile(0)
, m_bCastHorizontal(false)
{
    setDbgClassName("CActiveAbility");
}

CActiveAbility::~CActiveAbility()
{
}

const float CActiveAbility::CONST_MAX_CAST_BUFFER_RANGE = 50.0f;
const float CActiveAbility::CONST_MAX_HOR_CAST_Y_RANGE = 5.0f;

bool CActiveAbility::cast()
{
    if (isCoolingDown() == true)
    {
        return false;
    }

    if (checkConditions() == false)
    {
        return false;
    }

    CUnit* o = getOwner();
    LOG("%s%s%s..", o->getName(), o->getAttackAbilityId() == getId() ? "的" : "施放了", getName());
    coolDown();
    onUnitCastAbility();  // onCastAbility在cd变化下面，所以可以添加重置cd的逻辑
    
    return true;
}

bool CActiveAbility::checkConditions()
{
    return true;
}

void CActiveAbility::onUnitCastAbility()
{
}

void CActiveAbility::addCastAnimation( int id )
{
    m_vecCastAnis.push_back(id);
}

int CActiveAbility::getCastRandomAnimation() const
{
    if (m_vecCastAnis.empty())
    {
        return -1;
    }

    return m_vecCastAnis[rand() % m_vecCastAnis.size()];
}

// CPassiveAbility
CPassiveAbility::CPassiveAbility(const char* pRootId, const char* pName, float fCoolDown)
: CAbility(pRootId, pName, fCoolDown)
{
    setDbgClassName("CPassiveAbility");
}

CPassiveAbility::~CPassiveAbility()
{
}

// CBuffAbility
CBuffAbility::CBuffAbility(const char* pRootId, const char* pName, float fDuration, bool bStackable)
: CPassiveAbility(pRootId, pName, 0.0f)
, m_fDuration(fDuration)
, m_fElapsed(0.0f)
, m_bStackable(bStackable)
, m_iSrcUnit(0)
{
    setDbgClassName("CBuffAbility");
}

CBuffAbility::~CBuffAbility()
{
}

bool CBuffAbility::isDone() const
{
    return m_fElapsed >= m_fDuration;
}

// CAttackAct
const float CAttackAct::CONST_MIN_ATTACK_SPEED_INTERVAL = 0.1f; // 0.1s
const float CAttackAct::CONST_MIN_ATTACK_SPEED_MULRIPLE = 0.2f; // 20%
const float CAttackAct::CONST_MAX_ATTACK_SPEED_MULRIPLE = 5.0f; // 500%

CAttackAct::CAttackAct(const char* pRootId, const char* pName, float fCoolDown, const CAttackValue& rAttackValue, float fAttackValueRandomRange)
: CActiveAbility(pRootId, pName, fCoolDown, CCommandTarget::kUnitTarget, CUnitForce::kEnemy)
, m_oAttackValue(rAttackValue)
, m_fAttackValueRandomRange(fAttackValueRandomRange)
{
    setDbgClassName("CAttackAct");
}

CMultiRefObject* CAttackAct::copy() const
{
    CAttackAct* pRet = new CAttackAct(getRootId(), getName(), m_fCoolDown, m_oAttackValue, m_fAttackValueRandomRange);
    pRet->setCastTargetType(getCastTargetType());
    pRet->setEffectiveTypeFlags(getEffectiveTypeFlags());
    pRet->setCastMinRange(getCastMinRange());
    pRet->setCastRange(getCastRange());
    pRet->setCastTargetRadius(getCastTargetRadius());
    pRet->setTemplateProjectile(getTemplateProjectile());
    pRet->setCastHorizontal(isCastHorizontal());
    pRet->m_vecCastAnis = m_vecCastAnis;
    return pRet;
}

void CAttackAct::onUnitAddAbility()
{
    getOwner()->setAttackAbilityId(getId());
}

void CAttackAct::onUnitDelAbility()
{
    getOwner()->setAttackAbilityId(0);
}

bool CAttackAct::checkConditions()
{
    CUnit* o = getOwner();
    CUnitDraw2D* d = DCAST(o->getDraw(), CUnitDraw2D*);
    assert(d != NULL);

    CUnit* t = o->getUnit(d->getCastTarget().getTargetUnit());
    if (t == NULL || t->isDead())
    {
        return false;
    }
    
    return true;
}

void CAttackAct::onUnitCastAbility()
{
    CUnit* o = getOwner();
    CUnitDraw2D* d = DCAST(o->getDraw(), CUnitDraw2D*);
    CUnit* t = o->getUnit(d->getCastTarget().getTargetUnit());
    
    if (t == NULL)
    {
        return;
    }
    
    CAttackData* ad = new CAttackData();
    for (int i = 0; i < CAttackValue::CONST_MAX_ATTACK_TYPE; ++i)
    {
        ad->setAttackValue((CAttackValue::ATTACK_TYPE)i, getRealAttackValue((CAttackValue::ATTACK_TYPE)i));
    }
    
    ad = o->attackAdv(ad, t);
    
    // 这里模拟命中
    if (ad == NULL)
    {
        return;
    }
    
    ad->retain();
    TEST_ATTACK_INFO* pAi = new TEST_ATTACK_INFO;
    pAi->iTarget = t->getId();
    pAi->pAttackData = ad;
    o->runAction(new CCallFunc(this, (FUNC_CALLFUNC_ND)&CAttackAct::onTestAttackEffect, pAi));
}

float CAttackAct::getBaseAttackValue(CAttackValue::ATTACK_TYPE eAttackType) const
{
    return m_oAttackValue.getValue(eAttackType);
}

void CAttackAct::setExAttackValue(CAttackValue::ATTACK_TYPE eAttackType, const CExtraCoeff& roExAttackValue)
{
    m_aoExAttackValue[eAttackType] = roExAttackValue;
}

const CExtraCoeff& CAttackAct::getExAttackValue(CAttackValue::ATTACK_TYPE eAttackType) const
{
    return m_aoExAttackValue[eAttackType];
}

float CAttackAct::getRealAttackValue(CAttackValue::ATTACK_TYPE eAttackType, bool bUseRandomRange) const
{
    if (bUseRandomRange)
    {
        float fAttackValueRandomRange = m_oExAttackValueRandomRange.getValue(m_fAttackValueRandomRange);
        if (fAttackValueRandomRange > 0.001)
        {
            return m_aoExAttackValue[eAttackType].getValue(m_oAttackValue.getValue(eAttackType)) * (1 - fAttackValueRandomRange * 0.5 + (rand() % (int)(fAttackValueRandomRange * 1000)) * 0.001);
        }
    }
    return m_aoExAttackValue[eAttackType].getValue(m_oAttackValue.getValue(eAttackType));
}

float CAttackAct::getCoolDown() const
{
    return getRealAttackInterval();
}

void CAttackAct::setBaseAttackInterval(float fAttackInterval)
{
    m_fCoolDown = max(fAttackInterval, CONST_MIN_ATTACK_SPEED_INTERVAL);
}

float CAttackAct::getBaseAttackInterval() const
{
    return m_fCoolDown;
}

float CAttackAct::getBaseAttackSpeed() const
{
    return 1 / max(getBaseAttackInterval(), FLT_EPSILON);
}

float CAttackAct::getRealAttackInterval() const
{
    float fAttackInterval = getBaseAttackInterval();
    // 取攻击速度系数，不得小于最小值
    float fMulriple = m_oExAttackSpeed.getMulriple();
    fMulriple = max(fMulriple, CONST_MIN_ATTACK_SPEED_MULRIPLE);
    // 计算两种最短攻击间隔中的最大值作为最短攻击间隔
    float fMinAttackSpeedInterval = fAttackInterval / CONST_MAX_ATTACK_SPEED_MULRIPLE;
    fMinAttackSpeedInterval = max(CONST_MIN_ATTACK_SPEED_INTERVAL, fMinAttackSpeedInterval);
    // 计算实际攻击间隔，不得小于上述最短攻击间隔
    float fRealAttackInterval = fAttackInterval / fMulriple;
    return max(fRealAttackInterval, fMinAttackSpeedInterval);
}

float CAttackAct::getRealAttackSpeed() const
{
    return 1 / getRealAttackInterval();
}

void CAttackAct::setExAttackSpeed(const CExtraCoeff& roExAttackSpeed)
{
    m_oExAttackSpeed = roExAttackSpeed;  // 必须保留原值，尽管有可能超出范围，否则技能删除后无法恢复
    updateAttackSpeed();
}

const CExtraCoeff& CAttackAct::getExAttackSpeed() const
{
    return m_oExAttackSpeed;
}

void CAttackAct::updateAttackSpeed()
{
    CUnit* o = getOwner();
    o->updateAbilityCD(getId());
    CUnitDraw2D* d = DCAST(o->getDraw(), CUnitDraw2D*);
    if (d->getCastActiveAbilityId() == o->getAttackAbilityId())
    {
        float spd = getBaseAttackInterval() / max(FLT_EPSILON, getRealAttackInterval());
        d->setActionSpeed(d->getCastActionId(), spd);
    }
    
}

void CAttackAct::onTestAttackEffect(CMultiRefObject* pObj, void* pData)
{
    TEST_ATTACK_INFO* pAi = (TEST_ATTACK_INFO*)pData;
    
    CUnit* o = getOwner();
    CUnit* t = o->getUnit(pAi->iTarget);
    CAttackData* ad = pAi->pAttackData;
    
    if (t != NULL && t->isDead() == false)
    {
        t->damagedAdv(ad, o);
    }
    
    ad->release();
    
    delete pAi;
}

// CBuffMakerAct
CBuffMakerAct::CBuffMakerAct(const char* pRootId, const char* pName, float fCoolDown, int iTemplateBuff, CCommandTarget::TARGET_TYPE eCastType, uint32_t dwEffectiveTypeFlags)
: CActiveAbility(pRootId, pName, fCoolDown, eCastType, dwEffectiveTypeFlags)
, m_iTemplateBuff(iTemplateBuff)
, m_pTarget(NULL)
{
    setDbgClassName("CBuffMakerAct");
}

CMultiRefObject* CBuffMakerAct::copy() const
{
    CBuffMakerAct* pRet = new CBuffMakerAct(getRootId(), getName(), getCoolDown(), getTemplateBuff(), getCastTargetType(), getEffectiveTypeFlags());
    pRet->setCastRange(getCastRange());
    pRet->setCastTargetRadius(getCastTargetRadius());
    pRet->setTemplateProjectile(getTemplateProjectile());
    return pRet;
}

bool CBuffMakerAct::checkConditions()
{
    CUnit* o = getOwner();
    CUnitDraw2D* d = DCAST(o->getDraw(), CUnitDraw2D*);
    assert(d != NULL);

    switch (getCastTargetType())
    {
    case CCommandTarget::kNoTarget:
        m_pTarget = o;
        break;
        
    case CCommandTarget::kUnitTarget:
        m_pTarget = o->getUnit(d->getCastTarget().getTargetUnit());
        if (m_pTarget != NULL && m_pTarget->isDead())
        {
            m_pTarget = NULL;
        }
        break;
        
    default:
        ;
    }
    
    if (m_pTarget != NULL &&
        !o->isEffective(DCAST(m_pTarget, CUnitForce*), getEffectiveTypeFlags()))
    {
        // 如果有待选目标(自身或命令目标)，但是无法作用
        if (getCastTargetRadius() <= FLT_EPSILON ||
            getEffectiveTypeFlags() == CUnitForce::kSelf)
        {
            // 如果不能扩展周围单位
            return false;
        }
    }
    
    return true;
}

void CBuffMakerAct::onUnitCastAbility()
{
    CUnit* o = getOwner();
    switch (getCastTargetType())
    {
    case CCommandTarget::kNoTarget:
    case CCommandTarget::kUnitTarget:
        if (o->isEffective(DCAST(m_pTarget, CUnitForce*), getEffectiveTypeFlags()))
        {
            m_pTarget->addBuffAbility(getTemplateBuff(), o->getId(), getLevel());
        }
        break;
        
    default:
        ;
    }
    
    if (getCastTargetRadius() <= FLT_EPSILON)
    {
        return;
    }
    
    CWorld* w = o->getWorld();
    CUnitDraw2D* od  = DCAST(o->getDraw(), CUnitDraw2D*);
    assert(od != NULL);
    CBuffAbility* pBuff = NULL;
    CWorld::MAP_UNITS& mapUnits = w->getUnits();
    M_MAP_FOREACH(mapUnits)
    {
        CUnit* u = M_MAP_EACH;
        M_MAP_NEXT;
        
        if (u == NULL || u->isDead())
        {
            continue;
        }
        
        if (!o->isEffective(DCAST(u, CUnitForce*), m_dwEffectiveTypeFlags))
        {
            continue;
        }

        CUnitDraw2D* ud  = DCAST(u->getDraw(), CUnitDraw2D*);
        assert(ud != NULL);
        if (ud->getPosition().getDistance(od->getPosition()) > getCastTargetRadius())
        {
            continue;
        }
        
        if (pBuff == NULL)
        {
            w->copyAbility(getTemplateBuff())->dcast(pBuff);
        }
        else
        {
            pBuff->copy()->dcast(pBuff);
        }
        
        u->addBuffAbility(pBuff);
    }
}

// CAuraPas
CAuraPas::CAuraPas(const char* pRootId, const char* pName, float fInterval, int iTemplateBuff, float fRange, uint32_t dwEffectiveTypeFlags)
: CPassiveAbility(pRootId, pName)
, m_iTemplateBuff(iTemplateBuff)
, m_fRange(fRange)
, m_dwEffectiveTypeFlags(dwEffectiveTypeFlags)
{
    setDbgClassName("CAuraPas");
    setInterval(fInterval);
}

CAuraPas::~CAuraPas()
{
}

CMultiRefObject* CAuraPas::copy() const
{
    return new CAuraPas(getRootId(), getName(), m_fInterval, m_iTemplateBuff, m_fRange, m_dwEffectiveTypeFlags);
}

void CAuraPas::onUnitInterval()
{
    CUnit* o = getOwner();
    CWorld* w = o->getWorld();
    CBuffAbility* pBuff = NULL;
    
    CUnitDraw2D* od = DCAST(o->getDraw(), CUnitDraw2D*);
    assert(od != NULL);
    CWorld::MAP_UNITS& mapUnits = w->getUnits();
    M_MAP_FOREACH(mapUnits)
    {
        CUnit* u = M_MAP_EACH;
        M_MAP_NEXT;
        
        if (u == NULL || u->isDead())
        {
            continue;
        }
        
        if (!o->isEffective(DCAST(u, CUnitForce*), m_dwEffectiveTypeFlags))
        {
            continue;
        }

        CUnitDraw2D* ud = DCAST(u->getDraw(), CUnitDraw2D*);
        assert(ud != NULL);
        if (ud->getPosition().getDistance(od->getPosition()) > m_fRange)
        {
            continue;
        }
        
        if (pBuff == NULL)
        {
            w->copyAbility(m_iTemplateBuff)->dcast(pBuff);
        }
        else
        {
            pBuff->copy()->dcast(pBuff);
        }
        
        u->addBuffAbility(pBuff);
    }
}

// CAttackBuffMakerPas
CAttackBuffMakerPas::CAttackBuffMakerPas(const char* pRootId, const char* pName, float fProbability, int iTemplateBuff, bool bToSelf, const CExtraCoeff& roExAttackValue)
: CPassiveAbility(pRootId, pName)
, m_fProbability(fProbability)
, m_iTemplateBuff(iTemplateBuff)
, m_bToSelf(bToSelf)
, m_oExAttackValue(roExAttackValue)
{
    setDbgClassName("CAttackBuffMakerPas");
    setTriggerFlags(CUnit::kAttackTargetTrigger);
}

CMultiRefObject* CAttackBuffMakerPas::copy() const
{
    CAttackBuffMakerPas* ret = new CAttackBuffMakerPas(getRootId(), getName(), m_fProbability, m_iTemplateBuff, m_bToSelf, m_oExAttackValue);
    ret->setCoolDown(getCoolDown());
    return ret;
}

CAttackData* CAttackBuffMakerPas::onUnitAttackTarget(CAttackData* pAttack, CUnit* pTarget)
{
    if (isCoolingDown())
    {
        return pAttack;
    }

    coolDown();

    if (M_RAND_HIT(m_fProbability) == false)
    {
        return pAttack;
    }
    
    for (int i = 0; i < CAttackValue::CONST_MAX_ATTACK_TYPE; ++i)
    {
        const CAttackValue& rAtkVal = pAttack->getAttackValue();
        if (rAtkVal.getValue((CAttackValue::ATTACK_TYPE)i) > FLT_EPSILON)
        {
            pAttack->setAttackValue((CAttackValue::ATTACK_TYPE)i, m_oExAttackValue.getValue(rAtkVal.getValue((CAttackValue::ATTACK_TYPE)i)));
        }
    }
    
    if (m_iTemplateBuff != 0)
    {
        if (isToSelf())
        {
            CUnit* o = getOwner();
            o->addBuffAbility(m_iTemplateBuff, o->getId(), getLevel());
        }
        else
        {
            pAttack->addAttackBuff(CAttackBuff(m_iTemplateBuff, getLevel()));
        }
    }
    
    return pAttack;
}

// CVampirePas
CVampirePas::CVampirePas(const char* pRootId, const char* pName, float fPercentConversion)
: CPassiveAbility(pRootId, pName)
, m_fPercentConversion(fPercentConversion)
{
    setDbgClassName("CVampirePas");
    setTriggerFlags(CUnit::kDamageTargetDoneTrigger);
}

CMultiRefObject* CVampirePas::copy() const
{
    return new CVampirePas(getRootId(), getName(), getPercentConversion());
}

void CVampirePas::onUnitDamageTargetDone(float fDamage, CUnit* pTarget)
{
    if (isCoolingDown())
    {
        return;
    }

    coolDown();

    CUnit* o = getOwner();
    float fDtHp = fDamage * getPercentConversion();
    o->setHp(o->getHp() + fDtHp);
    LOG("%s恢复%d点HP", o->getName(), toInt(fDtHp));
}

// CStunBuff
CStunBuff::CStunBuff(const char* pRootId, const char* pName, float fDuration, bool bStackable)
: CBuffAbility(pRootId, pName, fDuration, bStackable)
{
    setDbgClassName("CStunBuff");
}

CMultiRefObject* CStunBuff::copy() const
{
    return new CStunBuff(getRootId(), getName(), m_fDuration, m_bStackable);
}

void CStunBuff::onUnitAddAbility()
{
    CUnit* o = getOwner();
    o->suspend();
    
    LOG("%s%s中", o->getName(), getName());
}

void CStunBuff::onUnitDelAbility()
{
    CUnit* o = getOwner();
    o->resume();
    
    if (!o->isSuspended())
    {
        LOG("%s不在%s", o->getName(), getName());
    }
}

// CDoubleAttackBuff
CDoubleAttackBuff::CDoubleAttackBuff(const char* pRootId, const char* pName)
: CBuffAbility(pRootId, pName, 0.0f, true)
{
    setDbgClassName("CDoubleAttackBuff");
}

CMultiRefObject* CDoubleAttackBuff::copy() const
{
    return new CDoubleAttackBuff(getRootId(), getName());
}

void CDoubleAttackBuff::onUnitAddAbility()
{
    CUnit* o = getOwner();
    if (o->getAttackAbilityId() == 0)
    {
        return;
    }
    
    CAttackAct* pAtk = NULL;
    o->getActiveAbility(o->getAttackAbilityId())->dcast(pAtk);
    
    pAtk->resetCD();
    
    LOG("%s将进行%s", o->getName(), getName());
}

// CSpeedBuff
CSpeedBuff::CSpeedBuff(const char* pRootId, const char* pName, float fDuration, bool bStackable, const CExtraCoeff& roExMoveSpeedDelta, const CExtraCoeff& roExAttackSpeedDelta)
: CBuffAbility(pRootId, pName, fDuration, bStackable)
, m_oExMoveSpeedDelta(roExMoveSpeedDelta)
, m_oExAttackSpeedDelta(roExAttackSpeedDelta)
{
    setDbgClassName("CSpeedBuff");
}

CMultiRefObject* CSpeedBuff::copy() const
{
    return new CSpeedBuff(getRootId(), getName(), m_fDuration, m_bStackable, m_oExMoveSpeedDelta, m_oExAttackSpeedDelta);
}

void CSpeedBuff::onUnitAddAbility()
{
    CUnit* o = getOwner();
    
    CUnitDraw2D* od = DCAST(o->getDraw(), CUnitDraw2D*);
    assert(od != NULL);
    const CExtraCoeff& rExMs = od->getExMoveSpeed();
    od->setExMoveSpeed(CExtraCoeff(rExMs.getMulriple() + m_oExMoveSpeedDelta.getMulriple(), rExMs.getAddend() + m_oExMoveSpeedDelta.getAddend()));
    
    CAttackAct* pAtkAct = NULL;
    o->getActiveAbility(o->getAttackAbilityId())->dcast(pAtkAct);
    if (pAtkAct == NULL)
    {
        return;
    }
    
    float fTestOld = pAtkAct->getRealAttackInterval();
    const CExtraCoeff& rExAs = pAtkAct->getExAttackSpeed();
    pAtkAct->setExAttackSpeed(CExtraCoeff(rExAs.getMulriple() + m_oExAttackSpeedDelta.getMulriple(), rExAs.getAddend() + m_oExAttackSpeedDelta.getAddend()));
    
    LOG("%s攻击速度变慢(%.1fs->%.1fs)\n", o->getName(), fTestOld, pAtkAct->getRealAttackInterval());
}

void CSpeedBuff::onUnitDelAbility()
{
    CUnit* o = getOwner();
    
    CUnitDraw2D* d = DCAST(o->getDraw(), CUnitDraw2D*);
    const CExtraCoeff& rExMs = d->getExMoveSpeed();
    d->setExMoveSpeed(CExtraCoeff(rExMs.getMulriple() - m_oExMoveSpeedDelta.getMulriple(), rExMs.getAddend() - m_oExMoveSpeedDelta.getAddend()));
    
    CAttackAct* pAtkAct = NULL;
    o->getActiveAbility(o->getAttackAbilityId())->dcast(pAtkAct);
    if (pAtkAct == NULL)
    {
        return;
    }
    
    float fTestOld = pAtkAct->getRealAttackInterval();
    const CExtraCoeff& rExAs = pAtkAct->getExAttackSpeed();
    pAtkAct->setExAttackSpeed(CExtraCoeff(rExAs.getMulriple() - m_oExAttackSpeedDelta.getMulriple(), rExAs.getAddend() - m_oExAttackSpeedDelta.getAddend()));
    
    LOG("%s攻击速度恢复(%.1fs->%.1fs)\n", o->getName(), fTestOld, pAtkAct->getRealAttackInterval());
}

// CHpChangeBuff
CHpChangeBuff::CHpChangeBuff(const char* pRootId, const char* pName, float fDuration, bool bStackable, float fInterval, float fHpChange, bool bPercentile, float fMinHp)
: CBuffAbility(pRootId, pName, fDuration, bStackable)
, m_fHpChange(fHpChange)
, m_bPercentile(bPercentile)
, m_fMinHp(fMinHp)
{
    setDbgClassName("CHpChangeBuff");
    setInterval(fInterval);
}

CMultiRefObject* CHpChangeBuff::copy() const
{
    return new CHpChangeBuff(getRootId(), getName(), m_fDuration, m_bStackable, m_fInterval, m_fHpChange, m_bPercentile, m_fMinHp);
}

void CHpChangeBuff::onUnitAddAbility()
{
    CUnit* o = getOwner();
    LOG("%s获得%s状态(%.1f%s/s)\n", o->getName(), getName(), isPercentile() ? (getHpChange() * 100 / getInterval()) : (getHpChange() / getInterval()), isPercentile() ? "%" : "");
}

void CHpChangeBuff::onUnitDelAbility()
{
    CUnit* o = getOwner();
    LOG("%s失去%s状态\n", o->getName(), getName());
}

void CHpChangeBuff::onUnitInterval()
{
    CUnit* o = getOwner();
    float fNewHp = o->getHp();
    if (isPercentile())
    {
        fNewHp += o->getMaxHp() * m_fHpChange;
    }
    else
    {
        fNewHp += m_fHpChange;
    }

    if (fNewHp < m_fMinHp)
    {
        fNewHp = m_fMinHp;
    }
    o->setHp(fNewHp);
}

// CRebirthPas
CRebirthPas::CRebirthPas( const char* pRootId, const char* pName, float fCoolDown )
    : CPassiveAbility(pRootId, pName, fCoolDown)
    , m_bRevivableBefore(false)
{
    setTriggerFlags(CUnit::kDieTrigger);
}

CMultiRefObject* CRebirthPas::copy() const
{
    return new CRebirthPas(getRootId(), getName(), getCoolDown());
}

void CRebirthPas::onUnitAddAbility()
{
    CUnit* o = getOwner();
    setRevivableBefore(o->isRevivable());
    o->setRevivable();
}

void CRebirthPas::onUnitDelAbility()
{
    CUnit* o = getOwner();
    o->setRevivable(isRevivableBefore());
}

void CRebirthPas::onUnitDie()
{
    if (isCoolingDown())
    {
        return;
    }

    coolDown();

    CUnit* o = getOwner();
    CWorld* w = o->getWorld();

    CUnit* oo = w->getUnit(o->getId());
    if (oo != NULL)
    {
        oo->revive(500);
    }
    else
    {
        w->reviveUnit(o->getId(), 500);
    }
}
