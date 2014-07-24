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
#include "HeroRoomScene.h"
#include "StageScene.h"
#include "GameData.h"
#include "UserData.h"
#include "LuaScriptEngine.h"
#include "UnitLibraryForCC.h"
#include "AbilityLibrary.h"


// CBattleWorld
const float CBattleWorld::CONST_MAX_REWARD_RANGE = 400;

CBattleWorld::CBattleWorld()
: m_bLuaWorldTickEnabled(true)
{
}

CBattleWorld::~CBattleWorld()
{
}

bool CBattleWorld::onInit()
{
    static Size wsz = Director::getInstance()->getVisibleSize();
    static Point org = Director::getInstance()->getVisibleOrigin();

    CUnit* u = nullptr;
    CUnitDrawForCC* d = nullptr;
    CAbility* a = nullptr;
    int id = 0;
    CAttackAct* atk = nullptr;
    BattleSceneLayer* l = DCAST(getLayer(), BattleSceneLayer*);

    M_DEF_GC(gc);

    gc->loadAnimation("Effects/Lightning", "Effects/Lightning", 0.05f);
    gc->loadAnimation("Effects/Lightning2", "Effects/Lightning2", 0.05f);
    gc->loadAnimation("Effects/Lightning3", "Effects/Lightning3", 0.05f);
    gc->loadAnimation("Effects/Stun/Big", "Effects/Stun/Big", 0.1f);
    gc->loadAnimation("Effects/Stun/Small", "Effects/Stun/Small", 0.1f);

    // init hero
    //CUserData* udt = CUserData::instance();
    //u = CUnitLibraryForCC::instance()->copyUnit(udt->getHeroSelected()->id);
    //addUnit(u);
    setCtrlUnit(0);
    setHero(nullptr);
    //u->setMaxLevel(20);
    //u->updateExpRange();
    //u->setRevivable();
    //u->setForceByIndex(2);
    

    // add abilities from SAL(lua)
    auto udt = CUserData::instance();
    auto hi = udt->getHeroSelected();
    if (hi != nullptr && u != nullptr)
    {
        auto& mapAbilities = hi->m_mapAbilitiesEquipped;
        M_MAP_FOREACH(mapAbilities)
        {
            auto id = M_MAP_IT->first;
            auto lv = M_MAP_EACH;
            M_MAP_NEXT;

            auto a = CAbilityLibrary::instance()->copyAbility(id);
            assert(a != nullptr);  // Ability id == item.id was not found
            a->setLevel(lv);

            auto act = DCAST(a, CActiveAbility*);
            if (act != nullptr)
            {
                u->addActiveAbility(act);
            }
            else
            {
                auto pas = DCAST(a, CPassiveAbility*);
                assert(pas != nullptr);
                u->addPassiveAbility(pas);
            }
        }
    }

    //m_pDragonStrikeBuff = new CStunBuff("DragonStrikeBuff", "DragonStrikeBuff", 5.0f, false);
    //u->addActiveAbility(DCAST(m_pDragonStrikeBuff, CActiveAbility*), false);

    //a = new CChangeHpPas("AutoHeal", "AutoHeal", 0.2f, CExtraCoeff(0.001f, 0.0f));
    //u->addPassiveAbility(DCAST(a, CPassiveAbility*), false);

    //d = DCAST(u->getDraw(), CUnitDrawForCC*);
    //d->setPosition(CPoint(800, 600));
    
    //u->setName(udt->getHeroSelected()->name.c_str());
    //u->setMaxHp(udt->getHeroSelected()->hp);
    //u->getActiveAbility(u->getAttackAbilityId())->dcast(atk);
    //atk->setBaseAttack(udt->getHeroSelected()->atkVal);
    //atk->setBaseAttackInterval(1 / udt->getHeroSelected()->attackSpeed);
    //u->setBaseArmor(udt->getHeroSelected()->armVal);
    //d->setBaseMoveSpeed(udt->getHeroSelected()->moveSpeed);

    l->m_pp.init(this, l->getCtrlLayer(), 5);
    //l->m_pp.setPortrait(0, getUnit(getCtrlUnit()), CC_CALLBACK_1(BattleSceneLayer::onClickHeroPortrait, l));

    onLuaWorldInit();
    auto hero = getHero();
    //u->addExp(udt->getHeroSelected()->exp);
    
    l->initTargetInfo();
    l->showTargetInfo(false);
    l->showTargetInfo(true);
    l->updateTargetInfo(getCtrlUnit());

    if (hero != nullptr)
    {
        l->updateResourceInfo(getHero()->getResource()->getGold());
    }

    l->initHeroAbilityPanel();
    l->updateHeroAbilityPanel();

    l->initCtrlPanel();

    // DemoTemp
    static SimpleAudioEngine* ae = SimpleAudioEngine::getInstance();
    
    char sz[1024];
    sprintf(sz, "sounds/Backgrounds/Background%02d.mp3", rand() % 5);
    ae->playBackgroundMusic(sz, true);
    ae->playEffect("sounds/Effects/WaveIncoming.mp3");

    return true;
}

// DemoTemp
int g_fightId = 0;
float g_fightFree = 0.0f;

void CBattleWorld::onTick(float dt)
{
    // DemoTemp
    static SimpleAudioEngine* ae = SimpleAudioEngine::getInstance();
    g_fightFree += dt;
    if (g_fightId != 0 && g_fightFree > 2.0f)
    {
        ae->stopEffect(g_fightId);
        g_fightId = 0;
    }
    
    if (isLuaWorldTickEnabled())
    {
        onLuaWorldTick(dt);
    }

    BattleSceneLayer* l = DCAST(getLayer(), BattleSceneLayer*);
    l->updateTargetInfo();
    l->m_pp.updateContent();
}

bool CBattleWorld::onLuaWorldInit()
{
    // lua
    lua_State* L = CLuaScriptEngine::instance()->getLuaHandle();
    BattleSceneLayer* layer = DCAST(getLayer(), BattleSceneLayer*);

    luaRegWorldFuncs(L, this);
    luaRegWorldFuncsForCC(L, this);

    int res = 0;
    res = luaL_includefilelog(L, CUserData::instance()->getStageSelected()->script.c_str());
    if (res != LUA_OK)
    {
        return false;
    }

    lua_getglobal(L, "onWorldInit");
    res = lua_pcall(L, 0, 0, 0);
    if (res != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        CCLOG("ERR | LuaErr(%s: %d): %s", __FILE__, (int)__LINE__, err);
        lua_pop(L, 1);
        layer->log("%s", err);

        return false;
    }

    assert(lua_gettop(L) == 0);

    return true;
}

void CBattleWorld::onLuaWorldTick(float dt)
{
    lua_State* L = CLuaScriptEngine::instance()->getLuaHandle();
    BattleSceneLayer* layer = DCAST(getLayer(), BattleSceneLayer*);

    lua_getglobal(L, "onWorldTick");
    lua_pushnumber(L, dt);
    int res = lua_pcall(L, 1, 0, 0);
    if (res != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        CCLOG("ERR | LuaErr(%s: %d): %s", __FILE__, (int)__LINE__, err);
        lua_pop(L, 1);
        layer->log("%s", err);

        return;
    }

    assert(lua_gettop(L) == 0);
}

void CBattleWorld::onLuaUnitDead(CUnit* pUnit)
{
    lua_State* L = CLuaScriptEngine::instance()->getLuaHandle();
    BattleSceneLayer* layer = DCAST(getLayer(), BattleSceneLayer*);

    lua_getglobal(L, "onUnitDead");
    luaL_pushobjptr(L, "Unit", pUnit);
    int res = lua_pcall(L, 1, 0, 0);
    if (res != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        CCLOG("ERR | LuaErr(%s: %d): %s", __FILE__, (int)__LINE__, err);
        lua_pop(L, 1);
        layer->log("%s", err);

        return;
    }

    assert(lua_gettop(L) == 0);
}

void CBattleWorld::onUnitDead(CUnit* pUnit)
{
    CUnitDrawForCC* d = DCAST(pUnit->getDraw(), CUnitDrawForCC*);
    auto hero = getHero();

    if (hero && pUnit != hero && pUnit->getRewardExp() && pUnit->isMyEnemy(hero) && M_RAND_HIT(1.0f))
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
        if (!uu->isDead() && uu->canIncreaseExp() && uu->isMyEnemy(pUnit) && dd->getPosition().getDistance(d->getPosition()) < CBattleWorld::CONST_MAX_REWARD_RANGE)
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
                dd->addBattleTip(szTip, "fonts/Comic Book.fnt", 18, Color3B(255, 247, 53));
            }
            M_VEC_NEXT;
        }
    }

    if (pUnit == hero)
    {
        //setHero(nullptr);
    }

    onLuaUnitDead(pUnit);
}

void CBattleWorld::onUnitAttackTarget(CUnit* pUnit, CAttackData* pAttack, CUnit* pTarget)
{
    M_DEF_GC(gc);
    CSpriteInfo* si = DCAST(pUnit->getDraw(), CUnitDrawForCC*)->getSpriteInfo();
    if (strcmp(si->getName(), "Soldier") == 0 || strcmp(si->getName(), "Templar") == 0)
    {
        if (!gc->isSoundPlaying(g_fightId))
        {
            g_fightId = gc->playSound("sounds/Effects/Fighting.mp3", 8.0f);
            g_fightFree = 0.0f;
        }
    }
}

void CBattleWorld::onUnitProjectileEffect(CUnit* pUnit, CProjectile* pProjectile, CUnit* pTarget)
{
#if 1
    // DemoTemp
    M_DEF_GC(gc);
    if (strcmp(DCAST(pProjectile, CProjectileForCC*)->getSpriteInfo()->getName(), "ThorHammer") == 0)
    {
        Node* sn = DCAST(pTarget->getDraw(), CUnitDrawForCC*)->getSprite()->getShadow();

        Animation* pAni = gc->getAnimation("Effects/Lightning");
        Sprite* sp = Sprite::createWithSpriteFrameName("Effects/Lightning/00.png");

        Animation* pAni2 = gc->getAnimation("Effects/Lightning2");
        Sprite* sp2 = Sprite::createWithSpriteFrameName("Effects/Lightning2/00.png");

        Animation* pAni3 = gc->getAnimation("Effects/Lightning3");
        Sprite* sp3 = Sprite::createWithSpriteFrameName("Effects/Lightning3/00.png");

        sn->getParent()->runAction(Shake::create(0.2f, 4, 10.0f));
        
        sn->addChild(sp, M_BASE_Z - sn->getPosition().y);
        sp->setPosition(Point(sn->getContentSize().width * sn->getAnchorPoint().x, sp->getContentSize().height * 0.5 - 100.0f));
        sp->runAction(Sequence::create(Animate::create(pAni), RemoveSelf::create(), nullptr));

        sn->addChild(sp2, M_BASE_Z - sn->getPosition().y);
        sp2->setPosition(Point(sn->getContentSize().width * sn->getAnchorPoint().x, sp2->getContentSize().height * 0.5 - 100.0f));
        sp2->runAction(Sequence::create(Animate::create(pAni2), RemoveSelf::create(), nullptr));
        
        sn->addChild(sp3, M_BASE_Z - sn->getPosition().y);
        sp3->setPosition(Point(sn->getContentSize().width * sn->getAnchorPoint().x, sp3->getContentSize().height * 0.5 - 100.0f));
        sp3->runAction(Sequence::create(Animate::create(pAni3), RemoveSelf::create(), nullptr)); 
    }
#endif
}

void CBattleWorld::onUnitAddActiveAbility(CUnit* pUnit, CActiveAbility* pAbility)
{
    auto u = getUnit(getCtrlUnit());
    if (pUnit != u)
    {
        return;
    }
    
    BattleSceneLayer* l = DCAST(getLayer(), BattleSceneLayer*);
    l->updateHeroAbilityPanel();
}

void CBattleWorld::onUnitDelActiveAbility(CUnit* pUnit, CActiveAbility* pAbility)
{
    auto u = getUnit(getCtrlUnit());
    if (pUnit != u)
    {
        return;
    }

    BattleSceneLayer* l = DCAST(getLayer(), BattleSceneLayer*);
    l->updateHeroAbilityPanel();
}

void CBattleWorld::onUnitAbilityCD(CUnit* pUnit, CAbility* pAbility)
{
    auto u = getUnit(getCtrlUnit());
    if (pUnit != u)
    {
        return;
    }

    BattleSceneLayer* l = DCAST(getLayer(), BattleSceneLayer*);
    auto btn = l->m_bp->getButton([pAbility](ButtonBase* btn)
    {
        return btn->getUserData() == (void*)pAbility->getId();
    });

    if (btn != nullptr)
    {
        btn->coolDown();
    }

    // ����Ŀ��Ȧ
    auto a = DCAST(pAbility, CActiveAbility*);
    if (a != nullptr && a->getCastTargetType() == CCommandTarget::kPointTarget && a->getCastTargetRadius() > 0.0f)
    {
        auto sp = DCAST(l->getChildByTag(105), Sprite*);
        if (sp != nullptr)
        {
            sp->setTag(-105);
            sp->stopAllActions();
            sp->runAction(Sequence::create(FadeTo::create(0.1f, 255), DelayTime::create(0.1f), Spawn::createWithTwoActions(Sequence::createWithTwoActions(FadeOut::create(0.1f), DelayTime::create(0.2f)), ScaleTo::create(0.3f, sp->getScale() * 1.5f)), RemoveSelf::create(), nullptr));
        }
    }
}

void CBattleWorld::onGoldChanged(int iChanged)
{
    BattleSceneLayer* l = DCAST(getLayer(), BattleSceneLayer*);
    CUnit* hero = getHero();
    if (hero != nullptr)
    {
        l->updateResourceInfo(hero->getResource()->getGold());
    }
}

void CBattleWorld::setHero(CUnit* hero)
{
    if (m_pHero == hero)
    {
        return;
    }

    m_pHero = hero;

    if (hero == nullptr)
    {
        return;
    }

    CForceResource* fr = new CForceResource(CC_CALLBACK_1(CBattleWorld::onGoldChanged, this)); // ������Դ
    hero->setResource(fr);
    hero->setRevivable();
    auto l = DCAST(getLayer(), BattleSceneLayer*);
    l->initResourceInfo();
    l->updateResourceInfo(fr->getGold());
}

// BattleScene
BattleScene::BattleScene()
: m_pWorld(nullptr)
{
}

BattleScene::~BattleScene()
{
    M_SAFE_RELEASE(m_pWorld);
}

bool BattleScene::init()
{
    m_pWorld = new CBattleWorld;
    m_pWorld->retain();

    return Scene::init();
}

// CPortraitPanel
CPortraitPanel::CPortraitPanel()
: m_w(nullptr)
, m_bp(nullptr)
{
}

void CPortraitPanel::init(CWorldForCC* world, Node* parent, int max)
{
    m_w = world;

    m_bp = ButtonPanel::create(max, 1, Size(112, 124), 0, 10, nullptr);
    parent->addChild(m_bp);
    m_bp->setPosition(Point(m_bp->getContentSize().width * 0.5 + 30, wsz().height * 0.5));

    m_vecPortraits.resize(max);
}

void CPortraitPanel::createPortrait(PORTRAIT_INFO& pi, CUnit* u, const ccMenuCallback& onClick)
{
    M_DEF_GC(gc);

    SpriteFrameCache* fc = gc->getfc();
    char sz[1024];
    sprintf(sz, "Sprites/%s/portrait_hero.png", DCAST(u->getDraw(), CUnitDrawForCC*)->getSpriteInfo()->getName());
    SpriteFrame* fr = fc->getSpriteFrameByName(sz);
    if (fr == nullptr)
    {
        sprintf(sz, "UI/HeroInfo/portrait_hero.png");
    }

    pi.portrait = ButtonNormal::createWithFrameName(sz, sz, nullptr, nullptr, nullptr, 0.0f, onClick, nullptr);
    pi.portrait->setUserData(&pi);

    Sprite* fill = Sprite::createWithSpriteFrameName("UI/status/HpBarFill.png");
    Sprite* border = Sprite::createWithSpriteFrameName("UI/status/HpBarBorder.png");
    pi.hpBar = ProgressBar::create(Size(104, 8), fill, border, 0, 0, true);
    pi.portrait->addChild(pi.hpBar);
    pi.hpBar->setPosition(Point(pi.portrait->getContentSize().width * 0.5, 16));

    fill = Sprite::createWithSpriteFrameName("UI/status/ExpBarFill.png");
    border = Sprite::createWithSpriteFrameName("UI/status/ExpBarBorder.png");
    pi.expBar = ProgressBar::create(Size(104, 8), fill, border, 0, 0, true);
    pi.portrait->addChild(pi.expBar);
    pi.expBar->setPosition(Point(pi.portrait->getContentSize().width * 0.5, 6));

    pi.levelNum = Label::createWithTTF("1", FONT_ARIAL, 24, Size(28, 28), TextHAlignment::CENTER);
    pi.portrait->addChild(pi.levelNum);
    pi.levelNum->setPosition(Point(94, 37));

    pi.level = 1;
    pi.unitId = u->getId();
}

void CPortraitPanel::updatePortrait(int index)
{
    auto& pi = m_vecPortraits[index];
    if (pi.portrait == nullptr)
    {
        return;
    }

    auto u = m_w->getUnit(pi.unitId);
    if (u == nullptr)
    {
        return;
    }

    float realMaxHp = u->getRealMaxHp();
    float hpPer = realMaxHp == 0.0f ? 0.0f : (u->getHp() / realMaxHp);
    if (hpPer != pi.hpPer)
    {
        pi.hpPer = hpPer;
        pi.hpBar->setPercentage(hpPer);
    }

    int expRange = u->getMaxExp() - u->getBaseExp();
    float expPer = expRange == 0 ? 0.0f : ((u->getExp() - u->getBaseExp()) * 1.0 / expRange);
    if (expPer != pi.expPer)
    {
        pi.expPer = expPer;
        pi.expBar->setPercentage(expPer);
    }

    if (u->getLevel() != pi.level)
    {
        pi.level = u->getLevel();
        char sz[64];
        sprintf(sz, "%d", u->getLevel());
        pi.levelNum->setString(sz);
    }
}

void CPortraitPanel::delPortrait(int index, bool follow /*= false*/)
{
    auto& pi = m_vecPortraits[index];
    if (pi.portrait == nullptr)
    {
        return;
    }

    m_bp->delButton(pi.portrait);

    if (follow)
    {
        m_bp->clearUpSlot(ButtonPanel::kTopToBottom);
    }

    pi.clear();
}

void CPortraitPanel::setPortrait(int index, CUnit* u, const ccMenuCallback& onClick)
{
    auto& pi = m_vecPortraits[index];
    if (pi.portrait != nullptr)
    {
        delPortrait(index, false);
    }

    createPortrait(pi, u, onClick);
    m_bp->addButton(pi.portrait, m_bp->getMaxCount() - index - 1);
}

void CPortraitPanel::updateContent()
{
    for (size_t i = 0; i < m_vecPortraits.size(); ++i)
    {
        updatePortrait(i);
    }
}

// BattleSceneLayer
BattleSceneLayer::BattleSceneLayer()
: m_ctrlLayer(nullptr)
, m_ctrlLayer2(nullptr)
, m_iMaxLogs(0)
, m_iBaseLogId(CIdGen::nextId())
, m_iCurLogId(m_iBaseLogId)
, m_pTargetAtk(nullptr)
, m_pTargetDef(nullptr)
, m_bShowTargetInfo(false)
, m_iTargetInfoUnitId(0)
, m_bp(nullptr)
{
    Label* lbl = Label::createWithTTF("TestSize", FONT_ARIAL, 20);
    const Size& sz = lbl->getContentSize();
    static Size winSz = Director::getInstance()->getVisibleSize();
    m_iMaxLogs = (winSz.height - 20) / sz.height;
    memset(&m_stTargetInfo, 0xFF, sizeof(m_stTargetInfo));
}

BattleSceneLayer::~BattleSceneLayer()
{
}

Scene* BattleSceneLayer::scene()
{
    // 'scene' is an autorelease object
    BattleScene* pScene = BattleScene::create();

    CBattleWorld* pWorld = DCAST(pScene->getWorld(), CBattleWorld*);
    // 'layer' is an autorelease object
    BattleSceneLayer* layer = BattleSceneLayer::createWithWorld(pWorld);
    //pWorld->setLayer(layer);

    // add layer as a child to scene
    pScene->addChild(layer);
    pScene->addChild(layer->m_ctrlLayer);
    pScene->addChild(layer->m_ctrlLayer2);
    
    // return the scene
    return pScene;
}

// on "init" you need to initialize your instance
bool BattleSceneLayer::initWithWorld(CWorldForCC* pWorld)
{
    //////////////////////////////
    // 1. super init first
    if (!UnitLayer::initWithWorld(pWorld))
    {
        return false;
    }

    m_ctrlLayer = TouchMaskLayer::create(Color4B(0, 0, 0, 0), 80);
    m_ctrlLayer2 = TouchMaskLayer::create(Color4B(0, 0, 0, 0), 120);

    static Size wsz = Director::getInstance()->getVisibleSize();
    M_DEF_GC(gc);

    auto ll = gc->defaultLoadingLayer();
    addChild(ll, 100, 100);

    vector<string> ts;
    ts.push_back("Global0");
    ts.push_back("Global1");
    ts.push_back("Global2");
    ts.push_back("Heroes0");
    ts.push_back("Heroes1");
    ts.push_back("Heroes2");
    ts.push_back("Heroes3");
    ts.push_back("Heroes4");
    ts.push_back("Heroes5");
    ts.push_back("Projectiles0");
    ts.push_back("Battle0");

    vector<string> ot;
    ot.push_back("UI/TargetInfoPanel.png");
    ot.push_back("UI/Button/BtnPauseNor.png");
    ot.push_back("UI/Button/BtnPauseSel.png");
    ot.push_back("UI/Button/BtnResumeNor.png");
    ot.push_back("UI/Button/BtnResumeSel.png");
    ot.push_back("UI/Button/BtnRestartNor.png");
    ot.push_back("UI/Button/BtnRestartSel.png");
    ot.push_back("UI/Button/BtnQuitNor.png");
    ot.push_back("UI/Button/BtnQuitSel.png");
    ot.push_back("UI/Button/BtnTryAgainNor.png");
    ot.push_back("UI/Button/BtnTryAgainSel.png");
    gc->loadTexturesAsync(ts, ot, CC_CALLBACK_0(BattleSceneLayer::onLoadingProgress, this), nullptr);

    return true;
}

void BattleSceneLayer::onLoadingProgress()
{
    M_DEF_GC(gc);
    if (gc->getLoaded() == max(1, gc->getLoadCount() - 1))
    {
        gc->preloadSound("sounds/Sprites/Thor/move/00.mp3");
        gc->preloadSound("sounds/Sprites/Thor/move/01.mp3");
        gc->preloadSound("sounds/Sprites/Thor/move/02.mp3");
        gc->preloadSound("sounds/Sprites/Thor/move/03.mp3");
        gc->preloadSound("sounds/Sprites/Thor/die/00.mp3");
        gc->preloadSound("sounds/Effects/WaveIncoming.mp3");
        gc->preloadSound("sounds/Effects/Fighting.mp3");
        gc->preloadSound("sounds/Effects/ThrowHammer.mp3");
        gc->preloadSound("sounds/Effects/LevelUp.mp3");
        gc->preloadSound("sounds/Effects/OpenDoor.mp3");
        gc->preloadSound("sounds/Effects/TeslaRay00.mp3");
        gc->preloadSound("sounds/Effects/TeslaRay01.mp3");
        gc->preloadSound("sounds/Effects/ThunderCap.mp3");
        gc->preloadSound("sounds/Effects/ArcaneRay.mp3");
        gc->preloadSound("sounds/Effects/UIMove.mp3");
        gc->preloadSound("sounds/Effects/ArrowRelease00.mp3");
        gc->preloadSound("sounds/Effects/ArrowRelease01.mp3");
        gc->preloadSound("sounds/Effects/MageShot.mp3");
        gc->preloadSound("sounds/Effects/Monk00.mp3");
        gc->preloadSound("sounds/Effects/Monk01.mp3");
        gc->preloadSound("sounds/Effects/Monk02.mp3");
        gc->preloadSound("sounds/Effects/Hero_Monk_Firedragon.mp3");
        gc->preloadSound("sounds/Effects/Sound_GUIButtonCommon.mp3");
        gc->preloadSound("sounds/Effects/GUITransitionOpen.mp3");
    }

    if (gc->getLoaded() < 2)
    {
        return;
    }

    float per = gc->getLoaded() * 1.0f / gc->getLoadCount();
    auto pb = DCAST(getChildByTag(100)->getChildByTag(101), ProgressBar*);
    if (pb == nullptr)
    {
        pb = ProgressBar::create(Size(wsz().width * 0.5, 50.0f), Sprite::createWithSpriteFrameName("UI/status/HpBarFill.png"), Sprite::createWithSpriteFrameName("UI/status/ExpBarBorder.png"), 0.0f, 0.0f, true);
        getChildByTag(100)->addChild(pb, 1, 101);
        pb->setPosition(Point(wsz().width * 0.5, wsz().height * 0.3));
    }
    //pb->stopAllActions();
    //pb->runActionForTimer(EaseExponentialOut::create(pb->setPercentageAction(per, 0.1f, per < 100 ? nullptr : CallFunc::create(CC_CALLBACK_0(BattleSceneLayer::onLoadingDone, this)))));
    pb->setPercentage(per);
    if (per >= 1.0f)
    {
        pb->runAction(Sequence::createWithTwoActions(DelayTime::create(0.5f), CallFunc::create(CC_CALLBACK_0(BattleSceneLayer::onLoadingDone, this))));
    }
}

void BattleSceneLayer::onLoadingDone()
{
    CUnitLibrary::instance()->loadDefaultLibrary();

    //OBJS_INFO;

    removeChildByTag(100);

    static Size wsz = Director::getInstance()->getVisibleSize();

    M_DEF_GC(gc);

    setBackgroundSprite(Sprite::create(CUserData::instance()->getStageSelected()->background.c_str()));
    setBufferEffectParam(1.5f, 0.9f, 20.0f, 0.1f);
    setPosition(Point(0, 0));

    /////////////////////////////
    // 3. add your codes below...
    
    m_mn = MenuEx::create();
    m_ctrlLayer->addChild(m_mn);
    m_mn->setPosition(Point::ZERO);
    
    auto btn = ButtonNormal::createWithFrameName("UI/Button/Fist/Normal.png", "UI/Button/Fist/On.png", "UI/Button/Fist/Disabled.png", "UI/Button/Fist/Blink.png", "UI/Button/Fist/Mask.png", 90.0f, CC_CALLBACK_1(BattleSceneLayer::onClickFist, this), nullptr);
    m_mn->addChild(btn);
    btn->setPosition(Point(wsz.width - btn->getContentSize().width * 0.5 - 30, 100));

    MenuItemSprite* btnPause = MenuItemSprite::create(
        Sprite::create("UI/Button/BtnPauseNor.png"),
        Sprite::create("UI/Button/BtnPauseSel.png"),
        nullptr,
        CC_CALLBACK_1(BattleSceneLayer::onClickPause, this));
    m_mn->addChild(btnPause);

    btnPause->setPosition(Point(wsz.width - btnPause->getContentSize().width * 0.5 - 50, wsz.height - btnPause->getContentSize().height * 0.5 - 50));

    auto w = getWorld();
    if (w->init() == false)
    {
    }

    setWorldInterval(0.02f);

#if 0  // Ӣ����Ѫ��������
    CUnit* hero = DCAST(getWorld(), CBattleWorld*)->getHero();
    auto mi = MenuItemFont::create("50%", bind([hero](Ref*)
    {
        hero->setHp(hero->getRealMaxHp() * 0.5);
    }, placeholders::_1));
    mn->addChild(mi, 5);
    mi->setPosition(Point(wsz.width * 0.1, wsz.height * 0.4));

    mi = MenuItemFont::create("100%", bind([hero](Ref*)
    {
        hero->setHp(hero->getRealMaxHp());
    }, placeholders::_1));
    mn->addChild(mi, 5);
    mi->setPosition(Point(wsz.width * 0.2, wsz.height * 0.4));
#endif
}

void BattleSceneLayer::onTouchesEnded(const std::vector<Touch*>& touches, cocos2d::Event* event)
{
    Touch* pTouch = touches.front();

    static SimpleAudioEngine* ae = SimpleAudioEngine::getInstance();

    WinLayer::onTouchesEnded(touches, event);
    if (!isClickAction())
    {
        return;
    }

    Point pos = convertTouchToNodeSpace(pTouch);
    CPoint p(pos.x, pos.y);

    CBattleWorld* w = DCAST(getWorld(), CBattleWorld*);
    CUnit* hero = w->getUnit(w->getCtrlUnit());
    if (hero == nullptr)
    {
        //return;
    }

    CUnit* t = CUnitGroup::getNearestUnitInRange(w, p, 50.0f);
    if (t != nullptr && !t->isDead())
    {
        showTargetInfo(true);
        updateTargetInfo(t->getId());
    }

    CUnitDrawForCC* d = hero ? DCAST(hero->getDraw(), CUnitDrawForCC*) : nullptr;
    if (d == nullptr)
    {
        return;
    }
    
    int wid = d->getWaitForCastTargetActiveAbilityId();
    if (wid != 0)
    {
        if (d->cmdCastSpell(CCommandTarget(p), wid) >= 0)
        {
            // �ɹ�ʩ���������ʩ����ʼ׷��
            cancelAbilityWaiting(wid, 10);
        }

        auto a = hero->getActiveAbility(wid);
        if (a != nullptr && a->getCastTargetRadius() > 0.0f)
        {
            auto sp = DCAST(getChildByTag(105), Sprite*);
            if (sp == nullptr)
            {
                sp = Sprite::createWithSpriteFrameName("UI/cmd/Target5.png");
                sp->setTag(105);
                addChild(sp);

                sp->setOpacity(40);
                sp->runAction(RepeatForever::create(Sequence::create(FadeTo::create(1.0f, 100), FadeTo::create(1.0f, 40), nullptr)));
            }
            
            sp->setPosition(Point(p.x, p.y));
            sp->setScale(a->getCastTargetRadius() * 2 / sp->getContentSize().width);
        }
        
        return;
    }

    auto sp = getChildByTag(105);
    if (sp != nullptr)
    {
        sp->setTag(-105);
        sp->stopAllActions();
        sp->runAction(Sequence::create(FadeOut::create(0.2f), RemoveSelf::create(), nullptr));
    }
    
    if (t != nullptr && hero->isMyEnemy(t))
    {
        if (hero != t)
        {
            d->cmdCastSpell(CCommandTarget(t->getId()), hero->getAttackAbilityId());
            if (!hero->isDead() && d != nullptr)
            {
                d->playRandomCtrlSound();
            }
        }
    }
    else
    {
        Sprite* sp = Sprite::createWithSpriteFrameName("UI/cmd/Target3.png");
        addChild(sp);
        sp->setPosition(Point(p.x, p.y));
        sp->runAction(Sequence::create(FadeOut::create(0.5f), RemoveSelf::create(true), nullptr));
        
        sp = Sprite::createWithSpriteFrameName("UI/cmd/Target2.png");
        addChild(sp);
        sp->setPosition(Point(p.x, p.y));
        sp->runAction(Sequence::create(Spawn::create(ScaleTo::create(0.5f, 1.5f, 1.5f), FadeOut::create(0.5f), nullptr), RemoveSelf::create(true), nullptr));

        d->cmdMove(p);
        // DemoTemp
        SimpleAudioEngine* ae = SimpleAudioEngine::getInstance();

        if (!hero->isDead())
        {
            d->playRandomCtrlSound();
        }
    }
}

void BattleSceneLayer::log(const char* fmt, ...)
{
    static Size winSz = Director::getInstance()->getVisibleSize();
    char sz[1024];
    va_list argv;
    va_start(argv, fmt);
    vsprintf(sz, fmt, argv);
    va_end(argv);

    Node* l = getCtrlLayer();
    int curLogId = getCurLogId();
    cirInc(curLogId, getBaseLogId(), getMaxLogs());
    setCurLogId(curLogId);

    Label* lbl = DCAST(l->getChildByTag(getCurLogId()), Label*);
    if (lbl != nullptr)
    {
        lbl->removeFromParentAndCleanup(true);
    }

    lbl = Label::createWithTTF(sz, FONT_ARIAL, 20);
    lbl->setHorizontalAlignment(TextHAlignment::LEFT);
    lbl->setColor(Color3B(79, 0, 255));
    l->addChild(lbl, 1, getCurLogId());
    const Size& rSz = lbl->getContentSize();
    lbl->setPosition(Point(rSz.width * 0.5 + 10, winSz.height + rSz.height * 0.5 - 10));

    for (int i = 0, j = getCurLogId(); i < getMaxLogs(); ++i)
    {
        Node* pNode = l->getChildByTag(j);
        if (pNode != nullptr)
        {
            pNode->runAction(MoveBy::create(0.1f, Point(0.0f, -lbl->getContentSize().height)));
        }

        cirDec(j, getBaseLogId(), getMaxLogs());
    }
}

void BattleSceneLayer::initTargetInfo()
{
    static Size vSz = Director::getInstance()->getVisibleSize();

    // ��ʼ��Ŀ����Ϣ���
    Sprite* pSprite = nullptr;
    Label* pLabel = nullptr;
    
    m_pTargetInfoPanel = Sprite::create("UI/TargetInfoPanel.png");
    m_ctrlLayer->addChild(m_pTargetInfoPanel);
    m_pTargetInfoPanel->setPosition(Point(vSz.width * 0.5, -m_pTargetInfoPanel->getContentSize().height * 0.5));

    float fFontSize = 28;
    float fBaseY = 46;

    // ѡ��ͷ��
    pSprite = Sprite::createWithSpriteFrameName("UI/status/portrait_sel.png");
    m_pTargetInfoPanel->addChild(pSprite, 10);
    pSprite->setPosition(Point(56, fBaseY - 2));

    m_pPortraitSel = Sprite::createWithSpriteFrameName("UI/status/portrait_sel.png");
    m_pTargetInfoPanel->addChild(m_pPortraitSel);
    m_pPortraitSel->setPosition(pSprite->getPosition());

    // ����
    float fW0 = 0;
    float fW1 = 100;
    float fBaseX = M_FIX_BASE_X(fW0, fW1, 80);
    m_pNameSel = Label::createWithTTF("", "fonts/DFYuanW7-GB2312.ttf", fFontSize);
    m_pNameSel->setAnchorPoint(Point(0.0, 0.5));
    m_pTargetInfoPanel->addChild(m_pNameSel);
    m_pNameSel->setHorizontalAlignment(TextHAlignment::LEFT);
    m_pNameSel->setPosition(Point(fBaseX, fBaseY + 2.0f));

    // �ȼ�
    fW0 = fW1;
    fW1 = 52;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 130);
    pLabel = Label::createWithTTF("Lv", FONT_COMIC_BOOK, fFontSize, Size(fW1, 32), TextHAlignment::LEFT);
    m_pTargetInfoPanel->addChild(pLabel);
    pLabel->setPosition(Point(fBaseX, fBaseY));

    fW0 = fW1;
    fW1 = 32;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 10);
    m_pTargetLv = Label::createWithTTF("12", FONT_COMIC_BOOK, fFontSize, Size(fW1, 32), TextHAlignment::LEFT);
    m_pTargetInfoPanel->addChild(m_pTargetLv);
    m_pTargetLv->setPosition(Point(fBaseX, fBaseY));

    // hp
    fW0 = fW1;
    fW1 = 52;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 84);
    pSprite = Sprite::createWithSpriteFrameName("UI/status/HP.png");
    m_pTargetInfoPanel->addChild(pSprite);
    pSprite->setPosition(Point(fBaseX, fBaseY));

    fW0 = fW1;
    fW1 = 200;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 10);
    m_pTargetHp = Label::createWithTTF("1320/3208", FONT_COMIC_BOOK, fFontSize, Size(fW1, 32), TextHAlignment::LEFT);
    m_pTargetInfoPanel->addChild(m_pTargetHp);
    m_pTargetHp->setPosition(Point(fBaseX, fBaseY));

    // ����
    fW0 = fW1;
    fW1 = 52;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 64);
    m_pTargetAtkIcon = Sprite::createWithSpriteFrameName("UI/status/PhysicalAttack.png");
    m_pTargetInfoPanel->addChild(m_pTargetAtkIcon);
    m_pTargetAtkIcon->setPosition(Point(fBaseX, fBaseY));


    fW0 = fW1;
    fW1 = 0; // ê��������
    fBaseX += M_FIX_BASE_X(fW0, fW1, 10);
    m_pTargetAtk = Label::createWithTTF("", FONT_COMIC_BOOK, fFontSize);
    m_pTargetAtk->setAnchorPoint(Point(0.0, 0.5));
    m_pTargetInfoPanel->addChild(m_pTargetAtk);
    m_pTargetAtk->setHorizontalAlignment(TextHAlignment::LEFT);
    m_pTargetAtk->setString("105 - 110");
    m_pTargetAtk->setPosition(Point(fBaseX, fBaseY));

    //fBaseX += m_pTargetAtk->getTextureRect().size.width;
    fBaseX += m_pTargetAtk->getContentSize().width;
    m_pTargetAtkEx = Label::createWithTTF("", FONT_COMIC_BOOK, fFontSize);
    m_pTargetAtkEx->setAnchorPoint(Point(0.0, 0.5));
    m_pTargetInfoPanel->addChild(m_pTargetAtkEx);
    m_pTargetAtkEx->setHorizontalAlignment(TextHAlignment::LEFT);
    m_pTargetAtkEx->setString(" +15000");
    m_pTargetAtkEx->setPosition(Point(fBaseX, fBaseY));
    m_pTargetAtkEx->setColor(Color3B(40, 220, 40));

    // ����
    fW1 = 52;
    //fBaseX += 260 - m_pTargetAtk->getTextureRect().size.width + fW1 * 0.5;
    fBaseX += 260 - m_pTargetAtk->getContentSize().width + fW1 * 0.5;
    m_pTargetDefIcon = Sprite::createWithSpriteFrameName("UI/status/HeavyArmor.png");
    m_pTargetInfoPanel->addChild(m_pTargetDefIcon);
    m_pTargetDefIcon->setPosition(Point(fBaseX, fBaseY));

    fW0 = fW1;
    fW1 = 0;
    fBaseX += M_FIX_BASE_X(fW0, fW1, 10);
    m_pTargetDef = Label::createWithTTF("32", FONT_COMIC_BOOK, fFontSize, Size(fW1, 32), TextHAlignment::LEFT);
    m_pTargetDef->setAnchorPoint(Point(0.0, 0.5));
    m_pTargetInfoPanel->addChild(m_pTargetDef);
    m_pTargetDef->setPosition(Point(fBaseX, fBaseY));

    fBaseX += m_pTargetDef->getContentSize().width;
    m_pTargetDefEx = Label::createWithTTF("", FONT_COMIC_BOOK, fFontSize);
    m_pTargetDefEx->setAnchorPoint(Point(0.0, 0.5));
    m_pTargetInfoPanel->addChild(m_pTargetDefEx);
    m_pTargetDefEx->setHorizontalAlignment(TextHAlignment::LEFT);
    m_pTargetDefEx->setString(" +50");
    m_pTargetDefEx->setPosition(Point(fBaseX, fBaseY));
    m_pTargetDefEx->setColor(Color3B(40, 220, 40));

    m_iTargetInfoUnitId = 0;
}

void BattleSceneLayer::updateTargetInfo(int id)
{
    if (m_bShowTargetInfo == false)
    {
        return;
    }

    if (m_iTargetInfoUnitId != id && id != 0)
    {
        m_iTargetInfoUnitId = id;
    }

    M_DEF_GC(gc);
    SpriteFrameCache* fc = gc->getfc();

    CBattleWorld* w = DCAST(getWorld(), CBattleWorld*);
    CUnit* pUnit = w->getUnit(m_iTargetInfoUnitId);
    auto ctrl = w->getUnit(w->getCtrlUnit());
    if (pUnit == nullptr)
    {
        if (ctrl == nullptr)
        {
            showTargetInfo(false);
            return;
        }

        const CCommandTarget& rTarget = DCAST(ctrl->getDraw(), CUnitDrawForCC*)->getCastTarget();
        if (rTarget.getTargetType() == CCommandTarget::kUnitTarget)
        {
            m_iTargetInfoUnitId = rTarget.getTargetUnit();
            pUnit = w->getUnit(m_iTargetInfoUnitId);
            if (!pUnit)
            {
                showTargetInfo(false);
                return;
            }
        }
        else
        {
            showTargetInfo(false);
            return;
        }
    }

    if (!m_pTargetInfoPanel->isVisible())
    {
        m_pTargetInfoPanel->setVisible(true);
    }

    char szBuf[1024];

    CUnitDrawForCC* d = DCAST(pUnit->getDraw(), CUnitDrawForCC*);
    // ѡ��ͷ��
    sprintf(szBuf, "Sprites/%s/portrait_sel.png", d->getSpriteInfo()->getName());
    SpriteFrame* sf = fc->getSpriteFrameByName(szBuf);
    if (sf == nullptr)
    {
        sf = fc->getSpriteFrameByName("UI/status/portrait_sel.png");
    }
    m_pPortraitSel->setSpriteFrame(sf);
    m_pNameSel->setString(gbk_to_utf8(pUnit->getName(), szBuf));
    
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
    if (atkAct != nullptr)
    {
        switch (atkAct->getBaseAttack().getType())
        {
        case CAttackValue::kPhysical:
            m_pTargetAtkIcon->setSpriteFrame(fc->getSpriteFrameByName("UI/status/PhysicalAttack.png"));
            break;

        case CAttackValue::kMagical:
            m_pTargetAtkIcon->setSpriteFrame(fc->getSpriteFrameByName("UI/status/MagicalAttack.png"));
            break;
        }
        int iAtk0 = 0;
        int iAtk1 = 0;
        float fAtk = 0;
        float fAtkRnd = 0;
        int iAtkEx = 0;

        fAtk = atkAct->getBaseAttackValue();
        fAtkRnd = atkAct->getAttackValueRandomRange() * 0.5f;
        iAtk0 = toInt(fAtk * (1 - fAtkRnd));
        iAtk1 = toInt(fAtk * (1 + fAtkRnd));
        iAtkEx = toInt(fAtk * (atkAct->getExAttackValue().getMulriple() - 1.0f) + atkAct->getExAttackValue().getAddend());

        if ((iAtk0 != m_stTargetInfo.iAtk0) || (iAtk1 != m_stTargetInfo.iAtk1) || (iAtkEx != m_stTargetInfo.iAtkEx))
        {
            sprintf(szBuf, "%d - %d", iAtk0, iAtk1);
            m_pTargetAtk->setString(szBuf);
            m_stTargetInfo.iAtk0 = iAtk0;
            m_stTargetInfo.iAtk1 = iAtk1;

            if (iAtkEx != 0)
            {
                sprintf(szBuf, iAtkEx < 0 ? " %d" : " +%d", iAtkEx);
                m_pTargetAtkEx->setString(szBuf);
                //m_pTargetAtkEx->setPosition(m_pTargetAtk->getPosition() + Point(m_pTargetAtk->getTextureRect().size.width, 0));
                m_pTargetAtkEx->setPosition(m_pTargetAtk->getPosition() + Point(m_pTargetAtk->getContentSize().width, 0));
                m_pTargetAtkEx->setColor(iAtkEx < 0 ? Color3B(220, 40, 40) : Color3B(40, 220, 40));
            }
            else
            {
                m_pTargetAtkEx->setString("");
            }
            m_stTargetInfo.iAtkEx = iAtkEx;
        }
    }
    
    // ����
    auto& armor = pUnit->getBaseArmor();
    switch (armor.getType())
    {
    case CArmorValue::kHeavy:
        m_pTargetDefIcon->setSpriteFrame(fc->getSpriteFrameByName("UI/status/HeavyArmor.png"));
        break;

    case CArmorValue::kCrystal:
        m_pTargetDefIcon->setSpriteFrame(fc->getSpriteFrameByName("UI/status/CrystalArmor.png"));
        break;
    }

    
    float fDef = armor.getValue();
    int iDef = toInt(fDef);
    int iDefEx = toInt(fDef * (pUnit->getExArmorValue().getMulriple() - 1.0f) + pUnit->getExArmorValue().getAddend());
    if (iDef != m_stTargetInfo.iDef || iDefEx != m_stTargetInfo.iDefEx)
    {
        sprintf(szBuf, "%d", iDef);
        m_pTargetDef->setString(szBuf);
        m_stTargetInfo.iDef = iDef;

        if (iDefEx != 0)
        {
            sprintf(szBuf, iDefEx < 0 ? " %d" : " +%d", iDefEx);
            m_pTargetDefEx->setString(szBuf);
            //m_pTargetDefEx->setPosition(m_pTargetDef->getPosition() + Point(m_pTargetDef->getTextureRect().size.width, 0));
            m_pTargetDefEx->setPosition(m_pTargetDef->getPosition() + Point(m_pTargetDef->getContentSize().width, 0));
            m_pTargetDefEx->setColor(iDefEx < 0 ? Color3B(220, 40, 40) : Color3B(40, 220, 40));
        }
        else
        {
            m_pTargetDefEx->setString("");
        }
        m_stTargetInfo.iDefEx = iDefEx;
    }

    if (pUnit->isDead())
    {
        m_iTargetInfoUnitId = 0;
        //showTargetInfo(false);
    }
}

void BattleSceneLayer::showTargetInfo(bool bShow /*= true*/)
{
    if (bShow == m_bShowTargetInfo)
    {
        return;
    }

    m_pTargetInfoPanel->stopAllActions();
    const Point& from = m_pTargetInfoPanel->getPosition();
    Point to(from.x, 0.0f);
    if (bShow)
    {
        to.y = m_pTargetInfoPanel->getContentSize().height * 0.5f;
    }
    else
    {
        to.y = -m_pTargetInfoPanel->getContentSize().height * 0.5f;
    }

    // DemoTemp
    static SimpleAudioEngine* ae = SimpleAudioEngine::getInstance();
    ae->playEffect("sounds/Effects/UIMove.mp3");
    m_pTargetInfoPanel->runAction(MoveTo::create(to.getDistance(from) * 0.002f, to));

    m_bShowTargetInfo = bShow;
}

ButtonBase* BattleSceneLayer::createAbilityButton(CAbility* ability)
{
    auto image = ability->getImageName();
    if (image[0] == 0)
    {
        return nullptr;
    }

    auto sp = Sprite::create(image);

    auto orgScale = sp->getScale();
    sp->setScale(0.95f);
    auto txSel = Utils::nodeToTexture(sp);
    sp->setScale(orgScale);

    auto txDis = Utils::nodeToTexture(sp, &Utils::tranGrayscale);

    auto ret = ButtonNormal::create(
        sp,
        Sprite::createWithTexture(txSel),
        Sprite::createWithTexture(txDis),
        Sprite::create("UI/Ability/AbilityBlink2.png"),
        Sprite::create("UI/Ability/AbilityMask.png"),
        ability->getCoolDown(),
        CC_CALLBACK_1(BattleSceneLayer::onClickAbilityButton, this),
        nullptr);
    ret->setUserData((void*)ability->getId());
        
    return ret;
}

void BattleSceneLayer::initHeroAbilityPanel()
{
    m_bp = ButtonPanel::create(4, 1, Size(144, 136), 0, 0, Sprite::create("UI/Ability/AbilityButtonPanel.png"), -5, 6);
    m_ctrlLayer->addChild(m_bp);
    m_bp->setPosition(Point(wsz().width - 100, wsz().height * 0.5));
    
    //auto btn = ButtonNormal::createWithFrameName("UI/Button/Fist/Normal.png", "UI/Button/Fist/On.png", "UI/Button/Fist/Disabled.png", "UI/Button/Fist/Blink.png", "UI/Button/Fist/Mask.png", 90.0f, CC_CALLBACK_1(BattleSceneLayer::onClickFist, this), nullptr);
    //m_bp->addButtonEx(btn, ButtonPanel::kBottomToTop, ButtonPanel::kRightToLeft);
}

void BattleSceneLayer::updateHeroAbilityPanel()
{
    if (m_bp == nullptr)
    {
        return;
    }

    CBattleWorld* w = DCAST(getWorld(), CBattleWorld*);
    CUnit* u = w->getUnit(w->getCtrlUnit());
    if (u == nullptr)
    {
        return;
    }

    // ����ȥ����ǰ�ٿص�λ���ϲ����ڵļ���
    int m = m_bp->getMaxCount();
    int n = m_bp->getCount();
    for (int i = 0, j = 0; i < m && j < n; ++i)
    {
        auto y = m - i - 1;
        auto btn = m_bp->getButton(0, y);
        if (btn != nullptr && u->getActiveAbility((int)btn->getUserData()) == nullptr)
        {
            m_bp->delButton(0, y);
            ++j;
        }
    }

    CUnit::MAP_ACTIVE_ABILITIES& as = u->getActiveAbilities();
    M_MAP_FOREACH(as)
    {
        auto id = M_MAP_IT->first;
        auto a = M_MAP_EACH;
        M_MAP_NEXT;

        if (a->isTemporary())
        {
            continue;
        }

        if (m_bp->getCount() < m_bp->getMaxCount() && m_bp->getButton([id](ButtonBase* btn)
        {
            return btn->getUserData() == (void*)id;
        }) == nullptr)
        {
            // ��ťû����������в����ڸü��ܵİ�ť
            auto btn = createAbilityButton(a);
            if (btn != nullptr)
            {
                m_bp->addButtonEx(btn, ButtonPanel::kTopToBottom);
                if (a->isCoolingDown())
                {
                    float per = a->getCoolingDownElapsed() / a->getRealCoolDown();
                    btn->coolDown(per);
                }
            }
        }
    }
}

void BattleSceneLayer::cancelAbilityWaiting(int abilityId, int cancelTag)
{
    auto btn = m_bp->getButton([abilityId](ButtonBase* b)
    {
        return (int)b->getUserData() == abilityId;
    });

    if (btn != nullptr)
    {
        btn->removeChildByTag(10);
    }

    auto w = DCAST(getWorld(), CBattleWorld*);
    auto u = w->getUnit(w->getCtrlUnit());
    if (u == nullptr)
    {
        return;
    }

    auto d = DCAST(u->getDraw(), CUnitDrawForCC*);
    d->setWaitForCastTargetActiveAbilityId(0);
}

void BattleSceneLayer::onClickFist(Ref* btn)
{
    M_DEF_GC(gc);
    gc->playSound("sounds/Effects/Sound_GUIButtonCommon.mp3");
    // DragonStrike
    DCAST(btn, ButtonNormal*)->coolDown();
    CBattleWorld* w = DCAST(getWorld(), CBattleWorld*);
    CUnit* u = w->getHero();
    if (u == nullptr)
    {
        return;
    }

    CUnitDrawForCC* d = DCAST(u->getDraw(), CUnitDrawForCC*);
    setMoveEnabled(false);
    Node* sp = d->getSprite();
    Effect* eff = Effect::create("Sprites/KungFu/move", 0.03f);
    eff->addAnimation("Sprites/KungFu/act8", 0.05f);
    addChild(eff, M_BASE_Z - d->getPosition().y, 77777);
    eff->setPosition(Point(getContentSize().width * getScale() + 100, d->getPosition().y + 150.0f));
    eff->playForever();
    eff->setFlippedX(true);
    m_dscur = 0;
    eff->runAction(
        Sequence::create(
        MoveTo::create(eff->getPosition().getDistance(sp->getPosition()) * 0.9 / 1000, Point((sp->getPositionX() * 0.9 + eff->getPositionX() * 0.1), sp->getPositionY()) + Point(0.0f, 133.0f)),
        CallFuncN::create(CC_CALLBACK_1(BattleSceneLayer::onDragonStrikeUpdate, this)),
        nullptr));

}

void BattleSceneLayer::onClickHeroPortrait(Ref* ref)
{
    M_DEF_GC(gc);
    gc->playSound("sounds/Effects/Sound_GUIButtonCommon.mp3");

    auto btn = DCAST(ref, ButtonNormal*);
    auto pi = (CPortraitPanel::PORTRAIT_INFO*)btn->getUserData();
    
    auto w = DCAST(getWorld(), CBattleWorld*);
    auto u = w->getUnit(pi->unitId);
    if (u == nullptr)
    {
        return;
    }

    showTargetInfo();
    updateTargetInfo(pi->unitId);
    w->setCtrlUnit(u);
    updateHeroAbilityPanel();
}

void BattleSceneLayer::onClickAbilityButton(Ref* mi)
{
    M_DEF_GC(gc);
    gc->playSound("sounds/Effects/Sound_GUIButtonCommon.mp3");

    auto btn = DCAST(mi, ButtonBase*);
    auto w = DCAST(getWorld(), CBattleWorld*);
    auto u = w->getUnit(w->getCtrlUnit());
    if (u == nullptr)
    {
        btn->setClickRetCode(-1);
        return;
    }

    auto d = DCAST(u->getDraw(), CUnitDrawForCC*);
    auto id = (int)btn->getUserData();
    auto a = u->getActiveAbility(id);
    bool cancelCurButton = false;

    auto sp = getChildByTag(105);
    if (sp != nullptr)
    {
        sp->setTag(-105);
        sp->stopAllActions();
        sp->runAction(Sequence::create(FadeOut::create(0.2f), RemoveSelf::create(), nullptr));
    }

    auto wid = d->getWaitForCastTargetActiveAbilityId();
    if (wid != 0)
    {
        // �еȴ�ȷ�ϵ�Ŀ��ļ���
        cancelAbilityWaiting(wid, 10);
        cancelCurButton = (wid == id);
    }

    if (u->isDead())
    {
        btn->setClickRetCode(-1);
        return;
    }

    if (a == nullptr)
    {
        updateHeroAbilityPanel();
        btn->setClickRetCode(-1);
        return;
    }

    if (a->isCoolingDown())
    {
        btn->setClickRetCode(-1);
        return;
    }

    switch (a->getCastTargetType())
    {
    case CCommandTarget::kNoTarget:
        //d->say("Taste my spear!");
        if (d->cmdCastSpell(CCommandTarget(), id) < 0)
        {
            btn->setClickRetCode(-1);
            return;
        }
        break;

    case CCommandTarget::kUnitTarget:
        if (w->getUnit(m_iTargetInfoUnitId) == nullptr)
        {
            btn->setClickRetCode(-1);
            return;
        }

        if (d->cmdCastSpell(CCommandTarget(m_iTargetInfoUnitId), id) < 0)
        {
            auto t = CUnitGroup::getNearestUnitInRange(w, d->getPosition(), a->getCastRange() + d->getHostilityRange(), CUnitGroup::matchLivingEnemy(u));
            if (t != nullptr)
            {
                if (d->cmdCastSpell(CCommandTarget(t->getId()), id) < 0)
                {
                    btn->setClickRetCode(-1);
                    return;
                }
            }
        }
        break;

    case CCommandTarget::kPointTarget:
        if (cancelCurButton == false && d->getWaitForCastTargetActiveAbilityId() == 0)
        {
            auto sp = Sprite::create("UI/Ability/AbilityCancel.png");
            btn->addChild(sp, 10, 10);
            sp->setPosition(Point(btn->getAnchorPointInPoints().x, sp->getContentSize().height * 0.5));

            d->setWaitForCastTargetActiveAbilityId(id);
        }
        break;
    }
}

void BattleSceneLayer::initResourceInfo()
{
    static Size oSz = Director::getInstance()->getVisibleSize();
    Sprite* sp = Sprite::create("UI/WavePanel.png");
    m_ctrlLayer->addChild(sp);
    sp->setTag(1000);
    const Size& spSz = sp->getContentSize();
    sp->setPosition(Point(spSz.width * 0.5 + 50, oSz.height - spSz.height * 0.5 - 50));
    m_pGold = Label::createWithTTF("      ", FONT_COMIC_BOOK, 28, Size(100, 48), TextHAlignment::LEFT);
    sp->addChild(m_pGold);
    m_pGold->setPosition(Point(310, 85));
}

void BattleSceneLayer::updateResourceInfo(int gold)
{
    char sz[64];
    sprintf(sz, "%d", gold);
    m_pGold->setString(sz);
}

void BattleSceneLayer::onDragonStrikeUpdate(Node* pNode)
{
    CBattleWorld* w = DCAST(getWorld(), CBattleWorld*);
    CUnit* u = w->getHero();
    if (u == nullptr)
    {
        return;
    }

    M_DEF_GC(gc);
    static Size wsz = Director::getInstance()->getVisibleSize();
    Effect* eff = nullptr;
    Effect* eff2 = nullptr;
    LayerColor* lc = nullptr;
    Point ctrlPos;
    Sprite* sp = nullptr;
    
    CUnitDrawForCC* d = DCAST(u->getDraw(), CUnitDrawForCC*);
    Node* sn = d->getSprite()->getShadow();

    char sz[1024];

    switch (m_dscur)
    {
    case 0:  // ��������д
        sprintf(sz, "sounds/Effects/Monk%02d.mp3", rand() % 3);
        gc->playSound(sz);
        eff = (Effect*)pNode;
        Director::getInstance()->getScheduler()->setTimeScale(0.2f);
        eff->runAction(
            Sequence::create(
                MoveTo::create(eff->getPosition().getDistance(sn->getPosition()) / 1000, Point(sn->getPosition().x, eff->getPosition().y)),
                CallFuncN::create(CC_CALLBACK_1(BattleSceneLayer::onDragonStrikeUpdate, this)),
                nullptr));

        ctrlPos = eff->getPosition();
        lc = LayerColor::create(Color4B(0, 0, 0, 128));
        lc->setContentSize(Size(wsz.width * 10, wsz.height));
        addChild(lc);
        lc->setTag(7771);
        lc->setPosition(Point(0.0f, wsz.height));
        lc->setOpacity(0);
        lc->runAction(EaseExponentialOut::create(Spawn::createWithTwoActions(MoveTo::create(0.15f, Point(0.0f, ctrlPos.y - 0)), FadeTo::create(0.15f, 200))));

        lc = LayerColor::create(Color4B(0, 0, 0, 128));
        lc->setContentSize(Size(wsz.width * 10, wsz.height));
        addChild(lc);
        lc->setTag(7772);
        lc->setPosition(Point(0.0f, -wsz.height));
        lc->setOpacity(0);
        lc->runAction(EaseExponentialOut::create(Spawn::createWithTwoActions(MoveTo::create(0.15f, Point(0.0f, ctrlPos.y - 200 - wsz.height)), FadeTo::create(0.15f, 200))));
        break;

    case 1:  // Dra
        eff = (Effect*)pNode;
        eff->playRelease(1);
        sp = Sprite::create("UI/Dra.png");
        addChild(sp, 10000);
        sp->setPosition(eff->getPosition() + Point(200 - eff->getContentSize().width, 0.0f));
        sp->setOpacity(0);
        sp->setScale(8.0f);
        sp->runAction(
            Sequence::create(
                EaseExponentialIn::create(
                    Spawn::create(
                        FadeIn::create(0.08f),
                        ScaleTo::create(0.08f, 0.5f),
                        nullptr)),
                CallFuncN::create(CC_CALLBACK_1(BattleSceneLayer::onDragonStrikeUpdate, this)),
                FadeOut::create(0.5f),
                RemoveSelf::create(),
                nullptr));
        
        break;

    case 2:  // Shake and Gon
        eff = (Effect*)getChildByTag(77777);
        M_MAP_FOREACH(w->getUnits())
        {
            CUnit* t = M_MAP_EACH;
            CUnitDraw2D* td = DCAST(t->getDraw(), CUnitDraw2D*);
            if (t != nullptr && CUnitGroup::matchLivingEnemy(w->getHero())(t) && td->getPosition().getDistance(CPoint(eff->getPositionX(), eff->getPositionY())) < 300.0f)
            {
                CAttackData* ad = new CAttackData;
                ad->setAttackValue(CAttackValue::kHoly, 100 + rand() % (50 * w->getHero()->getLevel()));
                t->damaged(ad, w->getHero(), CUnit::kMaskAll);
            }
            M_MAP_NEXT;
        }
        runAction(Shake::create(0.05f, 4, 30.0f));
        gc->playSound("sounds/Effects/ThunderCap.mp3");
        eff2 = Effect::create("Effects/Wave", 0.02f);
        eff->addChild(eff2);
        eff2->setPosition(eff->getAnchorPointInPoints() + Point(0.0f, -133.0f));
        eff2->playRelease(0, 2.0f);

        sp = Sprite::create("UI/Gon.png");
        addChild(sp, 10000);
        sp->setPosition(eff->getPosition() + Point(-50.0f, 0.0f));
        sp->setOpacity(0);
        sp->setScale(8.0f);
        sp->runAction(
            Sequence::create(
                EaseExponentialIn::create(
                    Spawn::create(
                        FadeIn::create(0.08f),
                        ScaleTo::create(0.08f, 0.5f),
                        nullptr)),
                CallFuncN::create(CC_CALLBACK_1(BattleSceneLayer::onDragonStrikeUpdate, this)),
                FadeOut::create(0.5f),
                RemoveSelf::create(),
                nullptr));
        break;

    case 3:  // Shake
        eff = (Effect*)getChildByTag(77777);
        M_MAP_FOREACH(w->getUnits())
        {
            CUnit* t = M_MAP_EACH;
            CUnitDraw2D* td = DCAST(t->getDraw(), CUnitDraw2D*);
            if (t != nullptr && CUnitGroup::matchLivingEnemy(w->getHero())(t) && td->getPosition().getDistance(CPoint(eff->getPositionX(), eff->getPositionY())) < 300.0f)
            {
                CAttackData* ad = new CAttackData;
                ad->setAttackValue(CAttackValue::kHoly, 100 + rand() % (100 * w->getHero()->getLevel()));
                t->damaged(ad, w->getHero(), CUnit::kMaskAll);
            }
            M_MAP_NEXT;
        }
        runAction(Sequence::createWithTwoActions(Shake::create(0.05f, 4, 30.0f), CallFuncN::create(CC_CALLBACK_1(BattleSceneLayer::onDragonStrikeUpdate, this))));
        gc->playSound("sounds/Effects/ThunderCap.mp3");
        eff2 = Effect::create("Effects/Wave", 0.02f);
        eff->addChild(eff2);
        eff2->setPosition(eff->getAnchorPointInPoints() + Point(0.0f, -133.0f));
        eff2->playRelease(0, 2.0f);
        break;

    case 4:  // Strike
        eff = (Effect*)getChildByTag(77777);
        sp = Sprite::create("UI/Strike.png");
        addChild(sp, 10000);
        sp->setPosition(eff->getPosition() + Point(eff->getContentSize().width * 0.5 - 50, 0.0f));
        sp->setOpacity(0);
        sp->setScale(8.0f);
        sp->runAction(
            Sequence::create(
                EaseExponentialIn::create(
                Spawn::create(
                    FadeIn::create(0.2f),
                    ScaleTo::create(0.2f, 0.5f),
                    nullptr)),
                CallFuncN::create(CC_CALLBACK_1(BattleSceneLayer::onDragonStrikeUpdate, this)),
                FadeOut::create(0.5f),
                RemoveSelf::create(),
                nullptr));
        gc->playSound("sounds/Effects/Hero_Monk_Firedragon.mp3");
        break;

    case 5:  // Shake
        eff = (Effect*)getChildByTag(77777);
        M_MAP_FOREACH(w->getUnits())
        {
            CUnit* t = M_MAP_EACH;
            CUnitDraw2D* td = DCAST(t->getDraw(), CUnitDraw2D*);
            if (t != nullptr && CUnitGroup::matchLivingEnemy(w->getHero())(t) && td->getPosition().getDistance(CPoint(eff->getPositionX(), eff->getPositionY())) < 300.0f)
            {
                CAttackData* ad = new CAttackData;
                ad->setAttackValue(CAttackValue::kHoly, 100 + rand() % (150 * w->getHero()->getLevel()));
                //ad->addAttackBuff(CAttackBuff(w->getDragonStrikeBuff()->getId(), 1));
                t->damaged(ad, w->getHero(), CUnit::kMaskAll);
                t->addBuffAbility(new CStunBuff("StunBuff", "Stun", 5.0f, true));
            }
            M_MAP_NEXT;
        }
        runAction(Shake::create(0.1f, 8, 60.0f));
        gc->playSound("sounds/Effects/ThunderCap.mp3");
        eff->runAction(
            Sequence::create(
                DelayTime::create(0.05f),
                CallFuncN::create(CC_CALLBACK_1(BattleSceneLayer::onDragonStrikeUpdate, this)),
                nullptr));

        getChildByTag(7771)->runAction(Sequence::createWithTwoActions(EaseExponentialOut::create(Spawn::createWithTwoActions(MoveTo::create(0.5f, Point(0.0f, wsz.height)), FadeTo::create(0.5f, 0))), RemoveSelf::create()));
        getChildByTag(7772)->runAction(Sequence::createWithTwoActions(EaseExponentialOut::create(Spawn::createWithTwoActions(MoveTo::create(0.5f, Point(0.0f, -wsz.height)), FadeTo::create(0.5f, 0))), RemoveSelf::create()));
        break;

    case 6:
        setMoveEnabled(true);
        eff = Effect::create("Effects/Wave", 0.1f);
        getChildByTag(77777)->addChild(eff);
        eff->setPosition(getChildByTag(77777)->getAnchorPointInPoints() + Point(0.0f, -133.0f));
        eff->playRelease();
        Director::getInstance()->getScheduler()->setTimeScale(1.0f);
        break;
    }
    ++m_dscur;
}

void BattleSceneLayer::onClickPause(Ref* obj)
{
    Director::getInstance()->getScheduler()->setTimeScale(1.0f);
    showCtrlPanel();
}

void BattleSceneLayer::initCtrlPanel()
{
    static Size wsz = Director::getInstance()->getVisibleSize();

    // ����pannel
    m_panel = PopPanel::create(Sprite::create("UI/PanelSmall.png"));
    //m_panel->getMenu()->setTouchPriority(-10);
    m_panel->getMenu()->setPosition(Point::ZERO);

    m_ctrlLayer2->addChild(m_panel);
    Size psz = m_panel->getContentSize();

    Sprite* sp = Sprite::create("UI/Chain.png");
    m_panel->addChild(sp, -1);
    sp->setPosition(Point(psz.width * 0.2, psz.height + sp->getContentSize().height * 0.2));

    sp = Sprite::create("UI/Chain.png");
    m_panel->addChild(sp, -1);
    sp->setPosition(Point(psz.width * 0.8, psz.height + sp->getContentSize().height * 0.2));

    sp = Sprite::create("UI/Title.png");
    m_panel->addChild(sp, 1, 2);
    sp->setPosition(Point(psz.width * 0.5, psz.height));

    Label* lbl = Label::createWithTTF("OPTIONS", FONT_YIKES, 72);
    lbl->setColor(Color3B(80, 60, 50));
    sp->addChild(lbl, 1, 2);
    lbl->setPosition(sp->getAnchorPointInPoints() + Point(0.0f, 20));
    
    MenuItemSprite* btnClose = MenuItemSprite::create(
        Sprite::create("UI/Button/BtnCloseNor.png"),
        Sprite::create("UI/Button/BtnCloseSel.png"),
        nullptr,
        CC_CALLBACK_1(BattleSceneLayer::onCloseCtrlPanel, this));
    m_panel->addButton(btnClose);
    btnClose->setPosition(Point(psz.width - btnClose->getContentSize().width * 0.5 - 50, psz.height - btnClose->getContentSize().height * 0.5 - 50));

    MenuItemSprite* btnResume = MenuItemSprite::create(
        Sprite::create("UI/Button/BtnResumeNor.png"),
        Sprite::create("UI/Button/BtnResumeSel.png"),
        nullptr,
        CC_CALLBACK_1(BattleSceneLayer::onClickResume, this));
    m_panel->addButton(btnResume);
    btnResume->setTag(1);
    btnResume->setPosition(Point(psz.width * 0.5, psz.height * 0.5 + btnResume->getContentSize().height + 20));
    
    MenuItemSprite* btnRestart = MenuItemSprite::create(
        Sprite::create("UI/Button/BtnRestartNor.png"),
        Sprite::create("UI/Button/BtnRestartSel.png"),
        nullptr,
        CC_CALLBACK_1(BattleSceneLayer::onClickRestart, this));
    m_panel->addButton(btnRestart);
    btnRestart->setTag(2);
    btnRestart->setPosition(Point(0.0f, -20 - btnRestart->getContentSize().height) + btnResume->getPosition());

    MenuItemSprite* btnQuit = MenuItemSprite::create(
        Sprite::create("UI/Button/BtnQuitNor.png"),
        Sprite::create("UI/Button/BtnQuitSel.png"),
        nullptr,
        CC_CALLBACK_1(BattleSceneLayer::onClickQuit, this));
    m_panel->addButton(btnQuit);
    btnQuit->setTag(3);
    btnQuit->setPosition(Point(0.0f, -20 - btnQuit->getContentSize().height) + btnRestart->getPosition());

    // panel����
    m_panel->setScale(min(wsz.width * 0.6 / psz.width, wsz.height * 0.6 / psz.height));
    psz = psz * m_panel->getScale();
    m_panel->setPosition(Point(wsz.width * 0.5, wsz.height + psz.height * 0.5));
}

void BattleSceneLayer::showCtrlPanel()
{
    static Size wsz = Director::getInstance()->getVisibleSize();
    M_DEF_GC(gc);
    // ����panel
    gc->playSound("sounds/Effects/GUITransitionOpen.mp3");
    m_panel->show();

    m_ctrlLayer2->setMaskEnabled(true);
    
    onExit();
    m_ctrlLayer->onExit();
}

void BattleSceneLayer::onCloseCtrlPanel(Ref* obj)
{
    static Size wsz = Director::getInstance()->getVisibleSize();
    M_DEF_GC(gc);
    gc->playSound("sounds/Effects/GUITransitionOpen.mp3");
    m_panel->hide();

    m_ctrlLayer2->setMaskEnabled(false);

    onEnter();
    m_ctrlLayer->onEnter();
}

void BattleSceneLayer::onClickResume(Ref* obj)
{
    onCloseCtrlPanel(nullptr);
}

void BattleSceneLayer::onClickRestart(Ref* obj)
{
    getWorld()->shutdown();
    Director::getInstance()->replaceScene(BattleSceneLayer::scene());
}

void BattleSceneLayer::onClickQuit(Ref* obj)
{
    getWorld()->shutdown();
    Director::getInstance()->replaceScene(StageSceneLayer::scene());
}

void BattleSceneLayer::onClickSound(Ref* obj)
{
}

void BattleSceneLayer::onClickMusic(Ref* obj)
{
}

void BattleSceneLayer::endWithVictory(int grade)
{
    Director::getInstance()->getScheduler()->setTimeScale(1.0f);
    static Size wsz = Director::getInstance()->getVisibleSize();
    M_DEF_GC(gc);
    gc->playSound("sounds/Effects/Sound_QuestCompleted.mp3");

    CUserData::instance()->newGrades(CUserData::instance()->m_stageSel, grade);

    CBattleWorld* w = DCAST(getWorld(), CBattleWorld*);
    w->setLuaWorldTickEnabled(false);

    m_ctrlLayer2->setMaskEnabled(true);
    Sprite* sp = Sprite::create("UI/VictoryBadge.png");
    sp->setUserData((void*)grade);
    
    m_ctrlLayer2->addChild(sp);
    sp->setPosition(Point(wsz.width * 0.5, wsz.height * 0.6));
    float scale = m_panel->getScale();
    sp->setScale(scale * 0.2);
    
    sp->runAction(Sequence::create(ScaleTo::create(0.2f, scale * 1.2), ScaleTo::create(0.1f, scale * 1.0), DelayTime::create(1.0f), CallFuncN::create(CC_CALLBACK_1(BattleSceneLayer::onShowVictoryDone, this)), nullptr));
}

void BattleSceneLayer::endWithDefeat()
{
    M_DEF_GC(gc);
    gc->playSound("sounds/Effects/Sound_QuestFailed.mp3");

    Director::getInstance()->getScheduler()->setTimeScale(1.0f);
    CBattleWorld* w = DCAST(getWorld(), CBattleWorld*);
    w->setLuaWorldTickEnabled(false);

    Label* lbl = DCAST(m_panel->getChildByTag(2)->getChildByTag(2), Label*);
    lbl->setString("DEFEATS");

    MenuItemSprite* btn = DCAST(m_panel->getMenu()->getChildByTag(2), MenuItemSprite*);
    btn->setNormalImage(Sprite::create("UI/Button/BtnTryAgainNor.png"));
    btn->setSelectedImage(Sprite::create("UI/Button/BtnTryAgainSel.png"));

    //btn = DCAST(m_panel->getMenu()->getChildByTag(1), MenuItemSprite*);
    //btn->setVisible(false);

    showCtrlPanel();
}

void BattleSceneLayer::onShowVictoryDone(Node* node)
{
    int grade = (int)node->getUserData();

    for (int i = 0; i < 3; ++i)
    {
        Sprite* sp = Sprite::createWithSpriteFrameName(i < grade ? "UI/Stage/Star.png" : "UI/Stage/Unstar.png");
        node->getParent()->addChild(sp, 10 - i);
        sp->setScale(node->getScale() * 3);
        const Size ndSz = node->getContentSize() * node->getScale();
        const Size spSz = sp->getContentSize() * sp->getScale();
        float bx = spSz.width + 0;
        float by = ndSz.height * 0.5 + spSz.height * 0.5 + 0;
        sp->setPosition(node->getPosition() + Point(0.0f, -by));
        sp->runAction(EaseExponentialOut::create(MoveTo::create(0.5f, node->getPosition() + Point(bx * (i - 1), -by))));
    }

    node->runAction(Sequence::create(DelayTime::create(5.0f), CallFunc::create(CC_CALLBACK_0(BattleSceneLayer::onClickQuit, this, nullptr)), nullptr));
}

