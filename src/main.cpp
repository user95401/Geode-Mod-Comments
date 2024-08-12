#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <GeodeUI.hpp>

using namespace geode::prelude;

#define SETTING(type, key_name) Mod::get()->getSettingValue<type>(key_name)

#define public_cast(value, member) \
[](auto* v) { \
	class FriendClass__; \
	using T = std::remove_pointer<decltype(v)>::type; \
	class FriendeeClass__: public T { \
	protected: \
		friend FriendClass__; \
	}; \
	class FriendClass__ { \
	public: \
		auto& get(FriendeeClass__* v) { return v->member; } \
	} c; \
	return c.get(reinterpret_cast<FriendeeClass__*>(v)); \
}(value)

namespace github {
    inline auto api_repo_url = std::string(
        "https://api.github.com/repos/user95401/Geode-Mod-Comments"
    );
    inline std::string get_token() {
        return Mod::get()->getSavedValue<std::string>("gh_access_token");
    }
    inline void set_token(std::string token) {
        Mod::get()->setSavedValue("gh_access_token", token);
    }
    inline bool has_token() {
        return (get_token().size() > 3);
    }
    inline const char* authorization_header_name() {
        return has_token() ? "Authorization" : "AuthDisabled";
    }
    inline auto get_basic_web_request() {
        auto req = web::WebRequest();
        req.userAgent(Mod::get()->getID());
        req.header("X-GitHub-Api-Version", "2022-11-28");
        req.header(authorization_header_name(), fmt::format("Bearer {}", get_token()));
        return req;
    }
}

inline auto issues = matjson::Value();
inline std::map<std::string, matjson::Value> mod_issues;
inline std::map<std::string, matjson::Value> mod_comments;

class IssueCommentItem : public CCMenuItem {
public:
    matjson::Value m_json;
    static auto create(CCNode* parent, matjson::Value json) {
        auto rtn = new IssueCommentItem();
        rtn->m_json = json;
        rtn->init();
        rtn->customSetup(parent);
        return rtn;
    }
    void customSetup(CCNode* parent) {
        this->setContentWidth(parent->getContentWidth());
        //row
        if (auto row = CCMenu::create()) {
            this->addChild(row);
            row->setAnchorPoint(CCPointZero);
            row->setLayout(
                RowLayout::create()
                ->setAxisAlignment(AxisAlignment::Start)
                ->setCrossAxisLineAlignment(AxisAlignment::End)
            );
            row->setContentWidth(parent->getContentWidth());
            //avatar
            auto avatar_size = CCSize(30.f, 30.f);
            {
                //sprite
                auto sprite = CCSprite::createWithSpriteFrameName("edit_eDamageSquare_001.png");
                sprite->setScale(avatar_size.width / sprite->getContentSize().width);
                //item
                auto avatar = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(IssueCommentItem::onAvatar));
                avatar->m_animationEnabled = 0;
                avatar->m_colorEnabled = 1;
                avatar->setContentSize(avatar_size);
                row->addChild(avatar);
                //
                auto filep = dirs::getTempDir() / ("." + m_json["user"]["login"].as_string());
                auto a = [this, sprite, filep, avatar](std::monostate const& asd) {
                    if (not sprite) return;
                    sprite->initWithFile(filep.string().c_str());
                    sprite->setScale(avatar->getContentWidth() / sprite->getContentSize().width);
                    };
                auto req = web::WebRequest();
                auto listener = new EventListener<web::WebTask>;
                listener->bind(
                    [this, a, filep](web::WebTask::Event* e) {
                        if (web::WebResponse* res = e->getValue()) {
                            std::string data = res->string().unwrapOr("");
                            //call the some shit
                            if (res->code() < 399) {
                                res->into(filep);
                                a(std::monostate());
                            }
                        }
                    }
                );
                listener->setFilter(req.send("GET", m_json["user"]["avatar_url"].as_string()));
                /* web::AsyncWebRequest().fetch(m_json["user"]["avatar_url"].as_string())
                        .into(filep).then(a).expect(b);*/
            }
            //text
            if (auto text = CCNode::create()) {
                row->addChild(text);
                text->setLayout(
                    ColumnLayout::create()
                    ->setCrossAxisLineAlignment(AxisAlignment::Start)
                    ->setAxisReverse(true)
                    ->setGap(0.f)
                );
                text->setContentWidth(parent->getContentWidth() - avatar_size.width);
                //user
                auto updated_at = m_json["updated_at"].as_string();
                updated_at = string::replace(updated_at, "T", " ");
                updated_at = string::replace(updated_at, "Z", "");
                updated_at = string::replace(updated_at, "-", ".");
                auto user = CCLabelTTF::create(
                    fmt::format(
                        "{} at {}",
                        m_json["user"]["login"].as_string(),
                        updated_at
                    ).c_str(),
                    "arial", 12.f
                );
                user->setID("user");
                text->addChild(user);
                //menu in end of user text
                if (auto menu = CCMenu::create()) {
                    menu->setPosition(user->getContentSize());
                    menu->setContentHeight(user->getContentHeight());
                    menu->setAnchorPoint({ -0.01f, 1.f });
                    menu->setLayout(
                        RowLayout::create()
                        ->setAxisAlignment(AxisAlignment::Start)
                        ->setAutoScale(false)
                        ->setCrossAxisOverflow(false)
                    );
                    user->addChild(menu);
                    //delete_btn
                    auto edit_delBtn_001 = CCSprite::createWithSpriteFrameName("edit_delBtn_001.png");
                    edit_delBtn_001->setScale(0.5f);
                    auto delete_btn = CCMenuItemSpriteExtra::create(
                        edit_delBtn_001,
                        this,
                        menu_selector(IssueCommentItem::deleteComment)
                    );
                    menu->addChild(delete_btn);
                    //menu update
                    menu->updateLayout();
                }
                //body
                auto body = public_cast(
                    MDTextArea::create(m_json["body"].as_string(), { text->getContentWidth(), 10 }),
                    m_content
                );
                body->setVisible(1);
                auto body_container = CCNode::create();
                body_container->addChild(body);
                body_container->setLayout(ColumnLayout::create());
                body_container->setContentHeight(body->getContentHeight());
                body_container->updateLayout();
                text->addChild(body_container);
                //set col height
                text->setContentHeight(user->getContentHeight() + body_container->getContentHeight());
                text->updateLayout();
            }
            //upd
            row->updateLayout();
        }
        this->setLayout(RowLayout::create());
    }
    //IssueCommentItem
    void deleteComment(CCObject*) {
        auto a = [this](std::string const& rtn)
            {
                if (auto reload = dynamic_cast<CCMenuItemSpriteExtra*>(CCDirector::get()->m_pRunningScene->getChildByIDRecursive("reload")))
                    reload->activate();
                //asd
                if (this) {
                    auto parent = this->getParent();
                    this->removeFromParentAndCleanup(false);
                    if (parent) parent->updateLayout();
                };
            };
        auto b = [this](std::string const& rtn)
            {
                auto message = rtn;
                auto asd = geode::createQuickPopup(
                    "Request exception",
                    message,
                    "Nah", nullptr, 420.f, nullptr, false
                );
                asd->show();
            };
        auto req = github::get_basic_web_request();
        auto listener = new EventListener<web::WebTask>;
        listener->bind(
            [this, a, b](web::WebTask::Event* e) {
                if (web::WebResponse* res = e->getValue()) {
                    std::string data = res->string().unwrapOr("");
                    //call the some shit
                    if (res->code() < 399) a(data);
                    else b(data);
                }
            }
        );
        listener->setFilter(req.send("DELETE", m_json["url"].as_string()));
    }
    void onAvatar(CCObject*) {
        if (not m_json.contains("user")) return;
        if (not m_json["user"].contains("html_url")) return;
        else web::openLinkInBrowser(m_json["user"]["html_url"].as_string());
    }
    void onCreateReaction(CCObject*) {
    }
};

class CommentsLayer : public CCLayer {
public:
    static auto create(CCContentLayer* contentLayer, std::string modID) {

        auto scroll = typeinfo_cast<ScrollLayer*>(contentLayer->getParent());

        float scroll_gap = 12.f;
        
        contentLayer->setAnchorPoint(CCPointZero);
        contentLayer->setPositionX(0);
        contentLayer->setLayout(
            ColumnLayout::create()
            ->setGap(scroll_gap)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
        );

        contentLayer->setContentHeight(0.f);
        if (mod_comments[modID].is_array()) for (auto comment : mod_comments[modID].as_array()) {
            auto item = IssueCommentItem::create(contentLayer, comment);
            contentLayer->setContentHeight(//make content layer longer
                contentLayer->getContentHeight() + 
                item->getContentHeight() + scroll_gap
            );
            contentLayer->addChild(item);
        }
        //fix some shit goes when content smaller than scroll
        if (contentLayer->getContentSize().height < scroll->getContentSize().height) {
            contentLayer->setContentSize({
                contentLayer->getContentSize().width,
                scroll->getContentSize().height
                });
        }
        contentLayer->updateLayout();

    }
};

class LoadCommentsLayer : public CCLayer {
public:
    EventListener<web::WebTask> m_webTaskListener;
    static auto create(CCContentLayer* contentLayer, std::string modID) {
        auto me = new LoadCommentsLayer();
        me->init();

        me->setAnchorPoint({ 0.0f, 0.0f });
        me->setContentSize(contentLayer->getParent()->getContentSize());
        contentLayer->setContentSize(me->getContentSize());
        contentLayer->addChild(me);

        if (issues.is_array()) for (auto issue : issues.as_array()) {
            auto title = issue.try_get<std::string>("title").value_or("NO_TITLE");
            auto number = issue.try_get<int>("number").value_or(1);
            if (title == modID) {
                mod_issues[modID] = issue;
                break;
            }
            else if (number == 1) mod_issues[modID] = issue;
        }

        if (mod_issues.contains(modID)) void();
        else {
            auto label = CCLabelBMFont::create(fmt::format(
                "Can't find any issue for {}!",
                modID
            ).data(), "chatFont.fnt"
            );
            label->setAlignment(kCCTextAlignmentCenter);
            label->setAnchorPoint({ 0.5f, 1.f });
            me->addChildAtPosition(label, Anchor::Top, { 0.f, -72.f });
        }

        auto bg = CCScale9Sprite::create(
            "square02_small.png"
        );
        bg->setOpacity(75);
        bg->setContentSize(me->getContentSize());
        me->addChildAtPosition(bg, Anchor::Center);

        auto label = CCLabelBMFont::create(fmt::format(
            "Loading comments for {}...", 
            modID
            ).data(), "chatFont.fnt"
        );
        label->setAlignment(kCCTextAlignmentCenter);
        label->setAnchorPoint({ 0.5f, 1.f });
        me->addChildAtPosition(label, Anchor::Top, {0.f, -72.f});

        auto circle = LoadingCircleSprite::create();
        circle->setScale(0.6f);
        me->addChildAtPosition(circle, Anchor::Top, {0.f, -44.f});

        me->m_webTaskListener.bind(
            [contentLayer, modID, me, label](web::WebTask::Event* e) {
                if (web::WebResponse* res = e->getValue()) {
                    auto json = res->json();
                    auto string = res->string();
                    if (json.has_value()) {
                        mod_comments[modID] = json.value();
                        me->removeFromParent();
                        CommentsLayer::create(contentLayer, modID);
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

        auto req = github::get_basic_web_request();
        me->m_webTaskListener.setFilter(req.get(
            mod_issues[modID].try_get<std::string>("comments_url").value_or("")
        ));

        return me;
    }
};

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
            [contentLayer, modID, me, label](web::WebTask::Event* e) {
                if (web::WebResponse* res = e->getValue()) {
                    auto json = res->json();
                    auto string = res->string();
                    if (json.has_value()) {
                        issues = json.value();
                        me->removeFromParent();
                        LoadCommentsLayer::create(contentLayer, modID);
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

        auto req = github::get_basic_web_request();
        me->m_webTaskListener.setFilter(req.get(
            github::api_repo_url + "/issues"
        ));

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
                            auto pop = FLAlertLayer::create(
                                nullptr,
                                "Create Comment",
                                "\n \n \n \n \n ",
                                "Close", "Create",
                                380.f
                            );
                            //md_prev
                            auto md_prev = MDTextArea::create("# input and preview here...\nJust tap on me and type your text!\n\nAlso you can type <co>`\\n`</c> and it will be replaced with newline char automatically. And remember, in **Markdown** you write two newlines to start new paragraph!", CCSize(290.f, 112.f));
                            md_prev->setID("md_prev");
                            md_prev->setPositionY(86.f);
                            pop->m_buttonMenu->addChild(md_prev, 1);
                            //input
                            auto input = TextInput::create(md_prev->getContentWidth(), "", "chatFont.fnt");
                            input->setID("input");
                            input->hideBG();
                            input->setContentHeight(md_prev->getContentHeight() * 2);
                            input->getInputNode()->setContentHeight(md_prev->getContentHeight() * 2);
                            input->getInputNode()->m_placeholderLabel->setOpacity(0);
                            input->getInputNode()->m_cursor->setOpacity(0);
                            input->getInputNode()->m_filterSwearWords = (0);
                            input->getInputNode()->m_allowedChars = (
                                " !\"#$ % &'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
                                );
                            input->setCallback(
                                [input, md_prev](std::string p0) {
                                    auto endl_filtered = string::replace(p0, "\\n", "\n");
                                    if (p0.find("\\n") != std::string::npos) {
                                        input->setString(endl_filtered.data());
                                    }
                                    //p0->setString();
                                    //log::debug("{}(text:{}, m_fontValue1={})", __FUNCTION__, p0->getString(), p0->m_fontValue2);
                                    md_prev->setString(endl_filtered.data());
                                    md_prev->getScrollLayer()->scrollLayer(9999.f);
                                }
                            );
                            input->setPosition(md_prev->getPosition() * 2);
                            pop->m_buttonMenu->addChild(input);
                            //last popup setup
                            handleTouchPriority(pop);
                            pop->setID("post_comment_popup");
                            pop->show();
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