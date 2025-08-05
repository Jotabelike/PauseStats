#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/cocos.hpp>
using namespace geode::prelude;

// Ahora el espaciado es configurable por setting
float getDeltaY() { return Mod::get()->getSettingValue<float>("stats_spacing"); }

void loadCompletions(const std::string& key, int& completions) {
    completions = Mod::get()->getSavedValue<int>("level_completions_" + key, 0);
}
void saveCompletions(const std::string& key, int completions) {
    Mod::get()->setSavedValue<int>("level_completions_" + key, completions);
}
std::string findDevice() {
    std::string device = "PC";
    #ifdef GEODE_IS_MACOS
    device = "macOS";
    #endif
    switch (CCApplication::sharedApplication()->getTargetPlatform()) {
        case kTargetIphone: return "iOS";
        case kTargetIpad: return "iPadOS";
        case kTargetAndroid: return "Android";
        default: return device;
    }
}
float getStatsX() { return Mod::get()->getSettingValue<float>("stats_x"); }
float getStatsY() { return Mod::get()->getSettingValue<float>("stats_y"); }

class $modify(PlayLayerHook, PlayLayer) {
    struct Fields { bool completed = false; };

    void resetLevel() {
        m_fields->completed = false;
        PlayLayer::resetLevel();
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        if (!m_fields->completed) {
            m_fields->completed = true;
            auto level = m_level;
            if (level) {
                std::string levelKey = fmt::format("{}", level->m_levelID.value());
                int completions = Mod::get()->getSavedValue<int>("level_completions_" + levelKey, 0);
                completions++;
                Mod::get()->setSavedValue<int>("level_completions_" + levelKey, completions);
            }
        }
    }
};

class $modify(PauseLayerHook, PauseLayer) {
    struct Fields {
        std::vector<CCLabelBMFont*> statLabels;
        std::vector<CCLabelBMFont*> statValues;
        std::vector<CCLabelBMFont*> idValuesLabels;
    };

    void customSetup() override {
        PauseLayer::customSetup();

        auto playLayer = GameManager::sharedState()->getPlayLayer();
        if (!playLayer) return;
        auto level = playLayer->m_level;
        if (!level) return;

        float textOpacity = Mod::get()->getSettingValue<float>("text_opacity");
        float statsScale = Mod::get()->getSettingValue<float>("stats_scale");
        float statsX = getStatsX();
        float statsY = getStatsY();
        float deltaY = getDeltaY();

        bool showAttempts = Mod::get()->getSettingValue<bool>("show_attempts");
        bool showJumps = Mod::get()->getSettingValue<bool>("show_jumps");
        bool showCompletions = Mod::get()->getSettingValue<bool>("show_completions");
        bool showObjectCount = Mod::get()->getSettingValue<bool>("show_object_count");
        bool showDevice = Mod::get()->getSettingValue<bool>("show_device");
        bool showGamemode = Mod::get()->getSettingValue<bool>("show_gamemode");
        bool showLevelID = Mod::get()->getSettingValue<bool>("show_level_id");
        bool showSongID = Mod::get()->getSettingValue<bool>("show_song_id");

        float y = statsY;

        for (auto lbl : m_fields->statLabels) if (lbl) lbl->removeFromParent();
        for (auto lbl : m_fields->statValues) if (lbl) lbl->removeFromParent();
        for (auto lbl : m_fields->idValuesLabels) if (lbl) lbl->removeFromParent();
        m_fields->statLabels.clear();
        m_fields->statValues.clear();
        m_fields->idValuesLabels.clear();

        if (showAttempts) {
            int totalAttempts = level->m_attempts;
            auto lbl = CCLabelBMFont::create("Attempts", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition({ statsX, y });
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(fmt::format("{}", totalAttempts).c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            val->setPosition({ statsX, y - 13.0f * statsScale });
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            y -= deltaY * statsScale;
        }
        if (showJumps) {
            int totalJumps = level->m_jumps;
            auto lbl = CCLabelBMFont::create("Jumps", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition({ statsX, y });
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(fmt::format("{}", totalJumps).c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            val->setPosition({ statsX, y - 13.0f * statsScale });
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            y -= deltaY * statsScale;
        }
        if (showCompletions) {
            std::string levelKey = fmt::format("{}", level->m_levelID.value());
            int completions = 0; loadCompletions(levelKey, completions);

            auto lbl = CCLabelBMFont::create("Completed", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition({ statsX, y });
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(fmt::format("{}", completions).c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            val->setPosition({ statsX, y - 13.0f * statsScale });
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            y -= deltaY * statsScale;
        }
        if (showObjectCount) {
            int objectCount = level->m_objectCount;
            auto lbl = CCLabelBMFont::create("All Objs", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition({ statsX, y });
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(fmt::format("{}", objectCount).c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            val->setPosition({ statsX, y - 13.0f * statsScale });
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            y -= deltaY * statsScale;
        }
        if (showDevice) {
            std::string device = findDevice();
            auto lbl = CCLabelBMFont::create("DEVICE", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition({ statsX, y });
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(device.c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            val->setPosition({ statsX, y - 13.0f * statsScale });
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            y -= deltaY * statsScale;
        }
        if (showGamemode) {
            std::string gamemode = playLayer->m_isPlatformer ? "Plat" : "Classic";
            auto lbl = CCLabelBMFont::create("Gamemode", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition({ statsX, y });
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(gamemode.c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            val->setPosition({ statsX, y - 13.0f * statsScale });
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            y -= deltaY * statsScale;
        }
        if (showLevelID) {
            std::string levelID = fmt::format("{}", level->m_levelID.value());
            auto lbl = CCLabelBMFont::create("LEVEL ID", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition({ statsX, y });
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(levelID.c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 100,220,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            val->setPosition({ statsX, y - 13.0f * statsScale });
            addChild(val, 100);
            m_fields->idValuesLabels.push_back(val);

            y -= deltaY * statsScale;
        }
        if (showSongID) {
            std::string songID = level->m_songID ? fmt::format("{}", level->m_songID) : "0";
            auto lbl = CCLabelBMFont::create("SONG ID", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition({ statsX, y });
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(songID.c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 100,220,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            val->setPosition({ statsX, y - 13.0f * statsScale });
            addChild(val, 100);
            m_fields->idValuesLabels.push_back(val);

            y -= deltaY * statsScale;
        }
    }

    void moveStatsTo(float newX, float newY) {
        float statsScale = Mod::get()->getSettingValue<float>("stats_scale");
        float deltaY = getDeltaY();
        float y = newY;
        size_t statIdx = 0, valueIdx = 0, idIdx = 0;
        for (; statIdx < m_fields->statLabels.size(); ++statIdx) {
            if (m_fields->statLabels[statIdx])
                m_fields->statLabels[statIdx]->setPosition({ newX, y });
            if (valueIdx < m_fields->statValues.size() && m_fields->statValues[valueIdx])
                m_fields->statValues[valueIdx++]->setPosition({ newX, y - 13.0f * statsScale });
            y -= deltaY * statsScale;
        }
        for (auto lbl : m_fields->idValuesLabels) {
            if (lbl) lbl->setPosition({ newX, y - 13.0f * statsScale });
            y -= deltaY * statsScale;
        }
    }
};