#include "CommHeader.h"

#include "Unit.h"
#include "Ability.h"
#include "BattleScene.h"
#include "GameControl.h"
#include "DrawForCC.h"
#include "AbilityForCC.h"
#include "ActionForCC.h"
#include "LuaBinding.h"
#include "LuaBindingForCC.h"
#include "ComponentForCC.h"


class CHeroLevelUp : public CLevelUpdate
{
public:
    int funcExp(int lvl)
    {
        return (lvl == 0) ? 0 : (lvl * lvl * 9 + lvl * 3 + 8);
    }

    virtual void updateExpRange(CLevelExp* pLevel)
    {
        int lvl = pLevel->getLevel();
        pLevel->setBaseExp(funcExp(lvl - 1));
        pLevel->setMaxExp(funcExp(lvl));
    }

    virtual void onLevelChange(CLevelExp* pLevel, int iChanged)
    {
        CCLOG("Level Up!!!");
        static SimpleAudioEngine* ae = SimpleAudioEngine::sharedEngine();
        
        CUnit* u = DCAST(pLevel, CUnit*);
        CUnitDraw2D* d = DCAST(u->getDraw(), CUnitDraw2D*);
        d->cmdStop();
        //d->stopAllActions();
        u->suspend();
        d->doAnimation(CUnitDraw::kAniAct5, NULL, 1, new CCallFuncData(u->getWorld(), (FUNC_CALLFUNC_ND)(&CBattleWorld::onAniDone)));

        ae->playEffect("sounds/Effect/LevelUp.mp3");
        
        u->setMaxHp(u->getMaxHp() * 1.2);
    }
};

class CMyAI : public CDefaultAI
{
public:
    CMyAI()
        : m_bFirstTick(true)
        , m_iBaseTag(CKeyGen::nextKey() * 10)
        , m_iCurTag(m_iBaseTag)
    {
    }

    virtual void onUnitTick(CUnit* pUnit, float dt)
    {
        CDefaultAI::onUnitTick(pUnit, dt);
        return;

        CUnitDraw2D* d = DCAST(pUnit->getDraw(), CUnitDraw2D*);
        if (isFirstTick())
        {
            setFirstTick(false);
            setDstPoint(d->getPosition());
            setOrgPoint(getDstPoint() + CPoint(300.0f, 300.0f));
            return;
        }

        if (getDstPoint().getDistance(d->getPosition()) < 10.0f)
        {
            CPoint tmp = getOrgPoint();
            setOrgPoint(getDstPoint());
            setDstPoint(tmp);
            d->cmdMove(getDstPoint());
        }
    }

    M_SYNTHESIZE_BOOL(FirstTick);
    M_SYNTHESIZE_PASS_BY_REF(CPoint, m_oOrgPoint, OrgPoint);
    M_SYNTHESIZE_PASS_BY_REF(CPoint, m_oDstPoint, DstPoint);
    M_SYNTHESIZE(int, m_iBaseTag, BaseTag);
    M_SYNTHESIZE(int, m_iCurTag, CurTag);
};

// CBattleWorld
const float CBattleWorld::CONST_MAX_REWARD_RANGE = 400;

CBattleWorld::CBattleWorld()
{
    lua_State* L = CWorld::getLuaHandle();
}

CBattleWorld::~CBattleWorld()
{
}

bool CBattleWorld::onInit()
{
    CCSize vs = CCDirector::sharedDirector()->getVisibleSize();
    CCPoint org = CCDirector::sharedDirector()->getVisibleOrigin();

    CUnit* u = NULL;
    CUnitDrawForCC* d = NULL;
    CAbility* a = NULL;
    int id = 0;
    CAttackAct* atk = NULL;

    M_DEF_GC(gc);
    gc->loadTexture("Global0");
    gc->loadTexture("Heroes0");
    gc->loadTexture("Heroes1");
    gc->loadTexture("Heroes2");
    gc->loadTexture("Heroes3");
    gc->loadTexture("Heroes4");
    gc->loadTexture("Heroes5");
    gc->loadTexture("Projectiles0");
    gc->loadTexture("Battle0");

    gc->loadAnimation("Effects/Lightning", "Effects/Lightning", 0.05f);
    gc->loadAnimation("Effects/Lightning2", "Effects/Lightning2", 0.05f);
    gc->loadAnimation("Effects/Lightning3", "Effects/Lightning3", 0.05f);
    gc->loadAnimation("Effects/BigStun", "Effects/BigStun", 0.1f);
    gc->loadAnimation("Effects/SmallStun", "Effects/SmallStun", 0.1f);

    m_oULib.init();

    // init hero
    u = m_oULib.copyUnit(CUnitLibraryForCC::kThor);
    addUnit(u);
    setControlUnit(u->getId());
    setHero(u);
    u->setMaxLevel(20);
    u->setLevelUpdate(new CHeroLevelUp);
    u->updateExpRange();
    u->setRevivable();
    u->setForceByIndex(2);
    CForceResource* fr = new CForceResource(this, (FUNC_CALLFUNC_N)(&CBattleWorld::onChangeGold)); // ������Դ
    u->setResource(fr);
    u->getActiveAbility(u->getAttackAbilityId())->dcast(atk);
    atk->setExAttackValue(CExtraCoeff(1.3f, 0.0f));

    a = new CSpeedBuff("ThunderCap", "ThunderCap", 5.0f, false, CExtraCoeff(-0.5f, 0.0f), CExtraCoeff(-0.5f, 0.0f));
    id = addTemplateAbility(a);
    a = new CDamageBuffMakerBuff("", CAttackValue(CAttackValue::kMagical, 200.0f), 1.0f, id);
    id = addTemplateAbility(a);
    m_pThunderCapAct = new CBuffMakerAct("", "ThunderCap", 5.0f, CCommandTarget::kNoTarget, CUnitForce::kEnemy, 1.0f, id);
    m_pThunderCapAct->setCastTargetRadius(150.0f);
    m_pThunderCapAct->addCastAnimation(CUnitDraw::kAniAct2);
    u->addActiveAbility(m_pThunderCapAct);

    a = new CStunBuff("Stun", "Stun", 5.0f, false);
    id = addTemplateAbility(a);
    a = new CDamageBuffMakerBuff("", CAttackValue(CAttackValue::kMagical, 300.0f), 1.0f, id);
    id = addTemplateAbility(a);
    m_pHammerThrowAct = new CBuffMakerAct("", "HammerThrow", 5.0f, CCommandTarget::kUnitTarget, CUnitForce::kEnemy, 1.0f, id);
    m_pHammerThrowAct->setCastRange(400.0f);
    m_pHammerThrowAct->setCastTargetRadius(150.0f);
    m_pHammerThrowAct->addCastAnimation(CUnitDraw::kAniAct3);
    m_pHammerThrowAct->setTemplateProjectile(CUnitLibraryForCC::kThorHammer);
    u->addActiveAbility(m_pHammerThrowAct);

    a = new CChangeHpPas("AutoHeal", "AutoHeal", 0.2f, CExtraCoeff(0.001f, 0.0f));
    id = addTemplateAbility(a);
    u->addPassiveAbility(id);

    d = DCAST(u->getDraw(), CUnitDrawForCC*);
    d->setPosition(CPoint(800, 600));

    onLuaWorldInit();
    
    CCBattleSceneLayer* l = DCAST(getLayer(), CCBattleSceneLayer*);
    l->initTargetInfo();
    l->updateTargetInfo(getControlUnit());

    l->initResourceInfo();
    l->updateResourceInfo(fr->getGold());

    l->initHeroPortrait();
    l->updateHeroPortrait();

    // DemoTemp
    static SimpleAudioEngine* ae = SimpleAudioEngine::sharedEngine();
    ae->preloadEffect("sounds/Units/Thor/move/00.mp3");
    ae->preloadEffect("sounds/Units/Thor/move/01.mp3");
    ae->preloadEffect("sounds/Units/Thor/move/02.mp3");
    ae->preloadEffect("sounds/Units/Thor/move/03.mp3");
    ae->preloadEffect("sounds/Units/Thor/die/00.mp3");
    ae->preloadEffect("sounds/Effect/WaveIncoming.mp3");

    ae->preloadEffect("sounds/Effect/Fighting.mp3");
    ae->preloadEffect("sounds/Effect/HammerThrow.mp3");
    ae->preloadEffect("sounds/Effect/LevelUp.mp3");
    ae->preloadEffect("sounds/Effect/OpenDoor.mp3");
    ae->preloadEffect("sounds/Effect/TeslaRay00.mp3");
    ae->preloadEffect("sounds/Effect/TeslaRay01.mp3");
    ae->preloadEffect("sounds/Effect/ThunderCap.mp3");
    ae->preloadEffect("sounds/Effect/ArcaneRay.mp3");
    ae->preloadEffect("sounds/Effect/UIMove.mp3");
    ae->preloadEffect("sounds/Effect/ArrowRelease01.mp3");
    ae->preloadEffect("sounds/Effect/ArrowRelease02.mp3");
    ae->preloadEffect("sounds/Effect/MageShot.mp3");
    

    ae->playBackgroundMusic("sounds/Background/Prebattle_Rising_Tides.mp3", true);
    ae->playEffect("sounds/Effect/WaveIncoming.mp3");


    return true;
}

// DemoTemp
float MAX_MOVE[] = {1.5f, 2.5f, 2.5f, 1.5f};
int g_moveIndex = 0;
float g_move = 100.0f;
float MAX_FIGHT = 8.0f;
float g_fight = 100.0f;
int g_fightId = 0;
float g_fightFree = 0.0f;

void CBattleWorld::onTick( float dt )
{
    // DemoTemp
    static SimpleAudioEngine* ae = SimpleAudioEngine::sharedEngine();
    g_move += dt;
    g_fight += dt;
    g_fightFree += dt;
    if (g_fightId != 0 && g_fightFree > 2.0f)
    {
        ae->stopEffect(g_fightId);
        g_fightId = 0;
        g_fight = 100.0f;
    }
    
    onLuaWorldTick(dt);

    CCBattleSceneLayer* l = DCAST(getLayer(), CCBattleSceneLayer*);
    l->updateTargetInfo();
    l->updateHeroPortrait();
}


bool CBattleWorld::onLuaWorldInit()
{
    // lua
    lua_State* L = getLuaHandle();
    CCBattleSceneLayer* layer = DCAST(getLayer(), CCBattleSceneLayer*);

    luaL_insertloader(L, luaModuleLoader4cc);
    luaRegCommFunc(L);
    luaRegWorldFuncs(L, this);
    luaRegWorldFuncsForCC(L, this);

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    string name = "/sdcard/ts/gamedemo/world.lua";
#elif CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    string name = "/var/mobile/Documents/world.lua";
#else
    string name = "scripts/world.lua";
#endif
    CCLOG("MSG | loadScript: %s", name.c_str());

    string err;
    if (luaL_loadscript4cc(L, name.c_str(), err) == false)
    {
        CCLOG("ERR | LuaErr: %s", err.c_str());
        layer->log("%s", err.c_str());

        return false;
    }

    lua_getglobal(L, "onWorldInit");
    int res = lua_pcall(L, 0, 0, 0);
    if (res != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        CCLOG("ERR | LuaErr: %s", err);
        lua_pop(L, 1);
        layer->log("%s", err);

        return false;
    }

    assert(lua_gettop(L) == 0);

    return true;
}

void CBattleWorld::onLuaWorldTick( float dt )
{
    lua_State* L = getLuaHandle();
    CCBattleSceneLayer* layer = DCAST(getLayer(), CCBattleSceneLayer*);

    lua_getglobal(L, "onWorldTick");
    lua_pushnumber(L, dt);
    int res = lua_pcall(L, 1, 0, 0);
    if (res != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        CCLOG("ERR | LuaErr: %s", err);
        lua_pop(L, 1);
        layer->log("%s", err);

        return;
    }

    assert(lua_gettop(L) == 0);
}

CProjectile* CBattleWorld::copyProjectile( int id ) const
{
    return m_oULib.copyProjectile(id);
}

void CBattleWorld::onUnitDying(CUnit* pUnit)
{
    static SimpleAudioEngine* ae = SimpleAudioEngine::sharedEngine();

    CUnitDraw2D* d = DCAST(pUnit->getDraw(), CUnitDraw2D*);
    if (pUnit == getHero())
    {
        ae->playEffect("sounds/Units/Thor/die/00.mp3");
    }
    else if (!d->isFixed())
    {
        char sz[256];
        sprintf(sz, "sounds/Effect/HumenDie%02d.mp3", rand() % 4);
        ae->playEffect(sz);
    }
    else
    {
    }

    if (pUnit != getHero() && pUnit->getRewardExp() && pUnit->isEnemyOf(getHero()) && M_RAND_HIT(1.0f))
    {
        // ������Ʒ
    }

    // TODO: ��Ǯ�������
    //CWorld* w = pUnit->getWorld();
    CWorld::MAP_UNITS& units = getUnits();
    vector<CUnit*> vec;
    
    M_MAP_FOREACH(units)
    {
        CUnit* uu = M_MAP_EACH;
        CUnitDraw2D* dd = DCAST(uu->getDraw(), CUnitDraw2D*);
        if (!uu->isDead() && uu->canIncreaseExp() && uu->isEnemyOf(pUnit) && dd->getPosition().getDistance(d->getPosition()) < CBattleWorld::CONST_MAX_REWARD_RANGE)
        {
            vec.push_back(uu);
        }
        M_MAP_NEXT;
    }

    int n = vec.size();
    if (n != 0)
    {
        int iG = toInt(randValue(pUnit->getRewardGold(), 0.2f) / n);
        int iE = pUnit->getRewardExp() / n;
        CForceResource* pRes;
        M_VEC_FOREACH(vec)
        {
            CUnit* uu = M_VEC_EACH;
            uu->addExp(iE);
            pRes = uu->getResource();
            if (pRes)
            {
                pRes->changeGold(+iG);
                CUnitDrawForCC* dd = DCAST(uu->getDraw(), CUnitDrawForCC*);
                char szTip[64];
                sprintf(szTip, "+%d Gold", iG);
                dd->addBattleTip(szTip, "fonts/Comic Book.ttf", 18, ccc3(255, 247, 53));
            }
            M_VEC_NEXT;
        }
    }

    if (pUnit == getHero())
    {
        //setHero(NULL);
    }
}

void CBattleWorld::onUnitAttackTarget( CUnit* pUnit, CAttackData* pAttack, CUnit* pTarget )
{
    // DemoTemp
    static SimpleAudioEngine* ae = SimpleAudioEngine::sharedEngine();
    if (strcmp(pUnit->getDraw()->getName(), "Soldier") == 0 || strcmp(pUnit->getDraw()->getName(), "Templar") == 0)
    {
        if (!ae->isEffectPlaying("sounds/Effect/Fighting.mp3") && g_fight > MAX_FIGHT)
        {
            g_fightId = ae->playEffect("sounds/Effect/Fighting.mp3");
            g_fight = 0.0f;
        }
        g_fightFree = 0.0f;
    }

    CUnitDraw2D* d = DCAST(pUnit->getDraw(), CUnitDraw2D*);
    CUnitDraw2D* hd = DCAST(getHero()->getDraw(), CUnitDraw2D*);
    if (pTarget == getHero() && hd->getCastActionId() == 0 && !pTarget->isDoingOr(CUnit::kObstinate))
    {
        float dis = hd->getPosition().getDistance(d->getPosition());
        if (dis < 100 && !getThunderCapAct()->isCoolingDown())
        {
            hd->setCastTarget(CCommandTarget());
            hd->cmdCastSpell(getThunderCapAct()->getId());
            ae->playEffect("sounds/Effect/ThunderCap.mp3");
        }
        else if (!getHammerThrowAct()->isCoolingDown())
        {
            hd->setCastTarget(pUnit->getId());
            hd->cmdCastSpell(getHammerThrowAct()->getId());
        }
        //DCAST(getHero()->getWorld(), CBattleWorld*)->getLayer()->runAction(CCShake::create(0.8f, 2, 50.0f));
    }
}

void CBattleWorld::onUnitProjectileEffect( CUnit* pUnit, CProjectile* pProjectile, CUnit* pTarget )
{
    // DemoTemp
    M_DEF_GC(gc);
    if (strcmp(pProjectile->getName(), "ThorHammer") == 0)
    {
        CCAnimation* pAni = gc->getAnimation("Effects/Lightning");
        CCSprite* sp = CCSprite::createWithSpriteFrameName("Effects/Lightning/00.png");

        CCAnimation* pAni2 = gc->getAnimation("Effects/Lightning2");
        CCSprite* sp2 = CCSprite::createWithSpriteFrameName("Effects/Lightning2/00.png");

        CCAnimation* pAni3 = gc->getAnimation("Effects/Lightning3");
        CCSprite* sp3 = CCSprite::createWithSpriteFrameName("Effects/Lightning3/00.png");

        CCNode* sn = DCAST(pTarget->getDraw(), CUnitDrawForCC*)->getSprite()->getShadow();
        
        sn->addChild(sp, M_BASE_Z - sn->getPosition().y);
        sp->setPosition(ccp(sn->getContentSize().width * sn->getAnchorPoint().x, sp->getContentSize().height * 0.5 - 100.0f));
        sp->runAction(CCSequence::create(CCAnimate::create(pAni), CCRemoveSelf::create(), NULL));

        sn->addChild(sp2, M_BASE_Z - sn->getPosition().y);
        sp2->setPosition(ccp(sn->getContentSize().width * sn->getAnchorPoint().x, sp2->getContentSize().height * 0.5 - 100.0f));
        sp2->runAction(CCSequence::create(CCAnimate::create(pAni2), CCRemoveSelf::create(), NULL));
        
        sn->addChild(sp3, M_BASE_Z - sn->getPosition().y);
        sp3->setPosition(ccp(sn->getContentSize().width * sn->getAnchorPoint().x, sp3->getContentSize().height * 0.5 - 100.0f));
        sp3->runAction(CCSequence::create(CCAnimate::create(pAni3), CCRemoveSelf::create(), NULL));
    }
}


void CBattleWorld::onChangeGold( CMultiRefObject* obj )
{
    CCBattleSceneLayer* l = DCAST(getLayer(), CCBattleSceneLayer*);
    CUnit* hero = getHero();
    if (hero != NULL)
    {
        l->updateResourceInfo(hero->getResource()->getGold());
    }
}

void CBattleWorld::onAniDone( CMultiRefObject* obj, void* data )
{
    getHero()->resume();
}




// CCBattleScene
CCBattleScene::CCBattleScene()
    : m_pWorld(NULL)
{
}

CCBattleScene::~CCBattleScene()
{
    M_SAFE_RELEASE(m_pWorld);
}

bool CCBattleScene::init()
{
    m_pWorld = new CBattleWorld;
    m_pWorld->retain();

    return CCScene::init();
}


// CCBattleSceneLayer
CCBattleSceneLayer::CCBattleSceneLayer()
    : m_pCtrlLayer(NULL)
    , m_iMaxLogs(0)
    , m_iBaseLogId(CKeyGen::nextKey())
    , m_iCurLogId(m_iBaseLogId)
    , m_pTargetAtk(NULL)
    , m_pTargetDef(NULL)
    , m_bShowTargetInfo(false)
{
    CCLabelTTF* lbl = CCLabelTTF::create("TestSize", "fonts/Arial.ttf", 20);
    const CCSize& sz = lbl->getContentSize();
    CCSize winSz = CCDirector::sharedDirector()->getVisibleSize();
    m_iMaxLogs = (winSz.height - 20) / sz.height;
    memset(&m_stTargetInfo, 0xFF, sizeof(m_stTargetInfo));
}

CCBattleSceneLayer::~CCBattleSceneLayer()
{
    if (m_pCtrlLayer != NULL)
    {
        if (m_pCtrlLayer->getParent() != NULL)
        {
            m_pCtrlLayer->removeFromParentAndCleanup(true);
        }
        
        m_pCtrlLayer->release();
    }
}

CCScene* CCBattleSceneLayer::scene()
{
    // 'scene' is an autorelease object
    CCBattleScene* pScene = CCBattleScene::create();

    CWorldForCC* pWorld = pScene->getWorld();
    // 'layer' is an autorelease object
    CCBattleSceneLayer* layer = CCBattleSceneLayer::create();
    pWorld->setLayer(layer);

    // add layer as a child to scene
    pScene->addChild(layer);
    pScene->addChild(layer->getCtrlLayer(), 1);

    if (pWorld->init() == false)
    {
        return NULL;
    }
    

    // return the scene
    return pScene;
}

// on "init" you need to initialize your instance
bool CCBattleSceneLayer::init()
{
    //////////////////////////////
    // 1. super init first
    if (!CCLayer::init())
    {
        return false;
    }

    m_pCtrlLayer = CCLayer::create();
    m_pCtrlLayer->retain();

    M_DEF_GC(gc);
    gc->getfc()->addSpriteFramesWithFile("Global0.plist");

    setBackGroundSprite(CCSprite::create("BackgroundHD.png"));
    setBufferEffectParam(0.9f, 10.0f, 0.1f);
    setPosition(ccp(0, 0));

    /////////////////////////////
    // 3. add your codes below...

    setWorldInterval(0.02f);

    return true;
}

void CCBattleSceneLayer::ccTouchEnded(CCTouch *pTouch, CCEvent *pEvent)
{
    static SimpleAudioEngine* ae = SimpleAudioEngine::sharedEngine();

    CCWinUnitLayer::ccTouchEnded(pTouch, pEvent);
    if (!isClickAction())
    {
        return;
    }

    CCPoint pos = ccpSub(pTouch->getLocation(), getPosition());

    CBattleWorld* w = DCAST(getWorld(), CBattleWorld*);
    CUnit* hero = w->getUnit(w->getControlUnit());
    if (hero == NULL)
    {
        //return;
    }

    CUnitDraw2D* d = hero ? DCAST(hero->getDraw(), CUnitDraw2D*) : NULL;
    if (d == NULL)
    {
        //return;
    }

    CPoint p(pos.x, pos.y);
    CUnit* t = CUnitGroup::getNearestUnitInRange(w, p, 50.0f);

    if (d != NULL && t != NULL && t->isEnemyOf(hero))
    {
        if (hero != t)
        {
            d->setCastTarget(CCommandTarget(t->getId()));
            d->cmdCastSpell(hero->getAttackAbilityId());

            // DemoTemp
            if (!w->getHammerThrowAct()->isCoolingDown() && d->getCastActionId() == 0)
            {
                d->setCastTarget(CCommandTarget(t->getId()));
                d->cmdCastSpell(w->getHammerThrowAct()->getId());
            }

            // DemoTemp
            char sz[256];
            sprintf(sz, "sounds/Units/Thor/move/%02d.mp3", g_moveIndex);
            if (!ae->isEffectPlaying(sz) && !hero->isDead() && g_move > MAX_MOVE[g_moveIndex])
            {
                int me = rand() % 4;
                while (me == g_moveIndex) me = rand() % 4;
                sprintf(sz, "sounds/Units/Thor/move/%02d.mp3", me);
                ae->playEffect(sz);
                g_moveIndex = me;
                g_move = 0.0f;
            }
        }
    }
    else if (d != NULL)
    {
        CCSprite* sp = CCSprite::createWithSpriteFrameName("UI/cmd/Target.png");
        addChild(sp);
        sp->setPosition(ccp(p.x, p.y));
        sp->runAction(CCSequence::create(CCFadeOut::create(0.5f), CCRemoveSelf::create(true), NULL));
        
        sp = CCSprite::createWithSpriteFrameName("UI/cmd/Target2.png");
        addChild(sp);
        sp->setPosition(ccp(p.x, p.y));
        sp->runAction(CCSequence::create(CCSpawn::create(CCScaleTo::create(0.5f, 1.5f, 1.5f), CCFadeOut::create(0.5f), NULL), CCRemoveSelf::create(true), NULL));

        d->cmdMove(p);
        // DemoTemp
        SimpleAudioEngine* ae = SimpleAudioEngine::sharedEngine();

        char sz[256];
        sprintf(sz, "sounds/Units/Thor/move/%02d.mp3", g_moveIndex);
        if (!ae->isEffectPlaying(sz) && !hero->isDead() && g_move > MAX_MOVE[g_moveIndex])
        {
            int me = rand() % 4;
            while (me == g_moveIndex) me = rand() % 4;
            sprintf(sz, "sounds/Units/Thor/move/%02d.mp3", me);
            ae->playEffect(sz);
            g_moveIndex = me;
            g_move = 0.0f;
        }
    }

    showTargetInfo(t != NULL);
    if (t != NULL)
    {
        updateTargetInfo(t->getId());
    }
}

void CCBattleSceneLayer::log( const char* fmt, ... )
{
    CCSize winSz = CCDirector::sharedDirector()->getVisibleSize();
    char sz[1024];
    va_list argv;
    va_start(argv, fmt);
    vsprintf(sz, fmt, argv);
    va_end(argv);

    CCNode* l = getCtrlLayer();
    int curLogId = getCurLogId();
    cirInc(curLogId, getBaseLogId(), getMaxLogs());
    setCurLogId(curLogId);

    CCLabelTTF* lbl = DCAST(l->getChildByTag(getCurLogId()), CCLabelTTF*);
    if (lbl != NULL)
    {
        lbl->removeFromParentAndCleanup(true);
    }

    lbl = CCLabelTTF::create(sz, "fonts/Arial.ttf", 20);
    lbl->setHorizontalAlignment(kCCTextAlignmentLeft);
    lbl->setColor(ccc3(79, 0, 255));
    l->addChild(lbl, 1, getCurLogId());
    const CCSize& rSz = lbl->getContentSize();
    lbl->setPosition(ccp(rSz.width * 0.5 + 10, winSz.height + rSz.height * 0.5 - 10));

    for (int i = 0, j = getCurLogId(); i < getMaxLogs(); ++i)
    {
        CCNode* pNode = l->getChildByTag(j);
        if (pNode != NULL)
        {
            pNode->runAction(CCMoveBy::create(0.1f, ccp(0.0f, -lbl->getContentSize().height)));
        }

        cirDec(j, getBaseLogId(), getMaxLogs());
    }
}

void CCBattleSceneLayer::initTargetInfo()
{
    static CCSize vSz = CCDirector::sharedDirector()->getVisibleSize();

    // ��ʼ��Ŀ����Ϣ���
    CCSprite* pSprite = NULL;
    CCLabelTTF* pLabel = NULL;
    
    m_pTargetInfoPanel = CCSprite::create("UI/TargetInfoPanel.png");
    m_pCtrlLayer->addChild(m_pTargetInfoPanel);
    m_pTargetInfoPanel->setPosition(ccp(vSz.width * 0.5, -m_pTargetInfoPanel->getContentSize().height * 0.5));

    float fFontSize = 28;
    float fBaseY = 46;

    // ѡ��ͷ��
    pSprite = CCSprite::createWithSpriteFrameName("UI/status/portrait_sel.png");
    m_pTargetInfoPanel->addChild(pSprite, 10);
    pSprite->setPosition(ccp(56, fBaseY - 2));

    m_pPortraitSel = CCSprite::createWithSpriteFrameName("UI/status/portrait_sel.png");
    m_pTargetInfoPanel->addChild(m_pPortraitSel);
    m_pPortraitSel->setPosition(pSprite->getPosition());

    // ����
    float fW0 = 0;
    float fW1 = 100;
    float fBaseX = M_FIX_BASE_X(fW0, fW1, 80);
    m_pNameSel = CCLabelTTF::create("", "fonts/Comic Book.ttf", fFontSize);
    m_pNameSel->setAnchorPoint(ccp(0.0, 0.5));
    m_pTargetInfoPanel->addChild(m_pNameSel);
    m_pNameSel->setHorizontalAlignment(kCCTextAlignmentLeft);
    m_pNameSel->setPosition(ccp(fBaseX, fBaseY));

    // �ȼ�
    fW0 = fW1;
    fW1 = 52;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 130);
    pLabel = CCLabelTTF::create("Lv", "fonts/Comic Book.ttf", fFontSize, CCSizeMake(fW1, 32), kCCTextAlignmentLeft);
    m_pTargetInfoPanel->addChild(pLabel);
    pLabel->setPosition(ccp(fBaseX, fBaseY));

    fW0 = fW1;
    fW1 = 32;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 10);
    m_pTargetLv = CCLabelTTF::create("12", "fonts/Comic Book.ttf", fFontSize, CCSizeMake(fW1, 32), kCCTextAlignmentLeft);
    m_pTargetInfoPanel->addChild(m_pTargetLv);
    m_pTargetLv->setPosition(ccp(fBaseX, fBaseY));

    // hp
    fW0 = fW1;
    fW1 = 52;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 84);
    pSprite = CCSprite::createWithSpriteFrameName("UI/status/HP.png");
    m_pTargetInfoPanel->addChild(pSprite);
    pSprite->setPosition(ccp(fBaseX, fBaseY));

    fW0 = fW1;
    fW1 = 160;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 10);
    m_pTargetHp = CCLabelTTF::create("1320/3208", "fonts/Comic Book.ttf", fFontSize, CCSizeMake(fW1, 32), kCCTextAlignmentLeft);
    m_pTargetInfoPanel->addChild(m_pTargetHp);
    m_pTargetHp->setPosition(ccp(fBaseX, fBaseY));

    // ����
    fW0 = fW1;
    fW1 = 52;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 64);
    m_pTargetAtkIcon = CCSprite::createWithSpriteFrameName("UI/status/PhysicalAttack.png");
    m_pTargetInfoPanel->addChild(m_pTargetAtkIcon);
    m_pTargetAtkIcon->setPosition(ccp(fBaseX, fBaseY));


    fW0 = fW1;
    fW1 = 0; // ê��������
    fBaseX += M_FIX_BASE_X(fW0, fW1, 10);
    m_pTargetAtk = CCLabelTTF::create("", "fonts/Comic Book.ttf", fFontSize);
    m_pTargetAtk->setAnchorPoint(ccp(0.0, 0.5));
    m_pTargetInfoPanel->addChild(m_pTargetAtk);
    m_pTargetAtk->setHorizontalAlignment(kCCTextAlignmentLeft);
    m_pTargetAtk->setString("105 - 110");
    m_pTargetAtk->setPosition(ccp(fBaseX, fBaseY));
    m_pTargetAtk->setString("150 - 110");

    fBaseX += m_pTargetAtk->getTextureRect().size.width;
    m_pTargetAtkEx = CCLabelTTF::create("", "fonts/Comic Book.ttf", fFontSize);
    m_pTargetAtkEx->setAnchorPoint(ccp(0.0, 0.5));
    m_pTargetInfoPanel->addChild(m_pTargetAtkEx);
    m_pTargetAtkEx->setHorizontalAlignment(kCCTextAlignmentLeft);
    m_pTargetAtkEx->setString(" +15000");
    m_pTargetAtkEx->setPosition(ccp(fBaseX, fBaseY));
    m_pTargetAtk->setString("151 - 167");
    m_pTargetAtkEx->setColor(ccc3(40, 220, 40));

    // ����
    fW1 = 52;
    fBaseX += 260 - m_pTargetAtk->getTextureRect().size.width + fW1 * 0.5;
    m_pTargetDefIcon = CCSprite::createWithSpriteFrameName("UI/status/HeavyArmor.png");
    m_pTargetInfoPanel->addChild(m_pTargetDefIcon);
    m_pTargetDefIcon->setPosition(ccp(fBaseX, fBaseY));

    fW0 = fW1;
    fW1 = 64;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 10);
    m_pTargetDef = CCLabelTTF::create("32", "fonts/Comic Book.ttf", fFontSize, CCSizeMake(fW1, 32), kCCTextAlignmentLeft);
    m_pTargetInfoPanel->addChild(m_pTargetDef);
    m_pTargetDef->setPosition(ccp(fBaseX, fBaseY));
}

void CCBattleSceneLayer::updateTargetInfo(int id)
{
    if (m_bShowTargetInfo == false)
    {
        return;
    }

    static int lastId = 0;
    if (lastId != id && id != 0)
    {
        lastId = id;
    }

    CUnit* pUnit = getWorld()->getUnit(lastId);
    if (!pUnit)
    {
        return;
    }

    M_DEF_GC(gc);
    CCSpriteFrameCache* fc = gc->getfc();
    CUnitDraw2D* d = DCAST(pUnit->getDraw(), CUnitDraw2D*);
    
    if (!m_pTargetInfoPanel->isVisible())
    {
        m_pTargetInfoPanel->setVisible(true);
    }

    char szBuf[1024];

    // ѡ��ͷ��
    sprintf(szBuf, "Units/%s/portrait_sel.png", pUnit->getDraw()->getName());
    CCSpriteFrame* sf = fc->spriteFrameByName(szBuf);
    if (sf == NULL)
    {
        sf = fc->spriteFrameByName("UI/status/portrait_sel.png");
    }
    m_pPortraitSel->setDisplayFrame(sf);
    m_pNameSel->setString(pUnit->getName());
    
    // �ȼ�
    uint32_t dwLevel = pUnit->getLevel();
    if (dwLevel != m_stTargetInfo.dwLevel)
    {
        sprintf(szBuf, "%u", pUnit->getLevel());
        m_pTargetLv->setString(szBuf);
        m_stTargetInfo.dwLevel = dwLevel;
    }

    // hp
    uint32_t dwHp = toInt(pUnit->getHp());
    uint32_t dwMaxHp = toInt(pUnit->getRealMaxHp());
    if ((dwHp != m_stTargetInfo.dwHp) || (dwMaxHp != m_stTargetInfo.dwMaxHp))
    {
        sprintf(szBuf, "%u/%u", dwHp, dwMaxHp);
        m_pTargetHp->setString(szBuf);
        m_stTargetInfo.dwHp = dwHp;
        m_stTargetInfo.dwMaxHp = dwMaxHp;
    }

    // ����
    CAttackAct* atkAct = DCAST(pUnit->getActiveAbility(pUnit->getAttackAbilityId()), CAttackAct*);
    if (atkAct != NULL)
    {
        switch (atkAct->getBaseAttack().getType())
        {
        case CAttackValue::kPhysical:
            m_pTargetAtkIcon->setDisplayFrame(fc->spriteFrameByName("UI/status/PhysicalAttack.png"));
            break;

        case CAttackValue::kMagical:
            m_pTargetAtkIcon->setDisplayFrame(fc->spriteFrameByName("UI/status/MagicalAttack.png"));
            break;
        }
        uint32_t dwAtk0 = 0;
        uint32_t dwAtk1 = 0;
        float fAtk = 0;
        float fAtkRnd = 0;
        uint32_t dwAtkEx = 0;

        fAtk = atkAct->getBaseAttackValue();
        fAtkRnd = atkAct->getAttackValueRandomRange() * 0.5;
        dwAtk0 = toInt(fAtk * (1 - fAtkRnd));
        dwAtk1 = toInt(fAtk * (1 + fAtkRnd));
        dwAtkEx = toInt(fAtk * (atkAct->getExAttackValue().getMulriple() - 1.0) + atkAct->getExAttackValue().getAddend());

        if ((dwAtk0 != m_stTargetInfo.dwAtk0) || (dwAtk1 != m_stTargetInfo.dwAtk1) || (dwAtkEx != m_stTargetInfo.dwAtkEx))
        {
            sprintf(szBuf, "%u - %u", dwAtk0, dwAtk1);
            m_pTargetAtk->setString(szBuf);
            m_stTargetInfo.dwAtk0 = dwAtk0;
            m_stTargetInfo.dwAtk1 = dwAtk1;

            if (dwAtkEx)
            {
                sprintf(szBuf, " +%u", dwAtkEx);
                m_pTargetAtkEx->setString(szBuf);
                m_pTargetAtkEx->setPosition(ccpAdd(m_pTargetAtk->getPosition(), ccp(m_pTargetAtk->getTextureRect().size.width, 0)));
            }
            else
            {
                m_pTargetAtkEx->setString("");
            }
            m_stTargetInfo.dwAtkEx = dwAtkEx;
        }
    }
    
    // ����
    switch (pUnit->getBaseArmor().getType())
    {
    case CArmorValue::kNormal:
    case CArmorValue::kHeavy:
        m_pTargetDefIcon->setDisplayFrame(fc->spriteFrameByName("UI/status/HeavyArmor.png"));
        break;

    case CArmorValue::kCrystal:
        m_pTargetDefIcon->setDisplayFrame(fc->spriteFrameByName("UI/status/CrystalArmor.png"));
        break;
    }
    uint32_t dwDef = toInt(pUnit->getRealArmorValue());
    if (dwDef != m_stTargetInfo.dwDef)
    {
        sprintf(szBuf, "%u", dwDef);
        m_pTargetDef->setString(szBuf);
        m_stTargetInfo.dwDef = dwDef;
    }

    if (pUnit->isDead())
    {
        showTargetInfo(false);
    }
}

void CCBattleSceneLayer::showTargetInfo( bool bShow /*= true*/ )
{
    if (bShow == m_bShowTargetInfo)
    {
        return;
    }

    m_pTargetInfoPanel->stopAllActions();
    const CCPoint& from = m_pTargetInfoPanel->getPosition();
    CCPoint to(from.x, 0.0f);
    if (bShow)
    {
        to.y = m_pTargetInfoPanel->getContentSize().height * 0.5;
    }
    else
    {
        to.y = -m_pTargetInfoPanel->getContentSize().height * 0.5;
    }

    // DemoTemp
    static SimpleAudioEngine* ae = SimpleAudioEngine::sharedEngine();
    ae->playEffect("sounds/Effect/UIMove.mp3");
    m_pTargetInfoPanel->runAction(CCMoveTo::create(to.getDistance(from) * 0.002, to));

    m_bShowTargetInfo = bShow;
}

void CCBattleSceneLayer::initHeroPortrait()
{
    CCPoint org = CCDirector::sharedDirector()->getVisibleOrigin();
    CCSize oSz = CCDirector::sharedDirector()->getVisibleSize();
    
    CBattleWorld* w = DCAST(getWorld(), CBattleWorld*);
    CUnit* hero = w->getHero();

    M_DEF_GC(gc);

    CCSpriteFrameCache* fc = gc->getfc();
    char sz[1024];
    sprintf(sz, "Units/%s/portrait_hero.png", hero->getDraw()->getName());
    CCSpriteFrame* fr = fc->spriteFrameByName(sz);
    
    m_pHeroPortrait = CCSprite::createWithSpriteFrame(fr);
    m_pCtrlLayer->addChild(m_pHeroPortrait);
    //m_pHeroPortrait->setPosition(ccpAdd(org, ccp(160, 1200)));
    m_pHeroPortrait->setPosition(ccp(oSz.width * 0.07, oSz.height * 0.75));
    CCLOG("HeroPortrait: (%d, %d)", (int)(oSz.width * 0.07), (int)(oSz.height * 0.75));

    CCSprite* fill = CCSprite::createWithSpriteFrameName("UI/status/HpBarFill.png");
    CCSprite* border = CCSprite::createWithSpriteFrameName("UI/status/HpBarBorder.png");
    m_pHeroHpBar = CCProgressBar::create(CCSizeMake(104, 8), fill, border, 0, 0, true);
    m_pHeroPortrait->addChild(m_pHeroHpBar);
    m_pHeroHpBar->setPosition(ccp(m_pHeroPortrait->getContentSize().width * 0.5, 16));
    
    fill = CCSprite::createWithSpriteFrameName("UI/status/ExpBarFill.png");
    border = CCSprite::createWithSpriteFrameName("UI/status/ExpBarBorder.png");
    m_pHeroExpBar = CCProgressBar::create(CCSizeMake(104, 8), fill, border, 0, 0, true);
    m_pHeroPortrait->addChild(m_pHeroExpBar);
    m_pHeroExpBar->setPosition(ccp(m_pHeroPortrait->getContentSize().width * 0.5, 6));

    m_pHeroLevel = CCLabelTTF::create("1", "fonts/Arial.ttf", 24, CCSizeMake(28, 28), kCCTextAlignmentCenter);
    m_pHeroPortrait->addChild(m_pHeroLevel);
    m_pHeroLevel->setPosition(ccp(93, 34));
    m_stHeroInfo.iLevel = 1;
}

void CCBattleSceneLayer::updateHeroPortrait()
{
    CBattleWorld* w = DCAST(getWorld(), CBattleWorld*);
    CUnit* hero = w->getHero();

    float fHpPer = hero->getHp() * 100 / hero->getRealMaxHp();
    if (fHpPer != m_stHeroInfo.fHpPer)
    {
        m_stHeroInfo.fHpPer = fHpPer;
        m_pHeroHpBar->setPercentage(fHpPer);
    }

    float fExpPer = (hero->getExp() - hero->getBaseExp()) * 100 / (hero->getMaxExp() - hero->getBaseExp());
    if (fExpPer != m_stHeroInfo.fExpPer)
    {
        m_stHeroInfo.fExpPer = fExpPer;
        m_pHeroExpBar->setPercentage(fExpPer);
    }

    if (hero->getLevel() != m_stHeroInfo.iLevel)
    {
        m_stHeroInfo.iLevel = hero->getLevel();
        char sz[64];
        sprintf(sz, "%d", hero->getLevel());
        m_pHeroLevel->setString(sz);
    }
}

void CCBattleSceneLayer::initResourceInfo()
{
    CCSize oSz = CCDirector::sharedDirector()->getVisibleSize();
    CCSprite* sp = CCSprite::create("UI/WavePanel.png");
    m_pCtrlLayer->addChild(sp);
    const CCSize& spSz = sp->getContentSize();
    sp->setPosition(ccp(spSz.width * 0.5 + 50, oSz.height - spSz.height * 0.5 - 50));
    m_pGold = CCLabelTTF::create("      ", "fonts/Comic Book.ttf", 28, CCSizeMake(100, 48), kCCTextAlignmentLeft);
    sp->addChild(m_pGold);
    m_pGold->setPosition(ccp(310, 85));
}

void CCBattleSceneLayer::updateResourceInfo( int gold )
{
    char sz[64];
    sprintf(sz, "%d", gold);
    m_pGold->setString(sz);
}
