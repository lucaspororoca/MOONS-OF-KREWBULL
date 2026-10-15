#ifndef SCREEN_CREATE_CHARACTER_H
#define SCREEN_CREATE_CHARACTER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <array>
#include <string>
#include <vector>

class TelaCreateCharacter {
public:
    TelaCreateCharacter(SDL_Renderer* renderer, int screenW, int screenH);
    ~TelaCreateCharacter();

    bool init();
    void handleEvent(const SDL_Event& event);
    void update(float dt);
    void render();

    bool isRunning() const { return running; }
    bool shouldStartGame() const { return startGame; }
    bool shouldReturnToIntro() const { return returnToIntro; }
    void stop() { running = false; }

private:
    enum class Kind {
        CLASS, SUBCLASS, RACE, SUBRACE, GENDER, ROLL, CONFIRM, RETURN
    };

    struct Button {
        Kind         kind;
        std::string  id;
        std::string  idlePath;
        SDL_Texture* tex;
        SDL_Texture* hoverTex;
        SDL_Surface* mask;
        SDL_Rect     rect;
        bool         hovered;
        bool         enabled;
        bool         selected;
    };

    struct Selection {
        std::string race;
        std::string subrace;
        std::string cls;
        std::string subclass;
        std::string gender;
    };

    struct Attempt {
        std::string race;
        std::string subrace;
        std::string cls;
        std::string subclass;
        std::string gender;
    };

    void parseRandomBackground(const std::string& path, Selection& out) const;
    void loadRandomBackground();

    bool loadBackgroundTexture(const std::string& path);
    bool loadButtonTexture(const std::string& path,
                           SDL_Texture*& outTex,
                           SDL_Surface*& outMask,
                           int& outW, int& outH);

    void rebuildButtons();
    void addButtonLayers(const std::vector<std::string>& ids, Kind kind);
    void clearButtons();
    bool createCharacterSave();
    bool loadNameFont();
    void updateHover();
    bool isButtonEnabled(const Button& button) const;

    bool resolveBackground();
    bool updateBackgroundFromSelection();

    bool isPixelOpaque(SDL_Surface* surf, int x, int y) const;
    bool isPointOverButton(int mx, int my, const Button& btn) const;
    void handleButtonClick(const Button& btn);

    SDL_Renderer* renderer;
    int screenW;
    int screenH;
    bool running;
    bool startGame;
    bool returnToIntro;

    SDL_Texture* backgroundTex;
    std::string  currentBackgroundPath;

    std::vector<std::string> backgroundPool;
    Selection selection;
    std::vector<Button> buttons;
    bool hasSelectedButton;
    Kind selectedButtonKind;
    std::string selectedButtonId;
    std::string characterName;
    std::string statusMessage;
    TTF_Font* nameFont;
    TTF_Font* attributeFont;
    SDL_Rect nameInputRect;
    bool nameInputFocused;
    std::array<int, 6> rolledAttributes;
    bool attributesRolled;
};

#endif