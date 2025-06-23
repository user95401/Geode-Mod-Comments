#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <GeodeUI.hpp>

#include <argon/argon.hpp>

using namespace geode::prelude;

#include <StupidAsfMultilineMDTextEditor.hpp>

#define API (getMod()->getSettingValue<std::string>("API"))

#define ARGON_ALLOWED (getMod()->getSettingValue<bool>("ARGON_ALLOWED"))
#define ARGON_ALLOW(setup) getMod()->setSettingValue<bool>("ARGON_ALLOWED", setup)

inline static auto ARGON_TOKEN = std::string();

class CommentsLayer : public CCLayer {
public:
    EventListener<web::WebTask> m_webTaskListener;
    std::string m_id;
    CCSize m_size;
    virtual void keyDown(enumKeyCodes key) override {
        CCLayer::keyDown(key);
        
        if (key == KEY_Enter) {
            auto button = typeinfo_cast<CCMenuItem*>(
                this->getChildByIDRecursive(
                    CCDirector::get()->getKeyboardDispatcher()->getShiftKeyPressed()
                    ? "inpExt"_spr : "sendButton"_spr
                )
            );
            if (button) button->activate();
        }
        
        auto commentsScroll = typeinfo_cast<ScrollLayer*>(this->getChildByIDRecursive("commentsScroll"_spr));
        if (!commentsScroll) return;
        
        if (key == KEY_F5) this->loadComments();
        if (key == KEY_F1) openInfoPopup(getMod());
        
        if (key == KEY_PageUp) commentsScroll->scrollLayer(
            - commentsScroll->getContentHeight()
        );
        if (key == KEY_PageDown) commentsScroll->scrollLayer(
            commentsScroll->getContentHeight()
        );
        if (key == KEY_End) commentsScroll->scrollLayer(
            commentsScroll->m_contentLayer->getContentHeight()
        );
        if (key == KEY_Home) commentsScroll->scrollToTop();
    }
    static CommentsLayer* create(std::string id, CCSize size) {
        auto ret = new CommentsLayer;
        ret->m_id = id;
        ret->m_size = size;
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
    void recreateMe() {
        this->getParent()->addChild(
            CommentsLayer::create(m_id, m_size)
        );
        this->removeFromParent();
    }
    void setupComments(matjson::Value comments) {
        
        auto commentsScroll = typeinfo_cast<ScrollLayer*>(this->getChildByIDRecursive("commentsScroll"_spr));
        if (!commentsScroll) return;
        
        auto contentLayer = commentsScroll->m_contentLayer;
        
        contentLayer->removeAllChildren();
        
        for (auto& comment : comments) {
            auto container = CCMenu::create();
            container->setContentWidth(commentsScroll->getContentWidth());
            container->setContentHeight(0.f);
            container->setID("comment-container"_spr);
            container->setTag(comment["id"].asInt().unwrapOrDefault());
            
            //id is timestamp .-.
            auto sys_time = std::chrono::system_clock::from_time_t(comment["id"].asInt().unwrapOrDefault());
            auto time_seconds = std::chrono::floor<std::chrono::seconds>(sys_time);
            auto time_val = fmt::format("{:%Y-%m-%d %H:%M:%S}", time_seconds);
            
            auto user = comment["user"].asString().unwrapOr("unk");
            auto accountID = comment["accountID"].asString().unwrapOr("-1");
            auto userID = comment["userID"].asString().unwrapOr("-1");
            auto body = comment["body"].asString().unwrapOr("...");
            
            auto textAreaStr = fmt::format(
                "#### [{}](user:{}) <c-99a>at {}</c>\n", 
                user, accountID, time_val
            ) + body;
            
            auto size_detect_dummy = MDTextArea::create(textAreaStr, 
                { container->getContentWidth(), 16.000f }, true
            );
            auto textArea = MDTextArea::create(textAreaStr,
                { 
                    size_detect_dummy->getContentWidth(),
                    size_detect_dummy->getScrollLayer()->m_contentLayer->getContentHeight()
                }, true
            );
            size_detect_dummy->cleanup();
            textArea->setID("body"_spr);
            textArea->setAnchorPoint(CCPointMake(0.f, 0.f));
            textArea->getScrollLayer()->m_cutContent = false;
            textArea->getScrollLayer()->m_disableMovement = true;
            textArea->getScrollLayer()->setMouseEnabled(false);
            textArea->getChildByType<CCScale9Sprite>(0)->setVisible(false);
            container->addChildAtPosition(textArea, Anchor::BottomLeft, { 5.f, -2.000f });
            container->setContentHeight(container->getContentHeight() + textArea->getContentHeight());
            
            //the <c-99a>at {}</c> part.
            findFirstChildRecursive<CCLabelBMFont>(
                textArea->getScrollLayer()->m_contentLayer, [](CCLabelBMFont* label) {
                    if (label->getColor() == cc3bFromHexString("#99a").unwrapOrDefault()) {
                        label->setFntFile("geode.loader/mdFontMono.fnt");
                        label->setAnchorPoint(CCPointMake(0.f, 0.4f));
                        label->setBlendFunc({ GL_SRC_ALPHA, GL_ONE });
                        label->setColor(ccc3(52, 52, 52));
                        return true;
                    }
                    return false;
                }
            );
            
            auto bg = CCScale9Sprite::create("square02_small.png");
            bg->setZOrder(-1);
            bg->setOpacity(75);
            bg->setContentSize(container->getContentSize());
            container->addChildAtPosition(bg, Anchor::Center);
            
            auto deleteBtnLabel = CCLabelBMFont::create("[DELETE]", "chatFont.fnt");
            deleteBtnLabel->setColor({ 255, 120, 120 });
            deleteBtnLabel->setScale(0.6f);
            auto deleteBtn = CCMenuItemExt::createSpriteExtra(
                deleteBtnLabel,
                [__this = Ref(this), container = Ref(container)](CCMenuItem* btn) {
                    
                    //hold shift to fast delete
                    if (not CCDirector::get()->getKeyboardDispatcher()->getShiftKeyPressed()) {
                        auto confirmed = btn->getUserObject("confirmed"_spr);
                        if (!confirmed) return (void)createQuickPopup(
                            "Delete comment?",
                            "Are you sure you want to delete this comment?",
                            "Yes", "No",
                            [btn = Ref(btn)](CCNode*, bool no) {
                                if (no) return;
                                btn->setUserObject("confirmed"_spr, btn->m_pParent);
                                btn->activate();
                                btn->setUserObject("confirmed"_spr, nullptr);
                            }
                        );
                    };
                    
                    web::WebRequest req = web::WebRequest();
                    req.header("Content-Type", "multipart/form-data");
                    
                    __this->m_webTaskListener.bind(
                        [__this, container](web::WebTask::Event* e) {
                            if (!__this) return e->cancel();
                            if (web::WebResponse* res = e->getValue()) {
                                auto parsed = res->string().unwrapOrDefault();
                                if (string::contains(parsed, "Deleted")) {
                                    __this->loadComments();
                                }
                                else createQuickPopup(
                                    "Smth went wrong...",
                                    fmt::format(
                                        "Your comment wasn't deleted.\n<cr>{} [code: {}]</c>",
                                        parsed, res->code()
                                    ),
                                    "OK", nullptr, nullptr
                                );
                            }
                        }
                    );
                    
                    req.bodyMultipart(web::MultipartForm()
                        .param("delete", __this->getID())
                        .param("comment_id", container->getTag())
                        .param("account_id", argon::getGameAccountData().accountId)
                        .param("user_id", argon::getGameAccountData().userId)
                        .param("username", argon::getGameAccountData().username)
                        .param("token", ARGON_TOKEN)
                    );
                    __this->m_webTaskListener.setFilter(req.post(API + std::string("?delete")));
                    
                }
            );
            deleteBtn->setID("delete-comment"_spr);
            deleteBtn->setAnchorPoint(CCPointMake(1.f, 0.5f));
            deleteBtn->setVisible(accountID == numToString(argon::getGameAccountData().accountId));
            container->addChildAtPosition(deleteBtn, Anchor::TopRight, { -4.f, -8.f });
            
            auto editBtnLabel = CCLabelBMFont::create("[EDIT]", "chatFont.fnt");
            editBtnLabel->setColor({ 255, 180, 28 });
            editBtnLabel->setScale(0.6f);
            auto editBtn = CCMenuItemExt::createSpriteExtra(
                editBtnLabel,
                [__this = Ref(this), comment](CCObject*) {
                    auto sendButton = __this->querySelector("sendButton"_spr);
                    if (!sendButton) return;
                    auto input = typeinfo_cast<TextInput*>(__this->querySelector("input"_spr));
                    if (!input) return;
                    //set text of comment to edit
                    input->focus();
                    input->setString(comment["body"].asString().unwrapOrDefault());
                    if (string::contains(input->getString(), "\n")) {
                        input->getInputNode()->getTextLabel()->setString(
                            ("* " + matjson::Value(input->getString()).dump()).c_str()
                            //make prev label in tar input a bit smaller
                            //f.e its replaces newlines to \n
                        );
                        //add wrapping
                        input->getInputNode()->getTextLabel()->setWidth(
                            input->getContentSize().width - 4.f
                        );
                        limitNodeSize(
                            input->getInputNode()->getTextLabel(),
                            input->getContentSize(), 1337.f, .1f
                        );
                    }
                    //add markup
                    sendButton->removeChildByID("edit-markup"_spr); //remove prev markups
                    auto label = CCLabelBMFont::create("edit", "chatFont.fnt");
                    label->setOpacity(130);
                    label->setRotation(25.f);
                    label->setScale(0.575f);
                    label->setID("edit-markup"_spr);
                    label->setTag(comment["id"].asInt().unwrapOrDefault());
                    sendButton->addChildAtPosition(label, Anchor::TopRight, { -10.f, -8.f });
                }
            );
            editBtn->setID("edit-comment"_spr);
            editBtn->setAnchorPoint(CCPointMake(1.f, 0.5f));
            editBtn->setVisible(accountID == numToString(argon::getGameAccountData().accountId));
            container->addChildAtPosition(editBtn, Anchor::TopRight, { -4.f, -20.f });
            
            container->updateLayout();
            
            commentsScroll->m_contentLayer->addChild(container);
        }
        contentLayer->setLayout(RowLayout::create()
            ->setCrossAxisAlignment(AxisAlignment::End)
            ->setCrossAxisOverflow(true)
            ->setGrowCrossAxis(true)
            ->setAxisReverse(true)
            ->setGap(3.f)
        );
        commentsScroll->m_disableMovement = contentLayer->getContentHeight() <= commentsScroll->getContentHeight();
        if (commentsScroll->m_disableMovement) commentsScroll->scrollToTop();
    }
    void loadComments() {
        
        web::WebRequest req = web::WebRequest();
        req.header("Content-Type", "application/json");
        
        m_webTaskListener.bind(
            [__this = Ref(this)](web::WebTask::Event* e) {
                if (!__this) return e->cancel();
                if (web::WebResponse* res = e->getValue()) {
                    auto parsed = res->json();
                    if (parsed.isOk()) __this->setupComments(parsed.unwrapOrDefault());
                    else log::error("{} err: {}", __FUNCTION__, parsed.err().value_or("unk err..."));
                }
            }
        );
        
        req.param("get", this->getID());
        m_webTaskListener.setFilter(req.get(API));
        
    }
    bool auth() {
        
        if (!getMod()->getSavedValue<bool>("ASKED_FOR_USING_ARGON", false)) {
            getMod()->setSavedValue("ASKED_FOR_USING_ARGON", true);
            auto qWindow = createQuickPopup(
                "Authentication",
                "Argon by Globed will be used to authenticate your Geometry Dash account for Mod Comments.\n\nDo you allow this?",
                "No", "Yes",
                [this, parent = Ref(getParent())](CCNode*, bool allow) {
                    if (allow) {
                        ARGON_ALLOW(true);
                        auth();
                    }
                    else {
                        ARGON_ALLOW(false);
                        recreateMe();
                    }
                }
                , false //dnt show
            );
            if (Loader::get()->getLoadedMod("timestepyt.deltarune_textboxes")) {
                qWindow->show();
            }
            else
            if (Loader::get()->getLoadedMod("zalphalaneous.minecraft")) {
                qWindow->show();
            }
            else
            {
                limitNodeSize(qWindow, this->getContentSize(), 800.f, .1f);
                qWindow->setOpacity(0);
                qWindow->setAnchorPoint(CCPointMake(.0f, .5f));
                qWindow->m_mainLayer->runAction(CCEaseElasticOut::create(CCScaleTo::create(.5f, 1.45f), 1.f));
                qWindow->m_mainLayer->setScale(0.f);
                qWindow->m_noElasticity = true;
                qWindow->m_scene = this;
                qWindow->show();
                return true;
            }
        }
        
        if (!ARGON_ALLOWED) {
            argon::clearToken();
            ARGON_TOKEN = "";
            return false;
        }
        if (ARGON_TOKEN.size() > 3) return false;
        
        argon::clearToken();
        
        this->removeChildByID("statusLabel"_spr); //remove prev status label
        auto statusLabel = CCLabelBMFont::create("Authenticating...", "chatFont.fnt");
        statusLabel->setID("statusLabel"_spr);
        statusLabel->setWidth(m_size.width - 60.f);
        statusLabel->setAlignment(kCCTextAlignmentCenter);
        this->addChildAtPosition(statusLabel, Anchor::Bottom, { 0.f, 55.f });
        
        auto ARGON_SERVER = getMod()->getSettingValue<std::string>("ARGON_SERVER");
        if (ARGON_SERVER.size() > 3) argon::setServerUrl(ARGON_SERVER).unwrap();
        
        auto res = argon::startAuth(
            [this, parent = Ref(getParent()), statusLabel = Ref(statusLabel)](Result<std::string> res) {
                
                if (!res) {
                    if (statusLabel) statusLabel->setString(fmt::format("{}", res.unwrapErr()).c_str());
                    log::warn("Auth failed: {}", res.unwrapErr());
                    return;
                }
                
                ARGON_TOKEN = std::move(res).unwrap();
                //log::debug("TOKEN: {}", TOKEN);
                
                recreateMe();
            },
            [statusLabel = Ref(statusLabel)](argon::AuthProgress progress) {
                if (statusLabel) statusLabel->setString(
                    fmt::format("Auth progress: {}", argon::authProgressToString(progress)).c_str()
                );
                log::info("Auth progress: {}", argon::authProgressToString(progress));
            }
        );
        
        if (!res) {
            if (statusLabel) statusLabel->setString(
                fmt::format("Failed to start auth attempt: {}", res.unwrapErr()).c_str()
            );
            log::warn("Failed to start auth attempt: {}", res.unwrapErr());
        }
        
        return true;
    }
    bool init() override {
        if (!CCLayer::init()) return false;
        
        this->setKeyboardEnabled(true);
        this->setContentSize(m_size);
        this->setAnchorPoint(CCPointMake(0.f, 0.f));
        this->setID(m_id);
        
        if ("loading ui") {
            auto blackoutRender = CCRenderTexture::create(m_size.width, m_size.height);
            blackoutRender->getSprite()->setColor(cocos::darken3B(ccWHITE, 120));
            blackoutRender->getSprite()->setAnchorPoint(CCPointMake(0.f, 1.f));
            blackoutRender->getSprite()->setID("blackoutRenderSprite"_spr);
            this->addChildAtPosition(blackoutRender->getSprite(), Anchor::BottomLeft);
            
            auto loadingSpinner = LoadingSpinner::create(42.000f);
            loadingSpinner->setID("loadingSpinner"_spr);
            this->addChildAtPosition(loadingSpinner, Anchor::Center);
            
            this->runAction(CCRepeatForever::create(
                CCSequence::create(CallFuncExt::create(
                    [__this = Ref(this),
                    loadingSpinner = Ref(loadingSpinner),
                    blackoutRender = Ref(blackoutRender)] {
                        if (__this) {
                            auto& req = __this->m_webTaskListener.getFilter();
                            if (blackoutRender and loadingSpinner) {
                                if (!req.isFinished()) {//big deals
                                    blackoutRender->beginWithClear(0, 0, 0, 0);
                                    
                                    //fucking scissor or wht evr
                                    std::map<CCScrollLayerExt*, bool> cutContentsMap;
                                    findFirstChildRecursive<CCScrollLayerExt>(__this,
                                        [&cutContentsMap](CCScrollLayerExt* aw) {
                                            cutContentsMap[aw] = aw->m_cutContent;
                                            aw->m_cutContent = false;
                                            return false;
                                        }
                                    );
                                    
                                    blackoutRender->getSprite()->setVisible(false);
                                    loadingSpinner->setVisible(false);
                                    
                                    __this->visit();
                                    blackoutRender->end();
                                    //setVisible
                                    for (auto& it : cutContentsMap) it.first->m_cutContent = it.second;
                                }
                                loadingSpinner->setVisible(!req.isFinished());
                                blackoutRender->getSprite()->setVisible(!req.isFinished());
                            }
                        }
                    }
                ), CCDelayTime::create(0.1f), nullptr)
            ));
        };
        
        if (auto bg = CCScale9Sprite::create("square02_small.png")) {
            bg->setZOrder(-1);
            bg->setOpacity(75);
            bg->setContentSize(this->getContentSize());
            this->addChildAtPosition(bg, Anchor::Center);
        }
        
        if (auth()) return true;
        
        auto container = CCNode::create();
        if (container) {
            container->setContentSize(this->getContentSize());
            container->setAnchorPoint(CCPointMake(0.f, 0.f));
            container->setID("container"_spr);
            this->addChild(container);
            
            auto inputMenuH = 30.f;
            
            //commentsScroll
            auto commentsScroll = ScrollLayer::create(CCSizeMake(
                this->getContentSize().width, this->getContentSize().height - inputMenuH - 4.f
            ));
            if (commentsScroll) {
                commentsScroll->setScale(0.98f);
                commentsScroll->setPositionY(inputMenuH + 3.f);
                commentsScroll->setAnchorPoint(CCPointMake(0.5f, 0.5f));
                commentsScroll->setID("commentsScroll"_spr);
                container->addChild(commentsScroll);
                loadComments();
            }
            
            //inputMenu
            auto inputMenu = CCMenu::create();
            if (inputMenu) {
                inputMenu->setContentSize(CCSizeMake(
                    this->getContentSize().width, inputMenuH
                ));//inputMenu size!
                inputMenu->setScale(0.98f);
                inputMenu->setPosition(CCPointMake(0.f, 3.f));
                inputMenu->setAnchorPoint(CCPointMake(0.5f, 0.f));
                inputMenu->setID("inputMenu"_spr);
                container->addChild(inputMenu);
                
                auto sendButtonSize = CCSizeMake(inputMenuH, inputMenuH);
                
                auto input = TextInput::create(
                    this->getContentSize().width - sendButtonSize.width - 3.f, 
                    ARGON_ALLOWED ? "Type your comment..." : "", "chatFont.fnt"
                );
                input->setAnchorPoint(CCPointMake(0.f, 0.f));
                input->setTextAlign(TextInputAlign::Left);
                input->setID("input"_spr);
                input->setCommonFilter(CommonFilter::Any);
                input->setMaxCharCount(500);
                inputMenu->addChild(input);
                
                auto sendButtonImage = ButtonSprite::create(CCSprite::create("send.png"_spr));
                sendButtonImage->setContentSize(sendButtonSize);
                sendButtonImage->m_subSprite->setPosition(sendButtonSize / 2);
                sendButtonImage->m_BGSprite->setPosition(sendButtonSize / 2);
                sendButtonImage->m_BGSprite->setContentSize(sendButtonSize);
                sendButtonImage->m_BGSprite->setColor(ccBLACK);
                sendButtonImage->m_BGSprite->setOpacity(input->getBGSprite()->getOpacity());
                
                auto sendButton = CCMenuItemExt::createSpriteExtra(
                    sendButtonImage, [__this = Ref(this), input = Ref(input)](CCMenuItem* sender) {
                        
                        web::WebRequest req = web::WebRequest();
                        req.header("Content-Type", "multipart/form-data");
                        
                        __this->m_webTaskListener.bind(
                            [__this, input, sender = Ref(sender)](web::WebTask::Event* e) {
                                if (!__this) return e->cancel();
                                if (web::WebResponse* res = e->getValue()) {
                                    auto isEdit = __this->querySelector("edit-markup"_spr);
                                    auto parsed = res->string().unwrapOrDefault();
                                    if (string::contains(parsed, isEdit ? "Updated" : "posted")) {
                                        if (isEdit) isEdit->removeFromParent();
                                        if (input) input->setString(""); //BEFORE loadComments
                                        __this->loadComments();
                                    }
                                    else FLAlertLayer::create(
                                        "Something went wrong...",
                                        fmt::format(
                                            "Your comment wasn't {}.\n<cr>{} [code: {}]</c>",
                                            isEdit ? "updated" : "posted",
                                            parsed, res->code()
                                        ),
                                        "OK"
                                    );
                                }
                            }
                        );
                        
                        if (auto edit = __this->querySelector("edit-markup"_spr)) {
                            req.bodyMultipart(web::MultipartForm()
                                .param("update", __this->getID())
                                .param("comment_id", edit->getTag())
                                .param("body", input->getString())
                                .param("account_id", argon::getGameAccountData().accountId)
                                .param("user_id", argon::getGameAccountData().userId)
                                .param("username", argon::getGameAccountData().username)
                                .param("token", ARGON_TOKEN)
                            );
                            return __this->m_webTaskListener.setFilter(req.post(API + std::string("?update")));
                        }
                        
                        req.bodyMultipart(web::MultipartForm()
                            .param("post", __this->getID())
                            .param("body", input->getString())
                            .param("account_id", argon::getGameAccountData().accountId)
                            .param("user_id", argon::getGameAccountData().userId)
                            .param("username", argon::getGameAccountData().username)
                            .param("token", ARGON_TOKEN)
                        );
                        __this->m_webTaskListener.setFilter(req.post(API + std::string("?post")));
                        
                    }
                );
                sendButton->m_scaleMultiplier = sendButton->getScale() * 0.82f;
                sendButton->setPositionX(this->getContentSize().width - (sendButtonSize.width / 2.f));
                sendButton->setPositionY((sendButtonSize.width / 2.f));
                sendButton->setID("sendButton"_spr);
                inputMenu->addChild(sendButton);
                
                auto inpExtImage = CCLabelBMFont::create("|||", "gjFont23.fnt");
                inpExtImage->setScaleX(0.3f);
                inpExtImage->setScaleY(0.375f);
                inpExtImage->setOpacity(80);
                auto inpExt = CCMenuItemExt::createSpriteExtra(
                    inpExtImage, [input = Ref(input)](CCObject* sender) {
                        StupidAsfMultilineMDTextEditor::create(input)->show();
                    }
                );
                inpExt->setPositionX(-12.f + this->getContentSize().width - (sendButtonSize.width));
                inpExt->setPositionY(6.000f);
                inpExt->setRotation(-90.f);// =
                inpExt->setID("inpExt"_spr);
                inputMenu->addChild(inpExt);
                
                if (!ARGON_ALLOWED) {
                    auto enableMeItem = getMod()->getSetting(
                        "ARGON_ALLOWED"
                    )->createNode(this->getContentSize().width);
                    enableMeItem->setID("enableMeItem"_spr);
                    enableMeItem->setScale(1.037f);
                    enableMeItem->ignoreAnchorPointForPosition(true);
                    inputMenu->addChild(enableMeItem);
                    
                    enableMeItem->runAction(CCRepeatForever::create(CCSequence::create(CallFuncExt::create(
                        [this, parent = Ref(getParent()), enableMeItem = Ref(enableMeItem)]() {
                            enableMeItem->commit();
                            if (ARGON_ALLOWED) recreateMe();
                        }
                    ), CCDelayTime::create(0.5f), nullptr)));
                };
            }
        }

        return true;
    }
};

//here goes mod popup ruinify
void hi() {
    new EventListener<EventFilter<ModPopupUIEvent>>(
        +[](ModPopupUIEvent* event) {
            
            //get popup
            auto popup = typeinfo_cast<FLAlertLayer*>(event->getPopup());
            if (!popup) return ListenerResult::Propagate;
            
            auto modID = event->getModID();
            auto modVersion = event->getMod().has_value() ? event->getMod().value()->getVersion().toNonVString() : "";
            
            //fix tabs menu
            CCNode* tabsMenu = popup->getChildByIDRecursive("tabs-menu");
            if (tabsMenu) {
                if (!tabsMenu->getUserObject("pointAndSizeFixMark"_spr)) {
                    tabsMenu->setUserObject("pointAndSizeFixMark"_spr, tabsMenu);
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
            if (!description) return ListenerResult::Propagate;
            auto description_sprite = typeinfo_cast<GeodeTabSprite*>(description->getNormalImage());
            
            CCMenuItemSpriteExtra* changelog = typeinfo_cast<CCMenuItemSpriteExtra*>
                (tabsMenu->getChildByID("changelog"));
            if (!changelog) return ListenerResult::Propagate;
            auto changelog_sprite = typeinfo_cast<GeodeTabSprite*>(changelog->getNormalImage());
            
            auto myTabID = tabsMenu->getChildrenCount();
            
            if (!tabsMenu->getChildByID("comments")) {
                auto tabspr = GeodeTabSprite::create("chat.png"_spr, "Comments", 140.f);
                tabspr->select(0);
                
                auto old_sel = description->m_pfnSelector;
                auto old_lis = description->m_pListener;
                auto onTab = [modID, old_lis, old_sel, myTabID, popup, rcontainer, description_sprite, changelog_sprite, tabspr]
                (CCMenuItemSpriteExtra* sender) {
                    auto textarea = rcontainer->getChildByIDRecursive("textarea");
                    if (!textarea) return;
                    auto pFetchTextArea = textarea->getParent();
                    if (!pFetchTextArea) return;
                    //remove comments layer if exists. for new one or other tabs
                    while (auto a = pFetchTextArea->getChildByType<CommentsLayer>(0)) a->removeFromParent();
                    //descr or changelog
                    if (sender->getTag() < 2) {
                        (old_lis->*old_sel)(sender);
                        //unsel my tab
                        tabspr->select(0);
                        //show descr or changelog
                        textarea->setVisible(1);
                    }
                    //my tab
                    else if (sender->getTag() == myTabID) {
                        //toggle out others
                        description_sprite->select(0);
                        changelog_sprite->select(0);
                        //sel my tab
                        tabspr->select(1);
                        //hide descr or changelog
                        textarea->setVisible(0);
                        //add comments layer really
                        auto comments = CommentsLayer::create(modID, pFetchTextArea->getContentSize());
                        pFetchTextArea->addChild(comments);
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
$execute{ hi(); } //intellisense goes insane in $execute block