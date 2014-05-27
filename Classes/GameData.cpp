#include "CommHeader.h"
#include "GameData.h"
#include "GameControl.h"
#include "AbilityLibrary.h"
#include "LuaBinding.h"
#include "LuaBindingForCC.h"
#include "LuaScriptEngine.h"


// CGameData
CGameData::CGameData()
{
    setDbgClassName("CGameData");
    auto L = CLuaScriptEngine::instance()->getLuaHandle();
    luaL_insertloader(L, luaModuleLoader4cc);
    luaRegCommFunc(L);
    luaRegCommFuncForCC(L);

    lua_getglobal(L, "setSearchPath");
    lua_call(L, 0, 0);

    luaL_includefile(L, "Init.lua");

    initStageData();
    initAbilityData();
}

void CGameData::initStageData()
{
    STAGE_INFO stage;

    // stage 0
    stage.name = "The Sunwell";
    stage.desc = "The Sunwell is a fount of mystical power located in Quel\'Thalas";
    stage.btnNorName = "UI/Stage/Stage00Nor.png";
    stage.btnDisName = "UI/Stage/Stage00Dis.png";
    stage.pos = Point(200, 200);
    stage.prevIndex.clear();
    stage.script = "stage00.lua";
    stage.background = "backgrounds/BackgroundHD01.png";
    m_stages.push_back(stage);

    // stage 1
    stage.name = "The Death of the Captain";
    stage.desc = "It was created from a vial of water from the Well of Eternity";
    stage.btnNorName = "UI/Stage/Stage01Nor.png";
    stage.btnDisName = "UI/Stage/Stage01Dis.png";
    stage.pos = Point(500, 200);
    stage.prevIndex.clear();
    stage.prevIndex.push_back(0);
    stage.script = "stage01.lua";
    stage.background = "backgrounds/BackgroundHD01.png";
    m_stages.push_back(stage);

    // stage 2
    stage.name = "Strike Back";
    stage.desc = "It empowered the high elves for approximately nine thousand years, until Arthas used it to resurrect Kel\'Thuzad as a lich";
    stage.btnNorName = "UI/Stage/Stage02Nor.png";
    stage.btnDisName = "UI/Stage/Stage02Dis.png";
    stage.pos = Point(800, 200);
    stage.prevIndex.clear();
    stage.prevIndex.push_back(1);
    stage.script = "stage02.lua";
    stage.background = "backgrounds/BackgroundHD01.png";
    m_stages.push_back(stage);

    // stage 3
    stage.name = "Doubt";
    stage.desc = "His resurrection tainted the Sunwell in the process, severing the high elves from their source of power";
    stage.btnNorName = "UI/Stage/Stage03Nor.png";
    stage.btnDisName = "UI/Stage/Stage03Dis.png";
    stage.pos = Point(500, 550);
    stage.prevIndex.clear();
    stage.prevIndex.push_back(1);
    stage.script = "stage03.lua";
    stage.background = "backgrounds/BackgroundHD01.png";
    m_stages.push_back(stage);

    // stage 4
    stage.name = "Siege";
    stage.desc = "Kael\'thas attempted to summon Kil\'jaeden into Azeroth through the Sunwell using Anveena\'s powers";
    stage.btnNorName = "UI/Stage/Stage04Nor.png";
    stage.btnDisName = "UI/Stage/Stage04Dis.png";
    stage.pos = Point(1100, 200);
    stage.prevIndex.clear();
    stage.prevIndex.push_back(2);
    stage.prevIndex.push_back(3);
    stage.script = "stage04.lua";
    stage.background = "backgrounds/BackgroundHD01.png";
    m_stages.push_back(stage);

    // stage 5
    stage.name = "The End";
    stage.desc = "In World of Warcraft, the Sunwell appears in the level 70 raid dungeon, Sunwell Plateau, on the Isle of Quel\'Danas";
    stage.btnNorName = "UI/Stage/Stage05Nor.png";
    stage.btnDisName = "UI/Stage/Stage05Dis.png";
    stage.pos = Point(1100, 550);
    stage.prevIndex.clear();
    stage.prevIndex.push_back(4);
    stage.script = "stage05.lua";
    stage.background = "backgrounds/BackgroundHD01.png";
    m_stages.push_back(stage);
}

void CGameData::initAbilityData()
{
    enum ABILITY_INDEX
    {
        kCurse,
        kStun
    };
    static auto al = CAbilityLibrary::instance();

    auto b = new CStunBuff("StunBuff", "Stun", 2.0f, true);
    auto id = al->addTemplateAbility(b);
    auto a = new CBuffMakerAct("BMA", "Ⱥ������", 15.0f, CCommandTarget::kNoTarget, CUnitForce::kEnemy, 1.0f, id);
    a->setCastTargetRadius(150);
    a->setImageName("UI/Ability/AbilityCurse.png");
    a->setMaxLevel(3);
    a->setLevel(0);
    a->setDescribe("����һƬ�����������Ӣ���ܵ�20��/����˺�������13�룬ÿ��4��ÿ��ʧ100�������ֵ�ͻ��ܵ�40��Ķ����˺�");
    a->setLevelInfo(1, 0, "ÿ��ʧ100������ʱ���ܵ����˺����40��");
    a->setLevelInfo(2, 1, "ͬʱ���͵�λ15%���ƶ��ٶ�");
    a->setLevelInfo(3, 2, "����Ч������17��");
    a->setGrade(CAbility::kEpic);
    a->setCost(3);
    a->setLevelUpdate(new CLevelUpdate(
        [](CLevelExp* pLevel)
    {
        auto a = DCAST(pLevel, CBuffMakerAct*);
        switch (pLevel->getLevel())
        {
        case 1:
            a->setCost(5);
            break;
        case 2:
            a->setCost(7);
            break;
        case 3:
            a->setCost(9);
            break;
        }
    },
        nullptr, nullptr));

    al->addTemplateAbility(kCurse, a);
    

    b = new CStunBuff("StunBuff", "Stun", 2.0f, true);
    id = al->addTemplateAbility(b);
    a = new CBuffMakerAct("BMA", "��������", 9.0f, CCommandTarget::kNoTarget, CUnitForce::kEnemy, 1.0f, id);
    a->setCastTargetRadius(150);
    a->setImageName("UI/Ability/GravitySurf.png");
    a->setMaxLevel(2);
    a->setLevel(0);
    a->setDescribe("����һ����������");
    a->setLevelInfo(1, 3, "�����뾶����100");
    a->setLevelInfo(2, 2, "������ǣ��ʱ�����ܵ�ÿ��50����˺�");
    a->setGrade(CAbility::kRare);
    a->setCost(1);
    a->setLevelUpdate(new CLevelUpdate(
        [](CLevelExp* pLevel)
    {
        auto a = DCAST(pLevel, CBuffMakerAct*);
        switch (pLevel->getLevel())
        {
        case 1:
            a->setCost(2);
            break;
        case 2:
            a->setCost(4);
            break;
        case 3:
            a->setCost(6);
            break;
        }
    },
        nullptr, nullptr));

    al->addTemplateAbility(kStun, a);
}
