#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/utils/ColorProvider.hpp>
using namespace geode::prelude;

//fucking bruh, export of this stuff when 

class GeodeTabSprite : public CCNode {
public:
//protected:
    CCScale9Sprite* m_deselectedBG;
    CCScale9Sprite* m_selectedBG;
    CCSprite* m_icon;
    CCLabelBMFont* m_label;

    bool init(const char* iconFrame, const char* text, float width, bool altColor) {
        if (!CCNode::init())
            return false;

        const CCSize itemSize{ width, 35 };
        const CCSize iconSize{ 18, 18 };

        this->setContentSize(itemSize);
        this->setAnchorPoint({ .5f, .5f });

        m_deselectedBG = CCScale9Sprite::createWithSpriteFrameName("geode.loader/tab-bg.png");
        m_deselectedBG->setScale(.8f);
        m_deselectedBG->setContentSize(itemSize / .8f);
        m_deselectedBG->setColor(ColorProvider::get()->color3b("geode.loader/mod-list-tab-deselected-bg"));
        this->addChildAtPosition(m_deselectedBG, Anchor::Center);

        m_selectedBG = CCScale9Sprite::createWithSpriteFrameName("geode.loader/tab-bg.png");
        m_selectedBG->setScale(.8f);
        m_selectedBG->setContentSize(itemSize / .8f);
        m_selectedBG->setColor(to3B(ColorProvider::get()->color(
            altColor ?
            "geode.loader/mod-list-tab-selected-bg-alt" :
            "geode.loader/mod-list-tab-selected-bg"
        )));
        this->addChildAtPosition(m_selectedBG, Anchor::Center);

        m_icon = CCSprite::createWithSpriteFrameName(iconFrame);
        limitNodeSize(m_icon, iconSize, 3.f, .1f);
        this->addChildAtPosition(m_icon, Anchor::Left, ccp(16, 0), false);

        m_label = CCLabelBMFont::create(text, "bigFont.fnt");
        m_label->limitLabelWidth(this->getContentWidth() - 45, clamp(width * .0045f, .35f, .55f), .1f);
        m_label->setAnchorPoint({ .5f, .5f });
        this->addChildAtPosition(m_label, Anchor::Left, ccp((itemSize.width - iconSize.width) / 2 + iconSize.width, 0), false);

        return true;
    };

//public:
    static GeodeTabSprite* create(const char* iconFrame, const char* text, float width, bool altColor = false) {
        auto ret = new GeodeTabSprite();
        if (ret->init(iconFrame, text, width, altColor)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    };
    void select(bool selected) {
        m_deselectedBG->setVisible(!selected);
        m_selectedBG->setVisible(selected);
    };
    void disable(bool disabled) {
        auto color = disabled ? ccc3(95, 95, 95) : ccc3(255, 255, 255);
        m_deselectedBG->setColor(color);
        m_selectedBG->setColor(color);
        m_icon->setColor(color);
        m_label->setColor(color);
    };
};