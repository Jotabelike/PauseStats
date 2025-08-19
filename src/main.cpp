#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/cocos.hpp>
#include <functional>
#include <algorithm>

using namespace geode::prelude;

// -------------------- Ajustes / Helpers de UI --------------------
float getDeltaY() { return Mod::get()->getSettingValue<float>("stats_spacing"); }
bool isHorizontal() { return Mod::get()->getSettingValue<bool>("stats_horizontal"); }
float getStatsX() { return Mod::get()->getSettingValue<float>("stats_x"); }
float getStatsY() { return Mod::get()->getSettingValue<float>("stats_y"); }

// -------------------- Helpers de normalización --------------------
static int clampAndFix(const std::string& key, int defaultIfMissing = 0) {
    int v = Mod::get()->getSavedValue<int>(key, defaultIfMissing);
    if (v < 0) {
        v = 0;
        Mod::get()->setSavedValue<int>(key, 0);
    }
    return v;
}

// -------------------- Persistencia de completions ----------------
void loadCompletions(const std::string& key, int& completions) {
    completions = clampAndFix(key, 0);
}
void saveCompletions(const std::string& key, int completions) {
    if (completions < 0) completions = 0;
    Mod::get()->setSavedValue<int>(key, completions);
}

// -------------------- Conteos --------------------
int getObjectCount(GJGameLevel* level, PlayLayer* playLayer) {
    if (playLayer && playLayer->m_objects && playLayer->m_objects->count() > 0)
        return playLayer->m_objects->count();
    if (level) return level->m_objectCount;
    return 0;
}

// Cuenta checkpoints de plataforma (objeto ID 2063)
int getPlatformerCheckpointCount(PlayLayer* playLayer) {
    if (!playLayer || !playLayer->m_objects) return 0;
    int count = 0;
    auto arr = CCArrayExt<GameObject*>(playLayer->m_objects);
    for (auto obj : arr) {
        if (obj && obj->m_objectID == 2063) {
            count++;
        }
    }
    return count;
}

// -------------------- Device string usando Geode --------------------
std::string getDeviceString() {
    // GEODE_PLATFORM_NAME ya es un nombre humano (Windows, macOS, Android, iOS, etc.)
    return GEODE_PLATFORM_NAME;
}

// -------------------- Normalizador de nombre --------------------
static std::string normalizeName(const std::string& in) {
    std::string s = in;
    if (s.empty()) s = "noname";
    for (char& c : s) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            c = '_';
        else if (c == ':' || c == '/' || c == '\\' || c == '|')
            c = '-';
    }
    return s;
}

// Genera hash base para niveles locales (id <= 0)
static std::string generateLocalBase(GJGameLevel* level) {
    std::string name = level->m_levelName.empty() ? "noname" : level->m_levelName;
    std::string norm = normalizeName(name);
    const std::string& ls = level->m_levelString;
    std::string prefix = ls.substr(0, std::min<size_t>(64, ls.size()));
    size_t h = std::hash<std::string>{}(norm + "|" + std::to_string(ls.size()) + "|" + prefix);
    return fmt::format("level_completions_local_{:016x}", (unsigned long long)h);
}

// Sin variantes: clave estable
static std::string computeLevelStatsKey(GJGameLevel* level) {
    if (!level) return "level_completions_invalid";
    int id = level->m_levelID.value();
    if (id > 0)
        return fmt::format("level_completions_{}", id);
    return generateLocalBase(level);
}

// -------------------- PlayLayer Hook --------------------
class $modify(PlayLayerHook, PlayLayer) {
    struct Fields {
        bool completedNormal = false;
        bool completedPractice = false;
        std::string cachedBaseKey;
    };

    void resetLevel() {
        m_fields->completedNormal = false;
        m_fields->completedPractice = false;
        PlayLayer::resetLevel();
    }

    void onEnter() {
        PlayLayer::onEnter();
        if (m_level) {
            m_fields->cachedBaseKey = computeLevelStatsKey(m_level);
            log::info("PlayLayer baseKey = {}", m_fields->cachedBaseKey);
        }
    }

    void levelComplete() {
        auto level = m_level;
        if (level) {
            if (m_fields->cachedBaseKey.empty())
                m_fields->cachedBaseKey = computeLevelStatsKey(level);

            std::string baseKey = m_fields->cachedBaseKey;

            if (m_isPracticeMode) {
                if (!m_fields->completedPractice) {
                    m_fields->completedPractice = true;
                    std::string practiceKey = baseKey + "_practice";
                    int completions = Mod::get()->getSavedValue<int>(practiceKey, 0);
                    if (completions < 0) { completions = 0; Mod::get()->setSavedValue<int>(practiceKey, 0); }
                    Mod::get()->setSavedValue<int>(practiceKey, ++completions);
                    log::info("Increment practice -> {} ({})", completions, practiceKey);
                }
            }
            else {
                if (!m_fields->completedNormal) {
                    m_fields->completedNormal = true;
                    std::string normalKey = baseKey + "_normal";
                    int completions = Mod::get()->getSavedValue<int>(normalKey, 0);
                    if (completions < 0) { completions = 0; Mod::get()->setSavedValue<int>(normalKey, 0); }
                    Mod::get()->setSavedValue<int>(normalKey, ++completions);
                    log::info("Increment normal -> {} ({})", completions, normalKey);
                }
            }
        }
        PlayLayer::levelComplete();
    }
};

// -------------------- PauseLayer Hook --------------------
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

        // Limpiar labels existentes
        for (auto lbl : m_fields->statLabels)     if (lbl) lbl->removeFromParent();
        for (auto lbl : m_fields->statValues)     if (lbl) lbl->removeFromParent();
        for (auto lbl : m_fields->idValuesLabels) if (lbl) lbl->removeFromParent();
        m_fields->statLabels.clear();
        m_fields->statValues.clear();
        m_fields->idValuesLabels.clear();

        float y = statsY;
        float x = statsX;
        int statIndex = 0;

        auto nextPos = [&](int index) {
            if (horizontal)
                return CCPoint{ x + index * deltaY * statsScale, y };
            return CCPoint{ x, y - index * deltaY * statsScale };
            };

        auto addStat = [&](const std::string& title, const std::string& value,
            ccColor3B valueColor = { 255,255,255 }, bool idLabel = false) {
                auto lbl = CCLabelBMFont::create(title.c_str(), "goldFont.fnt");
                lbl->setScale(0.33f * statsScale);
                lbl->setColor({ 200,200,200 });
                lbl->setOpacity(static_cast<GLubyte>(textOpacity * 255));
                lbl->setPosition(nextPos(statIndex));
                addChild(lbl, 100);
                m_fields->statLabels.push_back(lbl);

                auto val = CCLabelBMFont::create(value.c_str(), "bigFont.fnt");
                val->setScale(0.44f * statsScale);
                val->setColor(valueColor);
                val->setOpacity(static_cast<GLubyte>(textOpacity * 255));
                auto valuePos = nextPos(statIndex);
                valuePos.y -= 13.0f * statsScale;
                val->setPosition(valuePos);
                addChild(val, 100);
                if (idLabel)
                    m_fields->idValuesLabels.push_back(val);
                else
                    m_fields->statValues.push_back(val);

                ++statIndex;
            };

        if (showAttempts)
            addStat("Attempts", fmt::format("{}", level->m_attempts));
        if (showJumps)
            addStat("Jumps", fmt::format("{}", level->m_jumps));
        if (showCompletions) {
            std::string baseKey = computeLevelStatsKey(level);
            int normal = clampAndFix(baseKey + "_normal", 0);
            int practice = clampAndFix(baseKey + "_practice", 0);
            addStat("Completed", fmt::format("{} | P:{}", normal, practice));
            log::info("Pause baseKey={} normal={} practice={} attempts={}", baseKey, normal, practice, level->m_attempts);
        }
        if (showObjectCount) {
            int objectCount = getObjectCount(level, playLayer);
            addStat("All Objs", fmt::format("{}", objectCount));
        }
        if (showCheckpoints) {
            std::string value = "NA";
            if (playLayer->m_isPlatformer)
                value = fmt::format("{}", getPlatformerCheckpointCount(playLayer));
            addStat("Checkpoints", value);
        }
        if (showDevice)
            addStat("DEVICE", getDeviceString());
        if (showGamemode)
            addStat("Gamemode", playLayer->m_isPlatformer ? "Plat" : "Classic");
        if (showLevelID)
            addStat("LEVEL ID", fmt::format("{}", level->m_levelID.value()), { 100,220,255 }, true);
        if (showSongID) {
            std::string songID = level->m_songID ? fmt::format("{}", level->m_songID) : "0";
            addStat("SONG ID", songID, { 100,220,255 }, true);
        }
    }

    void moveStatsTo(float newX, float newY) {
        float statsScale = Mod::get()->getSettingValue<float>("stats_scale");
        float deltaY = getDeltaY();
        bool horizontal = isHorizontal();
        float x = newX, y = newY;
        int statIndex = 0;
        int valueIndex = 0;

        auto nextPos = [&](int index) {
            if (horizontal)
                return CCPoint{ x + index * deltaY * statsScale, y };
            return CCPoint{ x, y - index * deltaY * statsScale };
            };

        for (; statIndex < static_cast<int>(m_fields->statLabels.size()); ++statIndex) {
            if (m_fields->statLabels[statIndex])
                m_fields->statLabels[statIndex]->setPosition(nextPos(statIndex));

            if (valueIndex < static_cast<int>(m_fields->statValues.size()) &&
                m_fields->statValues[valueIndex]) {
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
            }
            idIndex++;
        }
    }
};