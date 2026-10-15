#ifndef PLAYER_SAVE_H
#define PLAYER_SAVE_H

#include <SDL2/SDL.h>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace player_save {

struct GroundItem {
    std::string name;
    float weight = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
};

struct BagItem {
    std::string name;
    int quantity = 0;
    float weight = 0.0f;
    std::string state;
};

struct DoorState {
    int id = 0;
    int row = 0;
    int col = 0;
    std::string status;
    int difficulty = 0;
};

struct Skill {
    std::string name;
    int level = 1;
    int cooldown = 0;
    std::string description;
    std::string effect;
};

struct Attributes {
    int strength = 0;
    int dexterity = 0;
    int constitution = 0;
    int intellect = 0;
    int wisdom = 0;
    int charisma = 0;
};

struct Position {
    float x = 0.0f;
    float y = 0.0f;
};

struct Character {
    std::string name;
    std::string cls;
    std::string subclass;
    std::string race;
    std::string subrace;
    std::string gender;
    std::string spritePath;
    int level = 1;
    int experience = 0;
    int hp = 0;
    int maxHp = 0;
    int hpFormulaVersion = 0;
    Attributes attributes;
    Position playerPosition;
    bool hasWorldState = false;
    std::vector<Position> enemyPositions;
    bool hasRatPositions = false;
    std::vector<Position> ratEnemyPositions;
    std::vector<BagItem> bag;
    std::vector<DoorState> doors;
    std::vector<Skill> skills;
    std::vector<GroundItem> items;
    std::vector<std::string> mapTiles;
};

inline std::string spriteDirectory(const std::string& cls,
                                  const std::string& race,
                                  const std::string& subrace,
                                  const std::string& gender) {
    std::string classDirectory;
    if (cls == "wizard" || cls == "ladino" || cls == "sorcerer" ||
        cls == "barbarian") {
        classDirectory = cls;
    } else if (cls == "paladin" || cls == "paladino") {
        classDirectory = "paladino";
    } else {
        return "";
    }

    std::string raceDirectory = race;
    if (race == "elf" && (subrace == "altelf" || subrace == "drow")) {
        raceDirectory = subrace;
    } else if (race != "human" && race != "elf" &&
               race != "goblin" && race != "demogon") {
        return "";
    }

    if (gender != "female" && gender != "male") return "";
    return "resources/sprites/characters/player/" + classDirectory + "/" +
           raceDirectory + "/" + gender;
}

inline const std::string& xorKey() {
    static const std::string key = "MoonsOfKrewbull2024SecretKey!";
    return key;
}

inline std::string crypt(const std::string& input) {
    std::string output(input);
    const std::string& key = xorKey();
    for (std::size_t i = 0; i < output.size(); ++i) {
        output[i] = static_cast<char>(
            static_cast<unsigned char>(output[i]) ^
            static_cast<unsigned char>(key[i % key.size()]));
    }
    return output;
}

inline std::string escape(const std::string& value) {
    std::string result;
    for (std::size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        if (c == '"' || c == '\\') result += '\\';
        if (c == '\n') {
            result += "\\n";
        } else if (c == '\r') {
            result += "\\r";
        } else if (c == '\t') {
            result += "\\t";
        } else {
            result += c;
        }
    }
    return result;
}

inline bool readString(const std::string& json, const std::string& key,
                       std::string& value) {
    const std::string token = "\"" + key + "\"";
    std::size_t pos = json.find(token);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos + token.size());
    if (pos == std::string::npos) return false;
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return false;

    value.clear();
    for (++pos; pos < json.size(); ++pos) {
        const char c = json[pos];
        if (c == '"') return true;
        if (c == '\\' && pos + 1 < json.size()) {
            const char escaped = json[++pos];
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 'r': value += '\r'; break;
                case 't': value += '\t'; break;
                default: value += escaped; break;
            }
        } else {
            value += c;
        }
    }
    return false;
}

inline bool readNumber(const std::string& json, const std::string& key,
                       double& value) {
    const std::string token = "\"" + key + "\"";
    std::size_t pos = json.find(token);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos + token.size());
    if (pos == std::string::npos) return false;
    ++pos;
    while (pos < json.size() &&
           std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
    std::istringstream number(json.substr(pos));
    return static_cast<bool>(number >> value);
}

inline std::string serializeItems(const std::vector<GroundItem>& items) {
    std::ostringstream json;
    json << "[";
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i != 0) json << ",";
        json << "{\"name\":\"" << escape(items[i].name)
             << "\",\"weight\":" << items[i].weight
             << ",\"x\":" << items[i].x
             << ",\"y\":" << items[i].y << "}";
    }
    json << "]";
    return json.str();
}

inline std::string serializeBag(const std::vector<BagItem>& items) {
    std::ostringstream json;
    json << "[";
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i != 0) json << ",";
        json << "{\"name\":\"" << escape(items[i].name)
             << "\",\"quantity\":" << items[i].quantity
             << ",\"weight\":" << items[i].weight
             << ",\"state\":\"" << escape(items[i].state) << "\"}";
    }
    json << "]";
    return json.str();
}

inline std::string serializeDoors(const std::vector<DoorState>& doors) {
    std::ostringstream json;
    json << "[";
    for (std::size_t i = 0; i < doors.size(); ++i) {
        if (i != 0) json << ",";
        json << "{\"id\":" << doors[i].id
             << ",\"row\":" << doors[i].row
             << ",\"col\":" << doors[i].col
             << ",\"status\":\"" << escape(doors[i].status)
             << "\",\"difficulty\":" << doors[i].difficulty << "}";
    }
    json << "]";
    return json.str();
}

inline std::string serializeSkills(const std::vector<Skill>& skills) {
    std::ostringstream json;
    json << "[";
    for (std::size_t i = 0; i < skills.size(); ++i) {
        if (i != 0) json << ",";
        json << "{\"name\":\"" << escape(skills[i].name)
             << "\",\"level\":" << skills[i].level
             << ",\"cooldown\":" << skills[i].cooldown
             << ",\"description\":\"" << escape(skills[i].description)
             << "\",\"effect\":\"" << escape(skills[i].effect) << "\"}";
    }
    json << "]";
    return json.str();
}

inline std::string serializeEnemies(const std::vector<Position>& enemies) {
    std::ostringstream json;
    json << std::setprecision(9) << "[";
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        if (i != 0) json << ",";
        json << "{\"x\":" << enemies[i].x
             << ",\"y\":" << enemies[i].y << "}";
    }
    json << "]";
    return json.str();
}

inline std::string serializeStringArray(
    const std::vector<std::string>& values) {
    std::ostringstream json;
    json << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) json << ",";
        json << "\"" << escape(values[i]) << "\"";
    }
    json << "]";
    return json.str();
}

inline bool readStringArray(const std::string& json, const std::string& key,
                            std::vector<std::string>& values) {
    const std::string token = "\"" + key + "\"";
    std::size_t pos = json.find(token);
    if (pos == std::string::npos) return false;
    pos = json.find('[', pos + token.size());
    if (pos == std::string::npos) return false;
    const std::size_t end = json.find(']', pos + 1);
    if (end == std::string::npos) return false;

    values.clear();
    while ((pos = json.find('"', pos + 1)) != std::string::npos && pos < end) {
        std::string value;
        bool closed = false;
        for (++pos; pos < end; ++pos) {
            const char c = json[pos];
            if (c == '\\' && pos + 1 < end) {
                const char escaped = json[++pos];
                switch (escaped) {
                    case 'n': value += '\n'; break;
                    case 'r': value += '\r'; break;
                    case 't': value += '\t'; break;
                    default: value += escaped; break;
                }
            } else if (c == '"') {
                closed = true;
                break;
            } else {
                value += c;
            }
        }
        if (!closed) return false;
        values.push_back(value);
    }
    return true;
}

inline bool readObjectArray(const std::string& json, const std::string& key,
                            std::vector<std::string>& objects) {
    const std::string token = "\"" + key + "\"";
    std::size_t pos = json.find(token);
    if (pos == std::string::npos) return false;
    pos = json.find('[', pos + token.size());
    if (pos == std::string::npos) return false;
    std::size_t arrayEnd = pos + 1;
    int arrayDepth = 1;
    bool inString = false;
    bool escaped = false;
    for (; arrayEnd < json.size(); ++arrayEnd) {
        const char c = json[arrayEnd];
        if (inString) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') inString = false;
            continue;
        }
        if (c == '"') inString = true;
        else if (c == '[') ++arrayDepth;
        else if (c == ']' && --arrayDepth == 0) break;
    }
    if (arrayEnd >= json.size()) return false;

    objects.clear();
    while ((pos = json.find('{', pos + 1)) != std::string::npos &&
           pos < arrayEnd) {
        std::size_t objectEnd = pos + 1;
        int objectDepth = 1;
        inString = false;
        escaped = false;
        for (; objectEnd < arrayEnd; ++objectEnd) {
            const char c = json[objectEnd];
            if (inString) {
                if (escaped) escaped = false;
                else if (c == '\\') escaped = true;
                else if (c == '"') inString = false;
                continue;
            }
            if (c == '"') inString = true;
            else if (c == '{') ++objectDepth;
            else if (c == '}' && --objectDepth == 0) break;
        }
        if (objectEnd >= arrayEnd) return false;
        objects.push_back(json.substr(pos, objectEnd - pos + 1));
        pos = objectEnd;
    }
    return true;
}

inline bool readObjectValue(const std::string& json, const std::string& key,
                            std::string& object) {
    const std::string token = "\"" + key + "\"";
    std::size_t pos = json.find(token);
    if (pos == std::string::npos) return false;
    pos = json.find('{', pos + token.size());
    if (pos == std::string::npos) return false;
    const std::size_t start = pos;
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (; pos < json.size(); ++pos) {
        const char c = json[pos];
        if (inString) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') inString = false;
            continue;
        }
        if (c == '"') inString = true;
        else if (c == '{') ++depth;
        else if (c == '}' && --depth == 0) {
            object = json.substr(start, pos - start + 1);
            return true;
        }
    }
    return false;
}

inline bool readAttributes(const std::string& json, Attributes& attributes) {
    std::string object;
    if (!readObjectValue(json, "attributes", object)) {
        attributes = Attributes();
        return json.find("\"attributes\"") == std::string::npos;
    }
    double value = 0.0;
    if (!readNumber(object, "strength", value)) return false;
    attributes.strength = static_cast<int>(value);
    if (!readNumber(object, "dexterity", value)) return false;
    attributes.dexterity = static_cast<int>(value);
    if (!readNumber(object, "constitution", value)) return false;
    attributes.constitution = static_cast<int>(value);
    if (!readNumber(object, "intellect", value)) return false;
    attributes.intellect = static_cast<int>(value);
    if (!readNumber(object, "wisdom", value)) return false;
    attributes.wisdom = static_cast<int>(value);
    if (!readNumber(object, "charisma", value)) return false;
    attributes.charisma = static_cast<int>(value);
    return true;
}

inline bool readWorldState(const std::string& json, Character& character) {
    std::string playerPosition;
    const bool hasPosition =
        readObjectValue(json, "playerPosition", playerPosition);
    if (json.find("\"playerPosition\"") != std::string::npos && !hasPosition) {
        return false;
    }
    if (hasPosition) {
        double value = 0.0;
        if (!readNumber(playerPosition, "x", value)) return false;
        character.playerPosition.x = static_cast<float>(value);
        if (!readNumber(playerPosition, "y", value)) return false;
        character.playerPosition.y = static_cast<float>(value);
        character.hasWorldState = true;
    }

    const bool hasEnemiesKey = json.find("\"enemies\"") != std::string::npos;
    std::vector<std::string> enemyObjects;
    if (hasEnemiesKey && !readObjectArray(json, "enemies", enemyObjects)) {
        return false;
    }
    character.enemyPositions.clear();
    for (const std::string& enemyObject : enemyObjects) {
        Position position;
        double value = 0.0;
        if (!readNumber(enemyObject, "x", value)) return false;
        position.x = static_cast<float>(value);
        if (!readNumber(enemyObject, "y", value)) return false;
        position.y = static_cast<float>(value);
        character.enemyPositions.push_back(position);
    }
    if (hasEnemiesKey) character.hasWorldState = true;

    const bool hasRatsKey =
        json.find("\"ratPositions\"") != std::string::npos;
    std::vector<std::string> ratObjects;
    if (hasRatsKey &&
        !readObjectArray(json, "ratPositions", ratObjects)) {
        return false;
    }
    character.hasRatPositions = hasRatsKey;
    character.ratEnemyPositions.clear();
    for (const std::string& ratObject : ratObjects) {
        Position position;
        double value = 0.0;
        if (!readNumber(ratObject, "x", value)) return false;
        position.x = static_cast<float>(value);
        if (!readNumber(ratObject, "y", value)) return false;
        position.y = static_cast<float>(value);
        character.ratEnemyPositions.push_back(position);
    }
    if (hasRatsKey) character.hasWorldState = true;
    return true;
}

inline bool replaceJsonValue(std::string& json, const std::string& key,
                             const std::string& replacement,
                             std::string& error) {
    const std::string token = "\"" + key + "\"";
    const std::size_t keyPos = json.find(token);
    if (keyPos == std::string::npos) {
        const std::size_t objectEnd = json.find_last_of('}');
        if (objectEnd == std::string::npos) {
            error = "Objeto JSON principal invalido ao inserir " + key;
            return false;
        }
        const std::size_t previous = json.find_last_not_of(" \t\r\n", objectEnd - 1);
        const std::string prefix = previous != std::string::npos &&
                                   json[previous] != '{' ? ",\n" : "\n";
        json.insert(objectEnd, prefix + "  \"" + key + "\": " + replacement + "\n");
        return true;
    }

    std::size_t start = json.find(':', keyPos + token.size());
    if (start == std::string::npos) {
        error = "Campo " + key + " invalido.";
        return false;
    }
    ++start;
    while (start < json.size() &&
           std::isspace(static_cast<unsigned char>(json[start]))) ++start;
    if (start >= json.size()) {
        error = "Valor " + key + " invalido.";
        return false;
    }

    std::size_t end = start;
    if (json[start] == '{' || json[start] == '[') {
        const char open = json[start];
        const char close = open == '{' ? '}' : ']';
        int depth = 0;
        bool inString = false;
        bool escaped = false;
        for (; end < json.size(); ++end) {
            const char c = json[end];
            if (inString) {
                if (escaped) escaped = false;
                else if (c == '\\') escaped = true;
                else if (c == '"') inString = false;
                continue;
            }
            if (c == '"') inString = true;
            else if (c == open) ++depth;
            else if (c == close && --depth == 0) break;
        }
        if (end >= json.size()) {
            error = "Valor " + key + " nao terminado.";
            return false;
        }
        ++end;
    } else if (json[start] == '"') {
        bool escaped = false;
        for (end = start + 1; end < json.size(); ++end) {
            const char c = json[end];
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') break;
        }
        if (end >= json.size()) {
            error = "Valor " + key + " nao terminado.";
            return false;
        }
        ++end;
    } else {
        while (end < json.size() && json[end] != ',' && json[end] != '}') {
            ++end;
        }
        while (end > start &&
               std::isspace(static_cast<unsigned char>(json[end - 1]))) {
            --end;
        }
        if (end == start) {
            error = "Valor " + key + " invalido.";
            return false;
        }
    }
    json.replace(start, end - start, replacement);
    return true;
}

inline bool replaceArray(const std::filesystem::path& path,
                         const std::string& key,
                         const std::string& replacement,
                         std::string& error) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream encrypted;
    encrypted << input.rdbuf();
    if (input.bad()) {
        error = "Erro ao ler o save para atualizar " + key + ": " +
                path.string();
        return false;
    }
    std::string json = crypt(encrypted.str());
    const std::string token = "\"" + key + "\"";
    std::size_t pos = json.find(token);
    if (pos == std::string::npos) {
        error = "Campo " + key + " ausente no save: " + path.string();
        return false;
    }
    const std::size_t start = json.find('[', pos + token.size());
    if (start == std::string::npos) {
        error = "Array " + key + " invalido no save: " + path.string();
        return false;
    }
    std::size_t end = start + 1;
    int depth = 1;
    bool inString = false;
    bool escaped = false;
    for (; end < json.size(); ++end) {
        const char c = json[end];
        if (inString) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') inString = false;
            continue;
        }
        if (c == '"') inString = true;
        else if (c == '[') ++depth;
        else if (c == ']' && --depth == 0) break;
    }
    if (end >= json.size()) {
        error = "Array " + key + " nao terminado no save: " + path.string();
        return false;
    }
    json.replace(start, end - start + 1, replacement);
    const std::string output = crypt(json);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        error = "Nao foi possivel atualizar o save: " + path.string();
        return false;
    }
    file.write(output.data(), static_cast<std::streamsize>(output.size()));
    if (!file) {
        error = "Falha ao atualizar o save: " + path.string();
        return false;
    }
    error.clear();
    return true;
}

inline bool readItems(const std::string& json,
                      std::vector<GroundItem>& items) {
    std::vector<std::string> objects;
    if (!readObjectArray(json, "items", objects)) return false;
    items.clear();
    for (std::size_t i = 0; i < objects.size(); ++i) {
        GroundItem item;
        double number = 0.0;
        if (!readString(objects[i], "name", item.name) ||
            !readNumber(objects[i], "weight", number)) return false;
        item.weight = static_cast<float>(number);
        if (!readNumber(objects[i], "x", number)) return false;
        item.x = static_cast<float>(number);
        if (!readNumber(objects[i], "y", number)) return false;
        item.y = static_cast<float>(number);
        items.push_back(item);
    }
    return true;
}

inline bool readBag(const std::string& json, std::vector<BagItem>& items) {
    std::vector<std::string> objects;
    if (!readObjectArray(json, "bag", objects)) return false;
    items.clear();
    for (std::size_t i = 0; i < objects.size(); ++i) {
        BagItem item;
        double number = 0.0;
        if (!readString(objects[i], "name", item.name) ||
            !readNumber(objects[i], "quantity", number)) return false;
        item.quantity = static_cast<int>(number);
        if (!readNumber(objects[i], "weight", number)) return false;
        item.weight = static_cast<float>(number);
        if (!readString(objects[i], "state", item.state)) return false;
        items.push_back(item);
    }
    return true;
}

inline bool readDoors(const std::string& json,
                      std::vector<DoorState>& doors) {
    std::vector<std::string> objects;
    if (!readObjectArray(json, "doors", objects)) return false;
    doors.clear();
    for (std::size_t i = 0; i < objects.size(); ++i) {
        DoorState door;
        double number = 0.0;
        if (!readNumber(objects[i], "id", number)) return false;
        door.id = static_cast<int>(number);
        if (!readNumber(objects[i], "row", number)) return false;
        door.row = static_cast<int>(number);
        if (!readNumber(objects[i], "col", number)) return false;
        door.col = static_cast<int>(number);
        if (!readString(objects[i], "status", door.status)) return false;
        if (!readNumber(objects[i], "difficulty", number)) return false;
        door.difficulty = static_cast<int>(number);
        doors.push_back(door);
    }
    return true;
}

inline bool readSkills(const std::string& json, std::vector<Skill>& skills) {
    skills.clear();
    if (json.find("\"skills\"") == std::string::npos) return true;

    std::vector<std::string> objects;
    if (!readObjectArray(json, "skills", objects)) return false;
    for (const std::string& object : objects) {
        Skill skill;
        double level = 0.0;
        double cooldown = 0.0;
        if (!readString(object, "name", skill.name) ||
            !readNumber(object, "level", level) ||
            !readString(object, "description", skill.description) ||
            !readString(object, "effect", skill.effect)) {
            return false;
        }
        if (object.find("\"cooldown\"") != std::string::npos &&
            !readNumber(object, "cooldown", cooldown)) {
            return false;
        }
        skill.level = std::max(1, static_cast<int>(level));
        skill.cooldown = std::max(0, static_cast<int>(cooldown));
        skills.push_back(skill);
    }
    return true;
}

inline std::filesystem::path saveDirectory() {
    std::filesystem::path base;
    if (char* executablePath = SDL_GetBasePath()) {
        base = executablePath;
        SDL_free(executablePath);
    } else {
        base = std::filesystem::current_path();
    }
    return base / "saves";
}

inline std::filesystem::path executableDirectory() {
    if (char* executablePath = SDL_GetBasePath()) {
        const std::filesystem::path base(executablePath);
        SDL_free(executablePath);
        return base;
    }
    return std::filesystem::current_path();
}

inline bool listSaveFiles(std::vector<std::filesystem::path>& paths,
                          std::string& error) {
    paths.clear();
    const std::vector<std::filesystem::path> directories = {
        saveDirectory(),
        executableDirectory() / "saves"
    };
    std::vector<std::filesystem::file_time_type> modifiedTimes;
    std::error_code ec;
    for (const std::filesystem::path& directory : directories) {
        const bool directoryExists = std::filesystem::exists(directory, ec);
        if (ec) {
            error = "Falha ao consultar diretorio de saves " +
                    directory.string() + ": " + ec.message();
            return false;
        }
        if (!directoryExists) continue;
        if (!std::filesystem::is_directory(directory, ec)) {
            if (ec) {
                error = "Falha ao acessar diretorio de saves " +
                        directory.string() + ": " + ec.message();
            } else {
                error = "O caminho de saves nao e um diretorio: " +
                        directory.string();
            }
            return false;
        }

        for (std::filesystem::directory_iterator it(directory, ec), end;
             it != end && !ec; it.increment(ec)) {
            if (!it->is_regular_file(ec)) {
                if (ec) break;
                continue;
            }
            const std::filesystem::path path = it->path();
            const std::string filename = path.filename().string();
            if (path.extension() != ".sav" ||
                filename.find("save_") != 0) continue;

            const std::filesystem::path normalized =
                std::filesystem::weakly_canonical(path, ec);
            if (ec) break;
            if (std::find(paths.begin(), paths.end(), normalized) != paths.end()) {
                continue;
            }
            const auto modified = it->last_write_time(ec);
            if (ec) break;
            paths.push_back(normalized);
            modifiedTimes.push_back(modified);
        }
        if (ec) {
            error = "Falha ao listar saves em " + directory.string() +
                    ": " + ec.message();
            return false;
        }
    }

    std::vector<std::size_t> order(paths.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(),
        [&modifiedTimes](std::size_t left, std::size_t right) {
            return modifiedTimes[left] > modifiedTimes[right];
        });
    std::vector<std::filesystem::path> sortedPaths;
    sortedPaths.reserve(paths.size());
    for (std::size_t index : order) sortedPaths.push_back(paths[index]);
    paths.swap(sortedPaths);
    error.clear();
    return true;
}

inline std::string serialize(const Character& character) {
    std::ostringstream json;
    json << "{\n"
         << "  \"name\": \"" << escape(character.name) << "\",\n"
         << "  \"class\": \"" << escape(character.cls) << "\",\n"
         << "  \"subclass\": \"" << escape(character.subclass) << "\",\n"
         << "  \"race\": \"" << escape(character.race) << "\",\n"
         << "  \"subrace\": \"" << escape(character.subrace) << "\",\n"
         << "  \"gender\": \"" << escape(character.gender) << "\",\n"
         << "  \"spritePath\": \"" << escape(character.spritePath) << "\",\n"
         << "  \"level\": " << std::max(1, std::min(character.level, 20)) << ",\n"
         << "  \"experience\": " << std::max(0, character.experience) << ",\n"
         << "  \"hp\": " << std::max(0, character.hp) << ",\n"
         << "  \"maxHp\": " << std::max(0, character.maxHp) << ",\n"
         << "  \"hpFormulaVersion\": "
         << std::max(0, character.hpFormulaVersion) << ",\n"
         << "  \"attributes\": {\"strength\": " << character.attributes.strength
         << ", \"dexterity\": " << character.attributes.dexterity
         << ", \"constitution\": " << character.attributes.constitution
         << ", \"intellect\": " << character.attributes.intellect
         << ", \"wisdom\": " << character.attributes.wisdom
         << ", \"charisma\": " << character.attributes.charisma << "},\n"
         << "  \"bag\": " << serializeBag(character.bag) << ",\n"
         << "  \"doors\": " << serializeDoors(character.doors) << ",\n"
         << "  \"skills\": " << serializeSkills(character.skills) << ",\n"
         << "  \"items\": " << serializeItems(character.items) << ",\n"
         << "  \"mapTiles\": " << serializeStringArray(character.mapTiles);
    if (character.hasWorldState) {
        json << ",\n  \"playerPosition\": {\"x\": "
             << std::setprecision(9) << character.playerPosition.x
             << ", \"y\": " << character.playerPosition.y << "},\n"
             << "  \"enemies\": " << serializeEnemies(character.enemyPositions)
             << ",\n  \"ratPositions\": "
             << serializeEnemies(character.ratEnemyPositions);
    }
    json << "\n"
         << "}\n";
    return json.str();
}

inline bool readFile(const std::filesystem::path& path, Character& character,
                     std::string& error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "Nao foi possivel abrir o save: " + path.string();
        return false;
    }
    std::ostringstream encrypted;
    encrypted << file.rdbuf();
    if (file.bad()) {
        error = "Erro ao ler o save: " + path.string();
        return false;
    }
    const std::string json = crypt(encrypted.str());
    double level = 1.0;
    double experience = 0.0;
    double hp = 0.0;
    double maxHp = 0.0;
    double hpFormulaVersion = 0.0;
    if (!readString(json, "name", character.name) ||
        !readString(json, "class", character.cls) ||
        !readString(json, "subclass", character.subclass) ||
        !readString(json, "race", character.race) ||
        !readString(json, "subrace", character.subrace) ||
        !readString(json, "gender", character.gender) ||
        (json.find("\"spritePath\"") != std::string::npos &&
         !readString(json, "spritePath", character.spritePath)) ||
        !readNumber(json, "level", level) ||
        !readAttributes(json, character.attributes) ||
        !readWorldState(json, character) ||
        !readBag(json, character.bag) ||
        !readDoors(json, character.doors) ||
        !readSkills(json, character.skills) ||
        !readItems(json, character.items) ||
        !readStringArray(json, "mapTiles", character.mapTiles)) {
        error = "Formato invalido no save: " + path.string();
        return false;
    }
    if ((json.find("\"hp\"") != std::string::npos &&
         !readNumber(json, "hp", hp)) ||
        (json.find("\"experience\"") != std::string::npos &&
         !readNumber(json, "experience", experience)) ||
        (json.find("\"maxHp\"") != std::string::npos &&
         !readNumber(json, "maxHp", maxHp)) ||
        (json.find("\"hpFormulaVersion\"") != std::string::npos &&
         !readNumber(json, "hpFormulaVersion", hpFormulaVersion))) {
        error = "Formato invalido nos pontos de vida do save: " +
                path.string();
        return false;
    }
    character.level = std::max(1, std::min(static_cast<int>(level), 20));
    character.experience = std::max(0, static_cast<int>(experience));
    character.hp = std::max(0, static_cast<int>(hp));
    character.maxHp = std::max(0, static_cast<int>(maxHp));
    character.hpFormulaVersion =
        std::max(0, static_cast<int>(hpFormulaVersion));
    error.clear();
    return true;
}

inline bool readLatest(std::filesystem::path& path, Character& character,
                       std::string& error) {
    const std::filesystem::path directory = saveDirectory();
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec) || ec) {
        error = "Diretorio de saves inexistente: " + directory.string();
        return false;
    }
    std::filesystem::directory_entry latest;
    bool found = false;
    for (std::filesystem::directory_iterator it(directory, ec), end;
         it != end && !ec; it.increment(ec)) {
        if (!it->is_regular_file() || it->path().extension() != ".sav" ||
            it->path().filename().string().find("save_") != 0) continue;
        if (!found || it->last_write_time(ec) > latest.last_write_time(ec)) {
            if (ec) break;
            latest = *it;
            found = true;
        }
    }
    if (ec) {
        error = "Falha ao procurar saves: " + ec.message();
        return false;
    }
    if (!found) {
        error = "Nenhum arquivo save_*.sav foi encontrado.";
        return false;
    }
    path = latest.path();
    return readFile(path, character, error);
}

inline bool create(const Character& character, std::filesystem::path& path,
                   std::string& error) {
    const std::filesystem::path directory = saveDirectory();
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) {
        error = "Falha ao criar diretorio de saves: " + ec.message();
        return false;
    }
    const std::uint64_t timestamp =
        static_cast<std::uint64_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::random_device random;
    const std::uint64_t suffix =
        (static_cast<std::uint64_t>(random()) << 32) ^ random();
    path = directory / ("save_" + std::to_string(timestamp ^ suffix) + ".sav");

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        error = "Nao foi possivel criar o save: " + path.string();
        return false;
    }
    const std::string encrypted = crypt(serialize(character));
    file.write(encrypted.data(), static_cast<std::streamsize>(encrypted.size()));
    if (!file) {
        error = "Falha ao gravar o save: " + path.string();
        file.close();
        std::filesystem::remove(path, ec);
        return false;
    }
    error.clear();
    return true;
}

inline bool writeItems(const std::filesystem::path& path,
                       const std::vector<GroundItem>& items,
                       std::string& error) {
    Character character;
    if (!readFile(path, character, error)) return false;
    return replaceArray(path, "items", serializeItems(items), error);
}

inline bool writeBag(const std::filesystem::path& path,
                     const std::vector<BagItem>& items,
                     std::string& error) {
    Character character;
    if (!readFile(path, character, error)) return false;
    return replaceArray(path, "bag", serializeBag(items), error);
}

inline bool writeDoors(const std::filesystem::path& path,
                       const std::vector<DoorState>& doors,
                       std::string& error) {
    Character character;
    if (!readFile(path, character, error)) return false;
    return replaceArray(path, "doors", serializeDoors(doors), error);
}

inline bool writeSkills(const std::filesystem::path& path,
                        const std::vector<Skill>& skills,
                        std::string& error) {
    Character character;
    if (!readFile(path, character, error)) return false;
    return replaceArray(path, "skills", serializeSkills(skills), error);
}

inline bool writeVitals(const std::filesystem::path& path, int hp, int maxHp,
                        std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Nao foi possivel abrir o save: " + path.string();
        return false;
    }
    std::ostringstream encrypted;
    encrypted << input.rdbuf();
    if (input.bad()) {
        error = "Erro ao ler o save: " + path.string();
        return false;
    }

    std::string json = crypt(encrypted.str());
    if (!replaceJsonValue(json, "hp", std::to_string(std::max(0, hp)), error) ||
        !replaceJsonValue(json, "maxHp",
                          std::to_string(std::max(0, maxHp)), error) ||
        !replaceJsonValue(json, "hpFormulaVersion", "1", error)) {
        error += " Save: " + path.string();
        return false;
    }
    const std::string output = crypt(json);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        error = "Nao foi possivel atualizar o save: " + path.string();
        return false;
    }
    file.write(output.data(), static_cast<std::streamsize>(output.size()));
    if (!file) {
        error = "Falha ao salvar pontos de vida: " + path.string();
        return false;
    }
    error.clear();
    return true;
}

inline bool writeMapTiles(const std::filesystem::path& path,
                          const std::vector<std::string>& mapTiles,
                          std::string& error) {
    Character character;
    if (!readFile(path, character, error)) return false;
    return replaceArray(path, "mapTiles", serializeStringArray(mapTiles), error);
}

inline bool writeWorldState(const std::filesystem::path& path,
                            const Position& playerPosition,
                            const std::vector<Position>& enemyPositions,
                            const std::vector<Position>& ratEnemyPositions,
                            int hp, int maxHp, int level, int experience,
                            std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Nao foi possivel abrir o save: " + path.string();
        return false;
    }
    std::ostringstream encrypted;
    encrypted << input.rdbuf();
    if (input.bad()) {
        error = "Erro ao ler o save: " + path.string();
        return false;
    }

    std::string json = crypt(encrypted.str());
    std::ostringstream playerJson;
    playerJson << std::setprecision(9) << "{\"x\":" << playerPosition.x
               << ",\"y\":" << playerPosition.y << "}";
    if (!replaceJsonValue(json, "playerPosition", playerJson.str(), error) ||
        !replaceJsonValue(json, "enemies",
                          serializeEnemies(enemyPositions), error) ||
        !replaceJsonValue(json, "ratPositions",
                          serializeEnemies(ratEnemyPositions), error) ||
        !replaceJsonValue(json, "hp", std::to_string(std::max(0, hp)), error) ||
        !replaceJsonValue(json, "maxHp",
                          std::to_string(std::max(0, maxHp)), error) ||
        !replaceJsonValue(json, "level",
                          std::to_string(std::max(1, std::min(level, 20))), error) ||
        !replaceJsonValue(json, "experience",
                          std::to_string(std::max(0, experience)), error) ||
        !replaceJsonValue(json, "hpFormulaVersion", "1", error)) {
        error += " Save: " + path.string();
        return false;
    }

    const std::string output = crypt(json);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        error = "Nao foi possivel atualizar o save: " + path.string();
        return false;
    }
    file.write(output.data(), static_cast<std::streamsize>(output.size()));
    if (!file) {
        error = "Falha ao salvar localizacoes no save: " + path.string();
        return false;
    }
    error.clear();
    return true;
}

} // namespace player_save

#endif
