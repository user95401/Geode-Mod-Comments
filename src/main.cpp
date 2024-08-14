#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <GeodeUI.hpp>

using namespace geode::prelude;

//hi

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

class ghAccount {
public:
    inline static auto api_repo_url = std::string(
        "https://api.github.com/repos/user95401/Geode-Mod-Comments"
    );
    inline static auto user = matjson::Value();
    //define
    static void try_load_user() {
        if (user.contains("id")) return;
        auto req = ghAccount::get_basic_web_request();
        auto listener = new EventListener<web::WebTask>;
        listener->bind(
            [](web::WebTask::Event* e) {
                if (web::WebResponse* res = e->getValue()) {
                    auto json = res->json();
                    auto string = res->string();
                    if (json.has_value()) user = json.value();
                }
            }
        );
        listener->setFilter(req.send(
            "GET",
            "https://api.github.com/user"
        ));
    }
    static std::string get_token() {
        return Mod::get()->getSavedValue<std::string>("gh_access_token");
    }
    static void set_token(std::string token) {
        Mod::get()->setSavedValue("gh_access_token", token);
        Mod::get()->saveData();
        user = matjson::Value();
        try_load_user();
    }
    static bool has_token() {
        return (get_token().size() > 3);
    }
    static web::WebRequest get_basic_web_request() {
        auto req = web::WebRequest();
        req.userAgent(Mod::get()->getID());
        req.header("X-GitHub-Api-Version", "2022-11-28");
        if (has_token()) req.header("Authorization", fmt::format("Bearer {}", get_token()));
        return req;
    }
    static web::WebRequest* create_basic_web_request() {
        auto req = new web::WebRequest();
        req->userAgent(Mod::get()->getID());
        req->header("X-GitHub-Api-Version", "2022-11-28");
        if (has_token()) req->header("Authorization", fmt::format("Bearer {}", get_token()));
        return req;
    }
};
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
                ghAccount::set_token(catgirl["access_token"].as_string());
                if (auto githubitem = typeinfo_cast<CCMenuItemSpriteExtra*>(
                    CCScene::get()->getChildByIDRecursive("githubitem"))) {
                    githubitem->setOpacity(ghAccount::has_token() ? 120 : 255);
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

$on_mod(Loaded){ ghAccount::try_load_user(); }

inline auto issues = matjson::Value();
inline std::map<std::string, matjson::Value> mod_issues;
inline std::map<std::string, matjson::Value> mod_comments;

//ryzen code :D
class IssueCommentItem : public CCLayer {
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

        auto comment_user_id = m_json["user"]["id"].as_int();
        auto loggedinuser_id = ghAccount::user.try_get<int>("id").value_or(0);
        auto is_owner = (comment_user_id == loggedinuser_id);

        auto padding = 8.f;

        auto cell = CCNode::create();
        cell->setContentWidth(parent->getContentWidth() - padding);

        //react_row
        if (m_json.contains("reactions")) {
            auto react_row = CCMenu::create();
            auto& reactions = m_json["reactions"];
            auto onBtn = [reactions](CCMenuItemSpriteExtra* sender)
                {
                    auto tab = typeinfo_cast<GeodeTabSprite*>(sender->getNormalImage());
                    auto should_delete = tab->m_selectedBG->isVisible();
                    sender->setEnabled(0);
                    auto req = ghAccount::get_basic_web_request();
                    auto listener = new EventListener<web::WebTask>;
                    listener->bind([tab, sender, should_delete](web::WebTask::Event* e)
                        {
                            if (web::WebResponse* res = e->getValue()) {
                                sender->setEnabled(1);
                                auto string = res->string();
                                auto json = res->json().value_or(matjson::Value());
                                if (res->code() < 399) {
                                    tab->select(!should_delete);
                                    sender->setTag(json.try_get<int>("id").value_or(sender->getTag()));
                                    auto count = numFromString<int>(tab->m_label->getString()).value_or(1);
                                    if (should_delete) --count; else ++count;
                                    tab->m_label->setString(fmt::format("{}", count).data());
                                }
                            }
                            else if (e->isCancelled()) {
                                sender->setEnabled(1);
                            }
                        }
                    );
                    auto url = reactions.try_get<std::string>("url").value_or("");
                    if (should_delete) {
                        listener->setFilter(req.send(
                            "DELETE", fmt::format("{}/{}", url, sender->getTag())
                        ));
                    }
                    else {
                        auto body = matjson::Value();
                        body["content"] = sender->getID();
                        req.bodyJSON(body);
                        listener->setFilter(req.send(
                            "POST", url
                        ));
                    };
                };
            auto addBtn = [reactions, react_row, onBtn](std::string name, std::string frame, std::string text = "")
                {
                    auto btnspr = GeodeTabSprite::create(
                        frame.data(),
                        (not text.empty() ? 
                            text : string::replace(
                                reactions[name].dump(), "\"", "")
                            ).data(),
                        52.f, 1
                    );
                    btnspr->select(0);
                    btnspr->m_deselectedBG->setContentHeight(32.f);
                    btnspr->m_selectedBG->setContentHeight(32.f);
                    btnspr->m_label->setFntFile("gjFont17.fnt");
                    btnspr->m_label->setScale(0.2f + btnspr->m_label->getScale());
                    btnspr->setScale(0.6f);
                    auto btn = CCMenuItemExt::createSpriteExtra(btnspr, onBtn);
                    btn->setID(name.data());
                    react_row->addChild(btn);
                    return btn;
                };
            auto plus1 = addBtn("+1", "+1.png"_spr);
            auto minus1 = addBtn("-1", "-1.png"_spr);
            auto laugh = addBtn("laugh", "laugh.png"_spr);
            auto confused = addBtn("confused", "confused.png"_spr);
            auto heart = addBtn("heart", "heart.png"_spr);
            auto hooray = addBtn("hooray", "hooray.png"_spr);
            auto rocket = addBtn("rocket", "rocket.png"_spr);
            auto eyes = addBtn("eyes", "eyes.png"_spr);
            auto loading = LoadingSpinner::create(1.f);
            react_row->addChild(loading);
            react_row->setTouchEnabled(0);
            react_row->setContentHeight(18.000f);//temp
            react_row->setContentWidth(parent->getContentWidth());
            react_row->setLayout(
                RowLayout::create()
                ->setAxisAlignment(AxisAlignment::Start)
                ->setAutoScale(0)
                ->setGrowCrossAxis(1)
                ->setCrossAxisOverflow(0)
            );
            react_row->setContentHeight(20.000f);
            cell->addChild(react_row);
            cell->setContentHeight(react_row->getContentHeight() + cell->getContentHeight());
            //loadthereactionsfk
            std::function<void(int)> load;
            auto req = ghAccount::create_basic_web_request();
            auto bindfn = [req, reactions, load, react_row, loading, loggedinuser_id](web::WebTask::Event* e)
                {
                    if (web::WebResponse* res = e->getValue()) {
                        auto json = res->json();
                        auto string = res->string();
                        if (res->code() < 399) if (json.has_value()) {
                            auto val = json.value();
                            if (val.is_array()) for (auto reaction : val.as_array()) {
                                auto itsme = reaction["user"]["id"].as_int() == loggedinuser_id;
                                auto name = reaction["content"].as_string();
                                auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(react_row->getChildByID(name));
                                auto tab = typeinfo_cast<GeodeTabSprite*>(item->getNormalImage());
                                if (itsme) tab->select(itsme);
                                item->setTag(reaction["id"].as_int());//for delete
                            };
                            auto params = req->getUrlParams();// .at("page");
                            auto page = utils::numFromString<int>(params.contains("page") ? params.at("page") : 0).value_or(1);
                            if (json.value().as_array().size() < 100) {
                                //final
                                loading->removeFromParent();
                                react_row->setTouchEnabled(1);
                            }
                            else load(page + 1);
                        }
                    };
                };
            load = [req, bindfn, reactions](int page)
                {
                    req->param("page", page);
                    req->param("per_page", 100);
                    auto listener = new EventListener<web::WebTask>;
                    listener->bind(bindfn);
                    listener->setFilter(req->get(
                        reactions.try_get<std::string>("url").value_or("")
                    ));
                };
            load(1);
        }

        //cell row
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
                    //std::filesystem::remove(filep, error_code);
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
                //menu user text
                CCLabelBMFont* user;
                MDTextArea* mdarea = MDTextArea::create(m_json["body"].as_string(), { text->getContentWidth(), 10 });
                if (auto menu = CCMenu::create()) {

                    //user
                    user = CCLabelBMFont::create(
                        fmt::format("{}", m_json["user"]["login"].as_string()).c_str(),
                        "chatFont.fnt"
                    );
                    user->setScale(0.8f);
                    user->setID("user");
                    menu->addChild(user);

                    //updated_at
                    auto updated_at = m_json["updated_at"].as_string();
                    updated_at = string::replace(updated_at, "T", " ");
                    updated_at = string::replace(updated_at, "Z", "");
                    updated_at = string::replace(updated_at, "-", ".");
                    auto updated_at_label = CCLabelBMFont::create(
                        fmt::format(" at {}", updated_at).c_str(),
                        "chatFont.fnt"
                    );
                    updated_at_label->setScale(0.68f);
                    updated_at_label->setOpacity(120);
                    updated_at_label->setID("updated_at_label");
                    menu->addChild(updated_at_label);

                    menu->setContentHeight(user->getContentHeight());
                    menu->setContentWidth(text->getContentWidth());
                    menu->setLayout(
                        RowLayout::create()
                        ->setAxisAlignment(AxisAlignment::Start)
                        ->setAutoScale(false)
                        ->setCrossAxisOverflow(false)
                        ->setGap(0.f)
                    );
                    //menu update
                    menu->updateLayout();

                    //delete_btn
                    auto edit_delBtn_001 = CCSprite::createWithSpriteFrameName("geode.loader/delete-white.png");
                    edit_delBtn_001->setScale(0.5f);
                    auto delete_btn = CCMenuItemSpriteExtra::create(
                        edit_delBtn_001,
                        this,
                        menu_selector(IssueCommentItem::deleteComment)
                    );
                    if (not is_owner) edit_delBtn_001->setOpacity(6);
                    menu->addChildAtPosition(
                        delete_btn, Anchor::TopRight, { -6.f, -6.f }, false
                    );

                    //edit
                    auto comment_edit_input = TextInput::create(120.f, "ew");
                    comment_edit_input->setVisible(0);
                    this->addChild(comment_edit_input);
                    auto comment_edit = CCMenuItemExt::createTogglerWithFilename(
                        "comment_upload.png"_spr, "comment_edit.png"_spr, 0.7f,
                        [this, mdarea, comment_edit_input](CCMenuItemToggler* item) {
                            if (not item->m_toggled) {
                                comment_edit_input->setString(mdarea->getString());
                                comment_edit_input->setCallback(
                                    [mdarea, comment_edit_input](std::string str) {
                                        auto endl_filtered = string::replace(str, "\\n", "\n");
                                        if (str.find("\\n") != std::string::npos) {
                                            comment_edit_input->setString(endl_filtered.data());
                                        }
                                        mdarea->setString(endl_filtered.data());
                                        if (auto body_container = public_cast(mdarea, m_content)->getParent()) {
                                            body_container->updateLayout();
                                            body_container->getParent()->updateLayout();
                                            body_container->getParent()->getParent()->updateLayout();
                                        };
                                    }
                                );
                                comment_edit_input->focus();
                            }
                            else {
                                comment_edit_input->defocus();
                                Ref<LoadingCircle> loadinlr = LoadingCircle::create();
                                loadinlr->setContentSize(this->getContentSize());
                                loadinlr->m_sprite->setPosition({ 12.f, 12.f });
                                loadinlr->m_sprite->setScale(0.25f);
                                loadinlr->setParentLayer(this);
                                loadinlr->addChild(this->getChildByID("bg"));
                                loadinlr->show();
                                auto req = ghAccount::get_basic_web_request();
                                auto listener = new EventListener<web::WebTask>;
                                listener->bind(
                                    [this, loadinlr, comment_edit_input](web::WebTask::Event* e) {
                                        if (web::WebResponse* res = e->getValue()) {
                                            if (loadinlr) loadinlr->removeFromParent();
                                            std::string data = res->string().unwrapOr("");
                                            auto json = res->json().value_or(m_json);

                                            auto ntfy = Notification::create(" ");
                                            if (res->code() < 399) ntfy->setString(
                                                "comment was updated"
                                            );
                                            else if (json.contains("message")) ntfy->setString(
                                                "" + json["message"].as_string()
                                            );
                                            else ntfy->setString(
                                                "" + res->string().error_or(data)
                                            );
                                            ntfy->show();

                                            auto body = json.try_get<std::string>("body").value_or(m_json["body"].as_string());
                                            comment_edit_input->setString(body, 1);
                                        }
                                    }
                                );
                                auto body = matjson::parse("{\"body\": \"\"}");
                                body.try_set("body", comment_edit_input->getString());
                                req.bodyJSON(body);
                                listener->setFilter(req.send("PATCH", m_json["url"].as_string()));
                            }
                        }
                    );
                    if (not is_owner) comment_edit->m_offButton->setOpacity(6);
                    if (not is_owner) comment_edit->m_onButton->setOpacity(6);
                    menu->addChildAtPosition(
                        comment_edit, Anchor::TopRight, { -24.f, -6.f }, false
                    );

                    text->addChild(menu);
                    text->updateLayout();
                }
                //body
                auto body = public_cast(mdarea, m_content);
                body->removeFromParentAndCleanup(0);
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
        cell->addChild(row);
        cell->setContentHeight(row->getContentHeight() + cell->getContentHeight());

        cell->setLayout(ColumnLayout::create()->setGap(0)->setCrossAxisOverflow(0));
        cell->setAnchorPoint(CCPoint(0.5f, 0.5f));

        this->setContentWidth(parent->getContentWidth());
        this->setContentHeight(cell->getContentHeight() + padding);
        this->addChildAtPosition(cell
            , Anchor::Center);

        auto bg = CCScale9Sprite::create(
            "square02_small.png"
        );
        bg->setID("bg");
        bg->setZOrder(-1);
        bg->setOpacity(75);
        bg->setContentSize(this->getContentSize());
        this->addChildAtPosition(bg, Anchor::Center, CCPointZero, false);
    }
    //IssueCommentItem
    void deleteComment(CCObject*) {
        Ref<LoadingCircle> loadinlr = LoadingCircle::create();
        loadinlr->setContentSize(this->getContentSize());
        loadinlr->m_sprite->setPosition({ 12.f, 12.f });
        loadinlr->m_sprite->setScale(0.25f);
        loadinlr->setParentLayer(this);
        loadinlr->addChild(this->getChildByID("bg"));
        loadinlr->show();
        auto a = [this, loadinlr](std::string const& rtn)
            {
                if (loadinlr) loadinlr->removeFromParent();
                if (this) {
                    auto contentLayer = typeinfo_cast<CCContentLayer*>(this->getParent());
                    auto scroll = typeinfo_cast<ScrollLayer*>(contentLayer->getParent());
                    if (contentLayer and scroll) {

                        contentLayer->setContentHeight(
                                contentLayer->getContentHeight() 
                                - this->getContentHeight()
                                - scroll->getChildByID("scroll_gap")->getTag()
                            );
                        if (contentLayer->getContentHeight() < scroll->getContentHeight())
                            contentLayer->setContentHeight(scroll->getContentHeight());

                        this->removeFromParentAndCleanup(false);

                        contentLayer->updateLayout();
                        scroll->scrollLayer(0.f);
                    };
                };
            };
        auto b = [this, loadinlr](std::string const& rtn)
            {
                if (loadinlr) loadinlr->removeFromParent();
                auto message = rtn;
                auto asd = geode::createQuickPopup(
                    "Request exception",
                    message,
                    "Nah", nullptr, 420.f, nullptr, false
                );
                asd->show();
            };
        auto req = ghAccount::get_basic_web_request();
        auto listener = new EventListener<web::WebTask>;
        listener->bind(
            [this, a, b](web::WebTask::Event* e) {
                if (web::WebResponse* res = e->getValue()) {
                    std::string data = res->string().unwrapOr("");
                    auto json = res->json().value_or(matjson::Value());
                    if (json.contains("message")) data = json["message"].as_string();
                    if (json["errors"].is_array()) for (auto err : json["errors"].as_array()) data += ", " + err["message"].as_string();
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
};

class CommentsLayer : public CCLayer {
public:
    static auto create(Ref<CCContentLayer> contentLayer, std::string modID) {
        if (!contentLayer) return;

        auto scroll = typeinfo_cast<ScrollLayer*>(contentLayer->getParent());
        if (!scroll) return;

        auto scroll_gap = CCNode::create();
        scroll_gap->setID("scroll_gap");
        scroll_gap->setTag(6);
        scroll->addChild(scroll_gap);
        
        contentLayer->setAnchorPoint(CCPointZero);
        contentLayer->setPositionX(0);
        contentLayer->setLayout(
            ColumnLayout::create()
            ->setGap(scroll->getChildByID("scroll_gap")->getTag())
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
        );

        contentLayer->setContentHeight(0.f);

        auto startLayer = CCMenu::create();
        startLayer->setAnchorPoint({ 0.5f, 0.f });
        startLayer->setContentSize(
            { contentLayer->getContentWidth(), 44.f }
        );
        if (startLayer) {
            auto header = CCLabelBMFont::create(fmt::format(
                "Welcome to comments about {}!", modID
            ).c_str(), "geode.loader/mdFontB.fnt");
            if ((startLayer->getContentWidth() - 32.f) < header->getContentWidth())
                header->setScale((startLayer->getContentWidth() - 36.f) / header->getContentWidth());
            header->setAnchorPoint({0.5f, 0.f});
            startLayer->addChildAtPosition(header, Anchor::Bottom, { 0.f, 28.f });
            auto startedby = CCLabelBMFont::create(fmt::format(
                "Started by {}.", mod_issues[modID]["user"].try_get<std::string>("login").value_or("broken user lol")
            ).c_str(), "chatFont.fnt");
            startedby->setOpacity(190);
            startedby->setScale(0.7);
            startedby->setAnchorPoint({0.5f, 0.f});
            startLayer->addChildAtPosition(startedby, Anchor::Bottom, {0.f, 12.f});
            auto line = CCSprite::createWithSpriteFrameName("floorLine_01_001.png");
            line->setScaleX((startLayer->getContentWidth()) / line->getContentWidth());
            startLayer->addChildAtPosition(line, Anchor::Bottom, {0.f, 6.f});
        }
        contentLayer->setContentHeight(//make content layer longer
            contentLayer->getContentHeight() + startLayer->getContentHeight()
        );
        contentLayer->addChild(startLayer);

        if (mod_comments[modID].is_array()) for (auto comment : mod_comments[modID].as_array()) {

            auto item = IssueCommentItem::create(contentLayer, comment);
            
            contentLayer->setContentHeight(//make content layer longer
                contentLayer->getContentHeight()
                + item->getContentHeight()
                + scroll->getChildByID("scroll_gap")->getTag()
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
        auto peek = (int)(contentLayer->getContentHeight() > scroll->getContentHeight()) * 42.f;
        scroll->m_peekLimitTop = peek;
        scroll->m_peekLimitBottom = peek;
        scroll->scrollLayer(contentLayer->getContentSize().height);
    }
};

class LoadCommentsLayer : public CCLayer {
public:
    EventListener<web::WebTask> m_webTaskListener;
    static auto create(Ref<CCContentLayer> contentLayer, std::string modID, int page = 1) {
        auto me = new LoadCommentsLayer();
        me->init();

        if (!contentLayer) return me;

        auto scroll = typeinfo_cast<ScrollLayer*>(contentLayer->getParent());
        if (!scroll) return me;

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
            "Loading comments for {}...\npage: {}", 
            modID, page
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
            [contentLayer, modID, page, me, label](web::WebTask::Event* e) {
                if (web::WebResponse* res = e->getValue()) {
                    auto json = res->json();
                    auto string = res->string();
                    if (json.has_value()) {
                        if (mod_comments[modID].is_array()) {
                            for (auto comment : json.value().as_array()) {
                                mod_comments[modID].as_array().push_back(comment);
                            }
                        }
                        else {
                            mod_comments[modID] = json.value();
                        }
                        me->removeFromParent();
                        if (json.value().as_array().size() < 100) {
                            if (contentLayer) CommentsLayer::create(contentLayer, modID);
                        }
                        else LoadCommentsLayer::create(contentLayer, modID, (page + 1));
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

        auto req = ghAccount::get_basic_web_request();
        req.param("page", page);
        req.param("per_page", 100);
        me->m_webTaskListener.setFilter(req.get(
            mod_issues[modID].try_get<std::string>("comments_url").value_or("")
        ));

        return me;
    }
};

class LoadIssuesLayer : public CCLayer {
public:
    EventListener<web::WebTask> m_webTaskListener;
    static LoadIssuesLayer* create(Ref<CCContentLayer> contentLayer, std::string modID, int page = 1, bool createNew = false) {
        auto me = new LoadIssuesLayer();
        me->init();

        if (!contentLayer) return me;

        auto scroll = typeinfo_cast<ScrollLayer*>(contentLayer->getParent());
        if (!scroll) return me;

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
            "{}...\n"
            "page: {}",
            not createNew ? "Loading issues" : "Creating new issue", 
            page
        ).data(),
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
            [contentLayer, modID, page, createNew, me, label](web::WebTask::Event* e) {
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
                            mod_comments.erase(modID);
                            if (contentLayer) LoadCommentsLayer::create(contentLayer, modID);
                        }
                        else {
                            if (page <= 5) {
                                me->removeFromParent();
                                if (contentLayer) LoadIssuesLayer::create(
                                    contentLayer, modID, (1 + page)
                                );
                            }
                            else if (ghAccount::has_token()) {
                                me->removeFromParent();
                                if (contentLayer) LoadIssuesLayer::create(
                                    contentLayer, modID, page, true
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

        auto req = ghAccount::get_basic_web_request();
        if (createNew) {
            auto body = matjson::Value();
            body["title"] = modID;
            req.bodyJSON(body);
        }
        else {
            req.param("page", page);
            req.param("per_page", 100);
            req.param("state", "all");
        }
        me->m_webTaskListener.setFilter(req.send(
            createNew ? "POST" : "GET",
            ghAccount::api_repo_url + "/issues"
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
                auto mark = CCNode::create();
                mark->setID("pointAndSizeFixMark");
                if (tabsMenu->getChildByID(mark->getID())) void();
                else {
                    tabsMenu->addChild(mark);
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
                                    Ref<LoadingCircle> loadinlr = LoadingCircle::create();
                                    loadinlr->setID("loadinlr");
                                    loadinlr->setFade(true);
                                    loadinlr->show();
                                    loadinlr->setTouchEnabled(1);
                                    handleTouchPriority(loadinlr);
                                    auto data = input->getString();
                                    auto body = matjson::parse("{\"body\": \"\"}");
                                    body["body"] = data;
                                    auto a = [sender, loadinlr](std::string const& rtn)
                                        {
                                            if (loadinlr) loadinlr->fadeAndRemove();
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
                                    auto b = [loadinlr](std::string const& rtn)
                                        {
                                            if (loadinlr) loadinlr->fadeAndRemove();
                                            auto message = rtn;
                                            auto asd = geode::createQuickPopup(
                                                "Request exception",
                                                message,
                                                "Nah", nullptr, 420.f, nullptr, false
                                            );
                                            asd->show();
                                        };
                                    auto req = ghAccount::get_basic_web_request();
                                    auto listener = new EventListener<web::WebTask>;
                                    listener->bind(
                                        [a, b](web::WebTask::Event* e) {
                                            if (web::WebResponse* res = e->getValue()) {
                                                std::string data = res->string().unwrapOr("");
                                                auto json = res->json().value_or(matjson::Value());
                                                if (json.contains("message")) data = json["message"].as_string();
                                                if (json["errors"].is_array()) for (auto err : json["errors"].as_array()) data += ", " + err["message"].as_string();
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
                            comments_scroll->m_peekLimitTop = 0;
                            comments_scroll->m_peekLimitBottom = 0;
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
                            ), Anchor::BottomRight, { -3.f, 6.f});
                            //github
                            auto githubitem = CCMenuItemExt::createSpriteExtraWithFrameName(
                                "geode.loader/github.png", 0.56f, [sender](CCMenuItemSpriteExtra*) {
                                    GitHubAuthPopup::show_info();
                                }
                            );
                            githubitem->setID("githubitem");
                            githubitem->setOpacity(ghAccount::has_token() ? 60 : 255);
                            menu->addChildAtPosition(githubitem, Anchor::TopRight, { -5.f, 9.f });
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