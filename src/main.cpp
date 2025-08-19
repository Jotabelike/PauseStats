#include <cvolton.level-id-api/include/EditorIDs.hpp>
#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/cocos.hpp>
using namespace geode::prelude;

// Espaciado configurable y soporte horizontal
float getDeltaY() { return Mod::get()->getSettingValue<float>("stats_spacing"); }
bool isHorizontal() { return Mod::get()->getSettingValue<bool>("stats_horizontal"); }

void loadCompletions(const std::string& key, int& completions) {
    completions = Mod::get()->getSavedValue<int>(key, 0);
}
void saveCompletions(const std::string& key, int completions) {
    Mod::get()->setSavedValue<int>(key, completions);
}
float getStatsX() { return Mod::get()->getSettingValue<float>("stats_x"); }
float getStatsY() { return Mod::get()->getSettingValue<float>("stats_y"); }

int getObjectCount(GJGameLevel* level, PlayLayer* playLayer) {
    if (playLayer && playLayer->m_objects && playLayer->m_objects->count() > 0)
        return playLayer->m_objects->count();
    if (level) return level->m_objectCount;
    return 0;
}

// Cuenta checkpoints de plataforma (objeto ID 2063) en el nivel
int getPlatformerCheckpointCount(PlayLayer* playLayer) {
    if (!playLayer || !playLayer->m_objects) return 0;
    int count = 0;
    auto arr = CCArrayExt<GameObject*>(playLayer->m_objects);
    for (auto obj : arr) {
        if (obj && obj->m_objectID == 2063) { // 2063 = Platformer Checkpoint (GD 2.2064+)
            count++;
        }
    }
    return count;
}

// Detecta el dispositivo y lo muestra como PC, Android o iOS
std::string getDeviceString() {
    switch (CCApplication::sharedApplication()->getTargetPlatform()) {
    case kTargetIphone:
    case kTargetIpad:
        return "iOS";
    case kTargetAndroid:
        return "Android";
    default:
        return "PC";
    }
}

class $modify(PlayLayerHook, PlayLayer) {
    struct Fields { bool completedNormal = false; bool completedPractice = false; };

    void resetLevel() {
        m_fields->completedNormal = false;
        m_fields->completedPractice = false;
        PlayLayer::resetLevel();
    }

    void update(float dt) {
        PlayLayer::update(dt);
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        auto level = m_level;
        if (!level) return;
        std::string baseKey = fmt::format("level_completions_{}", level->m_levelID.value());
        if (level->m_levelType == GJLevelType::Editor) baseKey = fmt::format("{}_editor_{}", baseKey, EditorIDs::getID(level));
        if (m_isPracticeMode) {
            if (!m_fields->completedPractice) {
                m_fields->completedPractice = true;
                std::string practiceKey = baseKey + "_practice";
                int completions = Mod::get()->getSavedValue<int>(practiceKey, 0);
                completions++;
                Mod::get()->setSavedValue<int>(practiceKey, completions);
            }
        }
        else {
            if (!m_fields->completedNormal) {
                m_fields->completedNormal = true;
                std::string normalKey = baseKey + "_normal";
                int completions = Mod::get()->getSavedValue<int>(normalKey, 0);
                completions++;
                Mod::get()->setSavedValue<int>(normalKey, completions);
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
        bool horizontal = isHorizontal();

        bool showAttempts = Mod::get()->getSettingValue<bool>("show_attempts");
        bool showJumps = Mod::get()->getSettingValue<bool>("show_jumps");
        bool showCompletions = Mod::get()->getSettingValue<bool>("show_completions");
        bool showObjectCount = Mod::get()->getSettingValue<bool>("show_object_count");
        bool showDevice = Mod::get()->getSettingValue<bool>("show_device");
        bool showGamemode = Mod::get()->getSettingValue<bool>("show_gamemode");
        bool showLevelID = Mod::get()->getSettingValue<bool>("show_level_id");
        bool showSongID = Mod::get()->getSettingValue<bool>("show_song_id");
        bool showCheckpoints = Mod::get()->getSettingValue<bool>("show_checkpoints");

        // Limpiar stats anteriores
        for (auto lbl : m_fields->statLabels) if (lbl) lbl->removeFromParent();
        for (auto lbl : m_fields->statValues) if (lbl) lbl->removeFromParent();
        for (auto lbl : m_fields->idValuesLabels) if (lbl) lbl->removeFromParent();
        m_fields->statLabels.clear();
        m_fields->statValues.clear();
        m_fields->idValuesLabels.clear();

        // Layout inicial
        float y = statsY;
        float x = statsX;
        int statIndex = 0;

        auto nextPos = [&](int index) {
            if (horizontal) {
                return CCPoint{ x + index * deltaY * statsScale, y };
            }
            else {
                return CCPoint{ x, y - index * deltaY * statsScale };
            }
            };

        if (showAttempts) {
            int totalAttempts = level->m_attempts;
            auto lbl = CCLabelBMFont::create("Attempts", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition(nextPos(statIndex));
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(fmt::format("{}", totalAttempts).c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            auto valuePos = nextPos(statIndex);
            valuePos.y -= 13.0f * statsScale;
            val->setPosition(valuePos);
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            ++statIndex;
        }
        if (showJumps) {
            int totalJumps = level->m_jumps;
            auto lbl = CCLabelBMFont::create("Jumps", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition(nextPos(statIndex));
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(fmt::format("{}", totalJumps).c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            auto valuePos = nextPos(statIndex);
            valuePos.y -= 13.0f * statsScale;
            val->setPosition(valuePos);
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            ++statIndex;
        }
        if (showCompletions) {
            std::string baseKey = fmt::format("level_completions_{}", level->m_levelID.value());
            std::string normalKey = baseKey + "_normal";
            std::string practiceKey = baseKey + "_practice";
            int normal = 0, practice = 0;
            loadCompletions(normalKey, normal);
            loadCompletions(practiceKey, practice);

            auto lbl = CCLabelBMFont::create("Completed", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition(nextPos(statIndex));
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(fmt::format("{} | P:{}", normal, practice).c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            auto valuePos = nextPos(statIndex);
            valuePos.y -= 13.0f * statsScale;
            val->setPosition(valuePos);
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            ++statIndex;
        }
        if (showObjectCount) {
            int objectCount = getObjectCount(level, playLayer);

            auto lbl = CCLabelBMFont::create("All Objs", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition(nextPos(statIndex));
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(fmt::format("{}", objectCount).c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            auto valuePos = nextPos(statIndex);
            valuePos.y -= 13.0f * statsScale;
            val->setPosition(valuePos);
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            ++statIndex;
        }
        if (showCheckpoints) {
            std::string label = "Checkpoints";
            std::string value = "NA";
            if (playLayer->m_isPlatformer) {
                int checkpointCount = getPlatformerCheckpointCount(playLayer);
                value = fmt::format("{}", checkpointCount);
            }
            auto lbl = CCLabelBMFont::create(label.c_str(), "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition(nextPos(statIndex));
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(value.c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            auto valuePos = nextPos(statIndex);
            valuePos.y -= 13.0f * statsScale;
            val->setPosition(valuePos);
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            ++statIndex;
        }
        if (showDevice) {
            std::string device = getDeviceString();
            auto lbl = CCLabelBMFont::create("DEVICE", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition(nextPos(statIndex));
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(device.c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            auto valuePos = nextPos(statIndex);
            valuePos.y -= 13.0f * statsScale;
            val->setPosition(valuePos);
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            ++statIndex;
        }
        if (showGamemode) {
            std::string gamemode = playLayer->m_isPlatformer ? "Plat" : "Classic";
            auto lbl = CCLabelBMFont::create("Gamemode", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition(nextPos(statIndex));
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(gamemode.c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 255,255,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            auto valuePos = nextPos(statIndex);
            valuePos.y -= 13.0f * statsScale;
            val->setPosition(valuePos);
            addChild(val, 100);
            m_fields->statValues.push_back(val);

            ++statIndex;
        }
        if (showLevelID) {
            std::string levelID = fmt::format("{}", level->m_levelID.value());
            auto lbl = CCLabelBMFont::create("LEVEL ID", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition(nextPos(statIndex));
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(levelID.c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 100,220,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            auto valuePos = nextPos(statIndex);
            valuePos.y -= 13.0f * statsScale;
            val->setPosition(valuePos);
            addChild(val, 100);
            m_fields->idValuesLabels.push_back(val);

            ++statIndex;
        }
        if (showSongID) {
            std::string songID = level->m_songID ? fmt::format("{}", level->m_songID) : "0";
            auto lbl = CCLabelBMFont::create("SONG ID", "goldFont.fnt");
            lbl->setScale(0.33f * statsScale);
            lbl->setColor({ 200,200,200 });
            lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            lbl->setPosition(nextPos(statIndex));
            addChild(lbl, 100);
            m_fields->statLabels.push_back(lbl);

            auto val = CCLabelBMFont::create(songID.c_str(), "bigFont.fnt");
            val->setScale(0.44f * statsScale);
            val->setColor({ 100,220,255 });
            val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
            auto valuePos = nextPos(statIndex);
            valuePos.y -= 13.0f * statsScale;
            val->setPosition(valuePos);
            addChild(val, 100);
            m_fields->idValuesLabels.push_back(val);

            ++statIndex;
        }
    }

    void moveStatsTo(float newX, float newY) {
        float statsScale = Mod::get()->getSettingValue<float>("stats_scale");
        float deltaY = getDeltaY();
        bool horizontal = isHorizontal();
        float x = newX, y = newY;
        int statIndex = 0, valueIndex = 0;

        auto nextPos = [&](int index) {
            if (horizontal) {
                return CCPoint{ x + index * deltaY * statsScale, y };
            }
            else {
                return CCPoint{ x, y - index * deltaY * statsScale };
            }
            };

        for (; statIndex < m_fields->statLabels.size(); ++statIndex) {
            if (m_fields->statLabels[statIndex])
                m_fields->statLabels[statIndex]->setPosition(nextPos(statIndex));
            if (valueIndex < m_fields->statValues.size() && m_fields->statValues[valueIndex]) {
                auto valuePos = nextPos(statIndex);
                valuePos.y -= 13.0f * statsScale;
                m_fields->statValues[valueIndex++]->setPosition(valuePos);
            }
        }
        int idIndex = 0;
        for (auto lbl : m_fields->idValuesLabels) {
            if (lbl) {
                auto valuePos = nextPos(statIndex + idIndex);
                valuePos.y -= 13.0f * statsScale;
                lbl->setPosition(valuePos);
                idIndex++;
            }
        }
    }
};