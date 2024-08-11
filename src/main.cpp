#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <GeodeUI.hpp>

using namespace geode::prelude;

namespace github {
    auto mod_api_repo_url = std::string(
        "https://api.github.com/repos/user95401/Ryzen-Mods/issues"
    );
}

auto last_issues = matjson::Value();

class LoadIssuesLayer : public CCLayer {
public:
    EventListener<web::WebTask> m_webTaskListener;
    static auto create(CCContentLayer* contentLayer, std::string modID) {
        auto me = new LoadIssuesLayer();
        me->init();

        me->setAnchorPoint({ 0.0f, 0.0f });
        me->setContentSize(contentLayer->getParent()->getContentSize());
        contentLayer->setContentSize(me->getContentSize());
        contentLayer->addChild(me);

        auto bg = CCScale9Sprite::create(
            "square02_small.png"
        );
        bg->setOpacity(75);
        bg->setContentSize(me->getContentSize());
        me->addChildAtPosition(bg, Anchor::Center);

        auto label = CCLabelBMFont::create(
            "Loading issues...\n(discussion channels)", 
            "chatFont.fnt"
        );
        label->setAlignment(kCCTextAlignmentCenter);
        label->setAnchorPoint({ 0.5f, 1.f });
        me->addChildAtPosition(label, Anchor::Top, {0.f, -72.f});

        auto circle = LoadingCircleSprite::create();
        circle->setScale(0.6f);
        me->addChildAtPosition(circle, Anchor::Top, {0.f, -44.f});

        me->m_webTaskListener.bind(
            [me, label](web::WebTask::Event* e) {
                if (web::WebResponse* res = e->getValue()) {
                    auto json = res->json();
                    auto string = res->string();
                    if (json.has_value()) {
                        last_issues = json.value();
                    }
                    else label->setString(fmt::format(
                        "{}\n{}",
                        label->getString(),
                        json.error_or("no err, str: " + string.value_or("no str"))
                    ).c_str());
                }
                else if (e->isCancelled()) label->setString(fmt::format(
                    "{}\n\n\nThe request was cancelled...",
                    label->getString()
                ).c_str());
            }
        );

        auto req = web::WebRequest();
        me->m_webTaskListener.setFilter(req.get("https://pastebin.com/raw/vNi1WHNF"));

        return me;
    }
};

//here goes mod popup ruinify
void hi() {
    new EventListener<EventFilter<ModPopupUIEvent>>(
        +[](ModPopupUIEvent* event) {

            //get popup
            FLAlertLayer* popup = nullptr;
            if (popup = typeinfo_cast<FLAlertLayer*>(event->getPopup())) void();
            else return ListenerResult::Propagate;

            //fix tabs menu
            CCNode* tabsMenu = nullptr;
            if (tabsMenu = popup->getChildByIDRecursive("tabs-menu")) {
                auto mark = CCComponent::create();
                mark->setName("pointAndSizeFixMark");
                if (tabsMenu->getComponent(mark->getName())) void();
                else {
                    tabsMenu->addComponent(mark);
                    tabsMenu->setAnchorPoint({ 0.f, 1.f });
                    tabsMenu->setPositionX(0.f);
                    tabsMenu->setContentWidth(tabsMenu->getContentWidth() - 21.f);
                };
            }
            else return ListenerResult::Propagate;

            //rcontainer
            auto rcontainer = tabsMenu->getParent();

            //get tab btns
            CCMenuItemSpriteExtra* description = typeinfo_cast<CCMenuItemSpriteExtra*>
                (tabsMenu->getChildByID("description"));
            if (tabsMenu) void();
            else return ListenerResult::Propagate;
            auto description_sprite = typeinfo_cast<GeodeTabSprite*>(description->getNormalImage());
            CCMenuItemSpriteExtra* changelog = typeinfo_cast<CCMenuItemSpriteExtra*>
                (tabsMenu->getChildByID("changelog"));
            if (tabsMenu) void();
            else return ListenerResult::Propagate;
            auto changelog_sprite = typeinfo_cast<GeodeTabSprite*>(changelog->getNormalImage());

            auto myTabID = tabsMenu->getChildrenCount();
            //event->getModID() is aa strange i dnt get that
            auto labelModID = typeinfo_cast<CCLabelBMFont*>(
                popup->getChildByIDRecursive("mod-id-label")
            );
            auto modID = labelModID
                ? string::replace(labelModID->getString(), "(ID: ", "")
                : "nil node!";
            modID = string::replace(modID, ")", "");

            if (tabsMenu->getChildByID("comments")) void();
            else {
                CCSpriteFrameCache::get()->addSpriteFrame(CCSprite::create("chat.png"_spr)->displayFrame(), "chat.png"_spr);
                auto tabspr = GeodeTabSprite::create("chat.png"_spr, "Comments", 140.f);
                tabspr->select(0);

                auto old_sel = description->m_pfnSelector;
                auto old_lis = description->m_pListener;
                auto onTab = [event, old_lis, old_sel, myTabID, modID, popup, rcontainer, description_sprite, changelog_sprite, tabspr]
                (CCMenuItemSpriteExtra* sender) {
                    if (sender->getTag() < 2) {
                        (old_lis->*old_sel)(sender);
                        tabspr->select(0);
                        tabspr->m_label->setString("Comments");
                        auto textarea = rcontainer->getChildByIDRecursive("textarea");
                        textarea->setVisible(1);
                        if (auto comments_scroll = rcontainer->getChildByIDRecursive("comments_scroll"))
                            comments_scroll->removeFromParent();
                    }
                    else if (sender->getTag() == myTabID) {
                        tabspr->m_label->setString("Send New");
                        //toggle out others
                        description_sprite->select(0);
                        changelog_sprite->select(0);
                        //sel this tab
                        tabspr->select(1);

                        if (sender->getParent()->getTag() == myTabID) {

                        }
                        else {
                            auto textarea = rcontainer->getChildByIDRecursive("textarea");
                            textarea->setVisible(0);
                            auto comments_scroll_size = textarea->getParent()->getContentSize();
                            auto comments_scroll = ScrollLayer::create(comments_scroll_size);
                            comments_scroll->setID("comments_scroll");
                            textarea->getParent()->addChild(comments_scroll);
                            LoadIssuesLayer::create(
                                comments_scroll->m_contentLayer, 
                                modID
                            );
                        };

                    }
                    sender->getParent()->setTag(sender->getTag());
                    };

                CCMenuItemExt::assignCallback<CCMenuItemSpriteExtra>(description, onTab);
                CCMenuItemExt::assignCallback<CCMenuItemSpriteExtra>(changelog, onTab);

                auto item = CCMenuItemExt::createSpriteExtra(tabspr, onTab);
                item->m_pListener = description->m_pListener;
                item->setID("comments");
                tabsMenu->addChild(item, 0, myTabID);
            };

            tabsMenu->updateLayout();

            return ListenerResult::Propagate;
        }
    );
}

$execute{ hi(); }