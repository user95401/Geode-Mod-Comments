#pragma once
#include <Geode/Geode.hpp>//aaawwwww
using namespace geode::prelude;

//ediror with cool name. requries the TextInput .-.
class StupidAsfMultilineMDTextEditor : public Popup<TextInput*>, TextInputDelegate
{
public:
    bool m_shouldUpdateStrings = false;
    Ref<TextInput> m_pTextInput;

    auto getLineInputs() {
        CCArrayExt<TextInput> rtn;

        auto linesContainer = this->querySelector("linesContainer"_spr);
        if (!linesContainer) return rtn;

        auto lines = CCArrayExt<CCNode>(linesContainer->getChildren());
        for (CCNode* node : lines) {
            auto line = typeinfo_cast<TextInput*>(node);
            if (!line) continue; else rtn.push_back(line);
        }

        return rtn;
    }
    auto getCurrentInputLine() {
        TextInput* rtn = nullptr;
        for (auto line : getLineInputs()) {
            if (line and line->getInputNode()->m_cursor->isVisible())
                return line;
        }
        return rtn;
    };

    void stringsUpdateSch(float dt = 1337.f) {
        if (!m_shouldUpdateStrings and (dt != 1337.f)) return;
        m_shouldUpdateStrings = false;

        auto pMDTextArea = findFirstChildRecursive<MDTextArea>(this, [](CCNode*) { return true; });
        if (!pMDTextArea) return;
        auto linesContainer = this->querySelector("linesContainer"_spr);
        if (!linesContainer) return;
        auto newStr = std::string();

        for (auto line : getLineInputs()) {
            newStr += line->getString() + "\n";
        }

        if (!newStr.empty()) while (newStr.back() == '\n') newStr.pop_back();

        pMDTextArea->setString(newStr.c_str());
        m_pTextInput->setString(newStr.c_str());

        if (string::contains(newStr, "\n")) {
            m_pTextInput->getInputNode()->getTextLabel()->setString(
                ("* " + matjson::Value(newStr).dump()).c_str()
                //make prev label in tar input a bit smaller
                //f.e its replaces newlines to \n
            );
            //add wrapping
            m_pTextInput->getInputNode()->getTextLabel()->setWidth(
                m_pTextInput->getContentSize().width - 4.f
            );
            limitNodeSize(
                m_pTextInput->getInputNode()->getTextLabel(), 
                m_pTextInput->getContentSize(), 1337.f, .1f
            );
        }

        if (!getCurrentInputLine()) getLineInputs()[0]->focus();
    }

    virtual void textChanged(CCTextInputNode* p0) override {
        this->m_shouldUpdateStrings = true;
    };
    virtual void keyDown(enumKeyCodes key) override {
        Popup::keyDown(key);
        //log::debug("{}", CCKeyboardDispatcher::get()->keyToString(key));

        if (key == KEY_Enter) {
            auto lines = getLineInputs(); if (!lines.inner()) return log::error("no lines");
            Ref linesArr = lines.inner(); if (!linesArr) return log::error("no linesArr");
            Ref line = getCurrentInputLine(); if (!line) return;

            auto nextLine = lines[linesArr->indexOfObject(line) + 1];
            if (!nextLine) nextLine = lines[0];
            nextLine->focus();

            auto uCursorPos = line->getInputNode()->m_textField->m_uCursorPos;
            auto currentStr = line->getString();
            auto nextStr = nextLine->getString();

            if (currentStr.empty() || uCursorPos == 0 || uCursorPos > currentStr.length()) void();
            else {
                // move text from current line to next
                nextStr.append(currentStr, uCursorPos);
                // clear current line
                currentStr.resize(uCursorPos);
                // update strings of inputs
                line->setString(currentStr.c_str());
				nextLine->setString(nextStr.c_str());
            };
        }
    }
    void onClose(CCObject* sender) override {
        for (auto line : getLineInputs()) {
            line->defocus();
            line->removeFromParent();
        }
        this->setKeyboardEnabled(false);
        m_pTextInput->focus();
        Popup::onClose(sender);
    }

    bool setup(TextInput*) override {

        this->m_bgSprite->initWithFile("GJ_square05.png");
        this->m_mainLayer->updateLayout();

        auto container = CCLayer::create();
        if (container) {
            container->setScale(0.925f);
            container->setAnchorPoint(CCPointMake(0.9f, 0.5f));
            container->setContentSize(m_mainLayer->getContentSize());
            container->setID("container"_spr);
            m_mainLayer->addChildAtPosition(container, Anchor::BottomLeft, {}, false);

            auto linesContainer = CCNode::create();
            if (linesContainer) {
                linesContainer->setContentSize(container->getContentSize());
                linesContainer->setContentWidth(container->getContentWidth() / 2);
                linesContainer->setAnchorPoint(CCPointMake(0.f, 0.f));
                linesContainer->setID("linesContainer"_spr);
                container->addChild(linesContainer);

                auto inpStrLines = string::split(m_pTextInput->getString(), "\n");

                auto inputheight = 10.f;
                for (int i = 0; i <= 27; i++) {
                    auto input = TextInput::create(linesContainer->getContentWidth(),
                        "", "chatFont.fnt"
                    );
                    if (i < inpStrLines.size()) input->setString(inpStrLines[i]);
                    input->setDelegate(this);
                    input->setID("input_" + std::to_string(i));
                    input->setPositionX(-3.000f);
                    input->setPositionY(linesContainer->getContentHeight() - ((i + 1) * inputheight));
                    input->setAnchorPoint(CCPointMake(0.f, 0.f));

                    input->setContentHeight(inputheight);
                    input->getInputNode()->setContentHeight(inputheight / 1.5f);
                    input->getInputNode()->setPositionY(inputheight / 2.5f);

                    input->getInputNode()->m_cursor->runAction(CCRepeatForever::create(
                        CCSpawn::create(CallFuncExt::create(
                            [m_cursor = Ref(input->getInputNode()->m_cursor)]() {
                                m_cursor->setScale(0.65f);
                                m_cursor->setAnchorPoint(CCPointMake(0.5f, 0.30f));
                            }
                        ), nullptr))
                    );
                    input->getInputNode()->m_cursor->runAction(CCRepeatForever::create(
                        CCSequence::create(
                            CCFadeTo::create(0.1f, 0),    // Исчезновение: 0.1 сек
                            CCDelayTime::create(0.3f),     // Пауза невидимым: 0.3 сек
                            CCFadeTo::create(0.1f, 150),  // Появление: 0.1 сек
                            CCDelayTime::create(0.5f)      // Пауза видимым: 0.5 сек
                            , nullptr
                        )
                    ));

                    input->hideBG();

                    auto line = CCLabelBMFont::create(fmt::format("{}|", i + 1).c_str(), "chatFont.fnt");
                    line->setAnchorPoint(CCPointMake(1.0f, 0.f));
                    line->setPosition(CCPointMake(4.000f, 0.f));
                    line->setScale(0.7f);
                    line->setColor(ccBLACK);
                    line->setOpacity(122);
                    input->addChild(line);

                    auto borderT = CCLayerColor::create();
                    auto borderB = CCLayerColor::create();
                    for (auto border : { borderT, borderB }) {
                        border->setContentWidth(linesContainer->getContentWidth() - 5.f);
                        border->setContentHeight(0.500f);
                        border->setColor(ccBLACK);
                        border->setOpacity(122);
                        border->setPositionX(fabs(input->getPositionX()));
                    }
                    borderT->setPositionY(input->getContentHeight() - 0.f);
                    input->addChild(borderT);
                    borderB->setPositionY(0.f);
                    input->addChild(borderB);

                    input->setTextAlign(TextInputAlign::Left);
                    input->setCommonFilter(CommonFilter::Any);
                    linesContainer->addChild(input);
                }

                auto linesContainerBG = CCScale9Sprite::create("game_bg_13_001.png");
                linesContainerBG->setColor(ccBLACK);
                linesContainerBG->setZOrder(-1);
                linesContainerBG->setOpacity(75);
                linesContainerBG->setContentSize(linesContainer->getContentSize());
                linesContainer->addChildAtPosition(linesContainerBG, Anchor::Center, {}, false);
            };

            auto mdprevContainer = CCNode::create();
            if (mdprevContainer) {
                mdprevContainer->setContentSize(container->getContentSize());
                mdprevContainer->setContentWidth((container->getContentWidth() / 2) + 5.f);
                mdprevContainer->setAnchorPoint(CCPointMake(-0.990, 0.f));
                mdprevContainer->setID("mdprevContainer"_spr);
                container->addChild(mdprevContainer);

                auto md = MDTextArea::create("", mdprevContainer->getContentSize(), true);
                md->setAnchorPoint(CCPointMake(0.f, 0.f));
                mdprevContainer->addChild(md);

                auto mdBG = md->getChildByType<CCScale9Sprite>(0);
                auto oldBGsize = mdBG->getContentSize();
                mdBG->initWithFile("game_bg_13_001.png");
                mdBG->setContentSize(oldBGsize);
                mdBG->setColor(ccBLACK);
            }

            auto borderT = CCLayerColor::create();
            auto borderB = CCLayerColor::create();
            for (auto border : { borderT, borderB }) {
                border->setContentWidth(container->getContentWidth());
                border->setContentHeight(1.000f);
                border->setColor(ccBLACK);
                border->setOpacity(122);
            }
            borderT->setPositionY(container->getContentHeight());
            container->addChild(borderT);
            borderB->setPositionY(0.f);
            container->addChild(borderB);

        }

        this->schedule(
            schedule_selector(StupidAsfMultilineMDTextEditor::stringsUpdateSch)
            , 0.1
        );

        stringsUpdateSch();

        return true;
    }
    static StupidAsfMultilineMDTextEditor* create(TextInput* text) {
        auto ret = new StupidAsfMultilineMDTextEditor();
        ret->m_pTextInput = text; //huh
        if (ret->initAnchored(440.000f, 280.000f, text)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};