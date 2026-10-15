#ifndef POTION_HP_MINOR_H
#define POTION_HP_MINOR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string>

class Player;

class PotionHpMinor {
public:
    PotionHpMinor();
    ~PotionHpMinor();

    bool load(SDL_Renderer* renderer);

    void renderSprite(SDL_Renderer* renderer, int x, int y) const;
    void renderSpriteScaled(SDL_Renderer* renderer, int x, int y,
                            int w, int h) const;
    void renderDescription(SDL_Renderer* renderer, int x, int y) const;

    void use(Player& player) const;

    int getSpriteWidth() const { return spriteW; }
    int getSpriteHeight() const { return spriteH; }
    int getDescriptionWidth() const { return descW; }
    int getDescriptionHeight() const { return descH; }

private:
    bool loadTexture(SDL_Renderer* renderer,
                     const std::string& path,
                     SDL_Texture*& out,
                     int& outW, int& outH);

    SDL_Texture* spriteTex;
    int spriteW;
    int spriteH;

    SDL_Texture* descTex;
    int descW;
    int descH;
};

#endif