#include "CommHeader.h"

#include "AbilityScene.h"
#include "GameControl.h"
#include "ComponentForCC.h"
#include "Ability.h"


// AbilitySceneLayer
AbilitySceneLayer::AbilitySceneLayer()
{
}

AbilitySceneLayer::~AbilitySceneLayer()
{
}

Scene* AbilitySceneLayer::scene()
{
    // 'scene' is an autorelease object
    Scene* pScene = Scene::create();

    AbilitySceneLayer* layer = AbilitySceneLayer::create();

    // add layer as a child to scene
    if (layer != NULL)
    {
        M_DEF_GC(gc);
        //gc->preloadSound("sounds/Effect/xxxxxxx.mp3");

        pScene->addChild(layer);
    }

    // return the scene
    return pScene;
}

class PopAbilityDetails : public Scale9Sprite
{
public:
    bool initWithAbility(CAbility* ability);
    M_CREATE_INITWITH_FUNC_PARAM(Ability, PopAbilityDetails, (CAbility* ability), ability);

    void updateContent(CAbility* ability);

CC_CONSTRUCTOR_ACCESS:
    PopAbilityDetails();

protected:
    Sprite* starByIndex(int index, bool on);
    Sprite* iconByType(int index, int type);

protected:
    Sprite* m_aIcon;  // ability icon
    Label* m_aNameBg;
    Label* m_aNameFg;
    Sprite* m_aStars[3];
    Label* m_aTypeCost;
    Label* m_aDesc;
    Sprite* m_aUpTitle;
    Label* m_aLevel;
    Sprite* m_aUpIcon[3];
    Label* m_aUpDesc[3];
    Sprite* m_aUpgraded[3];
};


// PopAbilityDetails
PopAbilityDetails::PopAbilityDetails()
: m_aIcon(nullptr)
, m_aNameBg(nullptr)
, m_aNameFg(nullptr)
, m_aTypeCost(nullptr)
, m_aDesc(nullptr)
, m_aUpTitle(nullptr)
, m_aLevel(nullptr)
{
    memset(m_aStars, 0, sizeof(m_aStars));
    memset(m_aUpIcon, 0, sizeof(m_aUpIcon));
    memset(m_aUpDesc, 0, sizeof(m_aUpDesc));
    memset(m_aUpgraded, 0, sizeof(m_aUpgraded));
}

bool PopAbilityDetails::initWithAbility(CAbility* ability)
{
    if (Scale9Sprite::initWithFile("UI/PopBorder.png") == false)
    {
        return false;
    }

    setInsetLeft(24);
    setInsetRight(24);
    setInsetTop(24);
    setInsetBottom(24);
    setContentSize(Size(620, 763));

    updateContent(ability);

    return true;
}

void PopAbilityDetails::updateContent(CAbility* ability)
{
    static auto fc = SpriteFrameCache::getInstance();
    static auto tc = Director::getInstance()->getTextureCache();

    char str[1024];  // buffer for sprintf
    char sz[1024];  //  buffer for gbk2utf8

    // icon
    if (m_aIcon == nullptr)
    {
        m_aIcon = Sprite::create(ability->getImageName());
        addChild(m_aIcon);
        m_aIcon->setPosition(Point(m_aIcon->getContentSize().width * 0.5 + 30, getContentSize().height - m_aIcon->getContentSize().height * 0.5 - 30));
    }
    else
    {
        auto tx = Director::getInstance()->getTextureCache()->addImage(ability->getImageName());
        auto sz = tx->getContentSize();
        m_aIcon->setSpriteFrame(SpriteFrame::createWithTexture(tx, Rect(0.0f, 0.0f, sz.width, sz.height)));
    }


    // name
    gbk_to_utf8(ability->getName(), sz);
    if (m_aNameBg == nullptr)
    {
        m_aNameBg = Label::createWithTTF(sz, "fonts/DFYuanW7-GB2312.ttf", 48);
        addChild(m_aNameBg, 1);
        m_aNameBg->setColor(Color3B::BLACK);
        m_aNameBg->setPosition(m_aIcon->getPosition() + Point(182, 20));
    }
    else
    {
        m_aNameBg->setString(sz);
    }
    
    if (m_aNameFg == nullptr)
    {
        m_aNameFg = Label::createWithTTF(m_aNameBg->getTTFConfig(), m_aNameBg->getString());
        addChild(m_aNameFg, 2);
        m_aNameFg->setPosition(m_aNameBg->getPosition() + Point(-3, 3));
    }
    else
    {
        m_aNameFg->setString(m_aNameBg->getString());
    }
    m_aNameFg->setColor(AbilityItem::abilityGradeColor3B(ability->getGrade()));

    
    // star
    const Point aistarCenter = m_aIcon->getPosition() + Point(182, -32);
    const float aistarBetween = 5;
    
    auto star = starByIndex(0, ability->getLevel() > 0);
    float starBaseWidth = aistarCenter.x - (ability->getMaxLevel() - 1) * star->getContentSize().width * 0.5 - (ability->getMaxLevel() - 1) * aistarBetween * 0.5;
    
    star->setPosition(Point(starBaseWidth, aistarCenter.y));
    
    for (auto i = 1; i < 3; ++i)
    {
        if (i < ability->getMaxLevel())
        {
            star = starByIndex(i, ability->getLevel() > i);
            star->setVisible(true);
            star->setPosition(Point(starBaseWidth + i * (star->getContentSize().width + aistarBetween), aistarCenter.y));
        }
        else if (m_aStars[i] != nullptr)
        {
            m_aStars[i]->setVisible(false);
        }
        
    }

    // ability type & cost
    const char* type;
    if (DCAST(ability, CActiveAbility*) != nullptr)
    {
        type = "����";
    }
    else if (DCAST(ability, CPassiveAbility*) != nullptr)
    {
        type = "����";
    }
    else
    {
        type = "δ֪";
    }

    if (ability->getCoolDown() > 0.0f)
    {
        sprintf(str, "%s(%d��)\n���辫��: %d", type, toInt(ability->getCoolDown()), ability->getCost());
    }
    else
    {
        sprintf(str, "%s\n���辫��: %d", type, ability->getCost());
    }
    
    gbk_to_utf8(str, sz);
    if (m_aTypeCost == nullptr)
    {
        m_aTypeCost = Label::createWithTTF(sz, "fonts/DFYuanW7-GB2312.ttf", 28);
        m_aTypeCost->setAnchorPoint(Point(0.0f, 0.5f));
        addChild(m_aTypeCost);
        m_aTypeCost->setColor(Color3B(132, 142, 255));
        m_aTypeCost->setPosition(m_aIcon->getPosition() + Point(295, 0.0f));
    }
    else
    {
        m_aTypeCost->setString(sz);
    }


    // describe
    gbk_to_utf8(ability->getDescribe(), sz);
    if (m_aDesc == nullptr)
    {
        m_aDesc = Label::createWithTTF(sz, "fonts/DFYuanW7-GB2312.ttf", 28, Size(520, 160));
        m_aDesc->setAnchorPoint(Point(0.0f, 1.0f));
        addChild(m_aDesc);
        m_aDesc->setPosition(m_aIcon->getPosition() + Point(-55, -100));
    }
    else
    {
        m_aDesc->setString(sz);
    }
    
    
    // level up
    float fBaseHeightDelta = -350;
    if (m_aUpTitle == nullptr)
    {
        m_aUpTitle = Sprite::create("UI/Ability/AbilityLevel.png");
        addChild(m_aUpTitle);
        m_aUpTitle->setPosition(Point(getContentSize().width * 0.5, m_aIcon->getPosition().y + fBaseHeightDelta + 50));
    }

    sprintf(str, "%d/%d", ability->getLevel(), ability->getMaxLevel());
    gbk_to_utf8(str, sz);
    if (m_aLevel == nullptr)
    {
        m_aLevel = Label::createWithTTF(sz, "fonts/DFYuanW7-GB2312.ttf", 28);
        m_aLevel->setAnchorPoint(Point(0.0f, 0.5f));
        addChild(m_aLevel);
        m_aLevel->setPosition(m_aUpTitle->getPosition() + Point(50, 0.0f));
    }
    else
    {
        m_aLevel->setString(sz);
    }

    for (auto i = 0; i < 3; ++i)
    {
        if (i < ability->getMaxLevel())
        {
            auto upIcon = iconByType(i, ability->getLevelType(i + 1));
            upIcon->setPosition(m_aIcon->getPosition() + Point(-27, fBaseHeightDelta - 95 * i - 28));
            upIcon->setVisible(true);

            gbk_to_utf8(ability->getLevelDescribe(i + 1), sz);
            if (m_aUpDesc[i] == nullptr)
            {
                m_aUpDesc[i] = Label::createWithTTF(sz, "fonts/DFYuanW7-GB2312.ttf", 28, Size(300, 80));
                m_aUpDesc[i]->setAnchorPoint(Point(0.0f, 1.0f));
                addChild(m_aUpDesc[i]);
                m_aUpDesc[i]->setPosition(m_aIcon->getPosition() + Point(20, fBaseHeightDelta - 95 * i));
            }
            else
            {
                m_aUpDesc[i]->setString(sz);
                m_aUpDesc[i]->setVisible(true);
            }
            //tmp = "ÿ��ʧ100������ʱ���ܵ����˺����40��";
            
        }
        else
        {
            if (m_aUpIcon[i] != nullptr)
            {
                m_aUpIcon[i]->setVisible(false);
            }

            if (m_aUpDesc[i] != nullptr)
            {
                m_aUpDesc[i]->setVisible(false);
            }
        }

        if (i < ability->getLevel())
        {
            if (m_aUpgraded[i] == nullptr)
            {
                m_aUpgraded[i] = Sprite::create("UI/Ability/AbilityUpgraded.png");
                addChild(m_aUpgraded[i]);
                m_aUpgraded[i]->setPosition(m_aUpIcon[i]->getPosition() + Point(439, 0));
            }
            else
            {
                m_aUpgraded[i]->setVisible(true);
            }
        }
        else
        {
            if (m_aUpgraded[i] != nullptr)
            {
                m_aUpgraded[i]->setVisible(false);
            }
        }
    }

    //Utils::nodeToFile(this, "AbilityPopPanel.png");
}

Sprite* PopAbilityDetails::starByIndex(int index, bool on)
{
    static auto fc = SpriteFrameCache::getInstance();
    static auto tc = Director::getInstance()->getTextureCache();

    auto file = on ? "UI/Ability/AbilityItemStar.png" : "UI/Ability/AbilityItemUnstar.png";
    if (m_aStars[index] == nullptr)
    {
        m_aStars[index] = Sprite::create(file);
        addChild(m_aStars[index], 1);
    }
    else
    {
        auto tx = Director::getInstance()->getTextureCache()->addImage(file);
        auto sz = tx->getContentSize();
        m_aStars[index]->setSpriteFrame(SpriteFrame::createWithTexture(tx, Rect(0.0f, 0.0f, sz.width, sz.height)));
    }

    return m_aStars[index];
}

Sprite* PopAbilityDetails::iconByType(int index, int type)
{
    static auto fc = SpriteFrameCache::getInstance();
    static auto tc = Director::getInstance()->getTextureCache();

    auto file = "UI/Ability/AbilityLevel1.png";
    switch (type)
    {
    case 0:
        file = "UI/Ability/AbilityLevel1.png";
        break;

    case 1:
        file = "UI/Ability/AbilityLevel2.png";
        break;

    case 2:
        file = "UI/Ability/AbilityLevel3.png";
        break;

    }

    if (m_aUpIcon[index] == nullptr)
    {
        m_aUpIcon[index] = Sprite::create(file);
        addChild(m_aUpIcon[index]);
    }
    else
    {
        auto tx = Director::getInstance()->getTextureCache()->addImage(file);
        auto sz = tx->getContentSize();
        m_aUpIcon[index]->setSpriteFrame(SpriteFrame::createWithTexture(tx, Rect(0.0f, 0.0f, sz.width, sz.height)));
    }

    return m_aUpIcon[index];
}

// on "init" you need to initialize your instance
bool AbilitySceneLayer::init()
{
    //////////////////////////////
    // 1. super init first
    if (!LayerColor::initWithColor(Color4B(0, 0, 0, 0)))
    {
        return false;
    }
    
    static Size wsz = Director::getInstance()->getVisibleSize();
    M_DEF_GC(gc);

    gc->loadFrames("Global0");
    gc->loadFrames("Global1");

    auto a = new CBuffMakerAct("BMA", "����һ��", 5.0f, CCommandTarget::kNoTarget, CUnitForce::kEnemy, 1.0f, 0);
    a->setImageName("UI/Ability/ThunderCap.png");
    a->setMaxLevel(3);
    a->setLevel(3);
    a->setGrade(CAbility::kEpic);
    a->setCost(9);
    auto ai = AbilityItem::create(a);

    auto bp = WinFormPanel::create(6, 2, 3, 2, ai->getContentSize(), 10, 10, 0.0f, 20.0f);
    bp->setColor(Color3B::GRAY);
    bp->setOpacity(128);
    addChild(bp);
    bp->setBufferEffectParam(1.0f, 0.9f, 20.0f, 0.1f);

    bp->addNodeEx(ai, WinFormPanel::kTopToBottom);

    // �ο�
    auto sp = Scale9Sprite::create("UI/ScaleBorder2_109x69_45x157.png");
    sp->setInsetTop(69);
    sp->setInsetBottom(69);
    sp->setInsetLeft(45);
    sp->setInsetRight(45);
    sp->setContentSize(bp->getWinSize() + Size(52, 52));

    addChild(sp, 1);
    sp->setPosition(Point(wsz.width * 0.5f, wsz.height - sp->getContentSize().height * 0.5f));
    bp->setWinPosition(sp->getPosition() - Point(bp->getWinSize().width * 0.5, bp->getWinSize().height * 0.5));
    bp->setOffsetPosition(Point(0, -9999));

    // Clipping Node
    auto sn = LayerColor::create(Color4B(0, 0, 0, 255), sp->getContentSize().width, sp->getContentSize().height);
    sn->ignoreAnchorPointForPosition(false);
    sn->setPosition(sp->getPosition());
    sn->setScaleX((sp->getContentSize().width - 10) / sp->getContentSize().width);
    sn->setScaleY((sp->getContentSize().height - 10) / sp->getContentSize().height);

    auto cn = ClippingNode::create(sn);
    cn->setContentSize(getContentSize());
    cn->setInverted(true);

    auto bg = Sprite::create("backgrounds/MapBackground.png");
    cn->addChild(bg, 10);
    bg->setPosition(getAnchorPointInPoints());

    addChild(cn);

    a = new CBuffMakerAct("BMA", "��������", 5.0f, CCommandTarget::kNoTarget, CUnitForce::kEnemy, 1.0f, 0);
    a->setImageName("UI/Ability/GravitySurf.png");
    a->setMaxLevel(2);
    a->setLevel(1);
    a->setGrade(CAbility::kRare);
    a->setCost(2);
    ai = AbilityItem::create(a);
    bp->addNodeEx(ai, WinFormPanel::kTopToBottom);


    a = new CBuffMakerAct("BMA", "Ⱥ������", 5.0f, CCommandTarget::kNoTarget, CUnitForce::kEnemy, 1.0f, 0);
    a->setImageName("UI/Ability/AbilityCurse.png");
    a->setMaxLevel(3);
    a->setLevel(0);
    a->setGrade(CAbility::kNormal);
    a->setCost(1);
    ai = AbilityItem::create(a);
    bp->addNodeEx(ai, WinFormPanel::kTopToBottom);

    a->setGrade((CAbility::GRADE)(rand() % 4));
    ai = AbilityItem::create(a->copy());
    bp->addNodeEx(ai, WinFormPanel::kTopToBottom);

    a->setGrade((CAbility::GRADE)(rand() % 4));
    ai = AbilityItem::create(a->copy());
    bp->addNodeEx(ai, WinFormPanel::kTopToBottom);

    a->setGrade((CAbility::GRADE)(rand() % 4));
    ai = AbilityItem::create(a->copy());
    bp->addNodeEx(ai, WinFormPanel::kTopToBottom);

    a->setGrade((CAbility::GRADE)(rand() % 4));
    ai = AbilityItem::create(a->copy());
    bp->addNodeEx(ai, WinFormPanel::kTopToBottom);


    a->setImageName("UI/Ability/AbilityCurse.png");
    a->setGrade(CAbility::kLegend);
    a->setCost(6);
    a->setDescribe("����һƬ�����������Ӣ���ܵ�20��/����˺�������13�룬ÿ��4��ÿ��ʧ100�������ֵ�ͻ��ܵ�40��Ķ����˺�");
    a->setMaxLevel(3);
    a->setLevelInfo(1, 0, "ÿ��ʧ100������ʱ���ܵ����˺����40��");
    a->setLevelInfo(2, 1, "ͬʱ���͵�λ15%���ƶ��ٶ�");
    a->setLevelInfo(3, 2, "����Ч������17��");
    a->setLevel(2);

    a->retain();

    bp->setActionCallback([this, a](int action)
    {
        if (action != WinLayer::kClickPoint)
        {
            return;
        }
        auto pop = PopAbilityDetails::create(a);
        this->addChild(pop, 3);
        pop->setPosition(getAnchorPointInPoints());
        pop->setCascadeOpacityEnabled(true);
        pop->setOpacity(0);

        pop->runAction(FadeIn::create(0.5f));
    });

    

    return true;
}


