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
        Mod::get()->saveData();
    }
    inline bool has_token() {
        return (get_token().size() > 3);
    }
    inline auto get_basic_web_request() {
        auto req = web::WebRequest();
        req.userAgent(Mod::get()->getID());
        req.header("X-GitHub-Api-Version", "2022-11-28");
        if(has_token()) req.header("Authorization", fmt::format("Bearer {}", get_token()));
        return req;
    }
}
class GitHubAuthPopup : public FLAlertLayer, FLAlertLayerProtocol {
public:
    virtual void FLAlert_Clicked(FLAlertLayer* p0, bool p1) {
        auto protocol = new GitHubAuthPopup;
        //info"Continue"
        if (p0->getID() == "info" and p1) {
            //open auth apps idk
            web::openLinkInBrowser("https://user95401.7m.pl/geode-mod-comments/auth");
            //finish pop
            auto pop = FLAlertLayer::create(
                protocol,
                "Authorization",
                "Put code from gray page here:" "\n \n \n",
                "Back", "Finish",
                360.f
            );
            //input
            auto input = TextInput::create(280.f, "the code");
            input->setID("input");
            input->setPositionY(42.f);
            pop->m_buttonMenu->addChild(input);
            //paste
            auto paste = CCMenuItemSpriteExtra::create(
                CCLabelBMFont::create(
                    "paste\ntext",
                    "chatFont.fnt"
                ),
                pop,
                menu_selector(GitHubAuthPopup::onPasteToInput)
            );
            paste->setPositionY(90.f);
            paste->setPositionX(-140.f);
            pop->m_buttonMenu->addChild(paste);
            //last popup setup
            pop->setID("finish");
            pop->show();
        };
        //finish"Finish"
        if (p0->getID() == "finish" and p1) {
            //code
            auto code = std::string("");
            auto input = dynamic_cast<TextInput*>(p0->getChildByIDRecursive("input"));
            if (input) code = input->getString();
            //
            auto a = [this, protocol](matjson::Value const& catgirl) {
                if (not catgirl.contains("access_token")) {
                    auto asd = geode::createQuickPopup(
                        "Failed getting token",
                        catgirl.dump(),
                        "Nah", nullptr, 420.f, nullptr, false
                    );
                    asd->show();
                    return;
                }
                github::set_token(catgirl["access_token"].as_string());
                if (auto githubitem = typeinfo_cast<CCMenuItemSpriteExtra*>(
                    CCScene::get()->getChildByIDRecursive("githubitem"))) {
                    githubitem->setOpacity(github::has_token() ? 120 : 255);
                };
                auto asd = geode::createQuickPopup(
                    "Access token saved!",
                    "Now u have no limits and able to create comments...",
                    "OK", nullptr,
                    nullptr
                );
                };
            auto b = [this](std::string const& error)
                {// something went wrong with our web request Q~Q
                    auto message = error;
                    auto asd = geode::createQuickPopup(
                        "Request exception",
                        message,
                        "Nah", nullptr, 420.f, nullptr, false
                    );
                    asd->show();
                };
            auto req = web::WebRequest();
            auto listener = new EventListener<web::WebTask>;
            listener->bind(
                [this, a, b](web::WebTask::Event* e) {
                    if (web::WebResponse* res = e->getValue()) {
                        std::string data = res->string().unwrapOr("");
                        //json
                        std::string error;
                        auto json_val = matjson::parse(data, error);
                        if (error.size() > 0) return b("Error parsing JSON: " + error);
                        //call the some shit
                        if (res->code() < 399) a(json_val);
                        else b(data);
                    }
                }
            );
            req.header("Accept", "application/json");
            req.bodyString(
                fmt::format("code={}", code) +
                "&" "client_id=Ov23lisVe58mXL4UNOsE"
                "&" "client_secret=9a2f156940cd3464a07d941a38160fd6bdf3dd78"
                //"&" ""
            );
            listener->setFilter(req.send("POST", "https://github.com/login/oauth/access_token"));
        }
        //back"Back"
        if (p0->getID() == "finish" and not p1) {
            show_info();
        }
    };
    static void show_info() {
        auto protocol = new GitHubAuthPopup;
        auto pop = FLAlertLayer::create(
            protocol,
            "Authorization",
            "<co>Authorize</c> your <cy>GitHub Account</c> to <cg>reduce</c> <cr>limits</c> and be <cg>able to create comments</c> in game.\nAfter click on the <co>\"Continue\" button</c> you will be <cr>redirected to</c> <cy>browser auth interfaces</c>.",
            "Abort", "Continue",
            360.f
        );
        pop->setID("info");
        pop->show();
    }
    void onPasteToInput(CCObject* btnObj) {
        auto btn = dynamic_cast<CCNode*>(btnObj);
        auto menu = btn->getParent();
        auto input = dynamic_cast<TextInput*>(menu->getChildByIDRecursive("input"));
        input->setString(utils::clipboard::read());
    };
    void onOpenupBtn(CCObject*) {
        show_info();
    }
};

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

        auto padding = 8.f;
        auto rowcell = CCNode::create();
        rowcell->setContentWidth(parent->getContentWidth() - padding);
        //row
        auto row = CCMenu::create();
        if (row) {
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
                    auto error_code = std::error_code();
                    std::filesystem::remove(filep, error_code);
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
                    auto edit_delBtn_001 = CCSprite::createWithSpriteFrameName("geode.loader/delete-white.png");
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
        rowcell->addChild(row);
        rowcell->setLayout(RowLayout::create());//contentsize
        rowcell->setAnchorPoint(CCPoint(0.5f, 0.5f));

        this->setContentWidth(parent->getContentWidth());
        this->setContentHeight(rowcell->getContentHeight() + padding);
        this->addChildAtPosition(rowcell, Anchor::Center);

        auto bg = CCScale9Sprite::create(
            "square02_small.png"
        );
        bg->setZOrder(-1);
        bg->setOpacity(75);
        bg->setContentSize(this->getContentSize());
        this->addChildAtPosition(bg, Anchor::Center);
    }
    //IssueCommentItem
    void deleteComment(CCObject*) {
        auto a = [this](std::string const& rtn)
            {
                if (auto comments = typeinfo_cast<CCMenuItemSpriteExtra*>(
                    CCScene::get()->getChildByIDRecursive("comments"))) {
                    comments->getParent()->setTag(0);//remove prev tab set
                    comments->activate();//reopen tab
                };
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
    static auto create(Ref<CCContentLayer> contentLayer, std::string modID) {
        if (!contentLayer) return;

        auto scroll = typeinfo_cast<ScrollLayer*>(contentLayer->getParent());
        if (!scroll) return;

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
        contentLayer->setContentHeight(//last line space
            contentLayer->getContentHeight() - scroll_gap
        );
        //fix some shit goes when content smaller than scroll
        if (contentLayer->getContentSize().height < scroll->getContentSize().height) {
            contentLayer->setContentSize({
                contentLayer->getContentSize().width,
                scroll->getContentSize().height
                });
        }
        contentLayer->updateLayout();
        scroll->scrollLayer(contentLayer->getContentSize().height);
    }
};

class LoadCommentsLayer : public CCLayer {
public:
    EventListener<web::WebTask> m_webTaskListener;
    static auto create(Ref<CCContentLayer> contentLayer, std::string modID) {
        auto me = new LoadCommentsLayer();
        me->init();

        auto scroll = typeinfo_cast<ScrollLayer*>(contentLayer->getParent());

        me->setAnchorPoint({ 0.0f, 0.0f });
        if (contentLayer) {
            if (scroll) me->setContentSize(scroll->getContentSize());
            contentLayer->setContentSize(me->getContentSize());
            contentLayer->addChild(me);
        };
        auto bg = CCScale9Sprite::create(
            "square02_small.png"
        );
        bg->setOpacity(75);
        bg->setContentSize(me->getContentSize());
        me->addChildAtPosition(bg, Anchor::Center);

        Ref<CCLabelBMFont> label = CCLabelBMFont::create(fmt::format(
            "Loading comments for {}...", 
            modID
            ).data(), "chatFont.fnt"
        );
        label->setAlignment(kCCTextAlignmentCenter);
        label->setAnchorPoint({ 0.5f, 1.f });
        label->setWidth(me->getContentSize().width - 32.f);
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
                        if (contentLayer) CommentsLayer::create(contentLayer, modID);
                    }
                    else if (label) label->setString(fmt::format(
                        "{}\n{}",
                        label->getString(),
                        json.error_or("no err, str: " + string.value_or("no str"))
                    ).c_str());
                }
                else if (e->isCancelled()) if (label) label->setString(fmt::format(
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
    static LoadIssuesLayer* create(Ref<CCContentLayer> contentLayer, std::string modID, bool createNew = false) {
        auto me = new LoadIssuesLayer();
        me->init();

        me->setAnchorPoint({ 0.0f, 0.0f });

        me->setAnchorPoint({ 0.0f, 0.0f });
        if (contentLayer) {
            me->setContentSize(contentLayer->getParent()->getContentSize());
            contentLayer->setContentSize(me->getContentSize());
            contentLayer->addChild(me);
        };
        auto bg = CCScale9Sprite::create(
            "square02_small.png"
        );
        bg->setOpacity(75);
        bg->setContentSize(me->getContentSize());
        me->addChildAtPosition(bg, Anchor::Center);

        Ref<CCLabelBMFont> label = CCLabelBMFont::create(
            "Loading issues...\n(discussion channels)", 
            "chatFont.fnt"
        );
        label->setAlignment(kCCTextAlignmentCenter);
        label->setAnchorPoint({ 0.5f, 1.f });
        label->setWidth(me->getContentSize().width - 32.f);
        me->addChildAtPosition(label, Anchor::Top, {0.f, -72.f});

        auto circle = LoadingCircleSprite::create();
        circle->setScale(0.6f);
        me->addChildAtPosition(circle, Anchor::Top, {0.f, -44.f});

        me->m_webTaskListener.bind(
            [contentLayer, modID, createNew, me, label](web::WebTask::Event* e) {
                if (web::WebResponse* res = e->getValue()) {
                    auto json = res->json();
                    auto string = res->string();
                    if (json.has_value()) {
                        issues = json.value();

                        if (createNew) mod_issues[modID] = issues;
                        else if (issues.is_array()) for (auto issue : issues.as_array()) {
                            auto title = issue.try_get<std::string>("title").value_or("NO_TITLE");
                            if (title == modID) {
                                mod_issues[modID] = issue;
                                break;
                            }
                        }

                        if (mod_issues.contains(modID)) {
                            me->removeFromParent();
                            if (contentLayer) LoadCommentsLayer::create(contentLayer, modID);
                        }
                        else {
                            if (github::has_token()) {
                                me->removeFromParent();
                                if (contentLayer) LoadIssuesLayer::create(
                                    contentLayer, modID, true
                                );
                            }
                            else {
                                if (label) label->setString(fmt::format(
                                    "Can't find any issue for {}!\nLogin to create new comments channel!",
                                    modID
                                ).data());
                            }
                        };

                    }
                    else if (label) label->setString(fmt::format(
                        "{}\n{}",
                        label->getString(),
                        json.error_or("no err, str: " + string.value_or("no str"))
                    ).c_str());
                }
                else if (e->isCancelled()) if (label) label->setString(fmt::format(
                    "{}\n\n\nThe request was cancelled...",
                    label->getString()
                ).c_str());
            }
        );

        auto req = github::get_basic_web_request();
        if (createNew) {
            auto body = matjson::Value();
            body["title"] = modID;
            req.bodyJSON(body);
        }
        me->m_webTaskListener.setFilter(req.send(
            createNew ? "POST" : "GET",
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
                        if (auto comments_menu = rcontainer->getChildByIDRecursive("comments_menu"))
                            comments_menu->removeFromParent();
                    }
                    else if (sender->getTag() == myTabID) {
                        tabspr->m_label->setString("Send New");
                        //toggle out others
                        description_sprite->select(0);
                        changelog_sprite->select(0);
                        //sel this tab
                        tabspr->select(1);
                        //upload comment pop or open list
                        if (sender->getParent()->getTag() == myTabID) {

                            //md_prev
                            auto md_prev = MDTextArea::create("# input and preview here...\nJust tap on me and type your text!\n\nAlso you can type <co>`\\n`</c> and it will be replaced with newline char automatically. And remember, in **Markdown** you write two newlines to start new paragraph!", CCSize(290.f, 112.f));
                            md_prev->setID("md_prev");
                            md_prev->setPositionY(86.f);

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

                            auto pop = createQuickPopup(
                                "Create Comment",
                                "\n \n \n \n \n ",
                                "Close", "Create",
                                380.f, 
                                [input, modID, sender, old_lis, old_sel](CCNode*, bool create) {
                                    if (not create) return;
                                    auto data = input->getString();
                                    auto body = matjson::parse("{\"body\": \"\"}");
                                    body["body"] = data;
                                    auto a = [sender, old_lis, old_sel](std::string const& rtn)
                                        {
                                            if (string::contains(rtn, "\"body\":")) {
                                                sender->getParent()->setTag(0);//remove prev tab
                                                sender->activate();//reopen tab
                                                return;
                                            }
                                            auto message = rtn;
                                            auto asd = geode::createQuickPopup(
                                                "Request",
                                                message,
                                                "Nah", nullptr, 420.f, nullptr, false
                                            );
                                            asd->show();
                                        };
                                    auto b = [](std::string const& rtn)
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
                                        [a, b](web::WebTask::Event* e) {
                                            if (web::WebResponse* res = e->getValue()) {
                                                std::string data = res->string().unwrapOr("");
                                                //call the some shit
                                                if (res->code() < 399) a(data);
                                                else b(data);
                                            }
                                        }
                                    );
                                    req.bodyJSON(body);
                                    listener->setFilter(req.send("POST", mod_issues[modID]["comments_url"].as_string()));
                                }
                            );

                            pop->m_buttonMenu->addChild(md_prev, 1);
                            pop->m_buttonMenu->addChild(input);
                            handleTouchPriority(pop);
                        }
                        else {
                            if (auto comments_scroll = rcontainer->getChildByIDRecursive("comments_scroll"))
                                comments_scroll->removeFromParent();
                            if (auto comments_menu = rcontainer->getChildByIDRecursive("comments_menu"))
                                comments_menu->removeFromParent();
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
                            //menuuu
                            auto menu = CCMenu::create();
                            menu->setID("comments_menu");
                            comments_scroll->getParent()->addChild(menu);
                            menu->setContentSize(comments_scroll_size);
                            menu->setAnchorPoint(CCPointZero);
                            menu->setPosition(CCPointZero);
                            //reload
                            menu->addChildAtPosition(CCMenuItemExt::createSpriteExtraWithFrameName(
                                "geode.loader/reload.png", 0.6f, [sender](CCMenuItemSpriteExtra*) {
                                    sender->getParent()->setTag(0);//remove prev tab set
                                    sender->activate();//reopen tab
                                }
                            ), Anchor::BottomRight, { -6.f, 6.f});
                            //github
                            auto githubitem = CCMenuItemExt::createSpriteExtraWithFrameName(
                                "geode.loader/github.png", 0.6f, [sender](CCMenuItemSpriteExtra*) {
                                    GitHubAuthPopup::show_info();
                                }
                            );
                            githubitem->setID("githubitem");
                            githubitem->setOpacity(github::has_token() ? 120 : 255);
                            menu->addChildAtPosition(githubitem, Anchor::TopRight, { -6.f, -6.f });
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