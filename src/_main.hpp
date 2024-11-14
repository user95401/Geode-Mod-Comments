#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
using namespace geode::prelude;

#include <regex>

//lol
#define SETTING(type, key_name) Mod::get()->getSettingValue<type>(key_name)

#ifndef __APPLE__ //
#define NOT_APPLE(...) __VA_ARGS__
#else
#define NOT_APPLE(...)
#endif

#define MEMBER_BY_OFFSET(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)
template<typename T, typename U> constexpr size_t OFFSET_BY_MEMBER(U T::* member) { return (char*)&((T*)nullptr->*member) - (char*)nullptr; }

#define public_cast(value, member) [](auto* v) { \
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

namespace fs {
    using namespace std::filesystem;
    inline std::error_code last_err_code;
    template <typename T> inline auto rtnWithErrLog(T rtn, std::string log) { log::error("{}", log); return rtn; }
    inline auto exists(path path) {
        return cocos::fileExistsInSearchPaths(path.string().c_str());
    }
    inline auto read(path path) {
        unsigned long file_size = 0;
        auto buffer = CCFileUtils::sharedFileUtils()->getFileData(path.string().c_str(), "rb", &file_size);
        std::string data = "read failed...";
        if (buffer && file_size != 0) data = std::string(reinterpret_cast<char*>(buffer), file_size);
        return data;
    }
    inline auto rename(path old_path, path new_path) {
        std::filesystem::rename(old_path, new_path, last_err_code);
        log::debug(
            "{}(\n\told_path \"{}\", \n\told_path \"{}\"\n): last_err_code={}, last_err_code.message={}",
            __func__, old_path, new_path, last_err_code, last_err_code.message()
        );
        return true;
    }
}

namespace geode::cocos {
    inline std::string getFrameName(CCNode* node) {
        if (node == nullptr) return "NIL_NODE";
        if (auto textureProtocol = typeinfo_cast<CCTextureProtocol*>(node)) {
            if (auto texture = textureProtocol->getTexture()) {
                if (auto spriteNode = typeinfo_cast<CCSprite*>(node)) {
                    auto* cachedFrames = CCSpriteFrameCache::sharedSpriteFrameCache()->m_pSpriteFrames;
                    const auto rect = spriteNode->getTextureRect();
                    for (auto [key, frame] : CCDictionaryExt<std::string, CCSpriteFrame*>(cachedFrames)) {
                        if (frame->getTexture() == texture && frame->getRect() == rect) {
                            return key.c_str();
                        }
                    }
                }
                auto* cachedTextures = CCTextureCache::sharedTextureCache()->m_pTextures;
                for (auto [key, obj] : CCDictionaryExt<std::string, CCTexture2D*>(cachedTextures)) {
                    if (obj == texture) {
                        return key.c_str();
                    }
                }
            }
        }
        auto btnSpriteTry = getFrameName(getChild(node, 0));
        if (
            btnSpriteTry != "NIL_NODE"
            and btnSpriteTry != "CANT_GET_FRAME_NAME"
            ) return btnSpriteTry;
        return "CANT_GET_FRAME_NAME";
    }
    inline std::string getTypeName(CCObject* object) {
#ifdef GEODE_IS_WINDOWS
        return typeid(*object).name() + 6;
#else 
        {
            std::string ret;

            int status = 0;
            auto demangle = abi::__cxa_demangle(typeid(*object).name(), 0, 0, &status);
            if (status == 0) {
                ret = demangle;
            }
            free(demangle);

            return ret;
        }
#endif
    }
    inline std::string idOrTypeOfNode(cocos2d::CCNode* node) {
        if (!node) return "NIL NODE";
        auto id = node->getID();
        auto type = getTypeName(node);
        return (id.size() > 1 ? id : type);
    }
    inline std::vector<std::string> getIdsTreeUpToNode(cocos2d::CCNode* start, cocos2d::CCNode* up_to) {
        auto rtn = std::vector<std::string>();
        if (start == nullptr) return rtn;
        //add start
        rtn.insert(rtn.begin(), idOrTypeOfNode(start));
        //add next parents
        auto next_parent = start->getParent();
        while (next_parent != nullptr) {
            rtn.insert(rtn.begin(), idOrTypeOfNode(next_parent));
            next_parent = next_parent->getParent();
            if (up_to == next_parent) next_parent = nullptr;
        }
        //rtn rly
        return rtn;
    }
    inline auto createDataNode(std::string id, std::string text = "", int tag = 0) {
        auto node = CCLabelBMFont::create("", "chatFont.fnt");
        node->setID(id);
        node->setString(text.c_str());
        if (tag != 0) node->setTag(tag);
        node->setVisible(0);
        return node;
    }
    inline auto findDataNode(CCNode* parent, std::string id) {
        auto node = typeinfo_cast<CCLabelBMFont*>(parent->getChildByIDRecursive(id));
        if (!node) log::warn("FAILED TO FIND DATA NODE! id: {}", id);
        return node;
    }
    //add this child to some node. node->addChild(LambdaNode::createToCallOnce([]() {}));
    class LambdaNode : public CCSprite {
    public:
        std::function<void()> m_callback;
        bool endless = false;
        float delay = false;
        LambdaNode() {};
        ~LambdaNode() {};
        void exec(float) {
            //log::error("{}->{}", this, __FUNCTION__);
            if (this->getParent() == nullptr) return;
            if (this->isVisible() == false) return NOT_APPLE(!endless ? scheduleOnce(schedule_selector(LambdaNode::exec), delay) : )void();
            if (!endless) this->removeFromParent();
            m_callback();
        };
        static LambdaNode* create(std::function<void()>&& callback) {
            auto ret = new (std::nothrow) LambdaNode();
            if (ret and ret->init()) {
                ret->m_callback = std::forward<std::remove_reference_t<decltype(callback)>>(callback);
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        };
        static auto createToCallOnce(std::function<void()>&& callback, float delay = 0.f) {
            auto rtn = LambdaNode::create(
                std::forward<std::remove_reference_t<decltype(callback)>>(callback)
            );
            rtn->delay = delay;
            rtn->scheduleOnce(schedule_selector(LambdaNode::exec), rtn->delay);
            return rtn;
        };
        static auto createToEndlessCalls(std::function<void()>&& callback, float interval = 0.f) {
            auto rtn = LambdaNode::create(
                std::forward<std::remove_reference_t<decltype(callback)>>(callback)
            );
            rtn->endless = 1;
            rtn->delay = interval;
            rtn->schedule(schedule_selector(LambdaNode::exec), interval);
            return rtn;
        };
    };
#ifndef __APPLE__
    class CCLambdaAction : public CCActionInterval {
    public:
        std::function<void()> m_callback;
        CCLambdaAction() {};
        ~CCLambdaAction() {};
        void update(float time) override {
            m_callback();
        };
        static CCLambdaAction* create(std::function<void()>&& callback) {
            auto ret = new (std::nothrow) CCLambdaAction();
            if (ret) {
                ret->m_callback = std::forward<std::remove_reference_t<decltype(callback)>>(callback);
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        };
    };
#endif
    void setTouchPriority(CCNode* node, int priority) {
#ifndef __APPLE__ //#endif
        if (auto delegate = typeinfo_cast<CCTouchDelegate*>(node)) {
            if (auto handler = CCTouchDispatcher::get()->findHandler(delegate)) {
                CCTouchDispatcher::get()->setPriority(priority, handler->getDelegate());
            }
        }
#endif
    }
};

namespace geode::utils::string {
    inline std::vector<std::string> explode(std::string separator, std::string input) {
        std::vector<std::string> vec;
        for (int i{ 0 }; i < input.length(); ++i) {
            int pos = input.find(separator, i);
            if (pos < 0) { vec.push_back(input.substr(i)); break; }
            int count = pos - i;
            vec.push_back(input.substr(i, count));
            i = pos + separator.length() - 1;
        }
        if (vec.size() == 0) vec.push_back(input);
        return vec;
    }
}

#include  <random>
#include  <iterator>
namespace geode::utils {
    template<typename Iter, typename RandomGenerator>
    Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
        std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
        std::advance(start, dis(g));
        return start;
    };
    template<typename Iter>
    Iter select_randomly(Iter start, Iter end) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        return select_randomly(start, end, gen);
    }
    bool rndb(int variants = 2) {
        auto varsVec = std::vector<bool>();
        auto tempb = true;
        auto tempvariants = variants;
        while (tempvariants > 0) {
            tempb = !tempb;
            tempvariants = tempvariants - 1;
            varsVec.push_back(tempb);
        }
        auto rtn = *select_randomly(varsVec.begin(), varsVec.end());
        //log::debug("{}({}) = {} of {}", __func__, variants, rtn, varsVec);
        return rtn;
    }
};

struct GJScoreKey {
    inline static auto TotalJumps = "1";
    inline static auto TotalAttempts = "2";
    inline static auto CompletedLevels = "3";
    inline static auto CompletedOnlineLevels = "4";
    inline static auto Demons = "5";
    inline static auto Stars = "6";
    inline static auto CompletedMapPacksCount = "7";
    inline static auto GoldCoins = "8";
    inline static auto PlayersDestroyed = "9";
    inline static auto LikedLevelsCount = "10";
    inline static auto RatedLevelsCount = "11";
    inline static auto UserCoins = "12";
    inline static auto Diamonds = "13";
    inline static auto CurrentOrbs = "14";
    inline static auto DailyCompletionCount = "15";
    inline static auto FireShards = "16";
    inline static auto IceShards = "17";
    inline static auto PoisonShards = "18";
    inline static auto ShadowShards = "19";
    inline static auto LavaShards = "20";
    inline static auto DemonKeys = "21";
    inline static auto TotalOrbs = "22";
    inline static auto EarthShards = "23";
    inline static auto BloodShards = "24";
    inline static auto MetalShards = "25";
    inline static auto LightShards = "26";
    inline static auto SoulShards = "27";
    inline static auto Moons = "28";
    inline static auto DiamondShards = "29";
    inline static auto FirePathStarProgress = "30";
    inline static auto IcePathStarProgress = "31";
    inline static auto PoisonPathStarProgress = "32";
    inline static auto ShadowPathStarProgress = "33";
    inline static auto LavaPathStarProgress = "34";
    inline static auto EarthPathStarProgress = "35";
    inline static auto BloodPathStarProgress = "36";
    inline static auto MetalPathStarProgress = "37";
    inline static auto LightPathStarProgress = "38";
    inline static auto SoulPathStarProgress = "39";
    inline static auto CompletedGauntlets = "40";
    inline static auto ListRewardsCollected = "41";
};

#ifdef GEODE_IS_ANDROID
//#define debug error
#endif // GEODE_IS_ANDROID